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
#include "bat3func.h"
#include "mysocket.h"
// #include "cmdsocket.h"

#define MAXSFD 16
static int writePrompt(int fd);
static int writeFd(int fd, const char *msg, ...);

static int cnt_connect=0;	// number of connected clients
static int sfd_connect[MAXSFD];	// list of the socket file discriptors of the
//static int sockfd=-1;	    // currently socketfd is passed by the main function
// connected clients
// TODO: should be replaced by a linked list

/* AcceptSocket: accepts the connection
 returns filedescriptor to the socket */
static int acceptSocket(int sockfd) {
    
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,  (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0) {
	fprintf(stderr, "ERROR on accept");
    } else {
	logabba(L_MIN, "Accepted connection");
	if (writeFd(newsockfd, "Welcome to bat3 ($Rev$) on fd %d\n", newsockfd)>0) {
	    logabba(L_MIN, "Wrote info message");
	} else {
	    logabba(L_MIN, "Failed writing info message");
	    close(newsockfd);
	    newsockfd = -1;
	}
    }
    
    if (newsockfd != -1) {
	if (! writePrompt(newsockfd)) {
	    logabba(L_MIN, "Writing prompt failed");
	    close(newsockfd);
	    newsockfd = -1;
	    
	} else {
	    logabba(L_MIN, "Wrote a correct prompt (I think)");
	}
    }
    
    return newsockfd;
    
}

void cmdHelp(int fd) {
    
    writeFd(fd, "Available commands:\n");
    writeFd(fd, " bat: get battery state\n" );
    writeFd(fd, " quit: close this connection\n");
    writeFd(fd, " exit: end the bat3 program\n");
    
}

mysock_t cmdQuit(int fd, char *rest) {
    writeFd(fd, "Have a nice day.\n");
    return MYSOCK_QUIT;
}

mysock_t cmdEnd(int fd, char *rest) {
    writeFd(fd, "I'll stop working");
    return MYSOCK_END;
}

mysock_t cmdBat(int fd, char *rest) {
    
    char *val;
    
    if (getBatI() == 0x01FF) {
	val = "NONE";
    } else {
	val = print_onoff(getBatRun());
    }
    
    writeFd(fd, "BatRun: %s", val);
    writeFd(fd, "BatRun: %04X", getBatI());
    
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
    logabba(L_MIN, "I received %d bytes", n);
    
    n = sscanf(buffer, "%s %s", cmd, buffer);
    logabba(L_MIN, "Scan returned %d", n);
    switch (n) {
	case 0:
	    writeFd(fd, "No command decoded");
	    break;
	case 1:
	    // writeFd(fd, "your command  = <%s>", cmd);
	    break;
	case 2:
	    // writeFd(fd, "your command = <%s>, buffer=<%s>, n = %d\n", cmd, buffer, n);
	    break;
	default:
	    logabba(L_MIN, "Could not decode your message: %s", buffer);
	    writeFd(fd, "Could not decode your message: %s", buffer);
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

/*
 processMySocket: verwerken van de netwerk connecties
TODO: socketfd should not be an argument.
 */
mysock_t processMySocket(int socketfd) {
    
    fd_set	    rfds;
    struct timeval  tv;
    int		    FlushTime = 1 * 1000; // 1 characters @ 9600bps
    
    
    int cnt;

    FD_ZERO(&rfds);
     // check if someone wants to connect
    FD_SET(socketfd, &rfds);

    // Set the connected sockets
    for (cnt=0; cnt<cnt_connect; cnt++) {
	FD_SET(sfd_connect[cnt], &rfds);
    }
    // check if someone wants to connect
    FD_SET(socketfd, &rfds);
    
    // set the flushtime
    tv.tv_sec  = FlushTime/1000000;
    tv.tv_usec = FlushTime%1000000;
    
    if (select( FD_SETSIZE, &rfds, NULL, NULL, &tv ) < 0) {
	fprintf(stderr, "processMySocket error: %s\n", strerror(errno));
	return -1;
    }
    
    if (FD_ISSET(socketfd, &rfds)) {
	logabba(L_MIN, "Someone knocks");
	sfd_connect[cnt_connect++]=acceptSocket(socketfd);
	if (sfd_connect[cnt_connect-1] == -1) {
	    cnt_connect--;
	    logabba(L_MIN, "but it was a little child");
	}
    }
    
    
    for (cnt=0; cnt<cnt_connect; cnt++) {
	
	if (FD_ISSET(sfd_connect[cnt], &rfds)) {
	    
	    int i;
	    mysock_t sockret;
	    
	    sockret = readSocket(sfd_connect[cnt]);
	    switch (sockret) {
		case MYSOCK_QUIT:
		    
		    // TODO: close should be done by an external function
		    close(sfd_connect[cnt]);
		    
		    for (i=cnt+1; i<cnt_connect; i++) {
			sfd_connect[i-1] = sfd_connect[i];
		    }
		    
		    cnt_connect--;
		    cnt--;
		    break;
		    
		case MYSOCK_END:
		    // for (i=0; i<cnt_connect; i++) close(sfd_connect[cnt]);
		    return MYSOCK_END;
		    
		    break;
		    
		case MYSOCK_OKAY:
		    break;
		    
	    }
	    
	    
	}
    }
    
    return MYSOCK_OKAY;
}


int openMySocket(int portno) {
    
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

int closeMySocket(int sockfd) {
    return close(sockfd);
}

static int writePrompt(int fd) {
    
    return writeFd(fd, "> ");
}

int writeFd(int fd, const char *msg, ...) {
    
    int cnt;
    static char mymsg[1024];
    memset(mymsg, 0, sizeof(mymsg));
    
    if (fd == -1) return -1;
    
    va_list v;
    
    va_start(v, msg);
    
    vsprintf(mymsg, msg, v);
    // mymsg[strlen(mymsg)] = '\n';
    // cnt = write(fd, mymsg, strlen(mymsg));
    // logabba(L_MIN, "Trying to send to socket: %s", mymsg);
    // cnt = send(fd, mymsg, strlen(mymsg), MSG_DONTWAIT);
    cnt = send(fd, mymsg, strlen(mymsg), MSG_DONTWAIT);
    logabba(L_MIN, "sending returned %d: %s", cnt, strerror(errno));
    if (cnt < 0) {
	logabba(L_MIN, "ERROR sending to socket: %s", strerror(errno));
	close(fd);
	cnt=-1;
    } else if (cnt != strlen(mymsg)){
	logabba(L_MIN, "Wrote a wrong number of bytes: %d != %d", cnt, strlen(mymsg) );
    }
    /*
     // write(fd, "\n", 1);
     cnt = send(fd, "\n", 2, MSG_DONTWAIT);
     logabba(L_MIN, "ERROR sending to socket(send returned %d): %s", cnt, strerror(errno));
     */
    
    va_end(v);
    
    return cnt;
    
}

