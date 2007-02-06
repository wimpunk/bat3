// copyright (c)2006 Technologic Systems
#ifndef __AVR_Z
#define __AVR_Z

#include "base.h"
#include "ByteArray.h"


ByteArrayRef AVRreplyGet(FILE *f,ByteArrayRef bRep);
ByteArrayRef AVRsendRequest(FILE *f,ByteArrayRef bReq,ByteArrayRef bRep,
			    int (*)(ByteArrayRef,ByteArrayRef));
void _AVRrun(FILE *f,void *data,int argc,char **argv,
	     void (*setup)(FILE *,void *),
	     int (*init)(ByteArrayRef,ByteArrayRef),
	     int (*opt)(FILE *,ByteArrayRef,void *,int *,int,char **),
	     void (*print)(ByteArrayRef,void *),
	     int (*task)(FILE *,int,ByteArrayRef,ByteArrayRef,void*),
	     int (*callback)(ByteArrayRef,ByteArrayRef)
	     );
void AVRrun(char *ser,void *data,int argc,char **argv,
	    void (*setup)(FILE *,void *),
	    int (*init)(ByteArrayRef,ByteArrayRef),
	    int (*opt)(FILE *,ByteArrayRef,void *,int *,int,char **),
	    void (*print)(ByteArrayRef,void *),
	    int (*task)(FILE *,int,ByteArrayRef,ByteArrayRef,void*),
	    int (*callback)(ByteArrayRef,ByteArrayRef)
	    );
#endif
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
