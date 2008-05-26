/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "convert.h"
#include "log.h"
#include "tsbat3.h"

float adcV(unsigned short ticks) {
	/* According to http://tech.groups.yahoo.com/group/ts-7000/message/5402
	 * These inputs are coming directly from an ADC. To get the ADC
	 * output the
	 * supply voltage is multiplied by ~0.07, then divided by 3.3V, then
	 * multiplied
	 * by 65536. This corresponds to approximately 700mV per unit, so 39812 *
	 * 700mV is around 28V. The regulated value is approximately 190mV per
	 * unit,
	 * so 26623 * 190mV is right around 5V. The values are only
	 * approximate due to
	 * part tolerances and lack of ADC calibration.
	 */
	
	// return (ticks * 0.07 / 3.3 * 65536);
	return (ticks * 0.700);
}

float battV(unsigned short ticks) {
	
	float f;
	
	f = (float)(ticks) / 12160.0;
	
	return f;
}

int battI(unsigned short ticks) // return uA
{
	
	return (ticks-29000) * 60;
	
}

unsigned short Ibatt(int current) {
	return (current/60)+29000;
}

// Converts TEMP124 signal to Celsius
float tempC(short int tmp) {
	
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

static void print_BAT3reply(FILE *fd, struct BAT3reply* reply) {
	
	if (fd != NULL){
		
		fprintf(fd, "->print_bat3reply\n");
		fprintf(fd, "adc = %04X - input supply voltage     %2.2fV\n", reply->adc0, battV(reply->adc0));
		fprintf(fd, "adc = %04X - regulated supply voltage %2.2fV\n", reply->adc2, battV(reply->adc2));
		fprintf(fd, "adc = %04X - battery voltage          %2.2fV\n", reply->adc6, battV(reply->adc6));
		fprintf(fd, "adc = %04X - battery current          %2.2fmA\n", reply->adc7, battI(reply->adc7)/1000.0);
		fprintf(fd, "pwm = %04X - high time                %dmA\n", reply->pwm_lo, reply->pwm_lo);
		fprintf(fd, "pwm = %04X - pulse time               %dmA\n", reply->pwm_t, reply->pwm_t);
		fprintf(fd, "temp= %04X - temperature in TMP124    %2.2fC\n", reply->temp, tempC(reply->temp));
		fprintf(fd, "out = %02X - outputs                  %d\n", reply->outputs, reply->outputs);
		fprintf(fd, "             PWM1en:    %s\n", reply->PWM1en?"ON":"OFF");
		fprintf(fd, "             PWM2en:    %s\n", reply->PWM2en?"ON":"OFF");
		fprintf(fd, "             _offsetEn: %s\n", reply->_offsetEn?"ON":"OFF");
		fprintf(fd, "             _buckEn:   %s\n", reply->_buckEn?"ON":"OFF");
		fprintf(fd, "             _led:      %s\n", reply->_led?"ON":"OFF");
		fprintf(fd, "             _jp3:      %s\n", reply->_jp3?"ON":"OFF");
		fprintf(fd, "             batRun:    %s\n", reply->batRun?"ON":"OFF");
		fprintf(fd, "             ee_read:   %s\n", reply->ee_read?"ON":"OFF");
		fprintf(fd, "             ee_write:  %s\n", reply->ee_write?"ON":"OFF");
		fprintf(fd, "             ee_ready:  %s\n", reply->ee_ready?"ON":"OFF");
		fprintf(fd, "             softJP3:  %s\n", reply->softJP3?"ON":"OFF");
		/* PWM2en:1 _offsetEn:1 opampEn:1 _buckEn:1, _led:1 _jp3:1 batRun:1 ee_read:1 ee_write:1 ee_ready:1 softJP3:1; */
		fprintf(fd, "add = %02X - address                  %d\n", reply->ee_addr, reply->ee_addr);
		fprintf(fd, "dat = %02X - date                     %d\n", reply->ee_data, reply->ee_data);
	}
}

int fdprintf(int fd, char* fmt, ...)
{
	va_list args;
	char line[256];
	
	va_start(args,fmt);
	snprintf(line, sizeof(line), fmt, args);
	va_end(args);
	
	return write(fd, line, strlen(line));
	
}

void print_bat3(int fd, struct bat3* mybat3) {
	if (fd>0) {
		fdprintf(fd, "->print_bat3\n");
		fdprintf(fd, "input supply voltage     = %04X (%3.2fV)\n",  mybat3->inp_u, 0.70/1000 * (mybat3->inp_u)); //
		fdprintf(fd, "regulated supply voltage = %04X (%3.2fV)\n",  mybat3->reg_u, 0.19/1000 * (mybat3->reg_u)); //
		fdprintf(fd, "battery voltage          = %04X (%3.2fV)\n",  mybat3->bat_u, battV(mybat3->bat_u)); //
		fdprintf(fd, "battery current          = %04X (%3.2fmA)\n", mybat3->bat_i, battI(mybat3->bat_i)/1000.0); //
		fdprintf(fd, "low time                 = %04X\n", mybat3->pwm_lo); //
		fdprintf(fd, "high & low time          = %04X\n", mybat3->pwm_t); //
		fdprintf(fd, "temperature              = %04X (%3.2fC)\n", mybat3->temp, tempC(mybat3->temp)); //  in TMP124 format
		fdprintf(fd, "EEPROM address           = %04X\n", mybat3->ee_addr);
		fdprintf(fd, "EEPROM data              = %04X\n", mybat3->ee_data);
		
		fdprintf(fd, "PWM1en   = %s\n", print_onoff(mybat3->PWM1en));
		fdprintf(fd, "PWM2en   = %s\n", print_onoff(mybat3->PWM2en));
		fdprintf(fd, "offsetEn = %s\n", print_onoff(mybat3->offsetEn));
		fdprintf(fd, "opampEn  = %s\n", print_onoff(mybat3->opampEn));
		fdprintf(fd, "buckEn   = %s\n", print_onoff(mybat3->buckEn));
		fdprintf(fd, "led      = %s\n", print_onoff(mybat3->led));
		fdprintf(fd, "jp3      = %s\n", print_onoff(mybat3->jp3));
		fdprintf(fd, "batRun   = %s\n", print_onoff(mybat3->batRun));
		
		fdprintf(fd, "ee_read  = %s\n", print_onoff(mybat3->ee_read));
		fdprintf(fd, "ee_write = %s\n", print_onoff(mybat3->ee_write));
		fdprintf(fd, "ee_ready = %s\n", print_onoff(mybat3->ee_ready));
		
		fdprintf(fd, "softJP3  = %s\n", print_onoff(mybat3->softJP3));
	}
}

// Converts an incomming msg to struct bat3
int decodemsg(char *msg, int size, struct bat3* mybat3, FILE* logfile) {
	struct BAT3reply *reply;
	
	if (size!=sizeof(struct BAT3reply)) {
		logabba(L_MIN, "Decode msg: wrong message size: %d != %d", size, sizeof(struct BAT3reply));
		// TODO: check if this is still needed
		return 1;
	}
	
	reply = (struct BAT3reply*) msg;
	
	print_BAT3reply(logfile, reply);
	// logabba(L_MIN,"Decoding got crc = %02X", reply->checksum);
	
	mybat3->inp_u    = reply->adc0 ; // input supply voltage
	mybat3->reg_u    = reply->adc2 ; // regulated supply voltage
	mybat3->bat_u    = reply->adc6 ; // battery voltage
	mybat3->bat_i    = reply->adc7 ; // battery current
	mybat3->pwm_lo   = reply->pwm_lo ; // high time
	mybat3->pwm_t    = reply->pwm_t ; // high time plus low time
	mybat3->temp     = reply->temp ; // temperature in TMP124 format
	
	mybat3->PWM1en   = reply->PWM1en?ON:OFF;
	mybat3->PWM2en   = reply->PWM2en?ON:OFF;
	mybat3->offsetEn = reply->_offsetEn?OFF:ON;
	mybat3->opampEn  = reply->opampEn?ON:OFF;
	mybat3->buckEn   = reply->_buckEn?ON:OFF;
	mybat3->led      = reply->_led?OFF:ON;
	mybat3->jp3      = reply->_jp3?OFF:ON;
	mybat3->batRun   = reply->batRun?ON:OFF;
	mybat3->softJP3  = reply->softJP3?ON:OFF;
	
	mybat3->ee_addr  = reply->ee_addr;
	mybat3->ee_data  = reply->ee_data;
	mybat3->ee_read  = reply->ee_read?ON:OFF;
	mybat3->ee_write = reply->ee_write?ON:OFF;
	mybat3->ee_ready = reply->ee_ready?ON:OFF;
	
	 
	// TODO: this should be done nicer
	print_bat3(logfile?logfile->_fileno:-1, mybat3);
	
	return 0;
	
}

// converts struct bat3 to outgoing msg, returns size
int encodemsg(char *msg, int size, struct bat3* mybat3) {
	
	struct BAT3request *req = (struct BAT3request *) msg;
	
	if (size<sizeof(struct BAT3request)) {
		logabba(L_INFO, "Encode msg: wrong message size: %d", size);
		return 1;
	}
	
	req->alarm=0; // Wondering what this is for
	
	req->PWM1en    = mybat3->PWM1en==ON;
	req->PWM2en    = mybat3->PWM2en==ON;
	req->_offsetEn = mybat3->offsetEn==OFF;
	req->opampEn   = mybat3->opampEn==ON;
	req->_buckEn   = mybat3->buckEn==OFF;
	req->_led      = mybat3->led==OFF;
	req->_jp3      = mybat3->jp3==OFF;
	
	req->batRun    = mybat3->batRun==ON;
	
	req->ee_read   = mybat3->ee_read==ON;
	req->ee_write  = mybat3->ee_write==ON;
	req->ee_ready  = mybat3->ee_ready==ON;
	
	req->softJP3   = mybat3->softJP3==ON;
	
	req->pwm_lo    = mybat3->pwm_lo;
	req->pwm_t     = mybat3->pwm_t;
	
	req->ee_addr   = mybat3->ee_addr;
	req->ee_data   = mybat3->ee_data;
	
	// logabba(L_MIN, "req->ee_addr = %04X, req->ee_data = %04X", req->ee_addr, req->ee_data);
	logabba(L_MAX, "Encodemsg: req->_led = %i, mybat3->led = %s", req->_led, mybat3->led==ON?"ON":"OFF");
	
	return (sizeof(struct BAT3request));
	
}
