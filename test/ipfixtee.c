/*

 */
/*
**     nodedegree.c - calculate node in/out-degree from ipfix flows
**
**     Copyright Fraunhofer FOKUS
** 
**     $Revision: 0.1 $
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <libgen.h>
#include <limits.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <time.h>

#include <ipfix.h>
#include <misc.h>
#include "ipfix_col.h"
#include "ipfix_def_fokus.h"
#include "ipfix_fields_fokus.h"

#include <libhashish.h>

#ifndef IPFIX_FOKUS_DEF_H
  #define IPFIX_ENO_FOKUS                12325
  #define IPFIX_FT_SOURCEIPV4FANOUT      315
  #define IPFIX_FT_DESTINATIONIPV4FANIN  316
#endif


/*------ globals ---------------------------------------------------------*/

static char       progname[30];
static int        verbose_level = 0;
static int        *tcp_s=NULL, ntcp_s=0;       /* socket */

static ipfix_col_info_t*   g_colinfo = NULL;
static hi_handle_t*        indegree;
static hi_handle_t*        outdegree;
static hi_handle_t*        flows;
static hi_handle_t*        templates;

typedef struct {
    uint32_t   srcIp;
    uint32_t   dstIp;
} flow_t;

ipfix_t           *ipfixh;
int               sourceid = 680335508;
int               degrade  = 0;
char             *chost    = NULL;
char             *datadir  = NULL;

/*------ static funcs ----------------------------------------------------*/

static void usage( char *taskname)
{
    const char helptxt[] =
        "[options]\n\n"
        "options:\n"
        "  -h                  this help\n"
        "  -p <portno>         listen on this port (default=4739)\n"
        "  -c <collector>      collector address\n"
        "  -o                  set the observation domain id\n"
        "  -P <portno>         collector port number (default=4740)\n"
        "  -r <seconds>        degradation/sliding window size (default:0=off)\n"
        "  -s                  send data via SCTP\n"
        "  -t                  send data via TCP (default)\n"
        "  -u                  send data via UDP\n"
        "  -l                  log received data also to stdout\n"
        "  -f <datadir>        log received data also to a txt-file in datadir\n"
        "  -v                  increase verbose level\n"
        "\n";

    fprintf( stderr, "\nipfix node fanin/fanout processor\n" );
    fprintf(stderr,"\nusage: %s %s\n", taskname, helptxt);

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

    if (datadir) 
	free (datadir);
    if (chost)
        free (chost);

    exit( 1 );
}


/*------ callback templates ----------------------------------------------*/

int export_newsource_cb( ipfixs_node_t *s, void *arg );
int export_newmsg_cb( ipfixs_node_t *s, ipfix_hdr_t *hdr, void *arg );
int export_trecord_cb( ipfixs_node_t *s, ipfixt_node_t *t, void *arg );
int export_drecord_cb( ipfixs_node_t *s, ipfixt_node_t *t,
                       ipfix_datarecord_t *data, void *arg );
void export_cleanup_cb( void *arg );

/*------ main ------------------------------------------------------------*/

int main (int argc, char *argv[])
{
    char          opt;             /* short options: character */
    char          optstr[] = "hp:c:P:r:stuvlf:";
    int           port;            /* port number */
    int           cport    = 4740; /* target collector port number */
    int           protocol = IPFIX_PROTO_TCP;
    int           do_log = 0; /* flag to stre whether logging to stdout was enabled */

    /** set default options
     */
    port    = 4739;
    datadir = NULL;
    snprintf( progname, sizeof(progname), "%s", basename( argv[0]) );

    /* --- command line parsing ---
     */
    int cisset = 0;
    while( ( opt = getopt( argc, argv, optstr ) ) != EOF ) {

        switch ( opt )
        {
          case 'o':
              if ((sourceid=atoi(optarg)) <0) {
                  fprintf( stderr, "Invalid -o argument!\n" );
                  exit(1);
              }
              break;

          case 'p':
              if ((port=atoi(optarg)) <0) {
                  fprintf( stderr, "Invalid -p argument!\n" );
                  exit(1);
              }
              break;

          case 'v':
              verbose_level ++;
              break;

          case 'l':
              do_log = 1;
              break;

          case 'P':
              if ((cport=atoi(optarg)) <0) {
                fprintf( stderr, "Invalid -P argument!\n" );
                exit(1);
              }
              break;

          case 'r':
              if ((degrade=atoi(optarg)) <0) {
                fprintf( stderr, "Invalid -r argument!\n" );
                exit(1);
              }
              break;

          case 'c':
              if (chost)
                  free(chost); /* executed in case of multiple "-c <collectorhost>" */
              chost = strdup(optarg);
              cisset = 1;
              break;

          case 'f':
              if (datadir) 
                  free(datadir); /* executed in case of multiple "-f <datadir>" */
              datadir = strdup(optarg);
              break;

          case 's':
              protocol = IPFIX_PROTO_SCTP;
              break;

          case 't':
              protocol = IPFIX_PROTO_TCP;
              break;

          case 'u':
              protocol = IPFIX_PROTO_UDP;
              break;

          case 'h':
          default:
              usage(progname);
              exit(1);
        }
    }

    if (!chost && !datadir && !do_log) {
        fprintf( stderr, "You must specify either an IPFIX target collector host or a datadir or enable log to stdout. Type option '-h' for help.\n");
        exit(1);
    }

    /** init hashtable structures */
    hi_init_uint32_t(&indegree,  1000);
    hi_init_uint32_t(&outdegree, 1000);
    hi_init_uint32_t(&flows,     1000);
    hi_init_str     (&templates, 100);

    /** init logging
     */
    mlog_set_vlevel( verbose_level );

    /** init ipfix lib
     */
    if ( ipfix_init() <0 ) {
        fprintf( stderr, "ipfix_init() failed: %s\n", strerror(errno) );
        exit(1);
    }
    if ( ipfix_add_vendor_information_elements( ipfix_ft_fokus ) <0 ) {
        fprintf( stderr, "ipfix_add_ie() failed: %s\n", strerror(errno) );
        exit(1);
    }
 
    /** init ipfix import/export 
     */
    if ( ipfix_open( &ipfixh, sourceid, IPFIX_VERSION ) <0 ) {
        fprintf( stderr, "ipfix_open() failed: %s\n", strerror(errno) );
        exit(1);
    }

    /** signal handler
     */
    signal( SIGKILL, exit_func );
    signal( SIGTERM, exit_func );
    signal( SIGINT,  exit_func );

    /** initialize callback methods
     */

    /** activate stdout log output
     *  if "-l" was specified on cmd line
     */
    if (do_log) {
        (void) ipfix_col_start_msglog( stdout );
    }

    /** activate file export
     *  if "-f <datadir>" was specified on cmd line
     */
    if (datadir) {
        (void) ipfix_col_init_fileexport( datadir );
    }

    if (chost) {
        if ( ipfix_add_collector( ipfixh, chost, cport, protocol ) <0 ) {
            fprintf( stderr, "ipfix_add_collector(%s,%d) failed: %s\n",
                     chost, cport, strerror(errno));
            exit(1);
        }

        /** activate callback for re-export
         */
        if ( (g_colinfo=calloc( 1, sizeof(ipfix_col_info_t))) ==NULL) {
          fprintf( stderr, "a calloc failed while initializing callback methods.\n" );
          return -1;
        }

        g_colinfo->export_newsource = export_newsource_cb;
        g_colinfo->export_newmsg    = export_newmsg_cb;
        g_colinfo->export_trecord   = export_trecord_cb;
        g_colinfo->export_drecord   = export_drecord_cb;
        g_colinfo->export_cleanup   = export_cleanup_cb;
        g_colinfo->data = NULL;

        if ( ipfix_col_register_export( g_colinfo ) <0 ) {
            fprintf( stderr, "ipfix_col_register_export() failed: %s\n", strerror(errno) );
            exit(1);
        }
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

/*------ functionality ----------------------------------------------*/

void print_ip ( uint32_t ipaddr ) {
  int j;
  for (j=0;j<3;j++) {
    fprintf (stdout, "%hhu.", *(uint8_t*)(((void*)&ipaddr)+j));
  }
  fprintf (stdout, "%hhu", *(uint8_t*)(((void*)&ipaddr)+j));
}

int export_newsource_cb( ipfixs_node_t *s, void *arg ) {
  fprintf (stdout, "New source detected: %i\n", s->odid);
  return 0;
}

int export_newmsg_cb( ipfixs_node_t *s, ipfix_hdr_t *hdr, void *arg ) {
  //fprintf (stdout, "Got IPFIX message. Yay.\n");
  return 0;
}

int export_trecord_cb( ipfixs_node_t *s, ipfixt_node_t *t, void *arg ) {
  return 0;
}

int reexport (ipfixt_node_t *t, ipfix_datarecord_t *d) {
  ipfix_template_t* outtemplate;

  if (hi_get_str(templates, t->ident, (void**)&outtemplate )) {
    if ( ipfix_new_data_template( ipfixh, &outtemplate, t->ipfixt->nfields ) <0 ) {
        fprintf( stderr, "ipfix_new_template() failed in reexport: %s\n", 
                 strerror(errno) );
        exit(1);
    }
    int i;
    fprintf( stdout, "reexporting template %s as id %i.\n", t->ident, outtemplate->tid );
    for (i=0; i < t->ipfixt->nfields; i++) {
      if (ipfix_add_field( ipfixh, outtemplate, 
                           t->ipfixt->fields[i].elem->ft->eno, t->ipfixt->fields[i].elem->ft->ftype, t->ipfixt->fields[i].flength )
           <0 ) {
        fprintf( stderr, "ipfix_add_field() failed in reexport: %s - eno = %i, type = %i, length = %i\n", 
                 strerror(errno), t->ipfixt->fields[i].elem->ft->eno, t->ipfixt->fields[i].elem->ft->ftype, t->ipfixt->fields[i].flength);
        exit(1);
      }
      //fprintf( stdout, "  added field %i:%i(%i||%i,%i)\n", outtemplate->fields[i].elem->ft->eno, outtemplate->fields[i].elem->ft->ftype, outtemplate->fields[i].elem->ft->length, t->ipfixt->fields[i].flength, t->ipfixt->fields[i].elem->ft->length );
    }
    hi_insert_str(templates, t->ident, outtemplate);
 }

  if (verbose_level>1) {
    int i; 
    char tmps[256];
    fprintf( stdout, "data record, tid = %i.\n", outtemplate->tid);
    for (i=0; i < outtemplate->nfields; i++) {
       t->ipfixt->fields[i].elem->snprint(tmps, 255, d->addrs[i], d->lens[i]);
       fprintf( stdout, "%i: %s\n", i, tmps);
    }
  }

  ipfix_export_array(ipfixh, outtemplate, outtemplate->nfields, d->addrs, d->lens);

  /* TODO FIXME do a flush only every now and then instead of each time */
  ipfix_export_flush(ipfixh);

  return 0;
}

int export_drecord_cb( ipfixs_node_t *s, ipfixt_node_t *t,
                       ipfix_datarecord_t *d, void *arg ) {

  reexport(t, d);

  return 0;
}

void export_cleanup_cb( void *arg ) {
  fprintf (stdout, "Cleanup time. Yay.\n");
  hi_iterator_t* cln; 
  uint32_t*      key;
  uint32_t*      noop;
  ipfix_template_t* template;
  
  fprintf (stdout, "Reexported template table had %i elements.\n", templates->no_objects);
  if (!hi_iterator_create(templates, &cln)) {
    while (hi_iterator_getnext(cln, (void**)&key, (void**)&template, noop)) {
      ipfix_delete_template( ipfixh, template );
    }
  }
  hi_fini(templates);

  ipfix_close( ipfixh );
  ipfix_cleanup();
}

