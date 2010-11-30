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

#include <time.h>
#include <sys/time.h>

#include "config.h"
#include "hashtable.h"

#ifdef   __cplusplus
extern "C" {
#endif

/** global logfile
 */
extern int  mlog_vlevel;
extern FILE *mlog_fp;

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

void errorf ( char fmt[], ... ) __attribute__ ((format (printf, 1, 2)));
void debugf ( char fmt[], ... ) __attribute__ ((format (printf, 1, 2)));
void mlogf  ( int verbosity,
              char fmt[], ... ) __attribute__ ((format (printf, 2, 3)));
int  mlog_open  ( char *logfile, char *prefix );
void mlog_close ( void );
void mlog_set_vlevel( int vlevel );

char *mgettimestr( time_t t );

/** poll funcs
 */
#define MPOLL_IN        1
#define MPOLL_OUT       2
#define MPOLL_EXCEPT    4

typedef void *mptimer_t;
typedef void (*pcallback_f)(int fd, int mask, void *arg);
typedef void (*tcallback_f)(void *arg);

int       mpoll_fdadd    ( int fd, int mask, pcallback_f callback, void *arg );
void      mpoll_fdrm     ( int fd );
mptimer_t mpoll_timeradd ( int32_t sec, tcallback_f callback, void *arg );
mptimer_t mpoll_utimeradd( int32_t usec, tcallback_f callback, void *arg );
void      mpoll_timerrm  ( mptimer_t timer );
int       mpoll_loop     ( int timeout );
void      mpoll_break    ( void );
void      mpoll_cleanup  ( void );

/** sting list
 */
typedef struct mstrnode
{
    struct mstrnode *next;
    char            *str;
} mstrnode_t;

void mstrlist_free( mstrnode_t **root );

/** get/setGlobal() hashtable 
 */

struct hashtable  *ht_globals;

void     initGlobals();
void*    setGlobal(char *key, void *value);
void*    getGlobal(char *key);
void*    removeGlobal(char *key);
int      countGlobals();
void     freeGlobals();

#ifdef   __cplusplus
}
#endif
#endif 
