/*
$$LIC$$
 *
 * probe.c - ipfix probe
 *
 * Copyright Fraunhofer FOKUS
 *
 * $Date: 2007/02/28 10:19:45 $
 *
 * $Revision: 1.6 $
 *
 * Remarks:
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#include <netdb.h>
#include <getopt.h>
#include <pcap.h>

#include <../config.h>
#include <ipfix.h>
#include <ipfix_def_fokus.h>
#include <ipfix_fields_fokus.h>
#include "ipflow.h"

#include "mlog.h"
#include "mpoll.h"

/*------ defines ---------------------------------------------------------*/

#define ETHER_HDRLEN      14
#define ETHERTYPE_IP      0x0800
#define ETHERTYPE_8021Q   0x8100
#define ETHERTYPE_IPV6    0x86dd

#ifndef DLT_RAW
#define DLT_RAW DLT_NULL;
#endif

#define PROBE_IDSTR       "ipfix_probe"
#define PROBE_VERSIONSTR  "v0.3"

#define CAFILE            "rootcert.pem"
#define CADIR             NULL
#define KEYFILE           "client.pem"
#define CERTFILE          "client.pem"

/*------ stuctures -------------------------------------------------------*/

typedef enum probe_flags
{
    PROBE_PROMISC=1, PROBE_OFFLINE=2, PROBE_DAEMON=4
} probe_flags_t;

struct probe_Options {
    char           progname[30];
    char           interface[PATH_MAX+1];
    char           *filter;
    char           *collector;
    int            port;
    int            protocol;          /* ipfix bearer */
    int            ssl;               /* ipfix over tls/ssl */
    char           *cafile;
    char           *cadir;
    char           *keyfile;          /* private key */
    char           *certfile;         /* certificate */
    char           *logfile;
    probe_flags_t  flags;
    int            vlevel;            /* verbosity */
    uint32_t       odid;              /* ipfix observation domain id */
    int            ipflow_timeout;    /* maximum flow idletime (inactive timer */
    int            ipflow_max;        /* limit no. of active flows */
    int            ipflow_lifetime;   /* maximum flow lifetime (active timer) */
    int            biflows;           /* detect and export biflows */
    int            export_timeout;    /* report interval */
};

typedef struct probe_struct {
    ipfix_t             *ipfix;
    ipfix_template_t    *templ;
    ipfix_template_t    *templ6;

    ipflow_t            *ipflows;
    time_t              next_export;
    int64_t             npkts;
    int64_t             nbytes;

    char                *device;
    pcap_t              *pcap;
    struct bpf_program  fprg;
    int                 fd;
    int                 cnt;
    int                 dltype;
    int                 offset;
} probe_t;

/*------ globals ---------------------------------------------------------*/

static struct probe_Options    g_par;
static struct probe_struct     g_probe;

static mptimer_t  export_tid;             /* timer */

bpf_u_int32     netmask =0;         /* The netmask of our sniffing device */
bpf_u_int32     localnet =0;             /* The IP of our sniffing device */
bpf_u_int32     g_snaplen =100; 
static char     errbuf[PCAP_ERRBUF_SIZE];

static char const cvsid[]="$Id: probe.c,v 1.6 2007/02/28 10:19:45 luz Exp $";

export_fields_t ipflow4_fields[] = { 
    { 0, IPFIX_FT_FLOWSTARTMILLISECONDS, 8 },
    { 0, IPFIX_FT_FLOWENDMILLISECONDS, 8 },
    { 0, IPFIX_FT_SOURCEIPV4ADDRESS, 4 },
    { 0, IPFIX_FT_DESTINATIONIPV4ADDRESS, 4 },
    { 0, IPFIX_FT_SOURCETRANSPORTPORT, 2 },
    { 0, IPFIX_FT_DESTINATIONTRANSPORTPORT, 2 },
    { 0, IPFIX_FT_PROTOCOLIDENTIFIER, 1 },
    { 0, IPFIX_FT_IPCLASSOFSERVICE, 1 },
    { 0, IPFIX_FT_PACKETDELTACOUNT, 4 },
    { 0, IPFIX_FT_OCTETDELTACOUNT, 4 } };

export_fields_t ipflow6_fields[] = { 
    { 0, IPFIX_FT_FLOWSTARTMILLISECONDS, 8 },
    { 0, IPFIX_FT_FLOWENDMILLISECONDS, 8 },
    { 0, IPFIX_FT_SOURCEIPV6ADDRESS, 16 },
    { 0, IPFIX_FT_DESTINATIONIPV6ADDRESS, 16 },
    { 0, IPFIX_FT_SOURCETRANSPORTPORT, 2 },
    { 0, IPFIX_FT_DESTINATIONTRANSPORTPORT, 2 },
    { 0, IPFIX_FT_PROTOCOLIDENTIFIER, 1 },
    { 0, IPFIX_FT_IPCLASSOFSERVICE, 1 },
    { 0, IPFIX_FT_PACKETDELTACOUNT, 4 },
    { 0, IPFIX_FT_OCTETDELTACOUNT, 4 } };

export_fields_t ipbiflow4_fields[] = { 
    { 0, IPFIX_FT_FLOWSTARTMILLISECONDS, 8 },
    { 0, IPFIX_FT_FLOWENDMILLISECONDS, 8 },
    { 0, IPFIX_FT_SOURCEIPV4ADDRESS, 4 },
    { 0, IPFIX_FT_DESTINATIONIPV4ADDRESS, 4 },
    { 0, IPFIX_FT_SOURCETRANSPORTPORT, 2 },
    { 0, IPFIX_FT_DESTINATIONTRANSPORTPORT, 2 },
    { 0, IPFIX_FT_PROTOCOLIDENTIFIER, 1 },
    { 0, IPFIX_FT_PACKETDELTACOUNT, 4 },
    { 0, IPFIX_FT_OCTETDELTACOUNT, 4 },
    { IPFIX_ENO_FOKUS, IPFIX_FT_REVPACKETDELTACOUNT, 4 },
    { IPFIX_ENO_FOKUS, IPFIX_FT_REVOCTETDELTACOUNT, 4 } };

export_fields_t ipbiflow6_fields[] = { 
    { 0, IPFIX_FT_FLOWSTARTMILLISECONDS, 8 },
    { 0, IPFIX_FT_FLOWENDMILLISECONDS, 8 },
    { 0, IPFIX_FT_SOURCEIPV6ADDRESS, 16 },
    { 0, IPFIX_FT_DESTINATIONIPV6ADDRESS, 16 },
    { 0, IPFIX_FT_SOURCETRANSPORTPORT, 2 },
    { 0, IPFIX_FT_DESTINATIONTRANSPORTPORT, 2 },
    { 0, IPFIX_FT_PROTOCOLIDENTIFIER, 1 },
    { 0, IPFIX_FT_PACKETDELTACOUNT, 4 },
    { 0, IPFIX_FT_OCTETDELTACOUNT, 4 },
    { IPFIX_ENO_FOKUS, IPFIX_FT_REVPACKETDELTACOUNT, 4 },
    { IPFIX_ENO_FOKUS, IPFIX_FT_REVOCTETDELTACOUNT, 4 } };

int ipflow4_nfields = sizeof(ipflow4_fields)/sizeof(export_fields_t);
int ipflow6_nfields = sizeof(ipflow6_fields)/sizeof(export_fields_t);
int ipbiflow4_nfields = sizeof(ipbiflow4_fields)/sizeof(export_fields_t);
int ipbiflow6_nfields = sizeof(ipbiflow6_fields)/sizeof(export_fields_t);

/*------ prototypes ------------------------------------------------------*/

void usage          ( char *taskname);
void exit_func      ( int signo );

/*------ tool usage function ---------------------------------------------*/

void usage (char *taskname)
{
    const char helptext[]=
        "[-dhPstuv] [-l logfile]\n"
        "                   [-f filter] [-i interface|-r pcapfile]\n"
        "                   [-s|-t|-u] [-c collector[:port]]\n"
        "options:\n"
        "  -c <collector>    collector (default localhost:4739)\n"
        "  -d                run as daemon\n"
        "  -f <bpf filter>   filter captured packets\n"
        "  -h                this help\n"
        "  -i <interface>    capture interface (e.g. eth0)\n"
        "  -l <logfile>      use logfile\n"
        "  -p                don't put the interface into promiscuous mode\n"
        "  -r <dumpfile>     read packets from dump file\n"
#ifdef SCTPSUPPORT
        "  -s                sctp export\n"
#endif
        "  -t                tcp export (default)\n"
        "  -u                udp export\n"
        "  -v                increase verbose level\n"
        "  --odid <observation domain>  observation domain id (default 1)\n"
#ifdef SSLSUPPORT
        "  --ssl                        use tls/ssl\n"
        "  --key    <file>              private key file to use\n"
        "  --cert   <file>              certificate file to use\n"
        "  --cafile <file>              file of CAs\n"
        "  --cadir  <dir>               directory of CAs\n"
#endif
        "  --biflows                    export biflows (default no)\n"
        "  --tidle <idle timeout>       idle flow lifetime (default 30s)\n"
        "  --tflow <flow lifetime>      maximum flow lifetime (default 300s)\n"
        "  --texport <export interval>  export interval (default 10s)\n"
        "\n";

    fprintf ( stderr, "\nIPFIX probe %s (%s)\n\nusage: %s %s",
              PROBE_VERSIONSTR, __DATE__, taskname, helptext );

    exit(1);
}


/* name       : export_biflows
 * remarks    : 
 */
static int export_biflows( probe_t *probe, time_t now, int flag )
{
    ipflow_t     *flows = probe->ipflows;
    ipflowinfo_t *n, *node, *r;
    int          a=0, d=0, e=0;
    long         idlesec, duration;
    uint64_t     msec_start, msec_end, rend;
    uint64_t     dummy = 0;

    node = flows?flows->flows:NULL;
    while( node ) {
        if ( ! node->iflow ) {
            node = node->next;
            continue;
        }

        a++;
        msec_start = ((uint64_t)node->tstart.tv_sec*1000)
            +((uint64_t)node->tstart.tv_usec/1000);
        msec_end   = ((uint64_t)node->tlast.tv_sec*1000)
            +((uint64_t)node->tlast.tv_usec/1000);
        if ( node->rflow ) {
            rend   = ((uint64_t)node->rflow->tlast.tv_sec*1000)
                +((uint64_t)node->rflow->tlast.tv_usec/1000);
            if ( rend > msec_end )
                msec_end = rend;
            r = node->rflow;
        }
        else {
            r = NULL;
        }
        idlesec  = now - (msec_end/1000);
        duration = (msec_end - msec_start)/1000;

        /* check if the flow is idle
         */
        if ( flag
             || (g_par.ipflow_timeout && (idlesec > g_par.ipflow_timeout))
             || (g_par.ipflow_lifetime
                 && (duration > g_par.ipflow_lifetime)) ) {
            if ( mlog_get_vlevel() > 2 ) {
                if ( g_par.ipflow_timeout 
                     && (idlesec > g_par.ipflow_timeout) ) {
                    mlogf( 3, "[%s] %ds idle timeout expired! "
                           "id=%d, start=%ld, duration=%ld, idle=%ld\n",
                           g_par.progname, g_par.ipflow_timeout,
                           node->flowid, node->tstart.tv_sec,
                           duration, idlesec );
                }
                else if ( g_par.ipflow_lifetime
                          && (duration > g_par.ipflow_lifetime) ) {
                    mlogf( 3, "[%s] %ds flow lifetime expired! "
                           "id=%d, start=%ld, duration=%ld, idle=%ld\n",
                           g_par.progname, g_par.ipflow_lifetime,
                           node->flowid, node->tstart.tv_sec,
                           duration, idlesec );
                }
            }
            /* flow has expired
             * -> export flow end
             * -> delete node
             */
            (void) ipfix_export( probe->ipfix, 
                                 (node->elems.version==6)?
                                 probe->templ6:probe->templ,
                                 &msec_start, &msec_end,
                                 node->saddr, node->daddr, 
                                 &node->elems.sport, 
                                 &node->elems.dport,
                                 &node->elems.protocol,
                                 &node->packets, &node->bytes,
                                 (r!=NULL)?&r->packets:(void*)&dummy,
                                 (r!=NULL)?&r->bytes:(void*)&dummy );

            n = node->next;
            (void) flow_drop( flows, node );
            node = n;
            e++;
            d++;
            continue;
        }

        if ( (g_par.ipflow_timeout<1) && (node->packets) ) {
            /* flow is still active
             * -> export counters
             */
            msec_end = 0;
            (void) ipfix_export( probe->ipfix, 
                                 (node->elems.version==6)?
                                 probe->templ6:probe->templ,
                                 &msec_start, &msec_end,
                                 node->saddr, node->daddr, 
                                 &node->elems.sport, 
                                 &node->elems.dport,
                                 &node->elems.protocol,
                                 &node->elems.tos,
                                 &node->packets, &node->bytes );

            node->packets =0;
            node->bytes   =0;
            e++;
        }
        
        node = node->next;
    }

    if ( ipfix_export_flush( probe->ipfix ) <0 ) {
        mlogf( 3, "[%s] ipfix_export() failed: %s\n", 
               __func__, strerror(errno) );
    } else {
        mlogf( 2, "[%s] %d active flows, %d exported, %d dropped.\n",
               g_par.progname, a, e, d );
    }

    probe->npkts  = 0;
    probe->nbytes = 0;
    return 0;
}

/* name       : export_ipflows
 * remarks    : 
 */
static int export_ipflows( probe_t *probe, time_t now, int flag )
{
    ipflow_t     *flows = probe->ipflows;
    ipflowinfo_t *n, *node;
    int          a=0, d=0, e=0;
    uint64_t     msec_start;
    uint64_t     msec_end;

    if ( flows->biflows )
        return export_biflows( probe, now, flag );

    node = flows?flows->flows:NULL;
    while( node ) {
        msec_start = ((uint64_t)node->tstart.tv_sec*1000)
            +((uint64_t)node->tstart.tv_usec/1000);
        msec_end   = ((uint64_t)node->tlast.tv_sec*1000)
            +((uint64_t)node->tlast.tv_usec/1000);
        a++;

        /* check if the flow is idle
         */
        if ( flag
             || (g_par.ipflow_timeout 
                 && ((now - node->tlast.tv_sec) > g_par.ipflow_timeout))
             || (g_par.ipflow_lifetime
                 && ((node->tlast.tv_sec-node->tstart.tv_sec)
                     > g_par.ipflow_lifetime)) ) {
            if ( mlog_get_vlevel() > 2 ) {
                if ( g_par.ipflow_timeout 
                     && ((now - node->tlast.tv_sec) > g_par.ipflow_timeout) ) {
                    mlogf( 3, "[%s] %ds idle timeout expired! "
                           "id=%d, start=%ld, duration=%ld, idle=%ld\n",
                           g_par.progname, g_par.ipflow_timeout,
                           node->flowid, node->tstart.tv_sec,
                           node->tlast.tv_sec-node->tstart.tv_sec,
                           now - node->tlast.tv_sec );
                }
                else if ( g_par.ipflow_lifetime
                          && ((node->tlast.tv_sec-node->tstart.tv_sec)
                              > g_par.ipflow_lifetime) ) {
                    mlogf( 3, "[%s] %ds flow lifetime expired! "
                           "id=%d, start=%ld, duration=%ld, idle=%ld\n",
                           g_par.progname, g_par.ipflow_lifetime,
                           node->flowid, node->tstart.tv_sec,
                           node->tlast.tv_sec-node->tstart.tv_sec,
                           now - node->tlast.tv_sec );
                }
            }
            /* flow has expired
             * -> export flow end
             * -> delete node
             */
            (void) ipfix_export( probe->ipfix, 
                                 (node->elems.version==6)?
                                 probe->templ6:probe->templ,
                                 &msec_start, &msec_end,
                                 node->saddr, node->daddr, 
                                 &node->elems.sport, 
                                 &node->elems.dport,
                                 &node->elems.protocol,
                                 &node->elems.tos,
                                 &node->packets, &node->bytes );

            n = node->next;
            (void) flow_drop( flows, node );
            node = n;
            e++;
            d++;
            continue;
        }

        if ( (g_par.ipflow_timeout<1) && (node->packets) ) {
            /* flow is still active
             * -> export counters
             */
            msec_end = 0;
            (void) ipfix_export( probe->ipfix, 
                                 (node->elems.version==6)?
                                 probe->templ6:probe->templ,
                                 &msec_start, &msec_end,
                                 node->saddr, node->daddr, 
                                 &node->elems.sport, 
                                 &node->elems.dport,
                                 &node->elems.protocol,
                                 &node->elems.tos,
                                 &node->packets, &node->bytes );

            node->packets =0;
            node->bytes   =0;
            e++;
        }
        
        node = node->next;
    }

    if ( ipfix_export_flush( probe->ipfix ) <0 ) {
        mlogf( 3, "[%s] ipfix_export() failed: %s\n", 
               __func__, strerror(errno) );
    } else {
        mlogf( 2, "[%s] %d active flows, %d exported, %d dropped.\n",
               g_par.progname, a, e, d );
    }

    probe->npkts  = 0;
    probe->nbytes = 0;
    return 0;
}


/* name       : cb_export
 * remarks    : 
 */
void cb_export( void *arg )
{
    probe_t         *probe = (probe_t*)arg;

    (void) export_ipflows( probe, time(NULL), 0 );

    export_tid = mpoll_timeradd( g_par.export_timeout,
                                 cb_export, probe );
}

/* name       : cb_packet
 * remarks    : 
 */
void cb_packet( u_char *args, 
                const struct pcap_pkthdr *header, 
                const u_char *packet )
{
    int            len, tos, ethertype, offset;
    uint32_t       flowid, caplen;
    uint8_t        *pp;
    ipflow_elems_t elems;
    probe_t        *data = (probe_t*)args;
    ipflowinfo_t   *finfo=NULL;

    if (mlog_get_vlevel() > 4) {
        mlogf( 4, "[%s] cb_packet() called. caplen=%d/%d offset=%d\n",
               data->device, header->caplen, header->len, data->offset );
    }

    len = header->len;
    if ( header->caplen > g_snaplen )
        caplen = g_snaplen;
    else
        caplen = header->caplen;

    /* skip to start of ip packet
     */
    switch( data->dltype ) {
      case DLT_EN10MB:
          if ( caplen < ETHER_HDRLEN )
              return;
          memcpy( &ethertype, packet+12, 2 );
          ethertype = ntohs(ethertype);
          offset = ETHER_HDRLEN;
          caplen -= ETHER_HDRLEN;
          pp = (uint8_t*)packet + offset;
          for ( ;caplen>0; ) {
              switch( ethertype ) {
                case ETHERTYPE_8021Q:
                    pp     += 4;
                    caplen -= 4;
                    offset += 4;
                    memcpy( &ethertype, pp-2, 2 );
                    ethertype = ntohs(ethertype);
                    continue;
                case ETHERTYPE_IPV6:
                case ETHERTYPE_IP:
                    break;
                default:
                    return;
              }
              break;
          }
          break;

      default:
          pp      = (uint8_t*)packet + data->offset;
          offset  = data->offset;
          caplen -= offset;
          break;

    }/*switch*/

    /* check ip version
     */
    switch ( (pp[0]&0xf0) >> 4 ) { /* ip version */
      case IPVERSION:
          tos = pp[1];
          break;
#ifdef INET6
      case 6:
          tos = ((pp[0]&0x0F)<<4) + (pp[1]&0xF0);
          break;
#endif
      default:
          return;
    }

    ipflow_get_elems( &elems, pp, caplen );
    if ( flow_update( data->ipflows, &elems,
                      (struct timeval*)&(header->ts), len,
                      &finfo ) <0 ) {
        /** todo: do some reasonable things ...
         */
        mlogf( 0, "[cb_packet] flow_update failed!\n" );
    }
    flowid = finfo?finfo->flowid:0;

    if ( finfo && (mlog_get_vlevel() > 4) ) {
        mlogf( 4, "[%s] flowid=%d (start=%ld, duration=%ld)\n",
               data->device, flowid, finfo->tstart.tv_sec,
               finfo->tlast.tv_sec-finfo->tstart.tv_sec );
    }

    /* execute action
     */
    data->npkts++;
    data->nbytes += (header->len - offset);

    if ( (g_par.flags & PROBE_OFFLINE)
         && (g_par.export_timeout>0) ) {
        if ( data->next_export == 0 ) {
            data->next_export = header->ts.tv_sec + g_par.export_timeout;
        }
        else {
            if ( header->ts.tv_sec >= data->next_export ) {
                (void) export_ipflows( data, header->ts.tv_sec, 0 );
                if ( g_par.export_timeout ) {
                    while ( data->next_export <= header->ts.tv_sec )
                        data->next_export += g_par.export_timeout;
                }
                sleep( 1 ); /* todo: remove this line! */
            }
        }
    } 
}

/* name       : cb_dispatch
 * remarks    : 
 */
void cb_dispatch( int fd, int mask, void *data )
{
    probe_t *p = (probe_t*)data;

    switch ( pcap_dispatch( p->pcap, p->cnt, cb_packet, data ) ) {
      case -1:
          mlogf( 0, "[%s] pcap_dispatch: %s\n", 
                 p->device, pcap_geterr(p->pcap));
          break;
      case 0:
          if ( g_par.flags & PROBE_OFFLINE ) {
              mlogf( 1, "[%s] end of file detected, break ...\n", 
                     p->device );
              mpoll_break();
          }
          break;;
      default:
          return;
    }

    return;
}

/* name       : input_init
 */
static int input_init( probe_t *probe, int flags )
{
    char *device = g_par.interface;
    int  dltype;

    if ( flags & PROBE_OFFLINE ) {
        /* open file */
        if ( (probe->pcap=pcap_open_offline( device, errbuf )) ==NULL) {
            errorf( "[%s] pcap_open_offline(): %s\n", device, errbuf );
            return -1;
        }
        probe->cnt = 1;
    }
    else {
        /* open interface */
        int         promisc = (flags&PROBE_PROMISC)?1:0;
        char        *p = probe->device;

        if ( *device == '\0' ) {
            if ((device=pcap_lookupdev( errbuf )) ==NULL) {
                errorf( "pcap_lookupdev() failed: %s\n", errbuf );
                return -1;
            }
        }

        if ( (probe->pcap=pcap_open_live( device, g_snaplen, promisc,
                                          100 /*ms*/, errbuf )) ==NULL ) {
            /* todo!! */
            errorf( "pcap_open_live(%s): %s\n", p, errbuf );
            return -1;
        }
        probe->cnt = 1000;
    }

    switch ( dltype = pcap_datalink(probe->pcap) ) {
      case DLT_EN10MB:
          probe->dltype = dltype;
          probe->offset = 14;
          break;
      case DLT_ATM_RFC1483:
          probe->dltype = dltype;
          probe->offset = 8;
          break;
      default:
          probe->dltype = DLT_RAW;
          probe->offset = 0;
    }

    if( g_par.filter != NULL ) {
        if( (pcap_compile( probe->pcap, &probe->fprg, g_par.filter, 1, 0 ) <0)
            || (pcap_setfilter( probe->pcap, &probe->fprg ) <0) ) {
            mlogf( 0, "[%s] unable to set filter: '%s'\n",
                   g_par.progname, g_par.filter );
        } else {
            mlogf( 1, "[%s] set filter to '%s'\n",
                   g_par.progname, g_par.filter );
        }
    }

    mlogf( 1, "[%s] device: %s, dltype=%d, %s\n", g_par.progname,
           (flags&PROBE_OFFLINE)?basename(device):device, probe->dltype, 
           (flags&PROBE_OFFLINE)?"offline"
           :(flags&PROBE_PROMISC)?"promisc.":"no promisc." );

    if ( flags & PROBE_OFFLINE ) {
        probe->fd = fileno( pcap_file( probe->pcap ) );
        probe->device = strdup( basename( device ) );
    }
    else {
        probe->fd = pcap_fileno( probe->pcap );
        probe->device = strdup( device );
    }

    if ( probe->fd >= 0) {
        if ( mpoll_fdadd( probe->fd, MPOLL_IN, 
                          cb_dispatch, (void*)probe ) <0 ) {
            mlogf( 0, "[%s] %s\n", g_par.progname, strerror(errno) );
            return -1;
        }
        mlogf( 2, "[%s] add fd %d to poll loop\n", g_par.progname, probe->fd );
    }

    if ( flow_init( &(probe->ipflows),
                    g_par.ipflow_max,
                    g_par.biflows,
                    g_par.ipflow_timeout ) <0 ) {
        mlogf( 0, "[%s] ipflow initialisation failed: %s",
               g_par.progname, strerror(errno) );
        return -1;
    }

    return 0;
}

/* name       : export_init
 */
static int export_init( probe_t *probe )
{
    ipfix_t          *ifh =NULL;
    ipfix_template_t *t4, *t6;
    char             *p, *host;
    int              port;

    if ( (ipfix_init() <0)
         || (ipfix_add_vendor_information_elements( ipfix_ft_fokus ) <0 ) ) {
        mlogf( 0, "[%s] cannot init ipfix module\n", __func__ );
        return -1;
    }

    if ( ipfix_open( &ifh, g_par.odid, IPFIX_VERSION ) <0 ) {
        mlogf( 0, "[%s] ipfix_open() failed: %s\n", 
               __func__, strerror(errno) );
        ipfix_cleanup();
        return -1;
    }

    for ( host=g_par.collector, port=g_par.port;; ) {
        /* check if hostname has :portno suffix
         */
        if ( ((p=strrchr( host, ':' )) !=NULL) 
             && (p==strchr( host, ':' )) && ((port=atoi(p+1))>0) ) {
            *p = '\0';
        }

        if ( !port )
            port = IPFIX_PORTNO;

#ifdef SSLSUPPORT
        if ( g_par.ssl ) {
            ipfix_ssl_opts_t opts;

            opts.cafile  = g_par.cafile;
            opts.cadir   = g_par.cadir;
            opts.keyfile = g_par.keyfile;
            opts.certfile= g_par.certfile;

            if ( ipfix_add_collector_ssl( ifh, host, port,
                                          g_par.protocol, &opts ) <0 ) {
                mlogf( 0, "[%s] ipfix_add_collector_ssl() failed: %s\n",
                       __func__, strerror(errno) );
                goto err;
            }
            break;
        }
#endif
        if ( ipfix_add_collector( ifh, host, port, g_par.protocol ) <0 ) {
            mlogf( 0, "[%s] ipfix_add_collector() failed: %s\n",
                   __func__, strerror(errno) );
            goto err;
        }
        break;
    }

    if ( g_par.biflows ) {
        if ( ipfix_make_template( ifh, &t4,
                                  ipbiflow4_fields,
                                  ipbiflow4_nfields ) <0) {
            goto err;
        }

        if ( ipfix_make_template( ifh, &t6,
                                  ipbiflow6_fields,
                                  ipbiflow6_nfields ) <0) {
            ipfix_delete_template( ifh, t4 );
            goto err;
        }
    }
    else {
        if ( ipfix_make_template( ifh, &t4,
                                  ipflow4_fields,
                                  ipflow4_nfields ) <0) {
            goto err;
        }

        if ( ipfix_make_template( ifh, &t6,
                                  ipflow6_fields,
                                  ipflow6_nfields ) <0) {
            ipfix_delete_template( ifh, t4 );
            goto err;
        }
    }

    probe->ipfix  = ifh;
    probe->templ  = t4;
    probe->templ6 = t6;
    return 0;

err:
    ipfix_close( ifh );
    ipfix_cleanup();
    return -1;
}

/* name       : prog_init
 * parameters : none, global parameters
 * description: some program initialisation
 * returns    : 0 if ok, -1 on error
 * remarks    : 
 */
int prog_init ( void )
{
    /* daemon
     */
    if ( (g_par.flags&PROBE_DAEMON) && (daemon ( 0, 0 ) < 0) ) {
        errorf ( "[%s] failed to run as daemon!\n\n", g_par.progname );
        return -1;
    }
#if 0
    if ( chdir ( "/tmp" ) <0 )
        mlogf( 0, "[%s] ERROR cannot change working dir: %s\n", 
               g_par.progname, strerror(errno) );
#endif

    /* open logfile
     */
    mlog_set_vlevel( g_par.vlevel );
    if ( g_par.logfile )
        (void) mlog_open( g_par.logfile, NULL );

    mlogf( 1, "\nIPFIX probe (%s)\n\n", __DATE__ );

    /* signal handler */
    signal( SIGKILL, exit_func );
    signal( SIGTERM, exit_func );
    signal( SIGINT,  exit_func );

    /* init exporter */
    if ( export_init( &g_probe ) <0 ) {
        return -1;
    }

    /* init pcap sniffing
     */
    if ( input_init( &g_probe, g_par.flags ) <0 ) {
        return -1;
    }

    return 0;
}

/* name       : exit_func
 */
void exit_func ( int signo )
{
    if ( signo )
        mlogf( 0, "\n[%s] Got signo %d. Bye.\n\n", g_par.progname, signo );

    /* export flows
     */
    (void)export_ipflows( &g_probe, time(NULL), 1 );

    /** clean up
     */
    mlogf( 1, "[%s] clean up.\n", g_par.progname );
    mpoll_fdrm( g_probe.fd );
    if ( g_par.filter ) {
        free( g_par.filter );
#ifdef HAVE_PCAP_FREECODE
        pcap_freecode( &(g_probe.fprg) );
#endif
    }
    pcap_close( g_probe.pcap );

    ipfix_delete_template( g_probe.ipfix, g_probe.templ );
    ipfix_delete_template( g_probe.ipfix, g_probe.templ6 );
    ipfix_close( g_probe.ipfix );
    ipfix_cleanup();

    if ( g_probe.device )
        free( g_probe.device );

    if ( export_tid )
        mpoll_timerrm( export_tid );

    flow_close( g_probe.ipflows );
    mlog_close();
    exit(0);
}

/* name       : probe_loop
 */
int probe_loop ( probe_t *probe )
{
    if ( g_par.flags & PROBE_OFFLINE ) {
        export_tid = NULL;
        probe->next_export = 0;
    }
    else {
        probe->next_export = 0;
        export_tid = mpoll_timeradd( g_par.export_timeout,
                                     cb_export, probe );
    }

    /** event loop
     */
    return mpoll_loop( -1 );
}

/*------ MAIN ------------------------------------------------------------*/

int main (int argc, char *argv[])
{
    char arg;                   /* short options: character */
    int loptidx;                /* long options: arg==0 and index */
    extern int optind;          /* actual option index, <=argc */
 
    char opt[] = "hvdpl:i:r:c:f:stu";
    struct option lopt[] = { 
        { "version", 0, 0, 0},
        { "tidle", 1, 0, 0},
        { "tflow", 1, 0, 0},
        { "texport", 1, 0, 0},
        { "odid", 1, 0, 0},
        { "ssl", 0, 0, 0},
        { "key", 1, 0, 0},
        { "cert", 1, 0, 0},
        { "cafile", 1, 0, 0},
        { "cadir", 1, 0, 0},
        { "biflows", 0, 0, 0},
        { "maxflows", 1, 0, 0},
        { "help", 0, 0, 0},
        { 0, 0, 0, 0 } 
    };

    /* set parameters to default values
     */
    g_par.protocol        = IPFIX_PROTO_TCP;
    g_par.logfile         = NULL;
    g_par.interface[0]    = '\0';
    g_par.filter          = NULL;
    g_par.collector       = "localhost";
    g_par.port            = 0;
    g_par.flags           = PROBE_PROMISC;
    g_par.vlevel          = 0;
    g_par.odid            = 1;
    g_par.ipflow_timeout  = 30;
    g_par.ipflow_max      = 25000;
    g_par.ipflow_lifetime = 300;
    g_par.biflows         = 0;
    g_par.export_timeout  = 10;
    g_par.ssl             = 0;
    g_par.cafile          = CAFILE;
    g_par.cadir           = CADIR;
    g_par.keyfile         = KEYFILE;
    g_par.certfile        = CERTFILE;

    snprintf( g_par.progname, sizeof(g_par.progname), "%s", basename(argv[0]) );

    /* process command line options 
     */
    while ((arg=getopt_long(argc,argv,opt,lopt,&loptidx)) >=0) {
        switch (arg) {
          case 0: 
              switch (loptidx) {
                case 0:
                    /* version */
                    mlogf( 0, "%s (%s, %s)\n", 
                           PROBE_IDSTR, PROBE_VERSIONSTR, __DATE__ );
                    exit(0);
                    break;
                case 1:
                    /* idle timeout */
                    sscanf( optarg, "%u", &g_par.ipflow_timeout );
                    break;
                case 2:
                    /* flow lifetime */
                    sscanf( optarg, "%u", &g_par.ipflow_lifetime );
                    break;
                case 3:
                    /* export interval */
                    sscanf( optarg, "%u", &g_par.export_timeout );
                    break;
                case 4:
                    /* observation domain id */
                    sscanf( optarg, "%u", &g_par.odid );
                    break;
                case 5: /* ssl */
                    g_par.ssl = 1;
                    break;
                case 6: /* key */
                    g_par.keyfile = optarg;
                    break;
                case 7: /* cert */
                    g_par.certfile = optarg;
                    break;
                case 8: /* cafile */
                    g_par.cafile = optarg;
                    break;
                case 9: /* cadir */
                    g_par.cadir = optarg;
                    break;
                case 10: /* biflows */
                    g_par.biflows = 1;
                    break;
                case 11: /* max number of flows */
                    sscanf( optarg, "%d", &g_par.ipflow_max );
                    break;
                case 12:
                    usage(g_par.progname);
              }
              break;

          case 'c':
              g_par.collector = optarg;
              break;
          case 'd':
              g_par.flags |= PROBE_DAEMON;
              break;
          case 'f':
              if ((*optarg=='\'') || (*optarg=='"')) {
                  g_par.filter = strdup( optarg+1 );
                  g_par.filter[strlen(optarg)-1] = '\0';
              }
              else {
                  g_par.filter = strdup( optarg );
              }
              break;
          case 'i':
              if ( *(g_par.interface) ) {
                  fprintf( stderr, "warning: parameter '-i %s' ignored.\n",
                           optarg );
                  break;
              }
              snprintf( g_par.interface, PATH_MAX, "%s", optarg ); 
              break;
          case 'p':
              g_par.flags &= ~PROBE_PROMISC;
              break;
          case 'r':
              if ( *(g_par.interface) ) {
                  fprintf( stderr, "warning: parameter '-r %s' ignored.\n",
                           optarg );
                  break;
              }
              g_par.flags |= PROBE_OFFLINE;
              if ( strcmp( optarg, "-" ) ==0 ) {
                  strcpy( g_par.interface, optarg );
              }
              else if ( realpath( optarg, g_par.interface ) ==NULL) {
                  fprintf( stderr, "cannot access file '%s': %s\n",
                           optarg, strerror(errno) );
                  exit(1);
              }
              break;
          case 'l':
              g_par.logfile = optarg;
              break;
          case 'h':
              usage(g_par.progname);
              break;
	  case 's':
              g_par.protocol = IPFIX_PROTO_SCTP;
              break;
	  case 't':
              g_par.protocol = IPFIX_PROTO_TCP;
              break;
	  case 'u':
              g_par.protocol = IPFIX_PROTO_UDP;
              break;
          case 'v':
              g_par.vlevel ++;
              break;
          default:
              usage(argv[0]);
        }
    }/*parse opts*/

    if (optind < argc) {
        errorf ( "No additional parameters needed!\n");
        exit(1);
    }

    if ( g_par.port==0 ) {
        g_par.port = g_par.ssl?IPFIX_TLS_PORTNO:IPFIX_PORTNO;
    }

    /* init program */
    if ( prog_init() <0 ) {
        exit(1);
    }

    mlogf( 2, "[%s] program initialized.\n", g_par.progname );

    /* do the work */
    if ( probe_loop( &g_probe ) < 0 ) {
        exit_func( 0 );
    }

    /* clean up */
    exit_func ( 0 );

    return 0;

} /* main */
