/*
** ipfix_db.h - export declarations for ipfix_db funcs
**
** Copyright Fraunhofer FOKUS
**
** $Id: ipfix_db.h 23 2008-08-14 11:30:44Z tor $
**
** Remarks:
**
*/
#ifndef _IPFIX_DB_H
#define _IPFIX_DB_H

#include <time.h>
#include <sys/time.h>
#include <mysql/mysql.h>
#include "ipfix.h"

#ifdef   __cplusplus
extern "C" {
#endif

#define IPFIX_DB_EXPORTERS         "ipfix_exporters"
#define IPFIX_DB_EXP_ID            "id"
#define IPFIX_DB_EXP_ADDR          "ipaddr"
#define IPFIX_DB_EXP_ODID          "observation_domain"
#define IPFIX_DB_EXP_DESCR         "description"

#define IPFIX_DB_MESSAGETABLE      "ipfix_messages"
#define IPFIX_DB_MSGT_ID           "id"
#define IPFIX_DB_MSGT_EXPID        "id_exporters"
#define IPFIX_DB_MSGT_TIME         "tstamp"

#define IPFIX_DB_TEMPLATETABLE     "ipfix_templates"
#define IPFIX_DB_TMPL_ID           "id"
#define IPFIX_DB_TMPL_IDENT        "template_ident"
#define IPFIX_DB_TMPL_TABLENAME    "table_name"

#define IPFIX_DB_MAPPINGTABLE      "ipfix_mapping"
#define IPFIX_DB_MT_MSGID          "id_ipfix_messages"
#define IPFIX_DB_MT_TMPLID         "id_ipfix_templates"

#define IPFIX_DB_TABLENAME_PREFIX  "ipfix_"
#define IPFIX_DB_TABLENAME_FORMAT  "ipfix_data_%u"
#define IPFIX_DB_DT_MSGID          "id_ipfix_messages"
#define IPFIX_DB_COLUMNNAME_FORMAT "ie%x_%x"

#define MAXTABLENAMELEN   64        /* todo: change this */
#define MAXBINARYIELEN    4096      /* todo: change this */

int  ipfix_db_open( MYSQL **mysqlp,
                    char *dbhost, char *dbuser, char *dbpw, char *dbname );
void ipfix_db_close( MYSQL **mysqlp );
int  ipfix_db_get_int( MYSQL *mysqlp, char *query, int *i );

int  ipfix_db_get_tablename( MYSQL *mysql,
                             char *buf, size_t buflen, uint32_t *id,
                             ipfix_template_t *t,  int create_table_flag );
int ipfix_db_get_columnname( int eno, int type, char *buf, size_t buflen );

#ifdef   __cplusplus
}
#endif
#endif 
