#ifndef __BAT3_Z
#define __BAT3_Z
#include <netinet/in.h>
#include "base.h"
#include "avr.h"

// copyright (c)2006 Technologic Systems

STRUCT(BAT3request);
STRUCT(BAT3reply);
STRUCT(bat3Info);
STRUCT(BAT3chargeState);
STRUCT(BAT3UDPcommand);

struct BAT3request {
  dword alarm;
  union {
    struct {
      word PWM1en:1,
           PWM2en:1,
	  _offsetEn:1,
          opampEn:1,
          _buckEn:1,
          _led:1,
	  _jp3:1,
	  batRun:1,
	  ee_read:1,
	  ee_write:1,
	  ee_ready:1,
          softJP3:1;
    } _PACK_;
    word outputs;
  } _PACK_;
  word pwm_lo; // low time
  word pwm_t; // high time plus low time
  byte ee_addr;
  byte ee_data;
  byte checksum;
} _PACK_;

struct BAT3reply {
  word adc0; // input supply voltage
  word adc2; // regulated supply voltage
  word adc6; // battery voltage
  word adc7; // battery current
  word pwm_lo; // high time
  word pwm_t; // high time plus low time
  word temp; // temperature in TMP124 format
  union {
    struct {
      word PWM1en:1,
           PWM2en:1,
	  _offsetEn:1,
          opampEn:1,
          _buckEn:1,
          _led:1,
	  _jp3:1,
          batRun:1,
	  ee_read:1,
	  ee_write:1,
	  ee_ready:1,
          softJP3:1;
    } _PACK_;
    word outputs;
  } _PACK_;
  byte ee_addr;
  byte ee_data;
  byte checksum;
} _PACK_;

struct bat3Info {
  int t; // time
  float supplyV;
  float regV;
  float battV;
  int battI;
  float tempF;
  word pwmt;
  float pwmlo;
  dword pwm1en:1,pwm2en:1,offset:1,opamp:1,buck:1,led:1,
	jp3:1,onbatt:1,softJP3:1,locked:1;
  float dVdT;
  dword reserved[17];
} _PACK_;

struct BAT3chargeState {
  int targetI,masterI;
  int time_on,time_off,timer;
  int locked:1,
    running:1,
    logging:1;
  word lock_hi,lock_lo,lock_max;
  float pwm[32];
  float V[32];
  float I[32];
  float T[32];
  time_t last_t;
  int numSamples;
  int txSocket,rxSocket;
  struct sockaddr_in to;
  char IPADRS[256];
  int toPort;
};

struct BAT3UDPcommand {
  int command;
  union {
    struct command1 { // start sending BAT3 info packets
      int port;
    } cmd1;
    struct command2 { // stop sending BAT3 info packets
    } cmd2;
    struct command3 { // change target current
      int targetI; // in milliAmps
    } cmd3;
    struct command4 { // set target current and duty cycle
      int targetI; // in milliAmps
      int time_on; // in seconds
      int time_off; // in seconds
    } cmd4;
  };
} _PACK_;

#endif
