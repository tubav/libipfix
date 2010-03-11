/*

 */
/*
**     example_collector_db.c - example ipfix collector
**
**     Copyright Fraunhofer FOKUS
**
**     $Date: 2005/12/19 17:54:46 $
**
**     $Revision: 1.8 $
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <ipfix.h>
#include "ipfix_db.h"
#include "ipfix_col.h"
#include "ipfix_def_fokus.h"
#include "ipfix_fields_fokus.h"
#include "misc.h"

/*------ defines ---------------------------------------------------------*/

#define MYSQL_HOST      "localhost"
#define MYSQL_DBNAME    "ipfix"
#define MYSQL_USER      "ipfix"
#define MYSQL_PASSWORD  "ipfix"

/*------ globals ---------------------------------------------------------*/

static char       progname[30];
static int        verbose_level = 0;
static int        *tcp_s=NULL, ntcp_s=0;       /* sockets */

/*------ static funcs ----------------------------------------------------*/

static void usage( char *taskname)
{
    const char helptxt[] =
        "[options]\n\n"
        "options:\n"
        "  -h                  this help\n"
        "  -p <portno>         listen on this port (default=4739)\n"
        "  -v                  increase verbose level\n"
        "\n";

    fprintf( stderr, "\nipfix example collector\n" );
    fprintf( stderr, "\nusage: %s %s\n", taskname, helptxt);

}/*usage*/

void exit_func ( int signo )
{
    if ( verbose_level && signo )
        fprintf( stderr, "\n[%s] got signo %d, bye.\n\n", progname, signo );

    if ( tcp_s ) {
        int i;

        for( i=0; i<ntcp_s; i++ )
            close( tcp_s[i] );
    }

    ipfix_col_cleanup();
    ipfix_cleanup();
    exit( 1 );

}/*exit_func*/

/*------ main ------------------------------------------------------------*/

int main (int argc, char *argv[])
{
    int           port;         /* port number */
    char          *dbuser;      /* db username */
    char          *dbpw;        /* db password */
    char          *dbname;      /* db name     */
    char          *dbhost;      /* hostname    */

    char          arg;          /* short options: character */
    char          opt[] = "hp:v";


    /** set default options
     */
    port    = 4739;
    dbhost  = MYSQL_HOST;
    dbname  = MYSQL_DBNAME;
    dbuser  = MYSQL_USER;
    dbpw    = MYSQL_PASSWORD;

    snprintf( progname, sizeof(progname), "%s", basename( argv[0]) );

    /* --- command line parsing ---
     */
    while( (arg=getopt( argc, argv, opt )) != EOF )
    {
        switch (arg) 
        {
          case 'p':
              if ((port=atoi(optarg)) <0)
              {
                  fprintf( stderr, "invalid -p argument!\n" );
                  exit(1);
              }
              break;

          case 'v':
              verbose_level ++;
              break;

          case 'h':
          default:
              usage(progname);
              exit(1);
        }
    }

    /** init loggin
     */
    mlog_set_vlevel( verbose_level );

    /** signal handler
     */
    signal( SIGKILL, exit_func );
    signal( SIGTERM, exit_func );
    signal( SIGINT,  exit_func );

    /** init ipfix lib
     */
    if ( ipfix_init() <0 ) {
        fprintf( stderr, "ipfix_init() failed: %s\n", strerror(errno) );
        exit(1);
    }
    if ( ipfix_add_vendor_information_elements( ipfix_ft_fokus ) <0 ) {
        fprintf( stderr, "ipfix_add_ie() failed: %s\n", strerror(errno) );
        ipfix_cleanup();
        exit(1);
    }

    /** activate database export
     */
    if ( ipfix_col_init_mysqlexport( dbhost, dbuser,
                                     dbpw, dbname ) <0 ) {
        fprintf( stderr, "cannot connect to database\n" );
        ipfix_cleanup();
        exit(1);
    }

    /** open ipfix collector port(s)
     */
    if ( ipfix_col_listen( &ntcp_s, &tcp_s, IPFIX_PROTO_TCP, 
                           port, AF_INET, 10 ) <0 ) {
        fprintf( stderr, "[%s] ipfix_listen(tcp) failed.\n",
                 progname );
        return -1;
    }

    /** event loop
     */
    (void) mpoll_loop( -1 );

    exit(1);
}
