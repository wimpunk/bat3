#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

#include "log.h"
#include "stream.h"
#include "bat3.h"
#include "convert.h"
#include "bat3func.h"
#include "mysocket.h"

#define BAT3DEV "/dev/ttyTS0"
#define DEFAULT_LOGLEVEL L_STD
#define DEFAULT_SAMPLES  10
#define DEFAULT_CURRENT  400
#define DEFAULT_ADDRESS  0x80
#define DEFAULT_VALUE    0xFF
#define DEFAULT_PORT     1302
#define MAX_RETRIES 5

int current  = DEFAULT_CURRENT;

typedef enum {
	E_OKAY,
	E_DEVICE,
	E_SAMPLE
} errors;

char *print_onoff(onoff_t onoff) {
	if (onoff == ON) {
		return "ON";
	} else {
		return "OFF";
	}
	return NULL;
}

int doRound(int fd, struct bat3 *sample, int current, FILE *logfile);

int setCurrent(int i) {
	
	if ((i<0) || (i>2000)) current = DEFAULT_CURRENT;
	else current = i;
	
	return current;
	
}

int getCurrent() {
	
	return current;
	
}

int setLoglevel(int i) {
	setloglevel(i, "bat3");
	
	return i;
	
}

int getLoglevel() {
	
	return getloglevel();
	
}

static int getsample(int fd, struct bat3 *sample, FILE *logfile) {
	
	struct bat3 temp;
	int cnt;
	char msg[56];
	int error=0;
	int retry=1;
	
	
	while (error<MAX_RETRIES && retry) {
		
		if (!(cnt=readStream(fd, msg, sizeof(msg)))) {
			logabba(L_NOTICE, "Could not readStream, error=%d, cnt=%d", error, cnt);
			error++;
			continue;
		}
		
		if (decodemsg(msg, cnt, &temp, logfile)!=0) {
			logabba(L_MIN, "Could not decodemsg, error=%d", error);
			error++;
			continue;
		}
		
		retry=0;
		
	}
	
	if (!retry) {
		logabba(L_INFO, "Good sample after error=%d tries ", error);
		*sample = temp;
		return 1;
	}
	
	logabba(L_MIN, "Could not get a sample after error=%d tries ", error);     ;
	
	return 0;
	
}

static void usage(char *progname) {
	
	printf("\n");
	printf("Usage:\n");
	printf("\n");
	printf("   %s [-c current][-d device][-l loglevel][-r][-s samples][-w value]\n", progname);
	printf("      -a address: address to use while reading/writing, default %i\n", DEFAULT_ADDRESS);
	printf("      -c current: current to load (mA), default %i\n", DEFAULT_CURRENT);
	printf("      -d device: device to use, default %s\n", BAT3DEV );
	printf("      -f logfile: file to dump arrays to, default /dev/null\n");
	printf("      -l loglevel: loglevel to use, default %i\n", DEFAULT_LOGLEVEL );
	printf("      -p portnr: portnumber to listen to, default %i\n", DEFAULT_PORT);
	printf("      -r : read from [address]\n");
	printf("      -s samples: maxsamples to use, default %i\n", DEFAULT_SAMPLES );
	printf("      -w value: write value to [address], default %i\n", DEFAULT_VALUE);
	printf("\n");
	printf("(bat3 release %s-r%s)\n", VERSION, REV);
	printf("\n");
	
}

static void initParams(struct action *params) {
	// init params
	params->address  = DEFAULT_ADDRESS;
	params->current  = DEFAULT_CURRENT;
	strncpy(params->device, BAT3DEV, sizeof(BAT3DEV));
	params->read		= 0;
	params->write	= 0;
	params->value	= 0;
	params->logfile	= NULL;
	params->loglevel = DEFAULT_LOGLEVEL;
	params->samples  = DEFAULT_SAMPLES;
	params->portno	= DEFAULT_PORT;
}

static int processArguments(int argc, char **argv, struct action *params) {
	
	int c;
	int errcnt=0;
	
	while ((c=getopt(argc, argv, "a:c:d:f:h?l:p:rs:w:"))!=EOF) {
		switch (c) {
			case 'a': // address
				params->address=atoi(optarg);
				break;
			case 'c': // current
				setCurrent(atoi(optarg));
				break;
			case 'd': // device
				strncpy(params->device, optarg, sizeof(params->device)-1);
				printf("I'll read device %s\n", params->device);
				break;
			case 'f': // logfile
				params->logfile = fopen(optarg, "w");
				if (params->logfile == NULL) {
					fprintf(stderr, "Error opening logfile %s: %s\n", optarg, strerror(errno));
				}
			case 'l': // loglevel
				params->loglevel = atoi(optarg);
				break;
			case 'p': // portnr to listen to
				params->portno = atoi(optarg);
				break;
			case 'r': // read
				params->read=1;
				break;
			case 's': // samples
				params->samples = atoi(optarg);
				break;
			case 'w':
				params->write = 1;
				params->value = atoi(optarg);
				break;
				
			case '?':
			case 'h':
			default:
				errcnt++;
				
		}
		if (errcnt) break;
	}
	
	if (errcnt) {
		usage(argv[0]);
		return(1);
	}
	
	return 0;
	
}

int runArgument(struct action *params) {
	
	
	int				fd;
	int				FlushTime = 20 * 1000; // 20 characters @ 9600bps
	fd_set			rfds;
	struct timeval  tv;
	int				sw_continue = 1;
	struct timeval  now, lastrun;
	struct bat3		state, prevstate;
	int				cntsamples=0;
	int				socketfd = 0;
	
	if ((fd = openStream(params->device)) == -1) {
		logabba(L_MIN, "Could not open device %s", params->device);
		return E_DEVICE;
	}
	
	// initialising
	// we need an inital value so we get a sample
	if (!getsample(fd, &state, params->logfile)) {
		logabba(L_MIN, "Did not get a sample");
		return E_SAMPLE;
	}
	
	
	// ignoring broken pipes
	signal(SIGPIPE, SIG_IGN);
	
	
	while (((cntsamples<params->samples) || (params->samples == -1)) && (sw_continue))	{
		
		FD_ZERO( &rfds );
		FD_SET( fd, &rfds );
		
		tv.tv_sec  = FlushTime/1000000;
		tv.tv_usec = FlushTime%1000000;
		
		int val;
		
		if (select( FD_SETSIZE, &rfds, NULL, NULL, &tv ) < 0) {
			fprintf(stderr, "Select error: %s\n", strerror(errno));
			return -1;
		}
		
		if (FD_ISSET(fd, &rfds)) {
			
			if (params->read) doread(&state, params->address);
			if (params->write) dowrite(&state, params->address, params->value);
			
			gettimeofday(&lastrun, NULL); //needed to calculate the wait time
			
			// TODO: we should verify the value of doRound.  When it returns
			// -1 it can't communicate with the TS-BAT3
			doRound(fd, &state, current, params->logfile);
			if (!((params->samples == -1) && (cntsamples>10000))) cntsamples++;
			
			
			if (params->read && cntsamples==3) {
				logabba(L_MIN, "Reading from address %i: %04X=%04X",
						params->address, state.ee_addr, state.ee_data);
			}
			
			// checking switched states
			if (state.batRun != prevstate.batRun) {
				logabba(L_MIN, "Running on %s",
						state.batRun==ON?"batteries":"current" );
			}
			
			if (state.softJP3 != prevstate.softJP3) {
				logabba(L_MIN, "Soft JP3 switched to %s after %i samples",
						print_onoff(state.softJP3), cntsamples );
			}
			
			// setting waittime for select
			tv.tv_sec  = FlushTime/1000000;
			tv.tv_usec = FlushTime%1000000;
			
		}
		
		
		prevstate = state;
		setBatState(&state);
		
		// Processing socket connections
		if (sw_continue) {
			sw_continue = MYSOCK_END != processMySocket(socketfd);
		}
		
		gettimeofday(&now, NULL);
		
		val = 25 * 1000 - ( now.tv_sec - lastrun.tv_sec ) * 1000 * 1000 -  ( now.tv_usec - lastrun.tv_usec );
		if (val <= 0 ) val = 0;
		usleep(val);
		
	}
	
	closeMySocket(socketfd);
	close(fd);
	
	return E_OKAY;
}

/*
 * doRound creates a structure based on the current state found in sample
 * and writes the result to the TS-BAT3 which should return a new sample.
 * TODO: it should also contain the reading and writing to eeprom.
 */
int doRound(int fd, struct bat3 *sample, int current, FILE *logfile) {
	
	struct bat3 state = *sample;
	int cnt;
	char msg[56];
	
	
	logabba(L_INFO, "Original led: %s", print_onoff(state.led));
	// Even when running on batteries, we have to kietel the opamp
	// TODO: is this still correct?  I think we just have to write
	// some info to the battery within 2 minutes and wait for reaction.
	if (state.batRun == ON) {
		state.led = OFF;
		doload(&state, current);
	} else {
		changeled(&state);
		// TODO: we should pass more info about the current loading state
		// to the battery
		doload(&state, current);
		logabba(L_NOTICE, "Switching led to %s", print_onoff(state.led));
	}
	
	logabba(L_INFO, "will encode led: %s", print_onoff(state.led));
	
	if ((cnt = encodemsg(msg, sizeof(msg), &state))) {
		logabba(L_INFO, "Writing msg");
		writeStream(fd, msg, cnt);
	}
	
	cnt=0;
	
	while (!getsample(fd, &state, logfile) && cnt<MAX_RETRIES) {
		cnt++;
	}
	
	if (cnt>=MAX_RETRIES) {
		logabba(L_MIN, "Did not get a sample even after retrying %d times", cnt);
		return -1;
	}
	
	logabba(L_INFO, "got state led: %s", print_onoff(state.led));
	*sample = state;
	logabba(L_INFO, "got sample led: %s", print_onoff(sample->led));
	
	return 0;
	
}

int main(int argc, char *argv[]) {
	
	struct action	params;	
	
	initMySocket();
	initParams(&params);
	processArguments(argc, argv, &params);
	setPortno(params.portno);
	
	if ((params.samples<-1)) params.samples = DEFAULT_SAMPLES;
	
	setloglevel(params.loglevel, "bat3");
	
	logabba(L_MIN, "%s (Ver %s) started, loglevel %i, getting %d samples, using %imA to load, listening to port %d",
			argv[0], VERSION, params.loglevel, params.samples, params.current, params.portno);
	
	runArgument(&params);
	
	
	
	return 0;
	
}

// vim:set autoindent:
