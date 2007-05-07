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
#include "bat3func.h"

#include "log.h"
#include "convert.h"

/*
 TODO Need 2 fid a solution for this
 */


typedef enum  {
    WRITE,
    LISTEN,
    DONE
} romstate_t;
romstate_t romstate=WRITE;


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
	// state->opampEn  = OFF;  // i think this halts the battery function
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

void doread(struct bat3* state, int address) {
    
    logabba(L_MIN, "doread: reading from address = %04X", address);
    
    if (romstate == WRITE) {
	if ((state->ee_addr == address) && (state->ee_read == ON)) {
	    romstate = LISTEN;
	} else {
	    state->ee_addr  = address;
	    state->ee_read  = ON ;
	    state->ee_write = OFF;
	    state->ee_data  = 0x00; // don't know if we need this
	}
    }
    
    if (romstate == LISTEN) {
	if ((state->ee_addr == address) && (state->ee_read == OFF)) {
	    romstate = DONE;
	} else {
	    state->ee_addr  = address;
	    state->ee_read  = OFF;
	    state->ee_write = OFF;
	}
    }
    
    logabba(L_MIN, "addr=%04X, read=%s, write=%s, data=%04X",
	state->ee_addr, print_onoff(state->ee_read), print_onoff(state->ee_write), state->ee_data);
    
    
}


