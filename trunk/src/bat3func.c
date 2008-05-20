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
 TODO Need 2 find a solution for this
 */

struct bat3 curState;


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
    
    int diff;
    int newpwm;
    int pwmdiff;
    
    
    if (state->batRun == ON) {
	if (state->softJP3 == ON) {
	    // TODO: I think I'm some bytes somewhere.  I always have to set this byte
	    // logabba(L_MIN, "Switching softJP3 OFF, softjp3=%s",print_onoff(state->softJP3));
	    state->softJP3 = OFF;
	}
	logabba(L_INFO, "Running on batt...");
	// state->opampEn  = OFF;  // i think this halts the battery function
	/* Currently the system stops when running on batteries... we'll remove this lines
	
	 state->opampEn  = OFF;
	 state->PWM1en   = OFF;
	 state->PWM2en   = OFF;
	 */
	state->opampEn  = ON;
	// state->PWM1en   = OFF;
	// state->PWM2en   = OFF;
	state->buckEn = OFF;
	state->offsetEn = OFF;
	
	
    } else {
	
	if (state->softJP3 == ON) {
	    // logabba(L_MIN, "Switching softJP3 OFF, softjp3=%s",print_onoff(state->softJP3));
	    state->softJP3 = OFF;
	}
	
	logabba(L_NOTICE, "NOT running on batt...");
	state->opampEn  = ON;
	state->PWM1en   = ON;
	state->PWM2en   = ON;
	state->buckEn   = OFF;
	state->offsetEn = OFF;
	state->pwm_t    = 500;
	
	logabba(L_NOTICE, "Doload: target=%imA, current=%imA" , target, battI(state->bat_i)/1000);
	
	diff = target - battI(state->bat_i)/1000;   // difference between wanted & current
	pwmdiff = (state->pwm_t - state->pwm_lo)/2; // if difference is big enough, we change it logarithmic
	
	newpwm = state->pwm_lo;
	
	if (diff>100) {
	    newpwm -= pwmdiff;
	} else if (diff<-100) {
	    newpwm += pwmdiff;
	} else if (diff>50) {
	    newpwm -= 1;
	} else if (diff<-50) {
	    newpwm += 1;
	}
	
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
    
    logabba(L_MIN, "doread: addr=%04X, read=%s, write=%s, data=%04X",
    state->ee_addr, print_onoff(state->ee_read), print_onoff(state->ee_write), state->ee_data);
    
    
}

void dowrite(struct bat3* state, int address, int value) {
    
    logabba(L_MIN, "dowrite: writing %04X to address = %04X", value, address);
    
    
    if (romstate == WRITE) {
	if ((state->ee_addr == address) && (state->ee_write == ON)) {
	    romstate = LISTEN;
	} else {
	    state->ee_addr  = address;
	    state->ee_read  = OFF ;
	    state->ee_write = ON;
	    state->ee_data  = value; // don't know if we need this
	}
    }
    
    if (romstate == LISTEN) {
	if ((state->ee_addr == address) && (state->ee_write == OFF)) {
	    romstate = DONE;
	} else {
	    state->ee_addr  = address;
	    state->ee_read  = OFF;
	    state->ee_write = OFF;
	}
    }
    
    logabba(L_MIN, "dowrite: addr=%04X, read=%s, write=%s, data=%04X",
    state->ee_addr, print_onoff(state->ee_read), print_onoff(state->ee_write), state->ee_data);
    
    
}

void setBatState(struct bat3* newState)
{
    curState = *newState;
}

onoff_t getBatRun()
{
    
    return curState.batRun;
}

int getBatI()
{
    return curState.bat_i;
}

int getAddress()
{
    return -1;
}
