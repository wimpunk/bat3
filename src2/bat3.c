#include <stdio.h>
#include <time.h>


#include "log.h"

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




main(int argc, char *argv[])
{
	printf("Hello world");
}
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
