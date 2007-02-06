include Makefile.common
CFLAGS+=-DNO_AA -I${U}/lib -g
LDFLAGS+=-g

#all: ts732 bat3 ups-monitor
all: bat3 ups-monitor


#ts732: ts732.o avr.o file.o IntArray.o ByteArray.o serial.o term.o
bat3.o: bat3.h
bat3: bat3.o avr.o file.o IntArray.o ByteArray.o serial.o term.o net.o Process.o
ups-monitor: ups-monitor.o avr.o file.o IntArray.o ByteArray.o serial.o term.o net.o Process.o

bat3dump: bat3dump.o
bat3save: file.o IntArray.o ByteArray.o net.o

draw: draw.c
	gcc -L/usr/X11R6/lib -I.. -lX11 -o draw ../draw.c filter.o

clean:
	rm -f ts732 bat3 *.o
