/*
 * $Id: stream.h 49 2007-05-07 09:59:44Z wimpunk $
 *
 * $LastChangedDate: 2007-05-07 11:59:44 +0200 (Mon, 07 May 2007) $
 * $Rev: 49 $
 * $Author: wimpunk $
 *
 * */

int openSocket(int portno);
int acceptSocket(int socketfd);
int readSocket(int fd);

#define MAXSOCKET 5
