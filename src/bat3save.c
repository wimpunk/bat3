#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include "net.h"
#include "file.h"
#include "bat3.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

/*
  This is a simple sample program that illustrates how a program can
  be written to communicate with a bat3 daemon to record its operation.
  The IP address of the TS-7XXX board with the TS-BAT3 installed is
  supplied as the command line argument.  This program tells the BAT3
  to start sending it data.  It keeps a current display of a few
  parameters of interest on the command line, and logs all data received
  to the file "bat.dat" which is used by the sample program "draw" to
  display a graph.
 */

inline int iabs(int val) { // oops, i think we already have abs()...
  return val > 0 ? val : -val;
}

void startup(char *adrs) {
  BAT3UDPcommand cmd;
  int fd;
  struct sockaddr_in to;

  fd = netUDPopen(4000,0);
  if (fd == -1) {
    perror("openSocket:");
    exit(1);
  }
  netAdrsFromName(&to,adrs,4000);

  cmd.command = 1;
  cmd.cmd1.port = 2000;
  netUDPtx(fd,BAR(&cmd,sizeof(cmd)),&to);

  close(fd);
}

void shutdown1(char *adrs) {
  BAT3UDPcommand cmd;
  int fd;
  struct sockaddr_in to;

  fd = netUDPopen(4000,0);
  if (fd == -1) {
    perror("openSocket:");
    exit(1);
  }
  netAdrsFromName(&to,adrs,4000);

  cmd.command = 2;
  netUDPtx(fd,BAR(&cmd,sizeof(cmd)),&to);

  close(fd);
}

void setI(char *adrs,int I) {
  BAT3UDPcommand cmd;
  int fd;
  struct sockaddr_in to;

  fd = netUDPopen(4000,0);
  if (fd == -1) {
    perror("openSocket:");
    exit(1);
  }
  netAdrsFromName(&to,adrs,4000);

  cmd.command = 3;
  cmd.cmd3.targetI = I;
  netUDPtx(fd,BAR(&cmd,sizeof(cmd)),&to);

  close(fd);
}

void setIT(char *adrs,int I,int on,int off) {
  BAT3UDPcommand cmd;
  int fd;
  struct sockaddr_in to;

  fd = netUDPopen(4000,0);
  if (fd == -1) {
    perror("openSocket:");
    exit(1);
  }
  netAdrsFromName(&to,adrs,4000);

  cmd.command = 4;
  cmd.cmd4.targetI = I;
  cmd.cmd4.time_on = on;
  cmd.cmd4.time_off = off;

  netUDPtx(fd,BAR(&cmd,sizeof(cmd)),&to);

  close(fd);
}


int main(int argc,char *argv[]) {
  int fd,j,server=0;
  fd_set readset;
  struct sockaddr_in from;
  int h,i,port=2000,timer=60,I=400;
  char buffer[256];
  BAref buf;
  bat3Info *bi = (bat3Info *)buffer;
  FILE *f;
  struct timeval tv;

  if (argc < 2) {
    printf("Usage: bat3save address-of-bat3\n");
    return 1;
  } else {
    startup(argv[1]);
    //setIT(argv[1],400,60,60);
    //setI(argv[1],I);
  }
  
  f = fopen("./bat.dat","a");
  if (!f) {
    perror("fopen:");
    return 1;
  }
  printf("Listening on port %d\n",port);
  fd = netUDPopen(port,1);
  if (fd == -1) {
    perror("openSocket:");
    return 1;
  }
  while (1) {
    FD_ZERO(&readset);
    FD_SET(fd,&readset);
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    j = select(fd+1,&readset,0,0,&tv);
    if (j) {
      buf = netUDPrx(fd,BAR(buffer,256),&from);
      fwrite(bi,sizeof(bat3Info),1,f);
      fflush(f);
      printf("%5d %.6fV %.3fmA %.1fF %.3f%%  (%s) (%s) (%s) (%s) \r",
	     bi->t,bi->battV,(float)bi->battI/1000,bi->tempF,
	     1.0-bi->pwmlo/bi->pwmt,bi->locked?"L":"U",
	     bi->jp3?"JP3":"---",bi->softJP3?"SJ3":"---",
	     bi->onbatt?"BATT":"LINE");
      fflush(stdout);
    } else {
      startup(argv[1]);
    }
  }
  close(fd);
}
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
