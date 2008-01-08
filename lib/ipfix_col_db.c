/*
$$LIC$$
 */
/*
**     ipfix_col_db.c - IPFIX collector related funcs
**
**     Copyright Fraunhofer FOKUS
**
**     $Date: 2007/02/21 12:16:47 $
**
**     $Revision: 1.5 $
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

#include <misc.h>
#include "ipfix.h"
#include "ipfix_col.h"
#ifdef DBSUPPORT
#include "ipfix_db.h"
#include "ipfix_col_db.h"
#endif

/*------ defines ---------------------------------------------------------*/

#define MAXQUERYLEN       2000

/*------ structs ---------------------------------------------------------*/

#ifdef DBSUPPORT
typedef struct ipfix_export_data_db
{
    MYSQL *mysql;
} ipfixe_data_db_t;
#endif

/*------ globals ---------------------------------------------------------*/

static ipfix_col_info_t *g_colinfo =NULL;

/*----- revision id ------------------------------------------------------*/

static const char cvsid[]="$Id: ipfix_col_db.c,v 1.5 2007/02/21 12:16:47 luz Exp $";

/*----- globals ----------------------------------------------------------*/

#ifdef DBSUPPORT
static char    query[MAXQUERYLEN+1];
#endif

/*----- static funcs -----------------------------------------------------*/

#ifdef DBSUPPORT
int ipfix_export_newsrc_db( ipfixs_node_t *s, void *arg ) 
{
    ipfixe_data_db_t *data = (ipfixe_data_db_t*)arg;
    int32_t          exporter_id =0;

    if ( !data->mysql ) {
        errno = EINVAL;
        return -1;
    }

    snprintf( query, MAXQUERYLEN, 
              "SELECT %s FROM %s WHERE %s='%u' AND %s='%s'", 
              IPFIX_DB_EXP_ID, IPFIX_DB_EXPORTERS, IPFIX_DB_EXP_ODID, s->odid,
              IPFIX_DB_EXP_ADDR, ipfix_col_input_get_ident( s->input ) );
    if ( ipfix_db_get_int( data->mysql, query, &exporter_id ) !=0 ) {
        mlogf( 0, "[export_newsrc_db] query(%s) failed!\n", query );
        return -1;
    }

    if ( exporter_id == 0 ) {
        snprintf( query, MAXQUERYLEN, 
                  "INSERT INTO %s SET %s='%u', %s='%s', %s='-'", 
                  IPFIX_DB_EXPORTERS, IPFIX_DB_EXP_ODID, s->odid,
                  IPFIX_DB_EXP_ADDR, ipfix_col_input_get_ident( s->input ),
                  IPFIX_DB_EXP_DESCR );
        if ( mysql_query( data->mysql, query ) !=0 ) {
            mlogf( 0, "[export_newmsg_db] mysql_query(%s) failed: %s\n", 
                   query, mysql_error(data->mysql) );
            return -1;
        }
        exporter_id = (int32_t)mysql_insert_id( data->mysql );
    }

    s->exporterid = (unsigned int)exporter_id;
    return 0;
}

int ipfix_export_newmsg_db( ipfixs_node_t *s, ipfix_hdr_t *hdr, void *arg )
{
    ipfixe_data_db_t *data = (ipfixe_data_db_t*)arg;

    if ( data->mysql ) {
        snprintf( query, MAXQUERYLEN, 
                  "INSERT INTO %s SET %s='%u', %s='%lu'", 
                  IPFIX_DB_MESSAGETABLE,
                  IPFIX_DB_MSGT_EXPID, s->exporterid, IPFIX_DB_MSGT_TIME, 
                  (hdr->version==IPFIX_VERSION_NF9)?
                  (u_long)hdr->u.nf9.unixtime:(u_long)hdr->u.ipfix.exporttime );

        if ( mysql_query( data->mysql, query ) !=0 ) {
            mlogf( 0, "[export_newmsg_db] mysql_query(%s) failed: %s\n", 
                   query, mysql_error(data->mysql) );
            return -1;
        }
        s->last_msgid = (unsigned int) mysql_insert_id( data->mysql );
    }

    return 0;
}

int ipfix_export_trecord_db( ipfixs_node_t *s, ipfixt_node_t *t, void *arg )
{
    ipfixe_data_db_t *data = (ipfixe_data_db_t*)arg;
    char             *func = "export_trecord_db";

    if ( !data->mysql )
        return -1;

    /** build tablename
     */
    strcpy( t->tablename, "" );
    t->message_snr = s->last_message_snr-1; /*!!*/
    if ( ipfix_db_get_tablename( data->mysql, t->tablename, MAXTABLENAMELEN, 
                                 &(t->template_id), t->ipfixt, 1 ) <0 ) {
        /** todo!
         */
        mlogf( 1, "[%s] cannot build table name for template %d\n", 
               func, t->ipfixt->tid );
    }
    mlogf( 4, "[%s] template %d use table %s\n", 
           func, t->ipfixt->tid, t->tablename );
    return 0;
}

int ipfix_export_drecord_db( ipfixs_node_t      *s,
                             ipfixt_node_t      *t,
                             ipfix_datarecord_t *d,
                             void               *arg )
{
    ipfixe_data_db_t *data = (ipfixe_data_db_t*)arg;
    char             *func = "export_drecord_db";
    int              i, nbytes, binary_f=0;

    if ( !data->mysql )
        return -1;

    /* write data set into database
     * todo: log error if query buffer is too small
     */
    snprintf( query, MAXQUERYLEN, 
              "INSERT INTO %s SET %s='%u'", 
              t->tablename, IPFIX_DB_DT_MSGID, s->last_msgid );
    nbytes = strlen(query);
    for ( i=0; i<t->ipfixt->nfields; i++ ) {
#ifdef IENAME_COLUMNS
        nbytes += snprintf( query+nbytes, sizeof(query)-nbytes, ", %s='",
                            t->ipfixt->fields[i].elem->ft->name );
#else
        nbytes += snprintf( query+nbytes, sizeof(query)-nbytes,
                            ", " IPFIX_DB_COLUMNNAME_FORMAT "='",
                            t->ipfixt->fields[i].elem->ft->eno,
                            t->ipfixt->fields[i].elem->ft->ftype );
#endif
        if ( nbytes >= sizeof(query) )
            goto err;

        if ( t->ipfixt->fields[i].elem->ft->coding == IPFIX_CODING_BYTES ) {
            if ( ((d->lens[i]*2)+2) < (sizeof(query)-nbytes) ) {  
                nbytes += mysql_real_escape_string( data->mysql, query+nbytes, 
                                                    d->addrs[i], d->lens[i] );
                binary_f++;
            }
        }
        else {
            nbytes += t->ipfixt->fields[i].elem->snprint( query+nbytes, 
                                                          sizeof(query)-nbytes, 
                                                          d->addrs[i],
                                                          d->lens[i] );
        }
        if ( (nbytes+1) >= sizeof(query) )
            goto err;
        nbytes += snprintf( query+nbytes, sizeof(query)-nbytes, "'" );
    }
    mlogf( 4, "[%s] query: %s\n", func, query );
    if ( mysql_real_query( data->mysql, query, nbytes ) !=0 ) {
        mlogf( 0, "[%s] mysql_query(%s) failed: %s\n", 
               func, binary_f?"":query, mysql_error(data->mysql) );
        return -1;
    }

    /** insert template id into mapping table (if not already done)
     */
    if ( t->message_snr != s->last_message_snr ) {
        snprintf( query, MAXQUERYLEN, 
                  "INSERT INTO %s SET %s=%u, %s=%u ", 
                  IPFIX_DB_MAPPINGTABLE, IPFIX_DB_MT_MSGID,
                  s->last_msgid, IPFIX_DB_MT_TMPLID, t->template_id );
        mlogf( 4, "[%s] query: %s\n", func, query );
        if ( mysql_query( data->mysql, query ) !=0 ) {
            mlogf( 0, "[%s] mysql_query(%s) failed: %s\n", 
                   func, query, mysql_error(data->mysql) );
            return -1;
        }

        /** mark that message uses this template
         */
        t->message_snr = s->last_message_snr;
    }

    return 0;

 err:
    return -1;
}

int ipfix_export_init_db( char *dbhost, char *dbuser,
                          char *dbpw, char *dbname, void **arg )
{
    ipfixe_data_db_t *data;

    if ( (data=calloc( 1, sizeof(ipfixe_data_db_t))) ==NULL)
        return -1;

    if ( ipfix_db_open( &(data->mysql), dbhost, dbuser, dbpw, dbname ) <0 ) {
        free(data);
        return -1;
    }

    *arg = (void**)data;
    return 0;
}

void ipfix_export_cleanup_db( void *arg )
{
    ipfixe_data_db_t *data = (ipfixe_data_db_t*)arg;

    if ( data ) {
        if ( data->mysql )
            ipfix_db_close( &(data->mysql) );
        free(data);
    }
}
#endif

/*----- export funcs -----------------------------------------------------*/

int ipfix_col_init_mysqlexport( char *dbhost, char *dbuser, 
                                char *dbpw, char *dbname )
{
#ifdef DBSUPPORT
    void *data;

    if ( ipfix_export_init_db( dbhost, dbuser, dbpw, dbname, &data ) <0 ) {
        return -1;
    }

    if ( (g_colinfo=calloc( 1, sizeof(ipfix_col_info_t))) ==NULL) {
        ipfix_export_cleanup_db( data );
        return -1;
    }

    g_colinfo->export_newsource = ipfix_export_newsrc_db;
    g_colinfo->export_newmsg    = ipfix_export_newmsg_db;
    g_colinfo->export_trecord   = ipfix_export_trecord_db;
    g_colinfo->export_drecord   = ipfix_export_drecord_db;
    g_colinfo->export_cleanup   = ipfix_export_cleanup_db;
    g_colinfo->data = data;

    return ipfix_col_register_export( g_colinfo );
#endif
    errno = ENODEV;
    return -1;
}

void ipfix_col_stop_mysqlexport() 
{
    if ( g_colinfo ) {
        (void) ipfix_col_cancel_export( g_colinfo );
        free( g_colinfo );
        g_colinfo = NULL;
    }
}
