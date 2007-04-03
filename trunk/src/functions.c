/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * This file is intended to contain all the possible functions of the bat3
 * */

#include "bat3.h"
#include "log.h"

void changeled(struct bat3* state) {
    if (state->led == ON) {
	logabba(L_NOTICE, "changeled: switching LED OFF");
	state->led = OFF;
    } else {
	logabba(L_NOTICE, "changeled: switching LED ON");
	state->led = ON;
	
    }
}

void doload(struct bat3* state, int target) {
    
    // OP AMP must be enabled for charging to work
    if (state->batRun == ON) {
	logabba(L_NOTICE, "Running on batt...");
	state->opampEn  = OFF;
	state->PWM1en   = OFF;
	state->PWM2en   = OFF;
	
    } else {
	logabba(L_NOTICE, "NOT running on batt...");
	state->opampEn  = ON;
	state->PWM1en   = ON;
	state->PWM2en   = ON;
	
	state->buckEn   = OFF;
	state->offsetEn = OFF;
	// 	state->pwm_lo   -= 0x10;
	state->pwm_t    = 500;
	
	int diff;
	int newpwm;
	int mod;
	
	logabba(L_NOTICE, "Doload: target=%imA, current=%imA" , target, battI(state->bat_i)/1000);
	
	// diff = target - battI(state->bat_i)/10000;
	diff = target - battI(state->bat_i)/1000;
	
	// mod  = (abs(target*1000 - (state->bat_i) - 30000) / 2);
	
	/* if diff>0: to less current, lesser pwm_lo */
	
	// if (diff>50) newpwm = state->pwm_lo-1;
	
	newpwm = state->pwm_lo;
	
	if (diff>50) {
	    newpwm -= 1;
	} else if (diff<-50) {
	    newpwm += 1;
	}
	
	// logabba(L_MIN, "Would add %04X ticks", (diff*100/12));
		
	if (newpwm<0) newpwm = 0;
	if (newpwm>state->pwm_t) newpwm = state->pwm_t;
	
	logabba(L_INFO,  "Changing pwm_lo from %04X to %04X, mychg = %04X", state->pwm_lo, newpwm);
	state->pwm_lo = newpwm;
	
    }
}

