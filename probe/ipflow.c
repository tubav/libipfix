/*
$$LIC$$
 *
 *  ipflow.c - manage ip flow data, generate flow ids
 *
 *  Copyright Fraunhofer FOKUS
 *
 *  $Date: 2007/02/28 10:39:17 $
 *
 *  $Revision: 1.3 $
 *
 *  remarks: 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif

#include "mlog.h"
#include "ipflow.h"

/*----- defines ----------------------------------------------------------*/

#define NODEBUG

#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif

/*----- revision id ------------------------------------------------------*/

static char const cvsid[]="$Id: ipflow.c,v 1.3 2007/02/28 10:39:17 luz Exp $";

/*----- globals ----------------------------------------------------------*/

static ipflowinfo_t  tmpflows[100];
static ipflowinfo_t  *flowinfo_freelist=NULL;
static size_t        ntmpflows = sizeof(tmpflows) / sizeof(ipflowinfo_t);
static size_t        usedflows = 0;

/*----- static funcs -----------------------------------------------------*/

static int _getflownode( ipflow_t *flows, ipflowinfo_t **finfo )
{
    /* limit number of flows
     */
    if ( flows->nflows >= flows->maxflows ) {
        *finfo = NULL;
        return 0;
    }

    /* are there elems in the freelist
     */
    if ( flowinfo_freelist ) {
        *finfo = flowinfo_freelist;
        flowinfo_freelist = flowinfo_freelist->next;
        usedflows ++;
        flows->nflows++;
        return 0;
    }

    /* alloc new flow node
     */
    if ( ((*finfo)=calloc( 1, sizeof(ipflowinfo_t))) ==NULL)
        return -1;

    (*finfo)->reuse = 0;
    usedflows ++;
    flows->nflows++;
    return 0;
}

static void _free_flownode( ipflowinfo_t *node )
{
    if ( node->reuse ) {
        node->next = flowinfo_freelist;
        flowinfo_freelist = node;
    }
    else {
        free(node);
    }
    usedflows--;
}

void flow_drop( ipflow_t *flows, ipflowinfo_t *flownode )
{
    ipflowinfo_t *r = NULL;
    ipflowinfo_t *last, *node;  /* look in hashtab */

#ifdef DEBUG
    if ( mlog_vlevel>3 ) {
        mlogf( 4, "[ipflow] FLOW_DROP START: %p\n", flownode );
        ipflow_listflows( stderr, flows );
    }
#endif

    /* remove flownode from list */
    if ( flows->flows == flownode ) {
        flows->flows = flownode->next;
        if ( flows->flows )
            flows->flows->prev = NULL;
    }
    else {
        if ( flownode->prev )
            flownode->prev->next = flownode->next;
        if ( flownode->next )
            flownode->next->prev = flownode->prev;
    }

    /* remove flownode from hashtab */
    node = flows->hashtab[flownode->hash];
    if ( node == flownode ) {
        flows->hashtab[flownode->hash] = flownode->nexthash;
        flows->nhashes --;
    }
    else {
        for ( last=node; node!=NULL; node=node->nexthash ) {
            if ( node == flownode )
                break;
            last = node;
        }

        if ( node ) {
            last->nexthash = node->nexthash;
            flows->nhashes --;
        }
        else {
            mlogf( 0, "[ipflow] INTERNAL ERROR: "
                   "INVALID FLOW NODE, hash=%d\n",
                   flownode->hash );
            exit(1);
        }
    }
    
    if ( flows->biflows && flownode->rflow ) {
        r = flownode->rflow;
        r->rflow = NULL;
    }

    _free_flownode( flownode );
    flows->nflows--;
        
    if ( flows->biflows && r ) {
        /* drop reverse flow */
        flow_drop( flows, r );
    }

#ifdef DEBUG
    if ( mlog_vlevel>3 ) {
        mlogf( 4, "[ipflow] FLOW_DROP END: %p\n", flownode );
        ipflow_listflows( stderr, flows );
    }
#endif
    return;
}

/* name:        flow_init()
 * parameters:
 * return:      0/-1
 */
int flow_init( ipflow_t **flows, int maxflows, int biflows, int timeout )
{
    ipflow_t *f;
    int      nflowbits = 18;
    int      nflows    = (1<<nflowbits), i;

    if ( (flowinfo_freelist==NULL) && (usedflows==0) ) {
        /* module init
         */
        ipflowinfo_t *node = tmpflows;

        for( i=0; i<(int)ntmpflows; i++ ) {
            node->next = flowinfo_freelist;
            node->reuse= 1;
            flowinfo_freelist = node; 
            node++;
        }
    }

    if ( (f=calloc( 1, sizeof(ipflow_t) )) ==NULL )
        return -1;

    f->nextflowid = 0;
    f->frags    = NULL;
    f->maxflows = maxflows?maxflows:5000;
    f->timeout  = timeout;
    f->biflows  = biflows?1:0;
    *flows = f;

    /* hash table
     */
    f->hashsize = nflows;
    if ( (f->hashtab=calloc( f->hashsize, sizeof(ipflowinfo_t*))) ==NULL)
        goto err;

    return 0;

 err:
    free( f );
    return -1;
}

/*
 * name:        flow_close()
 * parameters:
 * return:      0/-1
 */
void flow_close( ipflow_t *f )
{
    ipflowinfo_t *l, *n = f->flows;

    if ( !f )
        return;

    while( n ) {
        l = n;
        n = n->next;
        _free_flownode( l );
    }
    free( f->hashtab );
    free( f );
}

/* name       : flow_calchash()
 * description: 
 * remarks    : this is a first hack, todo: rewrite and speed-up
 */
int flow_calchash( ipflow_t *flows, ipflow_elems_t *elems, uint32_t *hash )
{
    uint32_t id=0, i;

    if ( elems->version == 6 ) {
        for ( id=0, i=0; i<16; i+=2 ) {
            id += ((elems->saddr[i] *256) + elems->saddr[i+1]);
            id += ((elems->daddr[i+1] *256) + elems->daddr[i]);
        }
    }
    else {
        for ( i=0; i<4; i+=2 ) {
            id += ((elems->saddr[i] *256) + elems->saddr[i+1]);
            id += ((elems->daddr[i+1] *256) + elems->daddr[i]);
        }
    }

    id += (elems->sport + elems->dport + elems->protocol + elems->tos);

    *hash = id % flows->hashsize;
    return 0;
}

/*
 * name:        flow_update()
 * parameters:  <> flows      - list of current flows
 *               > elems      - ip header elems of current packet         
 *               > tstamp     - timestamp of current packet
 *               > nbytes     - packet length
 *              <  flow       - flow of that packet 
 * return:      0/-1
 * remarks:     This func updates the list of current flows.
 */
int flow_update( ipflow_t *flows, ipflow_elems_t *elems, 
                 struct timeval *tv, size_t nbytes,
                 ipflowinfo_t **flowinfo )
{
    uint32_t      hash=0, newflow_f=0;
    ipflowinfo_t  *finfo=NULL;

    if ( elems->frag_off ) {
        /* search fragment node
         */
        ipfrag_info_t *n = flows->frags;

        while ( n ) {
            if ( (n->ipid==elems->ipid)
                 && (memcmp(n->thisflow->saddr, elems->saddr, 4) ==0 ) ) {
                /* found flow
                 */
                n->bytes_got += elems->pl_len;

                if ( !(elems->frag_off & 0x2000) ) {
                    n->bytes_exp = ((elems->frag_off&0xDFFF)*8) + elems->pl_len;
                    n->more_f = 0;
                }

                if ( (! n->more_f) && (n->bytes_exp==n->bytes_got) ) {
                    /* received all pdu fragments
                     * -> remove ipfrag node
                     * -> todo: trigger some actions
                     */ 
                    if ( n->prev ) {
                        n->prev->next = n->next;
                        if ( n->next ) {
                            n->next->prev = n->prev;
                        }
                    }
                    else {
                        flows->frags  = n->next;
                        if ( n->next ) {
                            n->next->prev = NULL;
                        }
                    }

                    n->ipid = 0;  /* mark node unused */
                    n->next = NULL;
                    n->prev = NULL;
                }

                /* we are done here and can update the flow record now
                 */
                finfo = n->thisflow;
                goto updateflow;
            }
            n = n->next;
        }

        /* no flow found
         */
    }

    /* get flowid to find flow
     */
    if ( finfo==NULL ) {
        ipflowinfo_t  *node;

        (void) flow_calchash( flows, elems, &hash );

        for( node = flows->hashtab[hash]; 
             node!=NULL; node=node->nexthash ) {
            if ( (node->elems.sport==elems->sport)
                 && (node->elems.dport==elems->dport)
                 && (node->elems.protocol==elems->protocol)
                 && (node->elems.tos==elems->tos)
                 && (memcmp( node->elems.saddr, elems->saddr,
                             (elems->version==6)?16:4 ) ==0 )
                 && (memcmp( node->elems.daddr, elems->daddr,
                             (elems->version==6)?16:4 ) ==0 ) ) {
                finfo = node;
                break;
            }
        }

        if ( finfo == NULL ) {
            if ( _getflownode( flows, &finfo ) <0 )
                return -1;
            if ( finfo == NULL )
                goto outofflows;
            finfo->hash = hash;
            finfo->nexthash = flows->hashtab[hash]; 
            flows->hashtab[hash] = finfo;
            flows->nhashes++;
            newflow_f++;
        }
    }

    if ( newflow_f ) {
        finfo->tstart   = *tv;
        finfo->packets  = 0;
        finfo->bytes    = 0;
        finfo->exported = 0;
        finfo->ipfrag.ipid = 0;    /* experimental */

        flows->nextflowid++;
        if ( flows->nextflowid<1 ) {
            /* wrap around, todo: handle this, care for duplicated flowids */
            flows->nextflowid=1;
        }
        finfo->flowid = flows->nextflowid;

        memcpy( finfo->saddr, elems->saddr, (elems->version==6)?16:4 );
        memcpy( finfo->daddr, elems->daddr, (elems->version==6)?16:4 );
        finfo->elems.saddr    = finfo->saddr;
        finfo->elems.daddr    = finfo->daddr;
        finfo->elems.version  = elems->version;
        finfo->elems.tos      = elems->tos;
        finfo->elems.protocol = elems->protocol;
        finfo->elems.sport    = elems->sport;
        finfo->elems.dport    = elems->dport;

        /* insert flow node */
        finfo->next       = flows->flows;
        if ( finfo->next )
            finfo->next->prev = finfo;
        finfo->prev       = NULL;
        flows->flows      = finfo;

        if ( flows->biflows ) {
            ipflow_elems_t relems;
            ipflowinfo_t   *node;

            /* look for the reverse flow */
            memcpy( &relems, elems, sizeof(relems) );
            relems.sport = relems.dport;
            relems.dport = elems->sport;
            relems.saddr = relems.daddr;
            relems.daddr = elems->saddr;

            finfo->rflow = NULL;
            finfo->iflow = 1;
            (void) flow_calchash( flows, &relems, &hash );

            for( node = flows->hashtab[hash]; 
                 node!=NULL; node=node->nexthash ) {
                if ( (node->elems.sport==relems.sport)
                     && (node->elems.dport==relems.dport)
                     && (node->elems.protocol==relems.protocol)
                     && (node->elems.tos==relems.tos)
                     && (memcmp( node->elems.saddr, relems.saddr,
                                 (relems.version==6)?16:4 ) ==0 )
                     && (memcmp( node->elems.daddr, relems.daddr,
                                 (relems.version==6)?16:4 ) ==0 ) ) {
                    node->rflow  = finfo;
                    finfo->rflow = node;
                    finfo->iflow = 0;
                }
            }
        }

        if ( mlog_get_vlevel() > 2 )
            mlogf( 4, "[ipflow] new flow%p, nflows=%d, hash=%u\n",
                   finfo, flows->nflows, hash );
    }

    if ( (elems->frag_off & 0x3FFF) == 0x2000 ) {
        /* first packet of a fragmented pdu
         * -> insert node into fragment list
         * (info: fragments that arrive prematurely 
         *        are not assigned to the correct flow)
         */
        finfo->ipfrag.bytes_got = elems->pl_len;
        finfo->ipfrag.bytes_exp = 0;
        finfo->ipfrag.more_f    = 1;
        finfo->ipfrag.thisflow  = finfo;

        if ( finfo->ipfrag.ipid ) {
            mlogf( 4, "[ipflow] info: defrag pdu %d aborted, pdu %d started.\n",
                   finfo->ipfrag.ipid, elems->ipid ); 
        }
        else {
            if ( flows->frags ) {
                if ( flows->frags == (&(finfo->ipfrag)) ) {
                    mlogf( 2, "[ipflow] WARNING: frag node already exists\n" );
                }
                else {
                    finfo->ipfrag.prev = NULL;
                    finfo->ipfrag.next = flows->frags;
                    flows->frags->prev = &(finfo->ipfrag);
                    flows->frags = &(finfo->ipfrag);
                }
            }
            else {
                finfo->ipfrag.prev = NULL;
                finfo->ipfrag.next = NULL;
                flows->frags = &(finfo->ipfrag);
            }
        }

        finfo->ipfrag.ipid = elems->ipid;
    }

 updateflow:
    /** todo: change this!
     */
    finfo->tlast = *tv;
    finfo->packets ++;
    finfo->bytes += nbytes;

    if ( flowinfo ) {
        *flowinfo = finfo;
    }

    return 0;

 outofflows:
    mlogf( 4, "[ipflow] out of flows: nflows=%d, nhashes=%d\n",
           flows->nflows, flows->nhashes );
    *flowinfo = NULL;
    return 0;
}

int ipflow_listflows( FILE *fp, ipflow_t *flows )
{
    ipflowinfo_t *node = flows->flows;
    unsigned i=0;
    char tstr[31], tstr2[31];

    while( node ) {
        strftime( tstr2, 30, "%T",
                  localtime( (const time_t*)&node->tlast.tv_sec ));
        strftime( tstr, 30, "%T",
                  localtime( (const time_t*)&node->tstart.tv_sec ));

        fprintf( fp, "flow%d/%p: %d %s/%s port=%d/%d %u\n", 
                 i, node, node->packets, tstr, tstr2, 
                 node->elems.sport, node->elems.dport,
                 *(uint32_t*)(void*)node->saddr );

        node = node->next;
        i++;
    }
    return 0;
}

/*------ static func -----------------------------------------------------*
 *
 * Name       : ipflow_get_elems
 * Parameters :
 * Description: parse packets
 * Remarks    : 
 */
int ipflow_get_elems( ipflow_elems_t  *elems,
                      uint8_t         *packet,
                      size_t          caplen )
{
    uint8_t *pp = packet;

    elems->version = (pp[0]&0xf0) >> 4;
    if ( (elems->version == IPVERSION) && (caplen >= 20) )
    {
        int hdrlen      = ((pp[0]&0x0f)*4);
        elems->tos      = pp[1];
        elems->protocol = pp[9];
        elems->saddr    = pp+12;
        elems->daddr    = pp+16;
        elems->frag_off = ((pp[6]&0xBF)<<8)|pp[7];
        if ( elems->frag_off ) {
            elems->ipid   = (pp[4]<<8)|pp[5];
            elems->pl_len = ((pp[2]<<8)|pp[3]) - hdrlen;
        }
        if ( (elems->protocol==IPPROTO_TCP) 
             || (elems->protocol==IPPROTO_SCTP)
             || (elems->protocol==IPPROTO_UDP) ) 
        {
            uint8_t *hp = pp + hdrlen;

            if ( elems->frag_off & 0x1FFF ) {
                /* fragment packet, no port info */
                elems->sport = elems->dport = 0;
            } else {
                elems->sport = (hp[0]*256) + hp[1];
                elems->dport = (hp[2]*256) + hp[3];
            }
        }
        else
        {
            elems->sport = elems->dport = 0;
        }

        return 0;
    }
    else if ( caplen >= 40 )
    {
        uint16_t  hdrlen, pktlen, prot;
        uint8_t   *pload, *p;

#ifdef INET6
        hdrlen = sizeof(struct ip6_hdr);
#else
        hdrlen = 40;
#endif
        pktlen = caplen;
        prot   = pp[6];

        elems->saddr = pp + 8;
        elems->daddr = pp + 24;
        elems->tos   = ((pp[0]&0x0F)<<4) + (pp[1]&0xF0);
        elems->frag_off = 0;

        for (; hdrlen<pktlen;)
        {
            if ( (prot==IPPROTO_TCP) || (prot==IPPROTO_SCTP)
                 || (prot==IPPROTO_UDP) )
                break;

            if ( (prot==IPPROTO_HOPOPTS)
                 || (prot==IPPROTO_DSTOPTS)
                 || (prot==IPPROTO_FRAGMENT) 
                 || (prot==IPPROTO_ROUTING) )
            {
                p = pp + hdrlen;
                prot   = p[0];
                hdrlen += (8 + (p[1] * 8)); 
                continue;
            }

            break;
        }
                              
        if ( (prot==IPPROTO_TCP) || (prot==IPPROTO_UDP)
             || (prot==IPPROTO_SCTP) ) {
            pload = pp + hdrlen;
            elems->sport = (pload[0]*256) + pload[1];
            elems->dport = (pload[2]*256) + pload[3];
        }
        else {
            elems->sport = elems->dport = 0;
        }
        elems->protocol = prot;
        return 0;
    }

    return -1;
}
