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
void writemsg(int fd, char* msg, int len, int read)
{
	/*
	   struct BAT3request 

	   unsigned alarm;

PWM1en:1,
PWM2en:1,
_offsetEn:1,
opampEn:1,
_buckEn:1,
_led:1,
_jp3:1,
batRun:1,
ee_read:1,
ee_write:1,
ee_ready:1,
softJP3:1;

unsigned short outputs;

unsigned short pwm_lo; // low time
unsigned short pwm_t; // high time plus low time
unsigned char ee_addr;
unsigned char ee_data;
unsigned char checksum;

	 */

	struct BAT3request req;
	struct BAT3reply   *rep = (struct BAT3reply *)msg;

	int cnt;
	
	// Try to read a byte
	req.outputs = rep->outputs;
	req.pwm_lo = rep->pwm_lo;
	req.pwm_t = rep->pwm_t;
	req.alarm = 0;
	req.softJP3 = 0;
	req._led = 1;

	// if (read)
	req.ee_addr = 128;
	/* else {
	req.ee_addr = -1;
	} */
	
	req.ee_write = 0;
	req.ee_read = read;

	// req.checksum = calcCrc((char*)&req, sizeof(req)-1);

	// cnt=write(fd, (char*)&req, sizeof(req));
	// logabba(L_MAX, "Write returned %d while sizeof(req)=%d, checksum is %04X", cnt,sizeof(req),req.checksum);
		writeStream(fd, (char*)&req, sizeof(req)-1);	
	// req.ee_read = 0;
	
/*
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

	if (rep->batRun) {
		logabba(L_MAX, "Running on bat\n");
		req._led = 1;
	} else {
		if (rep->PWM1en || rep->PWM2en) {
			req._led = !req._led;
			logabba(L_MAX, "invert led\n");
		} else {
			// req._led = 0;
			// forcing led off
			req._led  = 1;
			logabba(L_MAX, "Forcing led OFF");
		}
	}
*/
	/*
	req.checksum = calcCrc((char*)&req, sizeof(req)-1);

	write(fd, (char*)&req, sizeof(req));
	logabba(L_MAX, "Write returned %d while sizeof(req)=%d, checksum is %04X", cnt,sizeof(req),req.checksum);
*/
	return;
}

void usage(char *progname) 
{
	// TODO: shit opkuisen
	printf("\n");
	printf("Usage\n");
	printf("\n");
	printf("   %s [-d device]\n",progname);
	printf("      -d device: device to use, default %s\n",BAT3DEV );
	printf("\n");
}

void flush (int fd)
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
	int loglevel = -1;
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

	if ((loglevel<L_MIN) || (loglevel>L_MAX)) loglevel = L_STD;
	setloglevel(L_MAX,"bat3");
	logabba(L_MIN,"bat3 is started, loglevel %i", loglevel);
	
	cnt = readStream(fd,msg,sizeof(msg));
	decodemsg(msg,cnt,&state);
	
//	flush(fd); // maybe this helps?
	
	writemsg(fd, msg, cnt,  1);
	cnt   = readStream(fd,msg,sizeof(msg));
	decodemsg(msg,cnt,&state);
	
	writemsg(fd, msg, cnt, 0);
	cnt = readStream(fd,msg,sizeof(msg));
	decodemsg(msg,cnt,&state);
	
	close(fd);

	return 0;
	
}

// vim:set autoindent
