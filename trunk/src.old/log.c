/*****************************************************************************
// Project    : ABBA 2000 - ABBA Gent reengineering
// Subproject : mxlib - common functions
// Module     :
//*****************************************************************************
//
// $Rev$
// $Date$
// $Author$
//
// Comments:
//*****************************************************************************
// Revision History
//*****************************************************************************
/*
 * $Log$
 *
 ******************************************************************************
 */

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

static char milog_dd[1024];

/*
void trace(const char *mess, ...)
{
	va_list v;

	va_start (v,mess);

	if (loglevel <= L_MAX)
		syslog(loglevel | facility, mess,v);

	va_end(v);
	return;

}
*/


void logabba(int prio, const char *mess, ...)
{
	va_list v;

	va_start (v,mess);

	if (prio <= loglevel) {
		vsnprintf(milog_dd,1024,mess,v);
		syslog(LOG_WARNING | facility, milog_dd);
	}

	va_end(v);
	return;
}


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

/*
void setfacility(int newfacility)
{
	facility = newfacility;
}
*/

