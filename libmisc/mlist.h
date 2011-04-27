/*
$$LIC$$
 */
/*
** mlist.h
**
** Copyright Fraunhofer FOKUS
**
** $Date: 2008-10-14 12:40:12 +0200 (Tue, 14 Oct 2008) $
**
** $Revision: 1.1 $
**
*/
#ifndef _MLIST_H
#define _MLIST_H

#include <time.h>
#include <sys/time.h>

#include "config.h"
#ifdef   __cplusplus
extern "C" {
#endif

/** sting list
 */
typedef struct mstrnode
{
    struct mstrnode *next;
    char            *str;
} mstrnode_t;

void mstrlist_free( mstrnode_t **root );

#ifdef   __cplusplus
}
#endif
#endif 
