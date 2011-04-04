#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include <ipfix.h>
#include <misc.h>

#include "ipfix_col.h"
#include "ipfix_def_fokus.h"
#include "ipfix_fields_fokus.h"

#include "utilities.h"
#include "reexporter.h"
#include "thread2.h"

static ipfix_col_info_t* g_colinfo = NULL;

int *tcp_s = NULL, ntcp_s = 0;
int sourceid = 680335508;

ipfix_t *ipfixh = NULL;

struct templates* g_templates = NULL;
int bytecounter;

struct parameters par;

pthread_barrier_t barrier;
pthread_barrierattr_t attr;

void exit_func(int signo) {

    if (tcp_s) {
        int i;

        for (i = 0; i < ntcp_s; i++)
            close(tcp_s[i]);
    }

    ipfix_col_cleanup();
    ipfix_cleanup();

    exit(1);
}

int export_newsource_cb(ipfixs_node_t *s, void *arg) {
    fprintf(stdout, "New source: %i\n", s->odid);
    return 0;
}

int export_newmsg_cb(ipfixs_node_t *s, ipfix_hdr_t *hdr, void *arg) {
    fprintf(stdout, "New message\n");
    return 0;
}

int export_trecord_cb(ipfixs_node_t *s, ipfixt_node_t *t, void *arg) {
    ipfix_template_t* newtemplate;

    /* Create new template with IPFIX library*/
    if (ipfix_new_data_template(ipfixh, &newtemplate, t->ipfixt->nfields) < 0) {
        fprintf(stderr, "ipfix_new_template() failed in reexport: %s\n", strerror(errno));
        exit(1);
    }

    /* Fill outtemplate with values from incoming template*/
    int i;
    for (i = 0; i < t->ipfixt->nfields; i++) {
        if (ipfix_add_field(ipfixh, newtemplate, t->ipfixt->fields[i].elem->ft->eno,
                t->ipfixt->fields[i].elem->ft->ftype, t->ipfixt->fields[i].flength) < 0) {
            fprintf(stderr, "ipfix_add_field() failed in reexport: %s - eno = %i, type = %i, length = %i\n",
                    strerror(errno), t->ipfixt->fields[i].elem->ft->eno, t->ipfixt->fields[i].elem->ft->ftype, t->ipfixt->fields[i].flength);
            exit(1);
        }
    }
    /* Add the new template to my TemplatesList*/
    newtemplate->odid = s->odid;
    appendTemplate(&g_templates, t->ipfixt, newtemplate);

    return 0;
}

int export_drecord_cb(ipfixs_node_t *s, ipfixt_node_t *t, ipfix_datarecord_t *d, void *arg) {
    ipfix_template_t* newtemplate = NULL;
    struct templates* temp;

    temp = getTemplate(g_templates, t->ipfixt);

    if(temp == NULL) {
        return -1;
    } else {
        newtemplate = temp->newt;
    }
     
    ipfixh->sourceid = newtemplate->odid;
    ipfix_export_array(ipfixh, newtemplate, newtemplate->nfields, d->addrs, d->lens);
    ipfix_export_flush(ipfixh);

    return 0;
}

void export_cleanup_cb(void *arg) {
    dropTemplates(&g_templates);
    ipfix_close(ipfixh);
    ipfix_cleanup();
}

void *startReexporter() {
    int port = 4739;

    if (ipfix_init() < 0) {
        fprintf(stderr, "ipfix() failed: %s\n", strerror(errno));
        exit(1);
    }

    if (ipfix_add_vendor_information_elements(ipfix_ft_fokus) < 0) {
        fprintf(stderr, "ipfix_add_ie() failed: %s\n", strerror(errno));
        exit(1);
    }

    if (ipfix_open(&ipfixh, sourceid, IPFIX_VERSION) < 0) {
        fprintf(stderr, "ipfix_open() failed: %s\n", strerror(errno));
        exit(1);
    }

    signal(SIGKILL, exit_func);
    signal(SIGTERM, exit_func);
    signal(SIGINT, exit_func);

    pthread_barrier_wait(&barrier);

        if (ipfix_add_collector(ipfixh, par.host, par.port, IPFIX_PROTO_TCP) < 0) {
        fprintf(stderr, "ipfix_add_collector(%s,%d) failed: %s\n",
                par.host, par.port, strerror(errno));
        exit(1);
    }

    if ((g_colinfo = calloc(1, sizeof (ipfix_col_info_t))) == NULL) {
        fprintf(stderr, "a calloc failed while initializing callback methods.\n");
        exit(1);
    }

    g_colinfo->export_newsource = export_newsource_cb;
    g_colinfo->export_newmsg = export_newmsg_cb;
    g_colinfo->export_trecord = export_trecord_cb;
    g_colinfo->export_drecord = export_drecord_cb;
    g_colinfo->export_cleanup = export_cleanup_cb;
    g_colinfo->data = NULL;

    if (ipfix_col_register_export(g_colinfo) < 0) {
        fprintf(stderr, "ipfix_col_register_export() failed: %s\n", strerror(errno));
        exit(1);
    }

    if (ipfix_col_listen(&ntcp_s, &tcp_s, IPFIX_PROTO_TCP, port, AF_INET, 10) < 0) {
        fprintf(stderr, "ipfix_listen(tcp) failed.\n");
        exit(1);
    }

    (void) mpoll_loop(-1);

    pthread_exit(NULL);
}

static void usage() {
    const char helptxt[] =
        "[options]\n"
        "options:\n"
        "  -h                  this help\n"
        "  -f <datadir>        store files of collected data in this dir\n"
        "  -p <portno>         listen on this port (default=4739)\n"
        "  -s <size>           define the chunk size (default=1M)\n"
        "  -g <hostname>       instead of writing to a file forward ipfix\n"
        "                      to <hostname>";

    fprintf(stderr, "\nipfix reexporter (%s)\n",  __DATE__ );
    fprintf(stderr, "\nusage: %s\n", helptxt);
}

int main(int argc, char** argv) {
    pthread_t idC, idR;
    char opt;
    int remcol = 0;
    unsigned count = 2;
    char* dpath = "record";
    socklen_t sd;

   /* set default options
    */
    par.host = "127.0.0.1";
    par.port = 4740;
    par.path = dpath;
    par.sock = -1;
    par.prot = NULL;
    par.size = 5*1024;

    while ((opt = getopt(argc, argv, "f:hp:s:g:")) != EOF) {
        switch (opt) {
            case 's': /*"j"unksize*/
                par.size = atoi(optarg) * 1024*1024;
                break;
            case 'p': /*port*/
                if((par.port = atoi(optarg)) < 0) {
                    fprintf(stderr, "Invalid -p argument\n");
                    exit(1);
                }
                break;
            case 'f': /*path+filename*/
                par.path = optarg;
                break;
            case 'g':
                par.host = optarg;
                remcol = count = 1;
                break;
            case '?':
            case 'h':
            default:
                usage();
                exit(1);
                break;
        }
    }

    if (pthread_barrier_init(&barrier, &attr, count) != 0) {
            fprintf(stdout, "ERROR: pthread_barrier_init() failed");
            return 0;
        }

    /* Only start thread1 for remote collector*/
    if (remcol) {
        pthread_create(&idC, NULL, startReexporter, NULL);
        pthread_join(idC, NULL);
    } 
    /* Start both threads when collector is the filewriter*/
    else {
        sd = initCollector();
        pthread_create(&idC, NULL, startReexporter, NULL);
        while (1) {
            pthread_create(&idR, NULL, startCollector, (void*) &sd);
            pthread_join(idR, NULL);
        }
    }

    return 0;
}
