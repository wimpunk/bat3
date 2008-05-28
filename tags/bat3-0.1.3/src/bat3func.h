/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

void changeled(struct bat3* state);
void doload(struct bat3* state, int target); 
void doread(struct bat3* state, int address);
void dowrite(struct bat3* state, int address, int value);

void setBatState(struct bat3* newState);
int getBatI();
struct bat3* getState();	
onoff_t getBatRun();
