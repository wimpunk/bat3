/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#include "bat3.h"

float battV(unsigned short ticks) ;
int battI(unsigned short ticks); // return uA
float tempC(short int tmp);
int decodemsg(char *msg, int size, struct bat3* mybat3, FILE *logfile);
int encodemsg(char *msg, int size, struct bat3* mybat3);
void print_bat3(int fd, struct bat3* mybat3);
