#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
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

char *print_onoff(onoff_t onoff)
{
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


static void usage(char *progname) 
{
	// TODO: shit opkuisen
	printf("\n");
	printf("Usage\n");
	printf("\n");
	printf("   %s [-d device][-l loglevel]\n",progname);
	printf("      -d device: device to use, default %s\n",BAT3DEV );
	printf("      -l loglevel: loglevel to use, default %i\n",DEFAULT_LOGLEVEL );
	printf("\n");
}

static void flush (int fd)
{
	int cnt=0;
	char c;
	while ((read(fd, &c, 1))>0) cnt++;
	logabba(L_MAX, "Flush was reading %d bytes",cnt);
}

int main(int argc, char *argv[])
{
	int c,fd;
	char device[50]=""; // device to use
	int errcnt=0;
	char msg[56];
	int cnt;
	int loglevel = L_STD;
	struct bat3 state;

	strncpy(device, BAT3DEV, sizeof(BAT3DEV));

	while ((c=getopt(argc, argv, "d:h?l:"))!=EOF) {
		switch (c) {
			case 'd': //device
				strncpy(device,optarg, sizeof(device)-1);
				printf("I'll read device %s\n",device);
				break;
			case 'l': //loglevel
				loglevel = atoi(optarg);
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
		return;
	}

	if ((fd = open(device,O_NONBLOCK|O_RDWR))==-1) {
		fprintf(stderr,"Error opening %s: %m\n",device);
		return(1);
	}

	termConfigRaw(fd);

	if ((loglevel<L_MIN) || (loglevel>L_MAX)) loglevel = DEFAULT_LOGLEVEL;
	setloglevel(loglevel, "bat3");
	logabba(L_MIN, "%s started, loglevel %i", argv[0], loglevel);
	
	if (getsample)
	cnt = readStream(fd, msg, sizeof(msg));
	if (decodemsg(msg, cnt, &state))
	
	// quick hack for not overloading
	// state.pwm_lo = 500;

	cnt=0;
	
while (cnt<300)	{  // 300 samples

	// if (c) flush(fd);
	
	getsample(fd,&state);
	changeled(&state);
	doload(&state,400); // 400mA charging :-)

	if (cnt =  encodemsg(msg, sizeof(msg), &state))	writeStream(fd, msg, cnt);
	
//	cnt = readStream(fd,msg,sizeof(msg));
//	decodemsg(msg,cnt,&state);

	cnt++;
	// usleep(250*1000);
}
	close(fd);

	return 0;

}

// vim:set autoindent
