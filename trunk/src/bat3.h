/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#ifndef BAT3_H_INCLUDED
#define BAT3_H_INCLUDED


// loading parameters
#define MAXSEC 60 // we take the average every 60 seconds
#define MAXMIN 30 // we compare the changes during 30 minutes
#define FILTER 0.99 // 0.97 wasn good enough

#include <stdio.h>
#include "log.h"
#include <time.h>

typedef enum {
	ON,
	OFF
} onoff_t;

char *print_onoff(onoff_t onoff);

struct bat3 {

	unsigned short inp_u; // input supply voltage
	unsigned short reg_u; // regulated supply voltage
	unsigned short bat_u; // battery voltage
	unsigned short bat_i; // battery current
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
	
	time_t  updated;

};

struct action {
	
	char device[50];
	
	int current;		// ordered current
	int read;
	int write;
	int address;
	
	FILE *logfile;
	int loglevel;
	int portno;
	
	int value;
	
	int samples;
	int socketfd;
	time_t hours;
	
	float weight[MAXMIN];	// weighted mean
	int   weightpos;	// position
	
	time_t alarm;	// time when we have to record the current voltage
	time_t stable_time; // time since when the current has been stable

};

int getAddress();
int getCurrent();
int setCurrent(int i);
int getLoglevel();
int setLoglevel(int i);

#endif

