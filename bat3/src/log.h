#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

#include <syslog.h>

//#define LOG_EMERG       0       /* system is unusable */
//#define LOG_ALERT       1       /* action must be taken immediately */
//#define LOG_CRIT        2       /* critical conditions */
//#define LOG_ERR         3       /* error conditions */
//#define LOG_WARNING     4       /* warning conditions */
//#define LOG_NOTICE      5       /* normal but signification condition */
//#define LOG_INFO        6       /* informational */
//#define LOG_DEBUG       7       /* debug-level messages */

#define  L_MIN        LOG_ALERT
#define  L_MINPLUS    LOG_CRIT
#define  L_STD        LOG_ERR

#define  L_MORE       LOG_WARNING
#define  L_NOTICE     LOG_NOTICE
#define  L_INFO       LOG_INFO

#define  L_MAX        LOG_DEBUG 

#ifndef LOGFACILITY
    #define LOGFACILITY LOG_LOCAL7
#endif

void logabba(int prio,const char *mess, ...);
void setloglevel(int newlevel, char *progi);
int getloglevel(void);
#endif

