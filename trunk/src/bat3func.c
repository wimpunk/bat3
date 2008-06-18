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

    state->pwm_lo = newpwm;

}

static void process_first_degree(struct action* target, int newval) {

    int pos;
    float newweight;

    // calculate the new weight
    logabba(L_INFO, "process will add avg=%d", newval);

    if (!(target->weightpos)) {
        newweight = newval;
    } else {
        newweight =
                1.00 * newval * (1 - FILTER) +
                (target->weight[(target->weightpos) - 1]) * (FILTER);
        logabba(L_NOTICE, "New weight (pos=%d): %d*(1-FILTER)+(%0.2f*(FILTER))=%0.2f",
                (target->weightpos) - 1,
                newval,
                target->weight[(target->weightpos) - 1],
                newweight);

    }

    // this should be the new position
    // but we need to verify if there's still if there's  still room.
    target->weightpos += 1;
    if ((target->weightpos) == MAXMIN) {
        // the end, shift the values!
        for (pos = 1; pos < MAXMIN; pos++) target->weight[pos - 1] = target->weight[pos];
        target->weightpos -= 1;
    }

    // bring them together: new value on the correct place
    logabba(L_INFO, "New weight: %0.2f", newweight);
    target->weight[target->weightpos - 1] = newweight;

}

int check_stable(struct action *target) {
    // checks if we're loading stable
    float curr, prev, diff;

    // preventing buffer overflow
    if (target->weightpos < 2) return 0;

    curr = target->weight[(target->weightpos) - 1];
    prev = target->weight[(target->weightpos) - 2];

    diff = curr - prev;
    logabba(L_NOTICE, "Difference in weight (weightpos=%d): %0.2f - %0.2f = %0.2f",
            target->weightpos, curr, prev, diff);
    if ((diff > -1) && (diff < 1)) {
        // running stable
        if (target->stable_time == 0) {
            logabba(L_MIN, "Starting to run almost stable now, waiting 15min");
            time(&(target->stable_time));
            return 0;
        } else {
            if ((diff = time(NULL) - target->stable_time) > 15 * 60) { // 15 times 60 seconds
                if ((diff < 15 * 60 + 2))
                    logabba(L_MIN, "Running stable now for at least 15 min");
                return 1;
            }
            return 0;
        }

    } else {
        // not running stable
        target->stable_time = 0;
        return 0;
    }
}

void doload(struct bat3* state, struct action* target) {

    int minpos;
    int stable = 0;

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
            // reset the alarm;
            time(&(target->alarm));
            target->alarm += 1; // next second we will record again
            if (!check_stable(target)) {
                // If previous round didn't give us a stable result we try again
                // otherwise we should wait until time has passed
                process_first_degree(target, state->bat_u);
            }
        }

        if (check_stable(target)) {
            logabba(L_MIN, "Seem to be stable, switching off");
            switch_opamp(state, OFF);
        } else {
            calc_load(state, target);

            if (state->opampEn == OFF) logabba(L_MIN, "I start loading");
            switch_opamp(state, ON); // should only be done if target!=0

            state->buckEn = OFF;
            state->offsetEn = OFF;
            state->pwm_t = MAX_PWM_T;
        }

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
