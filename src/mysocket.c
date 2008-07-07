/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#include "bat3.h"
#include "bat3func.h"
#include "mysocket.h"
#include "convert.h"
// #include "cmdsocket.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MAXSFD 128
#define PROMPT "> "
static int writePrompt(int fd);
static int writeFd(int fd, const char *msg, ...);

static int portno = 0;

// static int cnt_connect=0;	// number of connected clients
static int sfd_connect[MAXSFD]; // list of the socket file discriptors
// of the connected clients
static int sockfd = -1;
static time_t lasttry = 0;
int closeFd(int fd);

void setPortno(int no) {
    portno = no;
}

void initMySocket() {
    int i;
    for (i = 0; i < MAXSFD; i++) sfd_connect[i] = 0;
}

static int openMySocket(int portno) {

    int sockfd;

    struct sockaddr_in serv_addr;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "ERROR opening socket");
    }
    memset((char *) & serv_addr, 0, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) & serv_addr,
            sizeof (serv_addr)) < 0) {
        logabba(L_MIN, "Could not bind to port %d: %s", portno, strerror(errno));
        shutdown(sockfd, SHUT_RDWR);
        return -1;
    }

    if (listen(sockfd, MAXSOCKET) == -1) {
        logabba(L_MIN, "Could not listen to port %d: %s", portno, strerror(errno));
        shutdown(sockfd, SHUT_RDWR);
        return -1;
    }

    return (sockfd);

}

/** AcceptSocket: accepts the connection
 * returns filedescriptor to the socket */
static int acceptSocket(int sockfd) {

    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t clilen;

    clilen = sizeof (cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) & cli_addr, &clilen);

    if (newsockfd < 0) {
        fprintf(stderr, "ERROR on accept");
    } else {
        logabba(L_MIN, "Accepted connection");
        if (writeFd(newsockfd, "Welcome to bat3 (%s-%s) on fd %d", VERSION, REV, newsockfd) > 0) {
            logabba(L_NOTICE, "Wrote info message to fd %d", newsockfd);
        } else {
            logabba(L_NOTICE, "Failed writing info message to fd %d");
            close(newsockfd);
            newsockfd = -1;
        }
    }

    if (newsockfd != -1) {
        if (!writePrompt(newsockfd)) {
            // logabba(L_MAX, "Writing prompt failed");
            close(newsockfd);
            newsockfd = -1;
        }
    }

    return newsockfd;

}

void cmdHelp(int fd) {

    writeFd(fd, "Available commands:");
    writeFd(fd, " bat: get battery state");
    writeFd(fd, " state: get current state (complete array)");
    writeFd(fd, " current i: set new target current to i");
    writeFd(fd, " loglevel i: set loglevel to i");
    writeFd(fd, " quit: close this connection");
    writeFd(fd, " exit: end the bat3 program");
    logabba(L_NOTICE, "Wrote help msg to %i", fd);

}

mysock_t cmdQuit(int fd, char *rest) {

    writeFd(fd, "Have a nice day.");
    close(fd);

    return MYSOCK_QUIT;
}

mysock_t cmdEnd(int fd, char *rest) {
    writeFd(fd, "I'll stop working");
    return MYSOCK_END;
}

mysock_t cmdRead(int fd, char *rest) {

    int address;

    if (sscanf(rest, "%d", &address) != 1)
        writeFd(fd, "Sorry, couldn't decode <%s> to a integer", rest);
    else {
        writeFd(fd, "I'll read %d", address);
        getAddress(address);
    }

    return MYSOCK_OKAY;
}

mysock_t cmdBat(int fd, char *rest) {

    char *val;

    if (getBatI() == 0x01FF) {
        val = "NONE";
    } else {
        val = print_onoff(getBatRun());
    }

    writeFd(fd, "BatRun: %s", val);
    // writeFd(fd, "BatRun: %04X", getBatI());

    return MYSOCK_OKAY;

}

mysock_t cmdState(int fd, char *rest) {

    writeFd(fd, "checking state");
    print_bat3(fd, getState()); // struct bat3* mybat3) {

    return MYSOCK_OKAY;
}

mysock_t cmdCurrent(int fd, char *rest) {

    int newcurrent;

    logabba(L_INFO, "Decoding <%s>", rest);

    if (1 != sscanf(rest, "%i", &newcurrent)) {
        writeFd(fd, "Currently charging to %imA", getCurrent());
        return MYSOCK_OKAY;
    }

    logabba(L_INFO, "Changing current from %i to %i", getCurrent(), newcurrent);

    writeFd(fd, "Changing current from %i to %i", getCurrent(), newcurrent);
    setCurrent(newcurrent);
    writeFd(fd, "New current set to %i", getCurrent());

    return MYSOCK_OKAY;

}

mysock_t cmdHours(int fd, char*rest) {

    time_t myHour =  getHours();
    
    if (myHour==0) {
        writeFd(fd, "Hours not set");
    } else {
        writeFd(fd, "Hours set to %s", ctime(&myHour));
    }
        
    return MYSOCK_OKAY;

}

mysock_t cmdLoglevel(int fd, char*rest) {

    int newloglevel;

    if (1 != sscanf(rest, "%i", &newloglevel)) {
        writeFd(fd, "Currently loglevel to %i", getLoglevel());
        return MYSOCK_OKAY;
    }

    logabba(L_INFO, "Changing loglevel from %i to %i", getLoglevel(), newloglevel);

    writeFd(fd, "Changing loglevel from %i to %i", getLoglevel(), newloglevel);
    setCurrent(newloglevel);
    writeFd(fd, "New loglevel set to %i", getLoglevel());

    return MYSOCK_OKAY;

}

mysock_t readSocket(int fd) {

    char buffer[256];
    char cmd[256];
    int n;
    mysock_t ret = MYSOCK_OKAY;

    bzero(buffer, 256);
    n = read(fd, buffer, 255);
    if (n < 0) {
        logabba(L_MIN, "ERROR reading from socket");
        return -1;
    }

    n = sscanf(buffer, "%s %s", cmd, buffer);
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
            writeFd(fd, "Could not decode your message: [%s]", buffer);
    }

    if (n <= 1) buffer[0] = 0;

    if ((strcmp(cmd, "help") == 0) || (strcmp(cmd, "q") == 0)) {
        cmdHelp(fd);
    } else if ((strcmp(cmd, "quit") == 0) || (strcmp(cmd, "q") == 0)) {
        ret = cmdQuit(fd, buffer);
    } else if ((strcmp(cmd, "bat") == 0) || (strcmp(cmd, "b") == 0)) {
        cmdBat(fd, buffer);
    } else if ((strcmp(cmd, "exit") == 0) || (strcmp(cmd, "e") == 0)) {
        ret = cmdEnd(fd, buffer);
    } else if ((strcmp(cmd, "read") == 0) || (strcmp(cmd, "r") == 0)) {
        ret = cmdRead(fd, buffer);
    } else if ((strcmp(cmd, "state") == 0) || (strcmp(cmd, "s") == 0)) {
        ret = cmdState(fd, buffer);
    } else if (cmd[0] == 'c') {
        ret = cmdCurrent(fd, buffer);
    } else if (cmd[0] == 'h') { // hours - time we will work
        ret = cmdHours(fd, buffer);
    } else if (cmd[0] == 'l') {
        ret = cmdLoglevel(fd, buffer);
    } else {
        writeFd(fd, "I got your message but didn't understand it: <%s>", cmd);
        writeFd(fd, "You could try help");
    }

    if ((ret != MYSOCK_END) && (ret != MYSOCK_QUIT)) writePrompt(fd);

    return ret;

}

/*
 * processMySocket: verwerken van de netwerk connecties
 */
mysock_t processMySocket() {

    fd_set rfds;
    struct timeval tv;
    int FlushTime = 1 * 1000; // 1 characters @ 9600bps


    int cnt;

    if (sockfd == -1) {

        if ((time(NULL) - lasttry) < 30) {
            return -1;
        } else {
            time(&lasttry);
        }

        if ((sockfd = openMySocket(portno)) == -1) {
            logabba(L_MIN, "Could not open port %i\n", portno);
            return MYSOCK_OKAY;
        }
    }

    FD_ZERO(&rfds);
    // check if someone wants to connect
    FD_SET(sockfd, &rfds);

    // Set the connected sockets
    for (cnt = 0; cnt < MAXSFD; cnt++) {
        if (sfd_connect[cnt] > 0) FD_SET(cnt, &rfds);
    }

    // check if someone wants to connect
    FD_SET(sockfd, &rfds);

    // set the flushtime
    tv.tv_sec = FlushTime / 1000000;
    tv.tv_usec = FlushTime % 1000000;

    if (select(FD_SETSIZE, &rfds, NULL, NULL, &tv) < 0) {
        fprintf(stderr, "processMySocket error: %s\n", strerror(errno));
        return -1;
    }

    if (FD_ISSET(sockfd, &rfds)) {
        int tempfd;

        logabba(L_MIN, "Someone knocks");
        tempfd = acceptSocket(sockfd);
        logabba(L_NOTICE, "Accepted knocking person on sockfd %d", tempfd);

        if (tempfd >= MAXSFD) {
            logabba(L_MIN, "To much connected clients");
            writeFd(tempfd, "To much connected clients");
            close(tempfd);
        } else {
            sfd_connect[tempfd] = 1;
        }

    }


    for (cnt = 0; cnt < MAXSFD; cnt++) {

        if (!(sfd_connect[cnt] > 0)) continue;

        if (FD_ISSET(cnt, &rfds)) {

            logabba(L_NOTICE, "FD_ISSET on fd=%i", cnt);
            mysock_t sockret;
            sockret = readSocket(cnt);

            switch (sockret) {

                case MYSOCK_QUIT:
                    closeFd(cnt);
                    break;

                case MYSOCK_END:
                    closeMySocket(sockfd);
                    return MYSOCK_END;
                    break;

                case MYSOCK_OKAY:
                    break;

            }


        }
    }

    return MYSOCK_OKAY;
}

static int writePrompt(int fd) {

    return writeFd(fd, PROMPT);

}

int closeFd(int fd) {

    if (sfd_connect[fd] > 0) {
        close(fd);
        sfd_connect[fd] = 0;
    }

    return 0;

}

void closeMySocket(int sockfd) {

    int fd;

    for (fd = 0; fd < MAXSFD; fd++) closeFd(fd);

    close(sockfd);
}

int writeFd(int fd, const char *msg, ...) {

    int cnt;
    static char mymsg[1024];
    memset(mymsg, 0, sizeof (mymsg));

    if (fd == -1) return -1;

    va_list v;

    va_start(v, msg);

    vsprintf(mymsg, msg, v);
    cnt = send(fd, mymsg, strlen(mymsg), MSG_DONTWAIT);
    // logabba(L_MIN, "sending returned %d: %s", cnt, strerror(errno));
    if (cnt < 0) {
        logabba(L_MIN, "ERROR sending to socket: %s", strerror(errno));
        closeFd(fd);
        cnt = -1; // forcing error
    } else if (cnt != strlen(mymsg)) {
        logabba(L_MIN, "Wrote a wrong number of bytes: %d != %d", cnt, strlen(mymsg));
    }

    va_end(v);

    if (strcmp(msg, PROMPT) != 0) {
        send(fd, "\n", strlen("\n"), MSG_DONTWAIT);
    }

    return cnt;

}

