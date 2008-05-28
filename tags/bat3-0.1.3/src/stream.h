/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

int openStream(char *device);
int writeStream(int fd, char *msg, int len);
int readStream(int fd,char *msg, int max);
