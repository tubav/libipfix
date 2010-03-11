/*
**     exporter.c - example exporter
**
**     Copyright Fraunhofer FOKUS
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>

#include <misc.h>
#include <ipfix.h>
#include <ipfix_def_fokus.h>
#include <ipfix_fields_fokus.h>

#define NEXPORTER 1

#define MODE_VENDOR  1
#define MODE_VARLEN  2
#define MODE_OPTION  4
#define MODE_REDUCED 8
#define MODE_MULTI   16

int g_countsize = 8;

int do_get_template( ipfix_t *ipfix, ipfix_template_t **template, int mode, int *nfields )
{
    ipfix_template_t *t;
    int              n = 3;

    if ( mode & MODE_VENDOR ) {
        n += 2;
    }

    if ( mode & MODE_VARLEN ) {
        n += 1;
    }

    if ( ipfix_new_data_template( ipfix, &t, n ) <0 ) {
        fprintf( stderr, "ipfix_get_template() failed: %s\n", 
                 strerror(errno) );
        return -1;
    }

    if ( ipfix_add_field( ipfix, t, 0,
                          IPFIX_FT_SOURCEIPV4ADDRESS, 4 ) <0 ) {
        fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                 strerror(errno) );
        goto end;
    }

    if ( mode & MODE_VENDOR ) {
        if ((ipfix_add_field( ipfix, t, IPFIX_ENO_FOKUS,
                              IPFIX_FT_PKTID, 4 ) <0 )
            || (ipfix_add_field( ipfix, t, IPFIX_ENO_FOKUS,
                                 IPFIX_FT_TASKID, 4 ) <0 ) ) {
            fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                     strerror(errno) );
            goto end;
        }
    }

    if ( mode & MODE_VARLEN ) {
        if (ipfix_add_field( ipfix, t, IPFIX_ENO_FOKUS,
                             IPFIX_FT_IDENT, IPFIX_FT_VARLEN ) <0 ) {
            fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                     strerror(errno) );
            goto end;
        }
    }

    if ( mode & MODE_MULTI ) {
        if ( (ipfix_add_field( ipfix, t, 
                               0, IPFIX_FT_OCTETTOTALCOUNT, g_countsize ) <0 )
             || (ipfix_add_field( ipfix, t, 
                                  0, IPFIX_FT_OCTETTOTALCOUNT, g_countsize ) <0 ) ) {
            fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                     strerror(errno) );
            goto end;
        }
    }
    else {
        if ( (ipfix_add_field( ipfix, t, 
                               0, IPFIX_FT_OCTETTOTALCOUNT, g_countsize ) <0 )
             || (ipfix_add_field( ipfix, t, 
                                  0, IPFIX_FT_PACKETTOTALCOUNT, g_countsize ) <0 ) ) {
            fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                     strerror(errno) );
            goto end;
        }
    }

    *template = t;
    *nfields  = n;
    return 0;

 end:
    ipfix_delete_template( ipfix, t );
    return -1;
}

int do_get_option_template( ipfix_t *ipfix, ipfix_template_t **template, int mode )
{
    ipfix_template_t *t;
    int              nfields = 2;

    if ( ipfix_new_option_template( ipfix, &t, nfields ) <0 ) {
        fprintf( stderr, "ipfix_get_template() failed: %s\n", 
                 strerror(errno) );
        return -1;
    }

    if ( ipfix_add_scope_field( ipfix, t, 0,
                                IPFIX_FT_FLOWID, 4 ) <0 ) {
        fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                 strerror(errno) );
        goto end;
    }

    if ( ipfix_add_field( ipfix, t, 0,
                          IPFIX_FT_SOURCEIPV4ADDRESS, 4 ) <0 ) {
        fprintf( stderr, "ipfix_add_field() failed: %s\n", 
                 strerror(errno) );
        goto end;
    }

    *template = t;
    return 0;

 end:
    ipfix_delete_template( ipfix, t );
    return -1;
}

int main ( int argc, char **argv )
{
    char      *optstr="stuvhc:p:m:";
    int       opt;
    char      chost[256];
    int       protocol = IPFIX_PROTO_TCP;
    int       ret      = 1, i, j, ix, mode=0;
    int       nfields  = 0;
    uint32_t  bytes    = 1234000;
    uint32_t  pkts     = 1234;
    uint64_t  bytes64;
    uint64_t  pkts64;
    uint32_t  taskid   = 1234567;
    uint32_t  flowid   = 54321;
    char      buf[31]  = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                           21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };
    ipfix_t   *ipfixh[3] = { NULL, NULL, NULL };
    ipfix_template_t   *templ[3]  = { NULL, NULL, NULL };
    ipfix_template_t   *otempl[3] = { NULL, NULL, NULL };
    int       sourceid[3] = { 12345, 23456, 34567 };
    void      *fields[10];
    uint16_t  lengths[10];
    int       port[]    = { 4739, 6767, 6768 };
    int       nexporter = NEXPORTER;
    char      smsident[20] = { 'a', 'b', 'c', 'd', '\0', };
    int       vlevel = 0;

    /* set default host */
    strcpy(chost, "localhost");

    /** process command line args
     */
    while( ( opt = getopt( argc, argv, optstr ) ) != EOF )
    {
	switch( opt )
	{
          case 'm':
              mode = atoi(optarg);
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
	  case 'v':
              vlevel++;
              break;
	  case 'p':
	    if ((port[0]=atoi(optarg)) <0) {
		fprintf( stderr, "Invalid -p argument!\n" );
		exit(1);
	    }
            break;
	  case 'c':
            strcpy(chost, optarg);
	    break;
	  case 'h':
	  default:
              fprintf( stderr,
                       "usage: %s [-stuh] [-m mode] [-c collector] [-p portno]\n"
                       "       mode 1  - use vendor ie \n"
                       "       mode 2  - send variable length ie\n"
                       "       mode 4  - send option data\n"
                       "       mode 8  - send reduced size ie\n"
                       "       mode 16 - send multiple ie of same type\n",
                       argv[0] );
              exit(1);
	}
    }

    mlog_set_vlevel( vlevel );

    /* init lib 
     */
    (void) ipfix_init();

    /* add fokus field types
     */
    printf( "[%s] add fokus information elements.\n", argv[0] );
    if ( ipfix_add_vendor_information_elements( ipfix_ft_fokus ) <0 ) {
        fprintf( stderr, "ipfix_add_vendor_ie() failed: %s\n",
                 strerror(errno) );
        exit(1);
    }
    printf( "done.\n" );

    if ( mode & MODE_REDUCED ) {
        g_countsize  = 4;
    }

    for ( i=0; i<nexporter; i++ ) {
        /* open ipfix exporter
         */
        printf( "[%i] open ipfix exporter.\n", i );
        if ( ipfix_open( &(ipfixh[i]), sourceid[i], IPFIX_VERSION ) <0 ) {
            fprintf( stderr, "ipfix_open() failed: %s\n", strerror(errno) );
            exit(1);
        }
        printf( "done.\n" );

        /** set collector to use
         */
        printf( "[%d] add ipfix collector.\n", i );
        if ( ipfix_add_collector( ipfixh[i], chost, port[i], protocol ) <0 ) {
            fprintf( stderr, "ipfix_add_collector(%s,%d) failed: %s\n", 
                     chost, port[i], strerror(errno));
            exit(1);
        }
        printf( "done.\n" );

        /* get template
         */
        printf( "[%d] create template.\n", i );
        if ( do_get_template( ipfixh[i], &(templ[i]), mode, &nfields ) <0 ) {
            goto end;
        }
        printf( "done.\n" );

        if ( mode & MODE_OPTION ) {
            /* get option template
             */
            printf( "[%d] create option template.\n", i );
            if ( do_get_option_template( ipfixh[i], &(otempl[i]), mode ) <0 ) {
                goto end;
            }
            printf( "done.\n" );
        }
    }
    sleep(2);

    /** export some data
     */
    for( j=0; j<100; j++ ) {
        for ( i=0; i<nexporter; i++ ) {
            ix = 0;
            fields[ix]    = buf;
            lengths[ix++] = 4;
            if ( mode & MODE_VENDOR ) {
                fields[ix]    = &flowid;
                lengths[ix++] = 4;
                fields[ix]    = &taskid;
                lengths[ix++] = 4;
            }
            if ( mode & MODE_VARLEN ) {
                fields[ix]    = smsident;
                lengths[ix++] = 5;
            }
            if ( mode & MODE_REDUCED ) {
                fields[ix]    = &bytes;
                lengths[ix++] = g_countsize;
                fields[ix]    = &pkts;
                lengths[ix++] = g_countsize;
            }
            else {
                pkts64        = pkts;
                fields[ix]    = &bytes64;
                lengths[ix++] = g_countsize;
                bytes64       = bytes;
                fields[ix]    = &pkts64;
                lengths[ix++] = g_countsize;
            }

            if ( mode & MODE_OPTION ) {
                printf( "[%i] export option data.\n", i );
                if ( ipfix_export( ipfixh[i], otempl[i], 
                                   &flowid, buf ) <0 ) {
                    fprintf( stderr, "ipfix_export() failed: %s\n", 
                             strerror(errno) );
                    goto end;
                }
            }

            printf( "[%i] export some data.\n", i );
            if ( (ipfix_export_array( ipfixh[i], templ[i], nfields, 
                                      fields, lengths ) <0 )
                 || (ipfix_export_flush( ipfixh[i] ) <0 )) {
                fprintf( stderr, "ipfix_export_array() failed: %s\n", 
                         strerror(errno) );
                goto end;
            }

            printf( "done.\n" );
            bytes++;
            pkts++;
            sleep(1);
        }

        printf( "data exported.\n" );
    }

 end:
    for ( i=nexporter-1; i>=0; i-- ) {
        ipfix_delete_template( ipfixh[i], templ[i] );
        ipfix_delete_template( ipfixh[i], otempl[i] );
        ipfix_close( ipfixh[i] );
    }

    ipfix_cleanup();
    exit(ret);
}
