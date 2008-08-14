/*
$$LIC$$
 *
 * ipfix_db.c - database access functions
 *
 * Copyright Fraunhofer FOKUS
 *
 * $Date$
 *
 * $Revision$
 *
 * Remarks: make this funcs mysql independent
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "misc.h"
#include "ipfix_db.h"

/*------ defines ---------------------------------------------------------*/

#define MAXQUERYLEN       2000

/*------ stuctures -------------------------------------------------------*/

/*------ globals ---------------------------------------------------------*/

static char    query[MAXQUERYLEN+1];

/*------ revision id -----------------------------------------------------*/

static char const cvsid[]="$Id$";

/*------ static funcs ----------------------------------------------------*/

/*
 * Name       : ipfix_db_check_table
 * Parameters : none
 * Description: 
 * Returns    : 0 if ok, -1 on error
 * Remarks    : 
 */
int ipfix_db_check_table( MYSQL *mysql, char *tablename, char *createcmd )
{
    MYSQL_RES   *result;
    char        *func = "db_check_table";

    /* check if table exists (show tables like 'tablename')
     */
    if ( (result=mysql_list_tables( mysql, tablename )) ==NULL ) {
        mlogf( 1, "[%s] mysql_list_tables() failed: %s\n", 
               func, mysql_error(mysql) );
        return -1;
    }

    if ( (mysql_num_rows( result )) <1 ) {
        /* create table */
        if ( mysql_query( mysql, createcmd ) !=0 ) {
            mlogf( 0, "[%s] %s\n", func, createcmd );
            mlogf( 0, "[%s] mysql_query() failed: %s\n",
                   func, mysql_error(mysql) );
            mysql_free_result( result );
            return -1;
        }

        mlogf( 2, "[%s] table %s created\n", func, tablename );
    }

    mysql_free_result( result );
    mlogf( 3, "[%s] table %s found\n", func, tablename );

    return 0;
}

/*------ export funcs ----------------------------------------------------*/

/*
 * Name       : ipfix_db_open
 * Parameters : none
 * Description: 
 * Returns    : 0 if ok, -1 on error
 * Remarks    : 
 */
int ipfix_db_open( MYSQL **mysqlp,
                   char *dbhost, char *dbuser, char *dbpw, char *dbname )
{
    char        sql[MAXQUERYLEN+1];
    char        *func = "ipfix_db_open";

    if ( ((*mysqlp)=mysql_init(NULL)) ==NULL ) {
        mlogf( 0, "[%s] cannot init DB access!\n", func );
        return -1;
    }
#if defined(MYSQL_OPT_RECONNECT)
    else {
        my_bool option = 1; /* true */

        if ( mysql_options( *mysqlp, MYSQL_OPT_RECONNECT, &option ) <0 )
            mlogf( 0, "[%s] warning: mysql_options( RECONNECT ) failed!\n",
                   func );
    }
#endif

    /* connect to database
     */
    if ( mysql_real_connect( (*mysqlp), dbhost, dbuser, 
                             dbpw, dbname, 0, NULL, 0 ) ==NULL ) {
        mlogf( 0, "[db_open] mysql_real_connect() failed: %s\n",
               mysql_error(*mysqlp) );
        return -1;
    }
#ifdef HAVE_MYSQL_OPT_RECONNECT
    if (  mysql_options( (*mysqlp), MYSQL_OPT_RECONNECT, 1 ) < 0 ) {
        mlogf( 1, "[%s] info: mysql_option(MYSQL_OPT_RECONNECT) failed: %s\n",
               func, mysql_error(*mysqlp) );
    }
#endif

    /* check if ipfix exporters table exists
     */
    snprintf( sql, MAXQUERYLEN,
              "CREATE TABLE %s ( "
              " %s INT NOT NULL AUTO_INCREMENT, %s INT UNSIGNED NOT NULL,"
              " %s BLOB NOT NULL, %s BLOB, PRIMARY KEY (%s) ) ",
              IPFIX_DB_EXPORTERS, 
              IPFIX_DB_EXP_ID, IPFIX_DB_EXP_ODID, IPFIX_DB_EXP_ADDR,
              IPFIX_DB_EXP_DESCR, IPFIX_DB_EXP_ID );

    if ( ipfix_db_check_table( *mysqlp, IPFIX_DB_EXPORTERS, sql ) <0 ) {
        mlogf( 0, "[%s] checking table %s failed: %s\n", 
               IPFIX_DB_EXPORTERS, func, mysql_error(*mysqlp) );
        return -1;
    }

    /* check if ipfix message table exists
     */
    snprintf( sql, MAXQUERYLEN,
              "CREATE TABLE %s ( "
              " %s INT NOT NULL AUTO_INCREMENT, "
              " %s INT NOT NULL, %s INT NOT NULL, PRIMARY KEY (%s) ) ",
              IPFIX_DB_MESSAGETABLE, 
              IPFIX_DB_MSGT_ID, IPFIX_DB_MSGT_EXPID,
              IPFIX_DB_MSGT_TIME, IPFIX_DB_MSGT_ID );

    if ( ipfix_db_check_table( *mysqlp, IPFIX_DB_MESSAGETABLE, sql ) <0 ) {
        mlogf( 0, "[%s] checking table %s failed: %s\n", 
               IPFIX_DB_MESSAGETABLE, func, mysql_error(*mysqlp) );
        return -1;
    }

    /** check if ipfix templates table exists
     */
    snprintf( sql, MAXQUERYLEN,
              "CREATE TABLE %s ( "
              " %s INT NOT NULL AUTO_INCREMENT, "
              " %s BLOB, %s BLOB, PRIMARY KEY (%s) ) ",
              IPFIX_DB_TEMPLATETABLE, 
              IPFIX_DB_TMPL_ID, IPFIX_DB_TMPL_IDENT,
              IPFIX_DB_TMPL_TABLENAME, IPFIX_DB_TMPL_ID );

    if ( ipfix_db_check_table( *mysqlp, IPFIX_DB_TEMPLATETABLE, sql ) <0 ) {
        mlogf( 0, "[%s] checking table %s failed: %s\n", 
               IPFIX_DB_TEMPLATETABLE, func, mysql_error(*mysqlp) );
        return -1;
    }

    /** check if mapping table exists
     */
    snprintf( sql, MAXQUERYLEN,
              "CREATE TABLE %s ( "
              " %s INT NOT NULL, %s INT NOT NULL, INDEX(%s) ) ",
              IPFIX_DB_MAPPINGTABLE, IPFIX_DB_MT_MSGID,
              IPFIX_DB_MT_TMPLID, IPFIX_DB_MT_MSGID );

    if ( ipfix_db_check_table( *mysqlp, IPFIX_DB_MAPPINGTABLE, sql ) <0 ) {
        mlogf( 0, "[%s] checking table %s failed: %s\n", 
               IPFIX_DB_MAPPINGTABLE, func, mysql_error(*mysqlp) );
        return -1;
    }

    return 0;
}

void ipfix_db_close( MYSQL **mysqlp )
{
    mysql_close( *mysqlp );
    (*mysqlp) = NULL;
}

int ipfix_db_get_int( MYSQL *mysqlp, char *query, int *i )
{
    MYSQL_RES     *result;
    MYSQL_ROW     row;

    if ( mysql_query( mysqlp, query ) !=0) {
        mlogf( 0, "[%s] mysql_query(%s) failed: %s\n",
               __func__, query, mysql_error(mysqlp) );
        return -1;
    }
    if ( (result=mysql_store_result(mysqlp)) ==NULL ) {
        mlogf( 0, "[%s] mysql_store_result() failed: %s\n",
               __func__, mysql_error(mysqlp) );
        return -1;
    }

    row = mysql_fetch_row(result);
    if ( row && row[0] ) {
        *i = atoi( row[0] );
    }
    else {
        *i = 0;
    }

    mysql_free_result( result );
    return 0;
}

/*
 * name       : ipfix_db_create_table
 * parameters : > tablename          - table name
 *              > template           - ipfix template 
 * description: crate table to store result data of that template
 * returns    : 0 if ok, -1 on error
 * todo       : 
 */
int ipfix_db_create_table( MYSQL *mysql, char *tablename, ipfix_template_t *t )
{
    int         i;
    char        tmpbuf[MAXTABLENAMELEN];
    char        *func = "ipfix_db";

    /** build query
     */
    snprintf( query, MAXQUERYLEN, 
              "CREATE TABLE %s ( %s INT UNSIGNED NOT NULL", 
              tablename, IPFIX_DB_DT_MSGID );
    for ( i=0; i<t->nfields; i++ ) {
        if ( ipfix_db_get_columnname( t->fields[i].elem->ft->eno,
                                      t->fields[i].elem->ft->ftype,
                                      tmpbuf, sizeof(tmpbuf)) <0 ) {
            return -1;
        }

        switch( t->fields[i].elem->ft->coding ) {
          case IPFIX_CODING_INT:
              snprintf( query+strlen(query), MAXQUERYLEN-strlen(query), 
                        ", %s %sINT ", tmpbuf,
                        (t->fields[i].elem->ft->length>4)?"BIG":"" );
              break;
          case IPFIX_CODING_UINT:
              snprintf( query+strlen(query), MAXQUERYLEN-strlen(query), 
                        ", %s %sINT UNSIGNED ", tmpbuf,
                        (t->fields[i].elem->ft->length>4)?"BIG":"" );
              break;
          case IPFIX_CODING_STRING:
              snprintf( query+strlen(query), MAXQUERYLEN-strlen(query), 
                        ", %s TEXT ", tmpbuf );
              break;
          case IPFIX_CODING_BYTES:
          default:
              snprintf( query+strlen(query), MAXQUERYLEN-strlen(query), 
                        ", %s VARBINARY(%d) ", tmpbuf, MAXBINARYIELEN );
              break;
        }
    }
    snprintf( query+strlen(query), MAXQUERYLEN-strlen(query), " ) " );

    /** crate table
     */
    if ( mysql_query( mysql, query ) !=0 ) {
        mlogf( 0, "[%s] %s\n", func, query );
        mlogf( 0, "[%s] mysql_query() failed: %s\n", 
               func, mysql_error(mysql) );
        return -1;
    }
    mlogf( 2, "[%s] table %s created\n", func, tablename );

    return 0;
}

/*
 * name       : ipfix_db_get_tablename
 * parameters : <> buf, buflen        - table name
 *              <  template_id        - template id (db table index)
 *               > template           - ipfix template 
 * description: get table name to store result data of that template
 * returns    : 0 if ok, -1 on error
 */
int ipfix_db_get_tablename( MYSQL *mysql, char *tablename, size_t tablenamelen, 
                            uint32_t *template_id, 
                            ipfix_template_t *t, int create_table_flag )
{
    MYSQL_RES     *result;
    MYSQL_ROW     row;
    my_ulonglong  num_rows;
    int           i, j, found;
    int           min, last, cur_eno, next_eno, ft, eno;
    char          *ident;
    size_t        len = t->nfields*10;  /* todo: calc real lenght! */
    char          *func = "ipfix_db";

    if ( tablenamelen > MAXTABLENAMELEN )
        tablenamelen = MAXTABLENAMELEN;

    /* alloc mem */
    if ( (ident=calloc( len+1, sizeof(char) )) ==NULL) {
        mlogf( 1, "[%s] out of memory", func );
        return -1;
    }

    /* in postgresql columns start with non-digit */
    snprintf( ident, len, "%s", IPFIX_DB_TABLENAME_PREFIX );

    /** build ident string (sorted ftids separated by '_')
     */
    for ( cur_eno=0, next_eno=0x7FFFFFFF;; ) {
        last = 0;
        min  = 0xFFFF;

        if ( cur_eno ) {
            snprintf( ident+strlen(ident), len-strlen(ident), 
                      "v%x_", cur_eno );
        }

        for ( j=0, found=0; j<t->nfields; j++ ) {
            for ( i=0; i<t->nfields; i++ ) {
                ft  = t->fields[i].elem->ft->ftype;
                eno = t->fields[i].elem->ft->eno;

                if ( (eno==cur_eno) && (ft<min) && (ft>last) ) {
                    min = ft;
                    found ++;
                }

                if ( (eno<next_eno) && (eno>cur_eno) )
                    next_eno=eno;
            }

            if ( min == 0xFFFF )
                break;

            if ( j && (found>1) )
                strcat( ident, "_" );

            snprintf( ident+strlen(ident), len-strlen(ident), "%x", min );
            last = min;
            min  = 0xFFFF;
        }

        if ( next_eno == 0x7FFFFFFF )
            break;
            
        if ( (next_eno>cur_eno) && found )
            strcat( ident, "_" );

        cur_eno  = next_eno;
        next_eno = 0x7FFFFFFF;
    }

    /** check if ident is in templates table
     */
    snprintf( query, MAXQUERYLEN,
              "SELECT %s, %s FROM %s WHERE %s='%s' ",
              IPFIX_DB_TMPL_TABLENAME, IPFIX_DB_TMPL_ID,
              IPFIX_DB_TEMPLATETABLE, IPFIX_DB_TMPL_IDENT, ident );

    if ( mysql_query( mysql, query ) !=0 ) {
        mlogf( 0, "[%s] %s\n", func, query );
        mlogf( 0, "[%s] mysql_query() failed: %s\n", 
               func, mysql_error(mysql) );
        free(ident);
        return -1;
    }

    if ( (result=mysql_store_result(mysql)) ==NULL ) {
        mlogf( 0, "[%s] mysql_store_result() failed: %s\n",
               func, mysql_error(mysql) );
        free(ident);
        return -1;
    }

    if ( (num_rows=mysql_num_rows( result )) <0 ) {
        mlogf( 0, "[%s] mysql_num_rows() failed: %s\n",
               func, mysql_error(mysql) );
        mysql_free_result(result);
        free(ident);
        return -1;
    }

    if ( num_rows == 0 ) {
        uint32_t      id;

        /** new ident -> insert ident into template table
         */
        mysql_free_result(result);
        snprintf( query, MAXQUERYLEN,
                  "INSERT INTO %s SET %s='x', %s='x' ",
                  IPFIX_DB_TEMPLATETABLE, IPFIX_DB_TMPL_IDENT,
                  IPFIX_DB_TMPL_TABLENAME );

        if ( mysql_query( mysql, query ) !=0 ) {
            mlogf( 0, "[%s] mysql_query(%s) failed: %s\n", 
                   func, query, mysql_error(mysql) );
            free(ident);
            return -1;
        }
        id = (uint32_t) mysql_insert_id( mysql );

        /** create real table name
         ** ident          <- ident fits into tablename maxlen
         ** template<idx>  <- otherwise idx is index of template table
         */
        if ( strlen(ident) > tablenamelen )
            snprintf( tablename, tablenamelen, "%s%u",
                      IPFIX_DB_TABLENAME_PREFIX, id );
        else
            snprintf( tablename, tablenamelen, "%s", ident );
        *template_id = id;

        /** update entry
         */
        snprintf( query, MAXQUERYLEN,
                  "UPDATE %s SET %s='%s', %s='%s' WHERE %s='%d' ",
                  IPFIX_DB_TEMPLATETABLE, IPFIX_DB_TMPL_IDENT, ident,
                  IPFIX_DB_TMPL_TABLENAME, tablename, IPFIX_DB_TMPL_ID, id );
        if ( mysql_query( mysql, query ) !=0 ) {
            mlogf( 0, "[%s] mysql_query(%s) failed: %s\n", 
                   func, query, mysql_error(mysql) );
            free(ident);
            return -1;
        }
        free(ident);
    }
    else {
        /** ident exists -> get table name
         */
        free(ident);
        row = mysql_fetch_row(result);
        if ( (row==NULL) || row[0]==NULL || row[1]==NULL) {
            mlogf( 0, "[%s] ups\n", func );
            mysql_free_result( result );
            return -1;
        }
        snprintf( tablename, tablenamelen, "%s", row[0] );
        *template_id = (uint32_t) atoi(row[1]);
        mysql_free_result( result );
    }

    if ( create_table_flag ) {
        /** check if tablename exists
         ** todo: check if table has right layout
         */
        if ( (result=mysql_list_tables( mysql, tablename )) ==NULL ) {
            mlogf( 1, "[%s] mysql_list_tables() failed: %s\n", 
                   func, mysql_error(mysql) );
            return -1;
        }

        if ( (mysql_num_rows( result )) <1 ) {
            mysql_free_result(result);
            if ( ipfix_db_create_table( mysql, tablename, t ) <0 )
                return -1;
        }
        else {
            mysql_free_result(result);
            mlogf( 2, "[%s] table %s found\n", func, tablename );
        }
    }

    return 0;
}

/* name       : ipfix_db_get_columnname
 * parameters :  > ifh                - handle
 *               > eno                - enterprise number
 *               > type               - field type (=element id)
 *              <> buf, buflen        - column name
 * description: get column name
 * returns    : 0 if ok, -1 on error
 */
int ipfix_db_get_columnname( int eno, int type, char *buf, size_t buflen )
{
#ifdef IENAME_COLUMNS
    ipfix_field_t *elem;

    if ( (elem=ipfix_get_ftinfo( int eno, int type )) !=NULL ) {
        snprintf( buf, buflen, "%s", elem->ft->name );
        return 0;
    }
    else {
        errno = EINVAL;
    }
#endif
    snprintf( buf, buflen, IPFIX_DB_COLUMNNAME_FORMAT, eno, type );
    return 0;
}
