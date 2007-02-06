/* 
   bat3 - ts-bat3 controller

   Copyright (C) 2007 wimpunk

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

#include <termios.h>
#include <grp.h>
#include <pwd.h>
*/

#include <sys/termios.h> // tcgetattr
#include <stdio.h>
#include <sys/select.h>
#include <alloca.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ByteArray.h"
#include "IntArray.h"
#include "file.h"
#include "bat3.h"
#include "net.h"
#include "Process.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

/*
  BAT3 Utility / Charging Daemon

  The program provides a number of options for experimenting with TS-BAT3
  operation, as well as a default charging algorithm.

  This program utilizes the generic AVR interface code which is also used
  in the TS-732 board.

  Note: The TS-BAT3 will automatically shut off charging if we don't send
  it a packet within 2 minutes.  However, we don't provide a similar
  feature if an external program is trying to implement its own charging
  algorithm on top of the duty cycled constant-current abstraction we
  provide.

  TO DO:
  1. Add another command we respond to while charging the battery,
     which puts the system to sleep for a while.
 */
static const char VERSION[] = "1.1.0";

char IPADRS[128] = { 0 };

void BAT3BatteryCharge(FILE *f,int mA,int on,int off,int shutdelay);

int BAT3requestInitFromReply(ByteArrayRef bRep,ByteArrayRef bReq) {
  BAT3request *req = (BAT3request *)(bReq.arr);
  BAT3reply *rep = (BAT3reply *)(bRep.arr);
  
  req->outputs = rep->outputs;
  req->pwm_lo = rep->pwm_lo;
  req->pwm_t = rep->pwm_t;
  req->alarm = 0;
  req->softJP3 = 0;
  if (rep->batRun) {
    req->_led = 1;
  } else {
    if (rep->PWM1en || rep->PWM2en) {
      req->_led = !req->_led;
    } else {
      req->_led = 0;
    }
  }
  return sizeof(BAT3request);
}

int BAT3processDummy(FILE *file,ByteArrayRef bReq,void *data,int *loops,
		       int argc,char **argv) {
  *loops = 0;
  return 2;
}

int BAT3EEPROMdoneCallback(ByteArrayRef req1,ByteArrayRef rep1) {
  BAT3request *req = (BAT3request *)req1.arr;
  BAT3reply *rep = (BAT3reply *)rep1.arr;

  if (rep->ee_ready == 0) {
    return 0;
  }
  return 1;
}

int BAT3EEPROMcallback(ByteArrayRef req1,ByteArrayRef rep1) {
  BAT3request *req = (BAT3request *)req1.arr;
  BAT3reply *rep = (BAT3reply *)rep1.arr;

  if (rep->ee_addr != req->ee_addr) {
    return 1;
  }
  if (rep->ee_ready) {
    return 0;
  }
  return 2;
}

#define BYTE_ARRAY_REF_STRUCT(name) BAR((byte *)name,sizeof(*(name)))

void BAT3writeEEPROM(FILE *f,byte adrs,byte value) {
  BAT3reply pkt;
  BAT3request req;
  ByteArrayRef b = BYTE_ARRAY_REF_STRUCT(&pkt);

  fileFlushInput(f);
  b = AVRreplyGet(f,b);
  BAT3requestInitFromReply(BYTE_ARRAY_REF_STRUCT(&pkt),
			    BYTE_ARRAY_REF_STRUCT(&req));
  req.ee_addr = adrs;
  req.ee_data = value;
  req.ee_write = 1;
  req.ee_read = 0;

  printf("EE WR %02X -> [%02X]\n",value,adrs);
  AVRsendRequest(f,BYTE_ARRAY_REF_STRUCT(&req),BYTE_ARRAY_REF_STRUCT(&pkt),
		 &BAT3EEPROMcallback);
}

byte BAT3readEEPROM(FILE *f,byte adrs,BAT3reply *pkt) {
  BAT3reply pkt2;
  BAT3request req;
  ByteArrayRef b = BYTE_ARRAY_REF_STRUCT(pkt);

  b = AVRreplyGet(f,b);
  BAT3requestInitFromReply(BYTE_ARRAY_REF_STRUCT(pkt),
			    BYTE_ARRAY_REF_STRUCT(&req));
  req.ee_addr = adrs;
  req.ee_write = 0;
  req.ee_read = 1;

  pkt->ee_addr = -1;
  AVRsendRequest(f,BYTE_ARRAY_REF_STRUCT(&req),BYTE_ARRAY_REF_STRUCT(pkt),
		 &BAT3EEPROMcallback);
  // printf("EE RD %02X <- [%02X]\n",pkt.pkt.EEdata,adrs);
  req.ee_read = 0; // required to return replies to send ADC data
  AVRsendRequest(f,BYTE_ARRAY_REF_STRUCT(&req),BYTE_ARRAY_REF_STRUCT(&pkt2),
		 &BAT3EEPROMdoneCallback);
  return pkt->ee_data;
}

int BAT3callback(ByteArrayRef bReq,ByteArrayRef bRep);

// & 1           -> send a request
// & 0xFFFFFFFE  -> other stuff to do first, call taskDo
int BAT3processOptions(FILE *file,ByteArrayRef bReq,void *data,int *loops,
		       int argc,char **argv) {
  BAT3request *req = (BAT3request *)(bReq.arr);
  static struct option long_options[] = {
    { "pwm1"    ,1,0,0 },
    { "pwm2"    ,1,0,0 },
    { "offset"  ,1,0,0 },
    { "opamp"   ,1,0,0 },
    { "buck"    ,1,0,0 },
    { "led"     ,1,0,0 },
    { "pwmlo"   ,1,0,0 },
    { "pwmt"    ,1,0,0 },
    { "help"    ,0,0,0 },
    { "sleep"   ,1,0,0 },
    { "eer"     ,1,0,0},
    { "eew"     ,1,0,0},
    { "eeval"   ,1,0,0},
    { "charge"  ,1,0,0},
    { "log"     ,1,0,0},
    { "on"      ,1,0,0},
    { "off"     ,1,0,0},
    { "version" ,0,0,0},
    { "shutdown",1,0,0},
    { 0,0,0,0 }
  };
  int c,option_index,doSomething = 0,on=60,off=60,shutdelay=-1;
  word val;
  BAT3reply pkt;
  ByteArrayRef bRep = BAR(&pkt,sizeof(BAT3reply));

  opterr = 0; // don't print any messages when we get a bad option
  *loops = 0;

  while (1) {
    c = getopt_long(argc,argv,"",long_options,&option_index);
    if (c == -1) {
      break;
    }
    if (c != 0) {
      continue;
    }
    switch (option_index) {
    case 0: // pwm1 en
      doSomething |= 1;
      req->PWM1en = atoi(optarg);
      printf("PWM1: %s\n",req->PWM1en?"on":"off");
      break;
    case 1: // pwm2 en
      doSomething |= 1;
      req->PWM2en = atoi(optarg);
      printf("PWM2: %s\n",req->PWM2en?"on":"off");
      break;
    case 2: // offset en
      doSomething |= 1;
      req->_offsetEn = !atoi(optarg);
      printf("Offset: %s\n",req->_offsetEn?"off":"on");
      break;
    case 3: // opamp en
      doSomething |= 1;
      req->opampEn = atoi(optarg);
      printf("OPAMP: %s\n",req->opampEn?"on":"off");
      break;
    case 4: // buck en
      doSomething |= 1;
      req->_buckEn = !atoi(optarg);
      printf("Buck: %s\n",req->_buckEn?"off":"on");
      break;
    case 5: // led en
      doSomething |= 1;
      req->_led = !atoi(optarg);
      printf("LED: %s\n",req->_led?"off":"on");
      break;
    case 6: // pwmlo time
      doSomething |= 1;
      req->pwm_lo = atoi(optarg);
      if (req->pwm_t < req->pwm_lo) {
	req->pwm_t = req->pwm_lo;
      }
      printf("PWM LO= %d\n",req->pwm_lo);
      break;
    case 7: // pwmtotal time
      doSomething |= 1;
      req->pwm_t = atoi(optarg);
      printf("PWM T= %d\n",req->pwm_t);
      break;
    case 8: // help
      printf(
"Usage options:\n"
"  --pwm1 [0|1]    disable or enable PWM1\n"
"  --pwm2 [0|1]    disable or enable PWN2\n"
"  --offset [0|1]  disable or enable offset\n"
"  --opamp [0|1]   disable or enable op amp\n"
"  --buck [0|1]    disable or enable buck\n"
"  --led [0|1]     disable or enable led\n"
"  --pwmlo [0|1]   set PWM low time\n"
"  --pwmt [0|1]    set PWM period\n"
"  --sleep <time>  go to sleep for number of seconds\n"
"  --help          get this help message\n"
"  --charge <I>    maintain battery charging with specified current\n"
"                  NOTE: most other options will be ignored with this one\n"
"                  until charge algorithm is terminated by hitting ENTER\n"
"  --on <seconds>  number of seconds to charge battery at I each period\n"
"                  The default value is 60.\n"
"                  NOTE: This option must come BEFORE the charge option\n"
"  --off <seconds> number of seconds to not charge battery each period\n"
"                  The default value is 60.\n"
"                  NOTE: This option must come BEFORE the charge option\n"
"  --log <ip>      send 1Hz battery information packets to give IP address\n"
"                  NOTE: This option must come BEFORE the charge option\n"
"  --shutdown <s>  Automatically shut down s seconds after power failure\n"
"                  NOTE: This option must come BEFORE the charge option\n"
      );
      break;
    case 9: // sleep
      doSomething |= 1;
      req->alarm = atoi(optarg);
      printf("Sleeping for %d seconds (approximately)\n",req->alarm);
      break;
    case 10: // eer
      c = atoi(optarg);
      val = BAT3readEEPROM(file,c & 0xFF,&pkt);
      printf("%02X:%02X\n",c&0xFF,pkt.ee_data);
      break;
    case 11: // eew
      c = atoi(optarg);
      printf("Writing %02X to %02X\n",val & 0xFF,c & 0xFF);
      BAT3writeEEPROM(file,c & 0xFF ,val & 0xFF);
      break;
    case 12: // eeval
      val = atoi(optarg);
      break;
    case 13: // charge
      // OP AMP must be enabled for charging to work
      req->opampEn = 1;
      req->_buckEn = 1;
      req->_offsetEn = 1;
      AVRsendRequest(file,bReq,bRep,&BAT3callback);
      val = atoi(optarg);
      if (val > 500 || val < 60) {
	printf("Can only charge the battery with 60mA < I < 400mA\n");
      } else {
	//printf("Charging with %dmA...\n",val);
	BAT3BatteryCharge(file,1000*val,on,off,shutdelay);
      }
      break;
    case 14: // log
      strncpy(IPADRS,optarg,128);
      optarg[127] = 0;
      printf("Logging to: %s\n",IPADRS);
      break;
    case 15: // on
      on = atoi(optarg);
      break;
    case 16: // off
      off = atoi(optarg);
      break;
    case 17: // version
      printf("bat3, version %s\n",VERSION);
      break;
    case 18: // shutdown
      shutdelay = atoi(optarg);
      printf("Automatic shutdown %d seconds after power failure\n",shutdelay);
      break;

    }
  }
  if ((doSomething & 1) == 0 && *loops == 0) {
    *loops = 1;
  }
  return doSomething;
}

// from tracker/eread.c
static float tempF(short int tmp) {
  short int tmpd;
  float f;

  tmpd = tmp & 0x7F;
  tmp >>= 7;
  f = (float)tmp + (float)tmpd / 128;
  //printf("TempC:%f\n",f);
  f = 9.0/5.0 * f + 32;
  //printf("TempF:%f\n",f);
  //  f = ((float)((int)(f * 10))) / 10;
  return f;
}

float battV(unsigned short ticks) {
  float f;

  f = (float)(ticks) / 12160.0;
  return f;
}

int battI(unsigned short ticks) { // return uA
  return (ticks-29000) * 60;
}

int BAT3task2Do(FILE *f2,int task,ByteArrayRef bReq,ByteArrayRef bRep,void *data) {
  return 0;
}

void BAT3replyPacket_Print(ByteArrayRef bRep,void *data) {
  BAT3reply *rep = (BAT3reply *)(bRep.arr);

  printf("Supply Voltage ADC    = %04X\n",rep->adc0);
  printf("Regulated Voltage ADC = %04X\n",rep->adc2);
  //printf("Battery Voltage ADC   = %04X\n",rep->adc6);
  printf("Battery Voltage       = %.2fV (%04X)\n",battV(rep->adc6),rep->adc6);
  //printf("Battery Current ADC   = %04X\n",rep->adc7);
  printf("Battery Current       = %duA\n",battI(rep->adc7));
  printf("PWM LO = %04X\n",rep->pwm_lo);
  printf("PWM T  = %04X\n",rep->pwm_t);
  //printf("TEMP   = %04X\n",rep->temp);
  printf("TEMP   = %.1fF\n",tempF(rep->temp-7));

  printf("PWM 1 EN  = %s\n",rep->PWM1en ? "ON":"OFF");
  printf("PWM 2 EN  = %s\n",rep->PWM2en ? "ON":"OFF");
  printf("OFFSET EN = %s\n",rep->_offsetEn ? "OFF":"ON");
  printf("OPAMP EN  = %s\n",rep->opampEn ? "ON":"OFF");
  printf("BUCK EN   = %s\n",rep->_buckEn ? "OFF":"ON");
  printf("LED       = %s\n",rep->_led ? "OFF":"ON");
  printf("JP3       = %s\n",rep->_jp3 ? "OFF":"ON");
  printf("Running on %s\n",rep->batRun ? "BATTERY":"LINE VOLTAGE");
}

void BAT3taskDo(FILE *f,int task,ByteArrayRef bRep,void *data) {
}

// 0: to indicate that its satified that the request has succeeded
// 1: to indicate that the request should be re-sent
// 2: to indicate that it wants another reply without a re-send
int BAT3callback(ByteArrayRef bReq,ByteArrayRef bRep) {
  BAT3request *req = (BAT3request *)(bReq.arr);
  BAT3reply *rep = (BAT3reply *)(bRep.arr);

  return (
    req->PWM1en != rep->PWM1en
    || req->PWM2en != rep->PWM2en
    || req->_offsetEn != rep->_offsetEn
    || req->opampEn != rep->opampEn
    || req->_buckEn != rep->_buckEn
    || req->_led != rep->_led
    || req->pwm_lo != rep->pwm_lo
    || req->pwm_t != rep->pwm_t
    ) ? 1 : 0;
}

float smoothAverage(float *arr,int count) {
  float min, max, minI, maxI, Sum, n;
  int j;

  if (count == 0) {
    return 0.0;
  }
  if (count == 1) {
    return arr[0];
  }
  if (count == 2) {
    return (arr[0] + arr[1]) / 2;
  }

  minI = maxI = arr[0];
  min = max = 0;
  for (j=1;j<count;j++) {
    if (arr[j] < min) {
      minI = j;
      min = arr[j];
    }
    if (arr[j] > max) {
      maxI = j;
      max = arr[j];
    }
  }
  n = 0.0; Sum = 0.0;
  for (j=0;j<count;j++) {
    if (j != minI && j != maxI) {
      Sum += arr[j];
      n += 1.0;
    }
  }
  //assert(n > 0.5);
  return Sum / n;
}

float average(float *arr,int count) {
  float Sum=0.0;
  int j,n=0;

  if (count == 0) {
    return 0.0;
  }
  for (j=0;j<count;j++) {
    Sum += arr[j];
    n++;
  }
  return Sum / n;
}

void BAT3chargeState_Init(BAT3chargeState *state,int targetI,int on,int off) {
  state->targetI = state->masterI = targetI;
  state->time_on = state->timer = on;
  state->time_off = off;
  state->locked = 0;
  state->running = 1;
  state->logging = 0;
  state->lock_max = 500;
  state->lock_hi = state->lock_max + 1;
  state->lock_lo = 0;
  state->numSamples = 0;
  state->txSocket = -1;
  state->last_t = 0;
  state->txSocket = -1;
  state->rxSocket = netUDPopen(4000,1);
  /*
  strcpy(state->IPADRS,"192.168.0.169");
  state->toPort = 2000;
  if (state->IPADRS[0]) {
    state->txSocket = netUDPopen(state->toPort,0);
    netAdrsFromName(&state->to,state->IPADRS,state->toPort);
  }
  */
}

void BAT3UDPlogUpdate(BAT3chargeState *state,BAT3reply *rep,time_t t){
  float V,I,T;
  bat3Info inf;

  V = battV(rep->adc6);
  I = (float)battI(rep->adc7);
  T = tempF(rep->temp-7);
  if (t != state->last_t) {
    if (state->numSamples > 0) {
      memset(&inf,0,sizeof(inf));
      inf.t = state->last_t;
      inf.supplyV = rep->adc0;
      inf.regV = rep->adc2;
      inf.pwmlo = average(state->pwm,state->numSamples);
      inf.pwmt = rep->pwm_t;
      inf.pwm1en = rep->PWM1en;
      inf.pwm2en = rep->PWM2en;
      inf.offset = !rep->_offsetEn;
      inf.opamp = rep->opampEn;
      inf.buck = !rep->_buckEn;
      inf.led = !rep->_led;
      inf.jp3= !rep->_jp3;
      inf.onbatt = rep->batRun;
      inf.softJP3 = rep->softJP3;
      inf.locked = state->locked;

      inf.battV = smoothAverage(state->V,state->numSamples);
      inf.battI = (int)smoothAverage(state->I,state->numSamples);
      inf.tempF = smoothAverage(state->T,state->numSamples);
      
      if (state->txSocket > -1) {
	netUDPtx(state->txSocket,BAR(&inf,sizeof(bat3Info)),&state->to);
      }
      state->numSamples = 0;
    }
    state->last_t = t;

    // implement automatic on/off at duty cycle
    // note: time on should be long enough to lock or we won't!
    if (!rep->batRun) {
      if (state->timer > 0) {
	state->timer--;
	if (state->timer == 0) {
	  if (state->time_off > 0) {
	    state->targetI = 0;
	  } else {
	    state->timer = state->time_on;
	  }
	}
      } else {
	state->timer--;
	if (state->timer <= -state->time_off) {
	  if (state->time_on > 0) {
	    state->targetI = state->masterI;
	    state->locked = 0;
	  }
	  state->timer = state->time_on;
	}
      }
    }
  }
  if (state->numSamples < 32) {
    state->V[state->numSamples] = V;
    state->I[state->numSamples] = I;
    state->T[state->numSamples] = T;
    state->pwm[state->numSamples] = (float)rep->pwm_lo;
    state->numSamples++;
  }
}


/*
  Listens on a UDP port for commands
  - enable/disable sending BAT3 info packets to specified IP/port
    - info packets are sent at 1Hz
    - analog values are smoothed/averaged over one second
    - analog values are not sent unless at least 16 samples are received
    - that's probably not necessary so we won't do that for now
  - change target current
  - change target current and on/off times
*/
void BAT3UDPcommandCheck(BAT3chargeState *state) {
  BAT3UDPcommand cmd;
  struct timeval tv;
  struct sockaddr_in from;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if (filesWaitRead(IAR(&state->rxSocket,1),&tv)) {
    netUDPrx(state->rxSocket,BAR(&cmd,sizeof(cmd)),&from);
    if (cmd.command == 1) {
      sprintf(state->IPADRS,"%d.%d.%d.%d",
	      (from.sin_addr.s_addr)&0xFF,
	      (from.sin_addr.s_addr>>8)&0xFF,
	      (from.sin_addr.s_addr>>16)&0xFF,
	      (from.sin_addr.s_addr>>24)&0xFF);
      state->toPort = cmd.cmd1.port;
      if (state->txSocket > -1) {
	close(state->txSocket);
      }
      state->txSocket = netUDPopen(state->toPort,0);
      netAdrsFromName(&state->to,state->IPADRS,state->toPort);
    } else if (cmd.command == 2) {
      if (state->txSocket > -1) {
	close(state->txSocket);
      }
      state->txSocket = -1;
    } else if (cmd.command == 3) {
      //printf("\nNew target current: %dmA       \n",cmd.cmd3.targetI);
      if (abs(state->targetI-cmd.cmd3.targetI*1000) > 3000) {
	state->locked = 0;
	state->lock_hi = state->lock_max + 1;
	state->lock_lo = 0;
      }
      state->targetI = state->masterI = cmd.cmd3.targetI * 1000;
      state->time_on = 1;
      state->time_off = 0;
      state->timer = 1;
    } else if (cmd.command == 4) {
      if (abs(state->targetI-cmd.cmd4.targetI*1000) > 3000) {
	state->locked = 0;
	state->lock_hi = state->lock_max + 1;
	state->lock_lo = 0;
      }
      state->targetI = state->masterI = cmd.cmd4.targetI * 1000;
      state->time_on = cmd.cmd4.time_on;
      if (state->time_on < 0) {
	state->time_on = 0;
      }
      state->time_off = cmd.cmd4.time_off;
      if (state->time_off < 0) {
	state->time_off = 0;
      }
      state->timer = state->time_on;
    }
  }

}

/*
  This routine does the following:
  1. Tries to maintain charge current at targetI (possibly 0)
    - if current falls out of regulation, reset (to 0)
  2. Listens on a UDP port for commands
  3. Whenever a power-failure occurs, waits for a user-defined
     delay and then initiates shutdown.  /etc/init.d/ups-monitor
     (or other file, depending on your distribution) must set the
     softJP3 bit.
  There are two states:
  SEEKING
  - trying to find the correct PWM value for the current we want
  - we use a binary search pattern
  LOCKED
  - trying to maintain the current PWM value for the current we want
  - we use a linear step to maintain the current we want

 */
void BAT3BatteryCharge(FILE *f,int targetI,int on,int off,int shutdelay) {
  BAT3chargeState state;
  byte req1[256],rep1[256];
  ByteArrayRef bReq = BAR(req1,256),bRep = BAR(rep1,256);
  BAT3request *req = (BAT3request *)(bReq.arr);
  BAT3reply *rep = (BAT3reply *)(bRep.arr);
  time_t t,last_t=0;
  int shutdown_pending = -1;

  processDaemonize();
  BAT3chargeState_Init(&state,targetI,on,off);
  while (state.running) {
    // check for commands
    BAT3UDPcommandCheck(&state);
    // take a sample
    bRep = AVRreplyGet(f,bRep);
    bReq.len = BAT3requestInitFromReply(bRep,bReq);
    // add sample to log
    time((time_t *)&t);
    BAT3UDPlogUpdate(&state,rep,t);
    // if running on battery, update shutdown state
    if (t != last_t) {
      if (rep->batRun) { // running on battery?
	if (shutdelay > 0) { // auto-shutdown enabled?
	  if (shutdown_pending > 0) { // already pending shutdown?
	    if (--shutdown_pending == 0) { // time to shutdown now?
	      // Change the next line if you need a different command
	      // to initiate a shut down on your system
	      system("shutdown -h now");
	      shutdown_pending = 15; // just in case shutdown fails???
	    }
	  } else { // not yet pending shutdown
	    shutdown_pending = shutdelay;
	  }
	}
      } else { // not running on battery
	shutdown_pending = -1;
      }
      last_t = t;
    }
    // update PWM
    req->PWM1en = req->PWM2en = (state.targetI > 0);
    req->opampEn = 1;
    req->pwm_t = state.lock_max;
    if (state.locked) {
      if (!rep->batRun && state.targetI > 0 
	  && battI(rep->adc7) - state.targetI > 3000) {
	if (req->pwm_lo < state.lock_max) {
	  req->pwm_lo++;
	} else {
	  //printf("\nCan't get current low enough\n");
	  state.targetI = 0;
	}
      } else if (!rep->batRun && state.targetI > 0 
		 && battI(rep->adc7) - state.targetI < -3000) {
	if (req->pwm_lo > 0) {
	  req->pwm_lo--;
	} else {
	  //printf("\nCan't get current high enough\n");
	  state.targetI = 0;
	}
      }
    } else {
      if (state.lock_hi > state.lock_max) {
	// do nothing - haven't got initial reading yet
	state.lock_hi = state.lock_max;
      } else if (battI(rep->adc7) > state.targetI) {
	state.lock_lo = req->pwm_lo;
      } else {
	state.lock_hi = req->pwm_lo;
      }
      req->pwm_lo = state.lock_lo + (state.lock_hi - state.lock_lo) / 2;
      if (state.lock_hi - state.lock_lo < 2) {
	state.locked = 1;
      }
    }
    
    // update state, take another sample and throw it away
    AVRsendRequest(f,bReq,bRep,BAT3callback);
    //bRep = AVRreplyGet(f,bRep);
  }
}


int main(int argc,char *argv[]) {
  AVRrun("/dev/ttyT3S0",0,argc,argv,0,&BAT3requestInitFromReply,
	 &BAT3processOptions,&BAT3replyPacket_Print,&BAT3task2Do,
	 &BAT3callback);
  return 0;
}

/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
