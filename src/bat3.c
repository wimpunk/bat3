#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

#include "log.h"
#include "stream.h"

#define BAT3DEV "/dev/ttyT3S0"

typedef enum {
	ON,
	OFF
} onoff_t;

struct bat3 {

	unsigned short adc0; // input supply voltage
	unsigned short adc2; // regulated supply voltage
	unsigned short adc6; // battery voltage
	unsigned short adc7; // battery current
	unsigned short pwm_lo; // high time
	unsigned short pwm_t; // high time plus low time
	unsigned short temp; // temperature in TMP124 format
	unsigned char ee_addr;
	unsigned char ee_data;

	onoff_t PWM1en;
	onoff_t PWM2en;
	onoff_t offsetEn;
	onoff_t opampEn;
	onoff_t buckEn;
	onoff_t led;
	onoff_t jp3;
	onoff_t batRun;
	onoff_t ee_read;
	onoff_t ee_write;
	onoff_t ee_ready;
	onoff_t softJP3;

};

struct BAT3request {
	unsigned alarm;
	union {
		struct {
			unsigned short PWM1en:1,
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
		} __attribute__((packed));
		unsigned short outputs;
	} __attribute__((packed));
	unsigned short pwm_lo; // low time
	unsigned short pwm_t; // high time plus low time
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;

} __attribute__((packed));

struct BAT3reply {
	unsigned short adc0; // input supply voltage
	unsigned short adc2; // regulated supply voltage
	unsigned short adc6; // battery voltage
	unsigned short adc7; // battery current
	unsigned short pwm_lo; // high time
	unsigned short pwm_t; // high time plus low time
	unsigned short temp; // temperature in TMP124 format
	union {
		struct {
			unsigned short PWM1en:1,
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
		} __attribute__((packed));
		unsigned short outputs;
	} __attribute__((packed));
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;
} __attribute__((packed));

unsigned char calcCrc(char *msg, int len) {

	int crc=0xFF,pos;
	for (pos=0; pos<len; pos++) crc = (crc - msg[pos]) & 0xFF;
	// printf("**crc=%04X",crc);
	return crc;

}


float battV(unsigned short ticks) 
{

	float f;

	f = (float)(ticks) / 12160.0;

	return f;
}

int battI(unsigned short ticks) // return uA
{ 

	return (ticks-29000) * 60;

}

static float tempC(short int tmp) 
{

	short int tmpd;
	float f;

	tmpd = tmp & 0x7F;
	tmp >>= 7;
	f = (float)tmp + (float)tmpd / 128;
	// f = 9.0/5.0 * f + 32;
	//printf("TempF:%f\n",f);
	//  f = ((float)((int)(f * 10))) / 10;
	return f;
}

void decodemsg(char *msg, int size) 
{
	unsigned short adc0; // input supply voltage
	unsigned short adc2; // regulated supply voltage
	unsigned short adc6; // battery voltage
	unsigned short adc7; // battery current
	unsigned short pwm_lo; // high time
	unsigned short pwm_t; // high time plus low time
	unsigned short temp; // temperature in TMP124 format
	/* PWM1en:1,PWM2en:1,_offsetEn:1,opampEn:1,_buckEn:1,_led:1,_jp3:1,batRun:1,ee_read:1,ee_write:1,ee_ready:1,softJP3:1;	 */
	unsigned short outputs;
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;
	struct BAT3reply *reply;

	int pos=0;

	// for (pos=0; pos<size-1; pos++) mycheck=(mycheck - msg[pos])&0xFF;
	// pos=0;

	if (size!=19) {
		printf("Decode msg: volgens mij heeft ie verkeerde lengte\n");
	}

	adc0    = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	adc2    = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	adc6    = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	adc7    = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	pwm_lo  = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	pwm_t   = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	temp    = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	outputs = msg[pos+0] + msg[pos+1]*0x100; pos+=2;
	ee_addr = msg[pos++];
	ee_data = msg[pos++];
	checksum = msg[pos++];

	printf("Converted:\n");
	printf("adc = %04X - input supply voltage     %2.2fV\n", adc0, battV(adc0));
	printf("adc = %04X - regulated supply voltage %2.2fV\n", adc2, battV(adc2));
	printf("adc = %04X - battery voltage          %2.2fV\n", adc6, battV(adc6));
	printf("adc = %04X - battery current          %2.2fmA\n", adc7, battI(adc7)/1000.0);
	printf("pwm = %04X - high time                %dmA\n", pwm_lo, pwm_lo);
	printf("pwm = %04X - pulse time               %dmA\n", pwm_t, pwm_t);
	printf("temp= %04X - temperature in TMP124    %2.2fC\n", temp, tempC(temp));
	printf("out = %02X - outputs                  %d\n", outputs, outputs);
	printf("add = %02X - address                  %d\n", ee_addr, ee_addr);
	printf("dat = %02X - date                     %d\n", ee_data, ee_data);
	printf("sum = %02X - checksum           mine: %04X\n", checksum, calcCrc(msg,size-1));

	printf("based on bat3:\n");
	reply = (struct BAT3reply*) msg;
	printf("adc = %04X - input supply voltage     %2.2fV\n", reply->adc0, battV(reply->adc0));
	printf("adc = %04X - regulated supply voltage %2.2fV\n", reply->adc2, battV(reply->adc2));
	printf("adc = %04X - battery voltage          %2.2fV\n", reply->adc6, battV(reply->adc6));
	printf("adc = %04X - battery current          %2.2fmA\n", reply->adc7, battI(reply->adc7)/1000.0);
	printf("pwm = %04X - high time                %dmA\n", reply->pwm_lo, reply->pwm_lo);
	printf("pwm = %04X - pulse time               %dmA\n", reply->pwm_t, reply->pwm_t);
	printf("temp= %04X - temperature in TMP124    %2.2fC\n", reply->temp, tempC(reply->temp));
	printf("out = %02X - outputs                  %d\n", reply->outputs, reply->outputs);
	printf("             PWM1en:    %s\n", reply->PWM1en?"ON":"OFF");
	printf("             PWM2en:    %s\n", reply->PWM2en?"ON":"OFF");
	printf("             _offsetEn: %s\n", reply->_offsetEn?"ON":"OFF");
	printf("             _buckEn:   %s\n", reply->_buckEn?"ON":"OFF");
	printf("             _led:      %s\n", reply->_led?"ON":"OFF");
	printf("             _jp3:      %s\n", reply->_jp3?"ON":"OFF");
	printf("             batRun:    %s\n", reply->batRun?"ON":"OFF");
	printf("             ee_read:   %s\n", reply->ee_read?"ON":"OFF");
	printf("             ee_write:  %s\n", reply->ee_write?"ON":"OFF");
	printf("             ee_ready:  %s\n", reply->ee_ready?"ON":"OFF");
	printf("             softJP3:  %s\n", reply->softJP3?"ON":"OFF");
	/* PWM2en:1 _offsetEn:1 opampEn:1 _buckEn:1, _led:1 _jp3:1 batRun:1 ee_read:1 ee_write:1 ee_ready:1 softJP3:1; */
	printf("add = %02X - address                  %d\n", reply->ee_addr, reply->ee_addr);
	printf("dat = %02X - date                     %d\n", reply->ee_data, reply->ee_data);
	
}

void writemsg(int fd, char* msg, int len, int read)
{
	/*
	   struct BAT3request 

	   unsigned alarm;

PWM1en:1,
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

unsigned short outputs;

unsigned short pwm_lo; // low time
unsigned short pwm_t; // high time plus low time
unsigned char ee_addr;
unsigned char ee_data;
unsigned char checksum;

	 */

	struct BAT3request req;
	struct BAT3reply   *rep = (struct BAT3reply *)msg;

	int cnt;
	
	// Try to read a byte
	req.outputs = rep->outputs;
	req.pwm_lo = rep->pwm_lo;
	req.pwm_t = rep->pwm_t;
	req.alarm = 0;
	req.softJP3 = 0;
	req._led = 1;

	// if (read)
	req.ee_addr = 128;
	/* else {
	req.ee_addr = -1;
	} */
	
	req.ee_write = 0;
	req.ee_read = read;

	// req.checksum = calcCrc((char*)&req, sizeof(req)-1);

	// cnt=write(fd, (char*)&req, sizeof(req));
	// logabba(L_MAX, "Write returned %d while sizeof(req)=%d, checksum is %04X", cnt,sizeof(req),req.checksum);
		writeStream(fd, (char*)&req, sizeof(req)-1);	
	// req.ee_read = 0;
	
/*
	// OP AMP must be enabled for charging to work
	req.opampEn = 1;
	req._buckEn = 1;
	req._offsetEn = 1;
	req.checksum = calcCrc((char*)&req, sizeof(req)-1);

	cnt = write(fd, (void *)&req, sizeof(req));

	logabba(L_MAX, "Write returned %d", cnt);		

	// OP AMP must be enabled for charging to work
	req.opampEn = 1;
	req.PWM1en = 1;
	req.PWM2en = 1;
	req._buckEn = 1;
	req._offsetEn = 1;

	req.outputs = rep->outputs;
	req.alarm   = 0;
	req.softJP3 = 0;
	req.pwm_lo  = rep->pwm_lo;
	req.pwm_t   = rep->pwm_t;

	if (rep->batRun) {
		logabba(L_MAX, "Running on bat\n");
		req._led = 1;
	} else {
		if (rep->PWM1en || rep->PWM2en) {
			req._led = !req._led;
			logabba(L_MAX, "invert led\n");
		} else {
			// req._led = 0;
			// forcing led off
			req._led  = 1;
			logabba(L_MAX, "Forcing led OFF");
		}
	}
*/
	/*
	req.checksum = calcCrc((char*)&req, sizeof(req)-1);

	write(fd, (char*)&req, sizeof(req));
	logabba(L_MAX, "Write returned %d while sizeof(req)=%d, checksum is %04X", cnt,sizeof(req),req.checksum);
*/
	return;
}

void usage(char *progname) 
{
	// TODO: shit opkuisen
	printf("\n");
	printf("Usage\n");
	printf("\n");
	printf("   %s [-d device]\n",progname);
	printf("      -d device: device to use, default %s\n",BAT3DEV );
	printf("\n");
}

void flush (int fd)
{
int cnt=0;
char c;
while ((read(fd, &c, 1))>0) cnt++;
logabba(L_MAX, "Flush was reading %d bytes",cnt);
}
int main(int argc, char *argv[])
{

	int c,fd;
	char device[50]=""; // device to use
	int errcnt=0;
	char msg[56];
	int cnt;

	strncpy(device, BAT3DEV, sizeof(BAT3DEV));

	while ((c=getopt(argc, argv, "d:h?"))!=EOF) {
		switch (c) {
			case 'd': //device
				strncpy(device,optarg, sizeof(device)-1);
				printf("I'll read device %s\n",device);
				break;
			case '?':
			case 'h': 
			default: errcnt++;
				 break;
		}
		if (errcnt) break;
	}

	if (errcnt) {
		usage(argv[0]);
		return;
	}
	
	if ((fd = open(device,O_NONBLOCK|O_RDWR))==-1) {
		fprintf(stderr,"Error opening %s: %m\n",device);
		return(1);
	}

	setloglevel(L_MAX,"bat3");
	logabba(L_MIN,"bat3 is started");

	termConfigRaw(fd);
	
	cnt = readStream(fd,msg,sizeof(msg));
	decodemsg(msg,cnt);
	
//	flush(fd); // maybe this helps?
	
	writemsg(fd, msg, cnt,  1);
	cnt = readStream(fd,msg,sizeof(msg));
	decodemsg(msg,cnt);
	
	writemsg(fd, msg, cnt, 0);
	cnt = readStream(fd,msg,sizeof(msg));
	decodemsg(msg,cnt);
	
	close(fd);

	return 0;
	
}

// vim:set autoindent