/*
$$LIC$$
 *
 *   ipfix_ssl.c - IPFIX tls/dtls related funcs
 *
 *   Copyright Fraunhofer FOKUS
 *
 *   $Date: 2009-03-27 20:19:27 +0100 (Fri, 27 Mar 2009) $
 *
 *   $Revision: 96 $
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef SCTPSUPPORT
#include <netinet/sctp.h>
#endif
#include <fcntl.h>
#include <netdb.h>

#include "mlog.h"
#include "ipfix.h"
#include "ipfix_col.h"
#include "ipfix_ssl.h"

/*----- defines ----------------------------------------------------------*/

#define READ16(b) ((*(b)<<8)|*((b)+1))
#define READ32(b) ((((((*(b)<<8)|*(b+1))<<8)|(*(b+2)))<<8)|*(b+3))

/*------ structs ---------------------------------------------------------*/

/*----- revision id ------------------------------------------------------*/

static const char cvsid[]="$Id: ipfix_ssl.c 96 2009-03-27 19:19:27Z csc $";

/*----- globals ----------------------------------------------------------*/

static DH *dh512  = NULL;
static DH *dh1024 = NULL;

/*----- prototypes -------------------------------------------------------*/

extern DH *get_dh512();
extern DH *get_dh1024();

/*----- funcs ------------------------------------------------------------*/

int ipfix_ssl_init()
{
  static int openssl_is_init = 0;
  if ( ! openssl_is_init ) {
      (void)SSL_library_init();
      SSL_load_error_strings();
      /* todo: seed prng? */
      openssl_is_init ++;
  }

  return openssl_is_init;
}

void ipfix_ssl_opts_free( ipfix_ssl_opts_t *opts )
{
    if ( !opts )
        return;

    if (opts->cafile)   free(opts->cafile);
    if (opts->cadir)    free(opts->cadir);
    if (opts->keyfile)  free(opts->keyfile);
    if (opts->certfile) free(opts->certfile);
    free( opts );
}

int ipfix_ssl_opts_new( ipfix_ssl_opts_t **ssl_opts,
                        ipfix_ssl_opts_t *vals )
{
    ipfix_ssl_opts_t *opts;

    if ( (opts=calloc( 1, sizeof(ipfix_ssl_opts_t))) ==NULL ) {
        return -1;
    }

    if ( ((vals->cafile) && ((opts->cafile=strdup(vals->cafile))==NULL))
         || ((vals->cadir) && ((opts->cadir=strdup(vals->cadir))==NULL))
         || ((vals->keyfile) && ((opts->keyfile=strdup(vals->keyfile))==NULL))
         || ((vals->certfile)
             && ((opts->certfile=strdup(vals->certfile))==NULL)) ) {
        ipfix_ssl_opts_free( opts );
        return -1;
    }

    *ssl_opts = opts;
    return 0;
}

int ipfix_ssl_verify_callback(int ok, X509_STORE_CTX *store)
{
    char data[256];

    if (!ok)
    {
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int  depth = X509_STORE_CTX_get_error_depth(store);
        int  err = X509_STORE_CTX_get_error(store);

        mlogf( 1, "[%s] Error with certificate at depth: %i\n",
               __func__, depth);
        X509_NAME_oneline( X509_get_issuer_name(cert), data, 256);
        mlogf( 1, "  issuer   = %s\n", data);
        X509_NAME_oneline( X509_get_subject_name(cert), data, 256);
        mlogf( 1, "  subject  = %s\n", data);
        mlogf( 1, "  err %i:%s\n", err, X509_verify_cert_error_string(err));
    }

    return ok;
}

long ipfix_ssl_post_connection_check(SSL *ssl, char *host)
{
    X509      *cert;
    X509_NAME *subj;
    char      data[256];
    int       extcount;
    int       ok = 0;

    /* Checking the return from SSL_get_peer_certificate here is not strictly
     * necessary.
     */
    if (!(cert = SSL_get_peer_certificate(ssl)) || !host)
        goto err_occured;
    if ((extcount = X509_get_ext_count(cert)) > 0)
    {
        int i;

        for (i = 0;  i < extcount;  i++)
        {
            char              *extstr;
            X509_EXTENSION    *ext;

            ext = X509_get_ext(cert, i);
            extstr = (char*) OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

            if (!strcmp(extstr, "subjectAltName"))
            {
                int                  j;
                const unsigned char  *data;
                STACK_OF(CONF_VALUE) *val;
                CONF_VALUE           *nval;
                X509V3_EXT_METHOD    *meth;
                void                 *ext_str = NULL;

                if (!(meth = X509V3_EXT_get(ext)))
                    break;
                data = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
                if (meth->it)
                  ext_str = ASN1_item_d2i(NULL, &data, ext->value->length,
                                          ASN1_ITEM_ptr(meth->it));
                else
                  ext_str = meth->d2i(NULL, &data, ext->value->length);
#else
                ext_str = meth->d2i(NULL, &data, ext->value->length);
#endif
                val = meth->i2v(meth, ext_str, NULL);
                for (j = 0;  j < sk_CONF_VALUE_num(val);  j++)
                {
                    nval = sk_CONF_VALUE_value(val, j);
                    if (!strcmp(nval->name, "DNS") && !strcmp(nval->value, host))
                    {
                        ok = 1;
                        break;
                    }
                }
            }
            if (ok)
                break;
        }
    }

    if (!ok && (subj = X509_get_subject_name(cert)) &&
        X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256) > 0)
    {
        data[255] = 0;
        if (strcasecmp(data, host) != 0)
            goto err_occured;
    }

    X509_free(cert);
    return SSL_get_verify_result(ssl);

err_occured:
    if (cert)
        X509_free(cert);
    return X509_V_ERR_APPLICATION_VERIFICATION;
}

int ipfix_ssl_init_con( SSL *con )
{
    extern FILE *mlog_fp;
    int i;
    char *str;
    long verify_error;
    char buf[100];

    if ((i=SSL_accept(con)) <= 0) {
        if (BIO_sock_should_retry(i)) {
            mlogf( 0, "[ipfix_ssl_init] DELAY\n");
            return -1;
        }

        mlogf( 0, "[ipfix_ssl_init] ERROR\n");
        verify_error=SSL_get_verify_result( con );
        if (verify_error != X509_V_OK) {
            mlogf( 0, "[ipfix_ssl_init] verify error: %s\n",
                   X509_verify_cert_error_string(verify_error));
        }
        else
            ERR_print_errors_fp( mlog_fp );

        return -1;
    }

    if ( 1 <= mlog_get_vlevel() ) {
        PEM_write_SSL_SESSION( mlog_fp, SSL_get_session(con));

        if ( SSL_get_shared_ciphers(con, buf, sizeof buf) != NULL) {
            mlogf( 3, "[ipfix] Shared ciphers:%s\n", buf);
        }
        str=(char*)SSL_CIPHER_get_name( SSL_get_current_cipher(con) );
        mlogf( 3,  "[ipfix] CIPHER is %s\n",(str != NULL)?str:"(NONE)");
        if (SSL_ctrl(con,SSL_CTRL_GET_FLAGS,0,NULL) &
            TLS1_FLAGS_TLS_PADDING_BUG) {
            mlogf( 1, "[ipfix] Peer has incorrect TLSv1 block padding\n");
        }
    }

    return 0;
}

static DH *load_dhparam( char *dhfile )
{
    DH *ret=NULL;
    BIO *bio;

    if ((bio=BIO_new_file(dhfile,"r")) == NULL)
        goto err;
    ret = PEM_read_bio_DHparams(bio,NULL,NULL,NULL);

 err:
    if (bio != NULL)
        BIO_free(bio);

    return ret;
}

static void init_dhparams( void )
{
    if ( (dh512==NULL)
         && ((dh512=load_dhparam("dh512.pem")) ==NULL) ) {
        dh512 = get_dh512();
    }

    if ( (dh1024==NULL)
         && ((dh1024=load_dhparam("dh1024.pem")) ==NULL) ) {
        dh1024 = get_dh1024();
    }
}

static DH *ipfix_ssl_tmp_dh_callback(SSL *ssl, int is_export, int keylength)
{
    DH *ret=NULL;

    switch (keylength) {
      case 512:
          ret = dh512;
          break;

      case 1024:
      default: /* generating DH params is too costly to do on the fly */
          ret = dh1024;
          break;
    }
    return ret;
}

static int ipfix_ssl_setup_ctx( SSL_CTX **ssl_ctx,
                                SSL_METHOD *method,
                                ipfix_ssl_opts_t *ssl_details )
{
    SSL_CTX *ctx;
    char buffer[PATH_MAX];

    if ( ((ctx=SSL_CTX_new(method)) ==NULL )
         || (SSL_CTX_load_verify_locations( ctx, ssl_details->cafile,
                                            ssl_details->cadir ) != 1)) {
        mlogf( 0, "[ipfix] error loading ca file and/or directory "
               "(cwd=%s,file=%s): %s\n", getcwd(buffer, sizeof(buffer)),
               ssl_details->cafile, strerror(errno) );
        goto err;
    }

    if ( SSL_CTX_set_default_verify_paths(ctx) != 1) {
        mlogf( 0, "[ipfix] error loading default CA file and/or dir\n");
        goto err;
    }

    if ( SSL_CTX_use_certificate_chain_file(ctx, ssl_details->certfile) != 1) {
        mlogf( 0, "[ipfix] error loading certificate from file");
        goto err;
    }

    if ( SSL_CTX_use_PrivateKey_file( ctx, ssl_details->keyfile,
                                      SSL_FILETYPE_PEM ) != 1) {
        mlogf( 0, "[ipfix] Error loading private key from file: %s\n",
               ssl_details->keyfile );
        goto err;
    }

    *ssl_ctx = ctx;
    return 0;

 err:
    SSL_CTX_free( ctx );
    return -1;
}

/*----- exporter funcs (client) ------------------------------------------*/

int ipfix_ssl_setup_client_ctx( SSL_CTX **ssl_ctx,
                                SSL_METHOD *method,
                                ipfix_ssl_opts_t *ssl_details )
{
    SSL_CTX *ctx;

    if ( ipfix_ssl_setup_ctx( &ctx, method?method:SSLv23_method(),
                              ssl_details ) <0 )
        return -1;

    SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER,
                        ipfix_ssl_verify_callback);
    SSL_CTX_set_verify_depth( ctx, 4);
    SSL_CTX_set_options( ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2);
    if (SSL_CTX_set_cipher_list( ctx, CIPHER_LIST) != 1) {
        mlogf( 0, "[ipfix] Error setting cipher list (no valid ciphers)");
        goto err;
    }

    *ssl_ctx = ctx;
    return 0;

 err:
    SSL_CTX_free( ctx );
    return -1;
}

/*----- collector funcs (server) -----------------------------------------*/

int ipfix_ssl_setup_server_ctx( SSL_CTX **ssl_ctx,
                                SSL_METHOD *method,
                                ipfix_ssl_opts_t *ssl_details )
{
    SSL_CTX *ctx;

    if ( ipfix_ssl_setup_ctx( &ctx, method?method:SSLv23_method(),
                              ssl_details ) <0 ) {
        return -1;
    }

    SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                        ipfix_ssl_verify_callback );
    SSL_CTX_set_verify_depth( ctx, 4 );
    SSL_CTX_set_options( ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
                         SSL_OP_SINGLE_DH_USE);
    if (!dh512 || !dh1024) {
        init_dhparams();
    }
    SSL_CTX_set_tmp_dh_callback( ctx, ipfix_ssl_tmp_dh_callback );
    if (SSL_CTX_set_cipher_list( ctx, CIPHER_LIST) != 1) {
        mlogf( 0, "[ipfix] error setting cipher list (no valid ciphers)");
        goto err;
    }

    *ssl_ctx = ctx;
    return 0;

 err:
    SSL_CTX_free( ctx );
    return -1;
}
