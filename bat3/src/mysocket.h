/*
 * $Id: stream.h 49 2007-05-07 09:59:44Z wimpunk $
 *
 * $LastChangedDate: 2007-05-07 11:59:44 +0200 (Mon, 07 May 2007) $
 * $Rev: 49 $
 * $Author: wimpunk $
 *
 * */


#ifndef MYSOCKET_H
#define MYSOCKET_H
#define MAXSOCKET 5

typedef enum {
    MYSOCK_OKAY,
    MYSOCK_QUIT,
    MYSOCK_END
} mysock_t;


void initMySocket();
void setPortno(int no);
// int openMySocket(int portno);
// int acceptSocket(int socketfd);
// mysock_t processMySocket(int socketfd);
mysock_t processMySocket();
// mysock_t readSocket(int fd);
// onoff_t getBatRun();
void closeMySocket(int sockfd);
#endif
