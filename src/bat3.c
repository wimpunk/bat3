// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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

// this one should be removed.
#include "tsbat3.h"

#define BAT3DEV "/dev/ttyTS0"
#define DEFAULT_LOGLEVEL L_STD
#define DEFAULT_SAMPLES  10
#define DEFAULT_CURRENT  400
#define DEFAULT_ADDRESS  0x80
#define DEFAULT_VALUE    0xFF
#define MAX_RETRIES 5

char *print_onoff(onoff_t onoff) {
    if (onoff == ON) {
	return "ON";
    } else {
	return "OFF";
    }
    return NULL;
}
/*
 void writemsg(int fd, char* msg, int len, int read)
 {


 // OP AMP must be enabled for charging to work
 req.opampEn = 1;
 req._buckEn = 1;
 req._offsetEn = 1;
 req.checksum = calcCrc((char*)&req, sizeof(req)-1);

 cnt = write(fd, (void *)&req, sizeof(req));

 logabba(L_MAX, "Write returned %d", cnt);

 // OP AMP must be enabled for charging to work
 req.opampEn = 1;
 req.PWM1en = 1;
 req.PWM2en = 1;
 req._buckEn = 1;
 req._offsetEn = 1;

 req.outputs = rep->outputs;
 req.alarm   = 0;
 req.softJP3 = 0;
 req.pwm_lo  = rep->pwm_lo;
 req.pwm_t   = rep->pwm_t;


 return;
 }
 */

static int getsample(int fd, struct bat3 *sample) {
    
    struct bat3 temp;
    int cnt;
    char msg[56];
    int error=0;
    int retry=1;
    
    
    while (error<MAX_RETRIES && retry) {
	
	if (!(cnt=readStream(fd, msg, sizeof(msg)))) {
	    logabba(L_MIN, "Could not readStream, error=%d, cnt=%d", error, cnt);
	    error++;
	    continue;
	}
	
	if (decodemsg(msg, cnt, &temp)!=0) {
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
    // TODO: shit opkuisen
    printf("\n");
    printf("Usage\n");
    printf("\n");
    printf("   %s [-c current][-d device][-l loglevel][-r][-s samples][-w value]\n", progname);
    printf("      -a address: address to use while reading/writing, default %i\n", DEFAULT_ADDRESS);
    printf("      -c current: current to load (mA), default %i\n", DEFAULT_CURRENT);
    printf("      -d device: device to use, default %s\n", BAT3DEV );
    printf("      -l loglevel: loglevel to use, default %i\n", DEFAULT_LOGLEVEL );
    printf("      -r : read from [address]");
    printf("      -s samples: maxsamples to use, default %i\n", DEFAULT_SAMPLES );
    printf("      -w value: write value to [address], default %i\n", DEFAULT_VALUE);
    printf("\n");
}


static void flush(int fd) {
    int cnt=0;
    char c;
    while ((read(fd, &c, 1))>0) cnt++;
    logabba(L_MAX, "Flush was reading %d bytes", cnt);
}

int main(int argc, char *argv[]) {
    int c, fd;
    char device[50]=""; // device to use
    int errcnt=0;
    char msg[56];
    int cnt, cntsamples;
    
    int loglevel = DEFAULT_LOGLEVEL;
    int samples  = DEFAULT_SAMPLES;
    int current  = DEFAULT_CURRENT;
    int address  = DEFAULT_ADDRESS;
    int read  = 0;
    int write = 0;
    
    struct bat3 state, prevstate;
    
    strncpy(device, BAT3DEV, sizeof(BAT3DEV));
    
    while ((c=getopt(argc, argv, "a:c:d:h?l:rs:"))!=EOF) {
	switch (c) {
	    case 'a': //address
		address=atoi(optarg);
		break;
	    case 'c': //current
		current = atoi(optarg);
		break;
	    case 'd': //device
		strncpy(device, optarg, sizeof(device)-1);
		printf("I'll read device %s\n", device);
		break;
	    case 'l': //loglevel
		loglevel = atoi(optarg);
		break;
	    case 'r': //read
		read=1;
		break;
	    case 's': //loglevel
		samples = atoi(optarg);
		break;
		
		
		
	    case '?':
	    case 'h':
	    default: errcnt++;
	    break;
	}
	if (errcnt) break;
    }
    
    if (errcnt) {
	usage(argv[0]);
	return(1);
    }
    
    
    if ((fd = openStream(device)) == -1) {
	logabba(L_MIN, "Could not open device %s", device);
	return 0;
    }
    
    
    if ((samples<-1)) samples = DEFAULT_SAMPLES;
    if ((current<0) || (current>1000)) current = DEFAULT_CURRENT;
    if ((loglevel<L_MIN) || (loglevel>L_MAX)) loglevel = DEFAULT_LOGLEVEL;
    
    setloglevel(loglevel, "bat3");
    
    logabba(L_MIN, "%s started, loglevel %i, getting %d samples, using %imA to load", argv[0], loglevel, samples, current);
    
    if (!getsample(fd, &state)) {
	logabba(L_MIN, "Did not get a sample");
	return 0;
    }
    
    cntsamples=0;
    while ((cntsamples<samples) || (samples == -1))	{
	
	if (read) doread(&state, address);
	
	if (state.batRun == ON) {
	    state.led = OFF;
	} else {
	    changeled(&state);
	    doload(&state, current);
	}
	
	
	if ((cnt = encodemsg(msg, sizeof(msg), &state))) writeStream(fd, msg, cnt);
	
	cnt=0;
	
	prevstate = state;
	while (!getsample(fd, &state) && cnt<MAX_RETRIES) {
	    cnt++;
	}
	
	if (cnt>=MAX_RETRIES) {
	    logabba(L_MIN, "Did not get a sample even after retrying %d times", cnt);
	    break;
	}
	
	if (cntsamples==2 && read) {
	    logabba(L_MIN, "Reading from address %i: %04X=%04X", address, state.ee_addr, state.ee_data);
	}

	if (state.batRun != prevstate.batRun) {
	    logabba(L_MIN, "Running on %s",state.batRun==ON?"batteries":"current" );
	}
	
	cntsamples++;
	
    }
    
    close(fd);
    
    return 0;
    
}

// vim:set autoindent
