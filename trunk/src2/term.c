#include <sys/termios.h> 
/*
 * $Id: term.c 14 2007-02-06 14:08:34Z wimpunk $
 *
 * $LastChangedDate: 2007-02-06 15:08:34 +0100 (Tue, 06 Feb 2007) $
 * $Rev: 14 $
 * $Author: wimpunk $
 *
 * */
void termConfigRaw(int fd) {
  struct termios newtio;

  tcgetattr(fd,&newtio); /* save current port settings */


  /* Set baudrate */
  cfsetispeed(&newtio,B115200);
  cfsetospeed(&newtio,B115200);
	    
  newtio.c_cflag |= (CLOCAL | CREAD);

  // 8N1
  newtio.c_cflag &= ~PARENB;
  newtio.c_cflag &= ~CSTOPB;
  newtio.c_cflag &= ~CSIZE;
  newtio.c_cflag |= CS8;        /* Set for 8 bits. */

  /* disable hardware flow control. */
  newtio.c_cflag &= ~CRTSCTS;

  /* When we read, we want completely raw access  */
  /* the last two get set by default on ARM but not CYGWIN */
  newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | NOFLSH | TOSTOP);
  newtio.c_lflag = 0;  // We don't believe above signals are set correctly...

  /* ignore flow control, don't do CR/LF translations */
  newtio.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
  //newtio.c_iflag = ( ICRNL );

  /* Set output filtering to raw. */
  newtio.c_oflag &= ~(OPOST | ONLCR | OCRNL | ONLRET);
  //newtio.c_oflag = ONLCR;

  /* Set readsize and timeout to some reasonable values, just to be safe*/
  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* nonblocking read */

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);
}
