#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED


#include <syslog.h>

//#define LOG_EMERG       0       /* system is unusable */
//#define LOG_ALERT       1       /* action must be taken immediately */
//#define LOG_CRIT        2       /* critical conditions */
//#define LOG_ERR         3       /* error conditions */
//#define LOG_WARNING     4       /* warning conditions */
//#define LOG_NOTICE      5       /* normal but signification condition */
//#define LOG_INFO        6       /* informational */
//#define LOG_DEBUG       7       /* debug-level messages */


// nur Start-, Ende-, Fehlermeldungen - was BASIS_LOG
#define  L_MIN        LOG_ALERT
#define  L_MINPLUS    LOG_CRIT
#define  L_STD        LOG_ERR
// WAS DETAIL_LOG
#define  L_MORE       LOG_WARNING
#define  L_NOTICE     LOG_NOTICE
#define  L_INFO       LOG_INFO

#define  L_MAX        LOG_DEBUG        // WAS ALL_LOG

#define  F_KK           LOG_LOCAL0      // for kk debugging
#define  F_DB           LOG_LOCAL1      // for database related stuff
#define  F_TR           LOG_LOCAL2      // for tracing execution

#define  F_ABBA         LOG_LOCAL7      // default dumpster

#define L_TRACE      L_MAX | F_TR       // convenience macros

void logabba(int prio,const char *mess, ...);
void setloglevel(int newlevel, char *progi);
void setfacility(int newfacility);
int getloglevel(void);
char *fmtt(void);

#endif
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
