#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include "Process.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

int processExec(char *path,char **args) { // IMPURE-SE
  pid_t pid;
  int p1[2],p2[2];
  int i=0;
  char buf[1024];

  args[0] = path;
  pipe(p1);
  pipe(p2);
  pid = fork();
  if (pid == 0) { // child
    dup2(p1[0],0);
    dup2(p2[1],1);
    close(p1[0]); 
    close(p1[1]); 
    close(p2[0]); 
    close(p2[1]);
    if (execvp(path,args)) {
      exit(1);
      return 0;
    }
  } else { // parent
    if (pid != -1) {
      dup2(p2[0],0);
      dup2(p1[1],1);
      close(p1[0]); 
      close(p1[1]); 
      close(p2[0]); 
      close(p2[1]);
    }
  }
  return pid;
}

void processDaemonize() {
  int i;

  for (i = 0; i < 3; ++i) {
    close(i);
  }
  open("/", O_RDONLY);
  dup2(0, 1);
  dup2(0, 2);

  i = open("/dev/tty", O_RDWR);
  if (i >= 0) {
    ioctl(i, TIOCNOTTY, 0);
    close(i);
  }
}
