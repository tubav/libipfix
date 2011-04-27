/*
** mlog.h - export declarations for mlog funcs
**
** Copyright Fraunhofer FOKUS
**
** $Id: mlog.h 22 2008-08-12 08:34:40Z tor $
**
** Remarks: This is experimental code!
**
*/
#ifndef _MLOG_H
#define _MLOG_H

#ifdef   __cplusplus
extern "C" {
#endif

void errorf ( char fmt[], ... ) __attribute__ ((format (printf, 1, 2)));
void debugf ( char fmt[], ... ) __attribute__ ((format (printf, 1, 2)));
void mlogf  ( int verbosity,
              char fmt[], ... ) __attribute__ ((format (printf, 2, 3)));
int  mlog_open  ( char *logfile, char *prefix );
void mlog_close ( void );
void mlog_set_vlevel( int vlevel );
int  mlog_get_vlevel();

#ifdef   __cplusplus
}
#endif
#endif 
