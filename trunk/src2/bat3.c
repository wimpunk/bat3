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
#define BAT3DEV "/dev/ttyT3S0"

#define _PACK_ __attribute__((packed))

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
		} _PACK_;
		unsigned short outputs;
	} _PACK_;
	unsigned short pwm_lo; // low time
	unsigned short pwm_t; // high time plus low time
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;

} _PACK_;

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
		} _PACK_;
		unsigned short outputs;
	} _PACK_;
	unsigned char ee_addr;
	unsigned char ee_data;
	unsigned char checksum;
} _PACK_;

struct bat3Info {
	int t; // time
	float supplyV;
	float regV;
	float battV;
	int battI;
	float tempF;
	unsigned short pwmt;
	float pwmlo;
	unsigned pwm1en:1,pwm2en:1,offset:1,opamp:1,buck:1,led:1,
		 jp3:1,onbatt:1,softJP3:1,locked:1;
	float dVdT;
	unsigned reserved[17];
} _PACK_;

/*
   struct BAT3chargeState {
   int targetI,masterI;
   int time_on,time_off,timer;
   int locked:1,
running:1,
logging:1;
unsigned short lock_hi,lock_lo,lock_max;
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
 */

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


int main(int argc, char *argv[])
{
	/*printf("Hello world\n");
	printf("Bat3 is currently doing nothing :-)\n");
	*/

	int c;
	char device[50]=""; // device to use
	int errcnt=0;
	
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
	/* original version: 

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

	 */
	int f;


	if ((f = open(device,O_NONBLOCK|O_RDWR))==-1) {
		fprintf(stderr,"Error opening %s: %m\n",device);
		return(1);
	}
	
	termConfigRaw(f);
	// 	_AVRrun(f,data,argc,argv,setup,init,opt,print,task,callback);
	close(f);
	printf("Except opening a port and closing it again\n");

	return 0;
}
