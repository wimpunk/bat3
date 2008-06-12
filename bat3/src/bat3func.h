/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#ifndef BAT3FUNC_H
#define BAT3FUNC_H
void changeled(struct bat3* state);

// should be changed by one do-action function
// which uses the current state and modifies it
// to the wanted state
void doload(struct bat3* state, int target); 
void doread(struct bat3* state, int address);
void dowrite(struct bat3* state, int address, int value);

void setBatState(struct bat3* newState);
int getBatI();
struct bat3* getState();	
onoff_t getBatRun();

struct action {
	
/* this should contain the action we want to take:
 * load
 * read
 * write
 */
	int current;
	int read;
	int write;
	int address;
	
};
#endif
