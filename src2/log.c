/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "log.h"

static int loglevel = L_MIN;
static int facility = F_ABBA;
static int started = 0;
static char progid[50] = "NIET GEGEVEN";

// WARNING!!!  Do not underestimate the length of possible logmessages.  Extreme
// known example = refresh from OT008, size approx. 8K!!!!

static char milog_dd[9000];


void trace(const char *mess, ...)
{
	va_list v;

	va_start (v,mess);

	if (loglevel <= L_MAX)
		syslog(loglevel | facility, mess,v);

	va_end(v);
	return;

}


void logabba(int prio, const char *mess, ...)
{
	va_list v;

	va_start (v,mess);

	if (prio <= loglevel) {
		vsprintf(milog_dd,mess,v);
		syslog(LOG_WARNING | facility, milog_dd);
		// printf("syslog(LOG_WARNING | facility, milog_dd);\n");
	// } else {
		// printf("Not writing to syslog because prio %d > loglevel %d",prio,loglevel);
	}

	va_end(v);
	return;
}

//#define LOG_EMERG       0       /* system is unusable */
//#define LOG_ALERT       1       /* action must be taken immediately */
//#define LOG_CRIT        2       /* critical conditions */
//#define LOG_ERR         3       /* error conditions */
//#define LOG_WARNING     4       /* warning conditions */
//#define LOG_NOTICE      5       /* normal but signification condition */
//#define LOG_INFO        6       /* informational */
//#define LOG_DEBUG       7       /* debug-level messages */


int getloglevel(void)
{
	return loglevel;
}


void setloglevel(int newlevel, char *progi)
{
	if (!started) {
		started++;
		if ( progi != NULL)
			strcpy(progid,progi);             // static copy
		openlog(progid,0,F_ABBA);

	}
	switch ( newlevel) {
		case 9 :
		case 8 :
		case 7 :
			loglevel = L_MAX;
			break;
		case 6 :
			loglevel = L_INFO;
			break;
		case 5 :
			loglevel = L_NOTICE;
			break;
		case 4 :
			loglevel = L_MORE;
			break;
		case 3 :
		case 2 :
			loglevel = L_STD;
			break;
		case 1 :
			loglevel = L_MIN;
			break;
		case 0 :
			break;
	}
}


void setfacility(int newfacility)
{
	facility = newfacility;
}


static char   date_time    [40];
char *fmtt(void)
{
	time_t now= time (NULL);

	strftime (date_time, sizeof(date_time), "%d.%m.%Y %H:%M:%S", localtime (&now));
	return date_time;
}
