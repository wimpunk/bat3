/*
 * $Id: convert.c 55 2007-08-20 15:42:15Z wimpunk $
 *
 * $LastChangedDate: 2007-08-20 17:42:15 +0200 (Mon, 20 Aug 2007) $
 * $Rev: 55 $
 * $Author: wimpunk $
 *
 * */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "bat3.h"
#include "mysocket.h"

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
	fprintf(stderr, "ERROR on binding");
    }
    listen(sockfd, 5);
    
    return (sockfd);
    
}


int acceptSocket(int sockfd) {
    
    int newsockfd;
    //  struct sockaddr_in serv_addr,
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,  (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0) {
	fprintf(stderr, "ERROR on accept");
    } else {
	logabba(L_MIN, "Accepted connection");
	write(newsockfd, "Yo de mannen", strlen("Yo de mannen"));
    }
    
    return newsockfd;
}


int readSocket(int fd) {
    char buffer[256];
    int n;
    
    bzero(buffer, 256);
    n = read(fd, buffer, 255);
    if (n < 0) logabba(L_MIN, "ERROR reading from socket");
    printf("Here is the message: %s\n", buffer);
    n = write(fd, "I got your message", 18);
    if (n < 0) logabba(L_MIN, "ERROR writing to socket");
    
    return 0;
    
}
