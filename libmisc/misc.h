/*
$$LIC$$
 */
/*
** misc.h - export declarations for misc. funcs
**
** Copyright Fraunhofer FOKUS
**
** $Id: misc.h 1023 2009-03-27 22:34:53Z csc $
**
*/
#ifndef _MISC_H
#define _MISC_H

#include "config.h"

#include <time.h>
#include <sys/time.h>


#ifdef   __cplusplus
extern "C" {
#endif

#if !defined(HAVE_BASENAME)
char *basename(const char *file);
char *dirname (const char *file);
#else
#include <libgen.h>
#endif

#if !defined(HAVE_DAEMON)
int daemon( int nochdir, int noclose );
#endif

#if !defined(HAVE_TIMEGM)
time_t timegm( struct tm *tm ); 
#endif

int writen     ( int fd, char *ptr, int nbytes);
int read_line  ( int fd, char *ptr, int maxlen);
int readselect ( int fd, int sec);

char *mgettimestr( time_t t );

#ifdef   __cplusplus
}
#endif
#endif 
