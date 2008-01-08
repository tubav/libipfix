/*
$$LIC$$
 *
 * ipfix_ssl.h - export declarations of ipfix_ssl.c
 *
 * Copyright Fraunhofer FOKUS
 *
 * $Id: ipfix_ssl.h,v 1.3 2007/02/07 13:56:01 luz Exp $
 *
 */
#ifndef IPFIX_SSL_H
#define IPFIX_SSL_H

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

extern int openssl_is_init;

void ipfix_ssl_opts_free( ipfix_ssl_opts_t *opts );
int  ipfix_ssl_opts_new( ipfix_ssl_opts_t **opts,
                         ipfix_ssl_opts_t *vals );
int  ipfix_ssl_setup_client_ctx( SSL_CTX **ctx,
                                 SSL_METHOD *method,
                                 ipfix_ssl_opts_t *vals );
int  ipfix_ssl_setup_server_ctx( SSL_CTX **ctx,
                                 SSL_METHOD *method,
                                 ipfix_ssl_opts_t *vals );
long ipfix_ssl_post_connection_check( SSL *ssl, char *host );

#endif
