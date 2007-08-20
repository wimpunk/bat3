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

#include <stdio.h>
#include "log.h"

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

};

#endif

