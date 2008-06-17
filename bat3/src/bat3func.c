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

#define MAX_PWM_T 0x0200   // max pwm_t
#define MIN_PWM_T 0x0100   // min pwm_min with default current
#define DEFTARGET    400   // default target current

struct bat3 curState;

typedef enum {
    WRITE,
    LISTEN,
    DONE
} romstate_t;
romstate_t romstate = WRITE;

void changeled(struct bat3* state) {
    if (state->led == ON) {
        logabba(L_NOTICE, "changeled: switching LED OFF");
        state->led = OFF;
    } else {
        logabba(L_NOTICE, "changeled: switching LED ON");
        state->led = ON;

    }
}

static void switch_opamp(struct bat3* state, onoff_t sw) {

    state->opampEn = sw;
    state->PWM1en = sw;
    state->PWM2en = sw;

}

static void reset_history(struct action* target) {
    // reset history timers
    target->alarm = 0;
    target->stable_time = 0;
    target->seccnt = 0;
    target->mincnt = 0;
    target->weightpos = 0;
}

static int calc_load(struct bat3* state, struct action* target) {

    int diff;
    int newpwm;

    diff = target->current - battI(state->bat_i) / 1000; // difference between wanted & current

    // based on the current version
    newpwm = state->pwm_lo;

    if (diff > 100) {
        // newpwm -= pwmdiff);
        newpwm -= 10;
        logabba(L_NOTICE, "Doload: diff>100, newpwm = %04X", newpwm);
    } else if (diff<-100) {
        // newpwm += pwmdiff;
        newpwm += 10;
        logabba(L_NOTICE, "Doload: diff<-100, newpwm = %04X", newpwm);
    } else if (diff > 50) {
        newpwm -= 1;
    } else if (diff<-50) {
        newpwm += 1;
    }

    // preventing going to low and overcharge battery.
    if (target->current == DEFTARGET) {
        if (newpwm < MIN_PWM_T) {

            newpwm = MIN_PWM_T;
        }
    } else {
        // this one is dangerous
        if (newpwm < 0) {
            newpwm = 0;

        }

    }

    // you can't go highter than the top
    if (newpwm > state->pwm_t) newpwm = state->pwm_t;

    logabba(L_INFO, "Changing pwm_lo from %04X to %04X", state->pwm_lo, newpwm);
    logabba(L_MIN, "Changing pwm_lo from %04X to %04X", state->pwm_lo, newpwm);

    state->pwm_lo = newpwm;

}

static float average(int cnt, int *a) {
    int i;
    int total = 0;

    for (i = 0; i < cnt; i++, a++) total += *a;

    return (1.0)*total / cnt;
}

static void process_first_degree(struct action* target) {

    int pos;
    float newweight;

    // calculate the new weight
    if (!(target->weightpos)) {
        newweight = target->min[(target->mincnt) - 1];
    } else {
        // TODO: is this -2 always correct?
        newweight =
                target->min[(target->mincnt) - 1]*(1 - FILTER) +
                target->min[(target->mincnt) - 2]*FILTER;
    }
    
    // this should be the new position
    // but we need to verify if there's still if there's  still room.
    target->weightpos+=1;
    if ((target->weightpos)==MAXMIN) {
        // the end, shift the values!
        for (pos=1; pos<MAXMIN; pos++) target->weight[pos-1] = target->weight[pos];
        target->weightpos -= 1;                
    }
    
    // bring them together: new value on the correct place
    target->weight[target->weightpos] = newweight;

}

void doload(struct bat3* state, struct action* target) {

    int minpos;

    if (state->batRun == ON) {
        // Running on batteries
        logabba(L_NOTICE, "Running on batt...");

        if (state->softJP3 == ON) {
            logabba(L_MIN, "Batrun, switching softJP3 OFF, softjp3=%s",
                    print_onoff(state->softJP3));
            state->softJP3 = OFF;
        }

        switch_opamp(state, OFF);
        state->buckEn = OFF;
        state->offsetEn = OFF;
        state->updated = time(NULL);

        reset_history(target);

    } else { // Not running on batteries

        logabba(L_NOTICE, "NOT running on batt...");

        if (state->softJP3 == ON) {
            logabba(L_MIN, "No batrun, Switching softJP3 OFF, softjp3=%s",
                    print_onoff(state->softJP3));
            state->softJP3 = OFF;
        }

        if (target->alarm <= time(NULL)) { // time to record the setting
            // logabba(L_MIN, "Alarm has expired");
            // reset the alarm;
            time(&(target->alarm));
            target->alarm += 1; // next second we will record again

            logabba(L_MIN, "Filing second %d", target->seccnt);
            target->sec[(target->seccnt)++] = state->bat_u;

            if (target->seccnt >= MAXSEC) {
                logabba(L_MIN, "More than MAXSEC=%d, filling min", MAXSEC);
                // find the new position
                minpos = target->mincnt;
                if (minpos < MAXMIN) { 
                    target->mincnt++;
                } else { // oops, the end, shift the values!
                    int i;
                    
                    for (i=1; i<MAXMIN; i++) {
                        target->min[i-1] = target->min[i];
                    }
                    minpos-=1;
                } 
                logabba(L_MIN, "More than MAXSEC=%d, filling minpos=%d", MAXSEC, minpos);
                
                target->min[minpos] =
                        average(target->seccnt, target->sec);
                target->seccnt = 0;
                logabba(L_MIN, "More than MAXSEC=%d, filed minpos=%d with avg=%d", MAXSEC, minpos, target->min[minpos]);
                // we need more than 1 value
                if (minpos>1) process_first_degree(target);
            }

        }
        
        calc_load(state, target);

        if (state->opampEn == OFF) logabba(L_MIN, "I start loading");
        switch_opamp(state, ON); // should only be done if target!=0

        state->buckEn = OFF;
        state->offsetEn = OFF;
        state->pwm_t = MAX_PWM_T;

    } 
    
}

void doread(struct bat3* state, int address) {

    logabba(L_MIN, "doread: reading from address = %04X", address);

    if (romstate == WRITE) {
        if ((state->ee_addr == address) && (state->ee_read == ON)) {
            romstate = LISTEN;
        } else {
            state->ee_addr = address;
            state->ee_read = ON;
            state->ee_write = OFF;
            state->ee_data = 0x00; // don't know if we need this
        }
    }

    if (romstate == LISTEN) {
        if ((state->ee_addr == address) && (state->ee_read == OFF)) {
            romstate = DONE;
        } else {
            state->ee_addr = address;
            state->ee_read = OFF;
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
            state->ee_addr = address;
            state->ee_read = OFF;
            state->ee_write = ON;
            state->ee_data = value; // don't know if we need this
        }
    }

    if (romstate == LISTEN) {
        if ((state->ee_addr == address) && (state->ee_write == OFF)) {
            romstate = DONE;
        } else {
            state->ee_addr = address;
            state->ee_read = OFF;
            state->ee_write = OFF;
        }
    }

    logabba(L_MIN, "dowrite: addr=%04X, read=%s, write=%s, data=%04X",
            state->ee_addr, print_onoff(state->ee_read), print_onoff(state->ee_write), state->ee_data);


}

void setBatState(struct bat3* newState) {
    curState = *newState;
}

onoff_t getBatRun() {

    return curState.batRun;
}

int getBatI() {
    return curState.bat_i;
}

int getAddress() {
    return -1;
}

struct bat3* getState() {
    return &curState;
}
