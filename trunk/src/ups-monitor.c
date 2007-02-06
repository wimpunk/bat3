#include <sys/termios.h> // tcgetattr
#include <stdio.h>
#include <sys/select.h>
#include <alloca.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ByteArray.h"
#include "IntArray.h"
#include "file.h"
#include "bat3.h"
#include "net.h"
#include "Process.h"

int callback(ByteArrayRef bReq,ByteArrayRef bRep) {
  BAT3request *req = (BAT3request *)(bReq.arr);
  BAT3reply *rep = (BAT3reply *)(bRep.arr);

  return (
    req->softJP3 != rep->softJP3
    ) ? 1 : 0;
}

int init(ByteArrayRef bRep,ByteArrayRef bReq) {
  BAT3request *req = (BAT3request *)(bReq.arr);
  BAT3reply *rep = (BAT3reply *)(bRep.arr);
  
  req->outputs = rep->outputs;
  req->pwm_lo = rep->pwm_lo;
  req->pwm_t = rep->pwm_t;
  req->alarm = 0;
  req->softJP3 = 1;
  return sizeof(BAT3request);
}

void _main(FILE *f) {
  byte req[256],rep[256];
  ByteArrayRef bReq = BAR(req,256),bRep = BAR(rep,256);
  int todo,loops;

  bRep = AVRreplyGet(f,bRep);
  bReq.len = init(bRep,bReq);
  AVRsendRequest(f,bReq,bRep,callback);
}

int main(int argc,char *argv[]) {
  FILE *f;
  struct termios saved;

  f = fileOpenOrDie("/dev/ttyT3S0","r+");
  fileSetBlocking(fileno(f),0);
  termConfigRaw(fileno(f),&saved);
  
  _main(f);

  tcsetattr(fileno(f),TCSANOW,&saved);
  fclose(f);
}
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
