/*
$$LIC$$
 */
/*
**     ipfix_col_files.c - IPFIX collector related funcs
**
**     Copyright Fraunhofer FOKUS
**
**     $Date: 2009-03-27 20:19:27 +0100 (Fri, 27 Mar 2009) $
**
**     $Revision: 96 $
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

#include "mlog.h"
#include "ipfix.h"
#include "ipfix_col.h"

/*------ structs ---------------------------------------------------------*/

typedef struct ipfix_export_data_file
{
    char *datadir;
    
} ipfixe_data_file_t;

/*------ globals ---------------------------------------------------------*/

static ipfix_col_info_t *g_colinfo =NULL;

/*----- revision id ------------------------------------------------------*/

static const char cvsid[]="$Id: ipfix_col_files.c 96 2009-03-27 19:19:27Z csc $";

/*----- static funcs -----------------------------------------------------*/

static int export_newsource_file( ipfixs_node_t *s, void *arg ) 
{
    char  *func = "newsource_file";
    char  *datadir = ((ipfixe_data_file_t*)arg)->datadir;

    if ( datadir ) {
        char   tmpbuf[30], fname[PATH_MAX], suffix[30];
        int    i;
        time_t t = time( NULL );

        /** create directory and open output file
         */
        snprintf( s->fname, PATH_MAX, "%s/%s", datadir,
                  ipfix_col_input_get_ident( s->input ) );
        if ( (access( s->fname, R_OK ) <0 )
             && (mkdir( s->fname, S_IRWXU ) <0) ) {
            mlogf( 0, "[%s] cannot access dir '%s': %s\n",
                   func, s->fname, strerror(errno) );
            return -1;
        }
        snprintf( s->fname+strlen(s->fname), PATH_MAX-strlen(s->fname),
                  "/%u", (unsigned int)s->odid );
        if ( (access( s->fname, R_OK ) <0 )
             && (mkdir( s->fname, S_IRWXU ) <0) ) {
            mlogf( 0, "[%s] cannot access dir '%s': %s\n",
                   func, s->fname, strerror(errno) );
            return -1;
        }

        /** get filename (YYMMDD-x), check if there is already a file
         */
        for (i=1, *suffix=0; i; i++) {
            strftime( tmpbuf, 30, "%Y%m%d", localtime( &t ));
            snprintf( fname, sizeof(fname), "%s/%s%s", 
                      s->fname, tmpbuf, suffix );
            if ( access( fname, R_OK ) <0 ) {
                snprintf( s->fname, PATH_MAX, "%s", fname );
                break;
            }
            sprintf( suffix, "-%d", i );
        }

        if ( (s->fp=fopen( s->fname, "a" )) ==NULL ) {
            mlogf( 0, "[%s] cannot open outfile %s: %s\n",
                   func, s->fname, strerror(errno) );
            return -1;
        }
    }
    return 0;
}

static int export_newmsg_file( ipfixs_node_t *s, ipfix_hdr_t *hdr, void *arg )
{
    if ( s->fp ) {
        fprintf( s->fp, "#hdr: v%d, ", hdr->version );
        if ( hdr->version == IPFIX_VERSION_NF9 ) {
            fprintf( s->fp, "%.3f, %lu, ", 
                     (double)(hdr->u.nf9.sysuptime)/1000.0, 
                     (u_long)hdr->u.nf9.unixtime );
        }
        else {
            fprintf( s->fp, "%lu, ", 
                     (u_long)hdr->u.ipfix.exporttime );
        }
        fprintf( s->fp, "%lu\n", (u_long)hdr->seqno );
    }

    return 0;
}

static int export_trecord_file( ipfixs_node_t *s, ipfixt_node_t *t, void *arg )
{
    char  tmpbuf[2000];
    int   i, nbytes;

    if ( s->fp ) {
        sprintf( tmpbuf, "#%stemplate: %d", 
                 (t->ipfixt->nscopefields)?"option ":"",
                 t->ipfixt->tid );
        for ( i=0; i<t->ipfixt->nfields; i++ ) {
            nbytes = strlen(tmpbuf);
            snprintf( tmpbuf+nbytes, sizeof(tmpbuf)-nbytes, ", %d%s(%s:%d)",
                      t->ipfixt->fields[i].elem->ft->ftype, 
                      (i<t->ipfixt->nscopefields)?"s":"",
                      t->ipfixt->fields[i].elem->ft->name,
                      t->ipfixt->fields[i].flength );
        }
        fprintf( s->fp, "%s\n", tmpbuf );
        fflush( s->fp );
    }

    return 0;
}

static int export_drecord_file( ipfixs_node_t      *s,
                                ipfixt_node_t      *t,
                                ipfix_datarecord_t *data,
                                void               *arg )
{
    char          tmpbuf[2000];
    int           i, nbytes;

    if ( s->fp ) {
        /** write record into file
         */
        sprintf( tmpbuf, "%d", t->ipfixt->tid );
        for ( i=0; i<t->ipfixt->nfields; i++ ) {

            nbytes = strlen(tmpbuf);
            snprintf( tmpbuf+nbytes, sizeof(tmpbuf)-nbytes, ", " );
            nbytes += 2;
            t->ipfixt->fields[i].elem->snprint( tmpbuf+nbytes, 
                                                sizeof(tmpbuf)-nbytes, 
                                                data->addrs[i], 
                                                data->lens[i] );
        }

        fprintf( s->fp, "%s\n", tmpbuf );
        fflush( s->fp );
    }

    return 0;
}

static void export_cleanup_file( void *arg )
{
    ipfixe_data_file_t *data = (ipfixe_data_file_t*)arg;

    if ( data ) {
        free(data);
    }
}

/*----- export funcs -----------------------------------------------------*/

int ipfix_col_init_fileexport( char *datadir ) 
{
    ipfixe_data_file_t *data =NULL;

    if ( g_colinfo ) {
        errno = EAGAIN;
        return -1;
    }

    if ( (data=calloc( 1, sizeof(ipfixe_data_file_t))) ==NULL)
        return -1;
    if ( (g_colinfo=calloc( 1, sizeof(ipfix_col_info_t))) ==NULL) {
        free(data);
        return -1;
    }

    g_colinfo->export_newsource = export_newsource_file;
    g_colinfo->export_newmsg    = export_newmsg_file;
    g_colinfo->export_trecord   = export_trecord_file;
    g_colinfo->export_drecord   = export_drecord_file;
    g_colinfo->export_cleanup   = export_cleanup_file;
    data->datadir = datadir;
    g_colinfo->data = (void*)data;

    return ipfix_col_register_export( g_colinfo );
}

void ipfix_col_stop_fileexport() 
{
    if ( g_colinfo ) {
        (void) ipfix_col_cancel_export( g_colinfo );
        free( g_colinfo );
        g_colinfo = NULL;
    }
}

