/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#include "convert.h"
#include "log.h"
#include "tsbat3.h"

#include "stdio.h" 
 
float adcV(unsigned short ticks)
{
/* According to http://tech.groups.yahoo.com/group/ts-7000/message/5402
These inputs are coming directly from an ADC. To get the ADC
 output the
 supply voltage is multiplied by ~0.07, then divided by 3.3V, then
 multiplied
 by 65536. This corresponds to approximately 700mV per unit, so 39812 *
 700mV is around 28V. The regulated value is approximately 190mV per
 unit,
 so 26623 * 190mV is right around 5V. The values are only
 approximate due to
 part tolerances and lack of ADC calibration.
 */
 
 // return (ticks * 0.07 / 3.3 * 65536);
 return (ticks * 0.700);
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

unsigned short Ibatt(int current)
{
	return (current/60)+29000;
}

// Converts TEMP124 signal to Celsius
float tempC(short int tmp) 
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

static void print_BAT3reply(struct BAT3reply* reply)
{
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

static void print_bat3(struct bat3* mybat3)
{

	printf("input supply voltage     = %04X (%3.2fV)\n",  mybat3->inp_u, 0.70/1000 * (mybat3->inp_u)); // 
	printf("regulated supply voltage = %04X (%3.2fV)\n",  mybat3->reg_u, 0.19/1000 * (mybat3->reg_u)); // 
	printf("battery voltage          = %04X (%3.2fV)\n",  mybat3->bat_u, battV(mybat3->bat_u)); // 
	printf("battery current          = %04X (%3.2fmA)\n", mybat3->bat_i, battI(mybat3->bat_i)/1000.0); // 
	printf("low time                 = %04X\n", mybat3->pwm_lo); // 
	printf("high & low time          = %04X\n", mybat3->pwm_t); // 
	printf("temperature              = %04X\n", mybat3->temp); //  in TMP124 format
	printf("EEPROM address           = %04X\n", mybat3->ee_addr);
	printf("EEPROM data              = %04X\n", mybat3->ee_data);

	printf("PWM1en   = %s\n",print_onoff(mybat3->PWM1en));
	printf("PWM2en   = %s\n",print_onoff(mybat3->PWM2en));
	printf("offsetEn = %s\n",print_onoff(mybat3->offsetEn));
	printf("opampEn  = %s\n",print_onoff(mybat3->opampEn));
	printf("buckEn   = %s\n",print_onoff(mybat3->buckEn));
	printf("led      = %s\n",print_onoff(mybat3->led));
	printf("jp3      = %s\n",print_onoff(mybat3->jp3));
	printf("batRun   = %s\n",print_onoff(mybat3->batRun));
	printf("ee_read  = %s\n",print_onoff(mybat3->ee_read));
	printf("ee_write = %s\n",print_onoff(mybat3->ee_write));
	printf("ee_ready = %s\n",print_onoff(mybat3->ee_ready));
	printf("softJP3  = %s\n",print_onoff(mybat3->softJP3));

}

// Converts an incomming msg to struct bat3
int decodemsg(char *msg, int size, struct bat3* mybat3) 
{
	struct BAT3reply *reply;

	if (size!=sizeof(struct BAT3reply)) {
		logabba(L_MIN,"Decode msg: wrong message size: %d != %d", size, sizeof(struct BAT3reply));
		// TODO: check if this is still needed
		// return 1;
	}

	reply = (struct BAT3reply*) msg;

	// print_BAT3reply(reply);
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
	mybat3->ee_read  = reply->ee_read;
	mybat3->ee_write = reply->ee_write;
	mybat3->ee_ready = reply->ee_ready;	
	
	print_bat3(mybat3);
	
	return 0;
	
}

// converts struct bat3 to outgoing msg, returns size
int encodemsg(char *msg, int size, struct bat3* mybat3) 
{

	struct BAT3request *req = (struct BAT3request *) msg;
	
	if (size<sizeof(struct BAT3request)) {
		logabba(L_INFO,"Encode msg: wrong message size: %d",size);
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
	
	logabba(L_MAX, "Encodemsg: req->_led = %i, mybat3->led = %s",req->_led, mybat3->led==ON?"ON":"OFF");
	
	return (sizeof(struct BAT3request));
	
}
