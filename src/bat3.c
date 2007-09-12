// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
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
#include "mysocket.h"

// #include "tsbat3.h"

#define BAT3DEV "/dev/ttyTS0"
#define DEFAULT_LOGLEVEL L_STD
#define DEFAULT_SAMPLES  10
#define DEFAULT_CURRENT  400
#define DEFAULT_ADDRESS  0x80
#define DEFAULT_VALUE    0xFF
#define DEFAULT_PORT     1302
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
	
	// if (logfile == NULL ) logabba(L_MIN, "Logfile == NULL");
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
    printf("      -f logfile: file to dump arrays to, default /dev/null\n   ");
    printf("      -l loglevel: loglevel to use, default %i\n", DEFAULT_LOGLEVEL );
    printf("      -p portnr: portnumber to listen to, default %i\n", DEFAULT_PORT);
    printf("      -r : read from [address]\n");
    printf("      -s samples: maxsamples to use, default %i\n", DEFAULT_SAMPLES );
    printf("      -w value: write value to [address], default %i\n", DEFAULT_VALUE);
    printf("\n");
    printf("(bat3 revision $Rev$)\n");
    printf("\n");
}

/*
 static void flush(int fd) {
 int cnt=0;
 char c;
 while ((read(fd, &c, 1))>0) cnt++;
 logabba(L_MAX, "Flush was reading %d bytes", cnt);
 }
 */


int doRound(int fd, struct bat3 *sample, int current, FILE *logfile) {
    
    struct bat3 state = *sample;
    int cnt;
    char msg[56];
    // while ((cntsamples<samples) || (samples == -1))	{
    
    // TODO: quick hack
    // if ((cntsamples == 0))
    state.softJP3 = ON;
    
    logabba(L_NOTICE, "Original led: %s", print_onoff(state.led));
    // Even when running on batteries, we have to kietel the opamp
    if (state.batRun == ON) {
	state.led = OFF;
	doload(&state, current);
    } else {
	changeled(&state);
	doload(&state, current);
	logabba(L_MAX, "Switching led to %s", print_onoff(state.led));
    }
    logabba(L_NOTICE, "will encode led: %s", print_onoff(state.led));
    if ((cnt = encodemsg(msg, sizeof(msg), &state))) {
	logabba(L_INFO, "Writing msg");
	writeStream(fd, msg, cnt);
    }
    
    cnt=0;
    
    //    prevstate = state;
    while (!getsample(fd, &state, logfile) && cnt<MAX_RETRIES) {
	cnt++;
    }
    
    if (cnt>=MAX_RETRIES) {
	logabba(L_MIN, "Did not get a sample even after retrying %d times", cnt);
	return -1;
    }
        logabba(L_NOTICE, "got state led: %s", print_onoff(state.led));
    *sample = state;
    logabba(L_NOTICE, "got sample led: %s", print_onoff(sample->led));
    
    // usleep(100);
    
    return 0;
    
}

int main(int argc, char *argv[]) {
    int c, fd;
    char device[50]=""; // device to use
    
    int errcnt=0;
    
    //    int cnt,
    int cntsamples;
    
    int loglevel = DEFAULT_LOGLEVEL;
    int samples  = DEFAULT_SAMPLES;
    int current  = DEFAULT_CURRENT;
    int address  = DEFAULT_ADDRESS;
    int portno   = DEFAULT_PORT;
    
    
    int read  = 0;
    int write = 0;
    int value = 0;
    
    int socketfd = 0;
    
    FILE *logfile = NULL;
    
    struct bat3 state, prevstate;
    
    strncpy(device, BAT3DEV, sizeof(BAT3DEV));
    
    
    while ((c=getopt(argc, argv, "a:c:d:f:h?l:p:rs:w:"))!=EOF) {
	switch (c) {
	    case 'a': // address
		address=atoi(optarg);
		break;
	    case 'c': // current
		current = atoi(optarg);
		break;
	    case 'd': // device
		strncpy(device, optarg, sizeof(device)-1);
		printf("I'll read device %s\n", device);
		break;
	    case 'f': // logfile
		logfile = fopen(optarg, "a");
		if (logfile == NULL){
		    fprintf(stderr, "Error opening logfile %s: %s", optarg, strerror(errno));
		}
	    case 'l': // loglevel
		loglevel = atoi(optarg);
		break;
	    case 'p': // pipe
		portno = atoi(optarg);
		break;
	    case 'r': // read
		read=1;
		break;
	    case 's': // samples
		samples = atoi(optarg);
		break;
	    case 'w':
		write = 1;
		value = atoi(optarg);
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
    
    
    if ((fd = openStream(device)) == -1) {
	logabba(L_MIN, "Could not open device %s", device);
	return 0;
    }
    
    setPortno(portno);
    
    // fprintf(stdout, "opening port returned %d", socketfd);
    
    
    if ((samples<-1)) samples = DEFAULT_SAMPLES;
    if ((current<0) || (current>1000)) current = DEFAULT_CURRENT;
    
    setloglevel(loglevel, "bat3");
    
    logabba(L_MIN, "%s ($Rev$) started, loglevel %i, getting %d samples, using %imA to load, listening to port %d", argv[0], loglevel, samples, current, portno);
    
    if (!getsample(fd, &state, logfile)) {
	logabba(L_MIN, "Did not get a sample");
	return 0;
    }
    
    cntsamples=0;
    // ignoring broken pipes
    signal(SIGPIPE, SIG_IGN);
    
    int		    FlushTime = 20 * 1000; // 20 characters @ 9600bps
    fd_set	    rfds;
    struct timeval  tv;
    int		    sw_continue = 1;
    struct timeval  now, lastrun;
    
    while (((cntsamples<samples) || (samples == -1)) && (sw_continue))	{
	
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
	    
	    if (read) doread(&state, address);
	    if (write) dowrite(&state, address, value);
	    gettimeofday(&lastrun, NULL);
	    
	    doRound(fd, &state, current, logfile);
	    cntsamples++;
	    if (cntsamples==3 && read) {
		logabba(L_MIN, "Reading from address %i: %04X=%04X", address, state.ee_addr, state.ee_data);
	    }
	    
	    if (state.batRun != prevstate.batRun) {
		logabba(L_MIN, "Running on %s", state.batRun==ON?"batteries":"current" );
	    }
	    
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
	
	// logabba(L_MIN, "Would sleep during %d usec",
	val = 25 * 1000 - ( now.tv_sec - lastrun.tv_sec ) * 1000 * 1000 -  ( now.tv_usec - lastrun.tv_usec );
	if (val <= 0 ) val = 0;
	usleep(val);
	// logabba(L_MIN, "  diff sec: %d", now.tv_sec  - lastrun.tv_sec);
	// logabba(L_MIN, " udiff sec: %d", now.tv_usec - lastrun.tv_usec);
	// usleep(100 * 1000);
	// usleep (20000);
    }
    
    close(fd);
    closeMySocket(socketfd);
    
    return 0;
    
}

// vim:set autoindent:
