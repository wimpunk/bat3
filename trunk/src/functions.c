/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * This file is intended to contain all the possible functions of the bat3
 * */

void changeled(struct bat3* state) {
	if (state->led == ON) {
		logabba(L_MAX, "changeled: switching LED OFF");
		state->led = OFF;
	} else {
		logabba(L_MAX, "changeled: switching LED ON");
		state->led = ON;
		
	}
}

void doload(struct bat3* state) {

	// OP AMP must be enabled for charging to work
	state->opampEn  = ON;
	state->PWM1en   = ON;
	state->PWM2en   = ON;
	state->buckEn   = OFF;
	state->offsetEn = OFF;
	state->pwm_lo   -= 0x10;
	state->pwm_t    = 500;
}
 
