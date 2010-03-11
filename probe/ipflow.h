/*

 *
 *
 * ipflow.h - export declarations of libipflow
 *
 * Copyright Fraunhofer FOKUS
 **
 * $Id: ipflow.h,v 1.3 2007/02/28 10:39:17 luz Exp $
 *
 * Remarks: This is experimental code!
 *
 */
#ifndef IMP_IPFLOW_H
#define IMP_IPFLOW_H

#define  IPFLOW_NP1ADDR1CHUNKS 8 
#define  IPFLOW_NP1ADDR2CHUNKS 8 
#define  IPFLOW_NP1PORTCHUNKS  2 
#define  IPFLOW_NP2ADDRCHUNKS  2 
#define  IPFLOW_NP2PORTCHUNKS  2 
#define  IPFLOW_NP3CHUNKS      2 

typedef struct ipflow_elems
{
    int       version;
    uint8_t   tos;
    uint8_t   protocol;
    uint8_t   *saddr;
    uint8_t   *daddr;
    uint16_t  sport;
    uint16_t  dport;

    uint16_t  frag_off;    /* experimental, ip frag offset */
    uint16_t  pl_len;      /* experimental, paylaod length */
    uint16_t  ipid;        /* experimental, ipv4 hdr id    */
    uint32_t  hash;        /* experimental, hashval        */
} ipflow_elems_t;

typedef struct ipfrag_info 
{
    struct ipfrag_info *prev;
    struct ipfrag_info *next;

    struct ipflowinfo  *thisflow;
    uint16_t           ipid;
    uint16_t           bytes_exp;
    uint16_t           bytes_got;
    uint16_t           more_f;      /* 1 => last fragment is missing */
} ipfrag_info_t;

typedef struct ipflowinfo
{
    struct ipflowinfo  *prev;
    struct ipflowinfo  *next;
    struct ipflowinfo  *nexthash; /*!< internal pointer */
    int                reuse;     /*!< internal flag */

    struct timeval     tstart;    /*!< flow start time       */
    struct timeval     tlast;     /*!< tstamp of last packet */
    int                bytes;     /*!< no. of bytes          */
    int                packets;   /*!< no. of packets        */
    int                flowid;    /*!< generated flow id     */
    ipfrag_info_t      ipfrag;    /*!< experimental          */
    uint32_t           hash;      /*!< experimental          */
    struct ipflowinfo  *rflow;    /*!< ptr to reverse flow   */
    int                iflow;     /*!< initiating flow flag  */

    int       exported;           /*!< flag */
    void      *user;              /*!< user data */

    ipflow_elems_t     elems;
    uint8_t            saddr[16];
    uint8_t            daddr[16];
#if defined(MACADDREXPORT)
    uint8_t            smac[6];
    uint8_t            dmac[6];
#endif
} ipflowinfo_t;

typedef struct ipflow
{
    ipflowinfo_t   *flows;     /* current flows */
    ipfrag_info_t  *frags;     /* experimental, current fragments */

    ipflowinfo_t   **hashtab;  /* hash table */
    int            hashsize;
    int            nhashes;
    int            nflows;

    int            maxflows;
    int            timeout;    /* flow expiration */
    int            biflows;    /* biflow detection flag */
    int            nextflowid;

} ipflow_t;

/*------ structs ---------------------------------------------------------*/

int  flow_init  ( ipflow_t **flows, int maxflows, int biflows, int timeout );
int  flow_update( ipflow_t *flows, ipflow_elems_t *elems, struct timeval *tv,
                  size_t nbytes, ipflowinfo_t **flowinfo );
void flow_close ( ipflow_t *flows );

void flow_drop  ( ipflow_t *flows, ipflowinfo_t *node );
int  ipflow_listflows( FILE *fp, ipflow_t *flows );
int  ipflow_get_elems( ipflow_elems_t *elems, uint8_t *packet, size_t caplen );

#endif
