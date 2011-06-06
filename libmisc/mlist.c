/*
$$LIC$$
 */
/*
** mlist.c
**
** Copyright Fraunhofer FOKUS
**
** $Date: 2008-10-14 12:40:12 +0200 (Di, 14. Okt 2008) $
**
** $Revision: 1.1 $
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#include "misc.h"

/*------ defines ---------------------------------------------------------*/

#define xfree(p)          {if ((p)) free((p));}

/*------ stuctures -------------------------------------------------------*/

/*------ globals ---------------------------------------------------------*/

/*------ revision id -----------------------------------------------------*/

static char const cvsid[]="$Id: mlist.c 956 2008-10-14 10:40:12Z hir $";

/*------ export funcs ----------------------------------------------------*/

/*
 * name       : mstrlist_free
 * description: 
 */
void mstrlist_free( mstrnode_t **root )
{
    mstrnode_t *n;

    if ( root==NULL )
        return;
    else
        n = *root;

    while ( n ) {
        n = (*root)->next;
        xfree( (*root)->str );
        xfree( (*root) );
        *root = n;
    }
}
