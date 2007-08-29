/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

#define REVISION "$Rev$"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include "bat3.h"
#include "mysocket.h"

static void writePrompt(int fd);
static int writeFd(int fd, const char *msg, ...);

int openSocket(int portno) {
    
    int sockfd;
    
    struct sockaddr_in serv_addr;
    
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
	fprintf(stderr, "ERROR opening socket");
    }
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
    sizeof(serv_addr)) < 0) {
	fprintf(stderr, "Could not bind to port %d: %s", portno, strerror(errno));
	return -1;
    }
    
    if (listen(sockfd, MAXSOCKET)==-1) {
	fprintf(stderr, "Could not listen to port %d: %s", portno, strerror(errno));
	return -1;
    }
    
    
    return (sockfd);
    
}


int acceptSocket(int sockfd) {
    
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,  (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0) {
	fprintf(stderr, "ERROR on accept");
    } else {
	logabba(L_MIN, "Accepted connection");
	writeFd(newsockfd, "Welcome to bat3 ($Rev$) on fd %d", newsockfd);
    }
    
    writePrompt(newsockfd);
    
    return newsockfd;
    
}

static void writePrompt(int fd) {
    char prompt[]="> ";
    write(fd, "> ", strlen(prompt) );
}

void cmdHelp(int fd) {
    
    writeFd(fd, "Available commands:");
    writeFd(fd, " bat: get battery state" );
    writeFd(fd, " quit: close this connection");
    
}

mysock_t cmdQuit(int fd, char *rest) {
    writeFd(fd, "Have a nice day.");
    return MYSOCK_QUIT;
}

mysock_t cmdEnd(int fd, char *rest) {
    writeFd(fd, "I'll stop working");
    return MYSOCK_END;
}

mysock_t cmdBat(int fd, char *rest) {

    writeFd(fd, "BatRun: %s", print_onoff(getBatRun()));
    return MYSOCK_OKAY;
    
}

mysock_t readSocket(int fd) {
    
    char buffer[256];
    char cmd[256];
    int n;
    mysock_t ret=MYSOCK_OKAY;
    
    bzero(buffer, 256);
    n = read(fd, buffer, 255);
    if (n < 0) {
	logabba(L_MIN, "ERROR reading from socket");
	return -1;
    }
    
    n = sscanf(buffer, "%s %s", cmd, buffer);
    switch (n) {
	case 0:
	    writeFd(fd,"No command decoded");
	    break;
	case 1:
	    // writeFd(fd, "your command  = <%s>", cmd);
	    break;
	case 2:
	    // writeFd(fd, "your command = <%s>, buffer=<%s>, n = %d\n", cmd, buffer, n);
	    break;
	default:
	    writeFd(fd, "Could not decode your message: %s", strerror(errno));
    }
    
    if (n<=1) buffer[0] = 0;
    
    if (strcmp(cmd, "help")==0) {
	cmdHelp(fd);
    } else if (strcmp(cmd, "quit")==0) {
	ret = cmdQuit(fd, buffer);
    } else if (strcmp(cmd, "bat")==0) {
	cmdBat(fd, buffer);
    } else if (strcmp(cmd, "end")==0) {
	ret = cmdEnd(fd, buffer);
    } else {
	writeFd(fd, "I got your message but didn't understand it: <%s>", cmd);
	writeFd(fd, "You could try help");
    }
    
    if (ret!=-1) writePrompt(fd);
    
    return ret;
    
}

int writeFd(int fd, const char *msg, ...) {
    
    int cnt;
    static char mymsg[1024];
    memset(mymsg, 0, sizeof(mymsg));
    
    va_list v;
    
    va_start(v, msg);
    
    vsprintf(mymsg, msg, v);
    cnt = write(fd, mymsg, sizeof(mymsg));
    if (cnt < 0) {
	logabba(L_MIN, "ERROR writing to socket");
    } else {
	write(fd, "\n", 1);
    }
    va_end(v);
    
    return cnt;
    
}

