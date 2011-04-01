#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "../thread2.h"

char *chost = "127.0.0.1";
int bytecounter;

struct parameters par;

static void usage(char* taskname) {
    const char helptxt[] =
        "[options]\n\n"
        "options:\n"
        "  -h                  this help\n"
        "  -f <datadir>        store files of collected data in this dir\n"
        "  -p <portno>         listen on this port (default=4739)\n"
        "  -s <size>           define the chunk size (default=1M)\n"
        "  -P <protocol>       set protocol, either TCP or UDP\n";

    fprintf(stderr, "\nipfixtofile SINGLEFLOW (%s %s)\n", "$Revision: 0.1 $", __DATE__ );
    fprintf(stderr, "\nusage: %s %sexample: %s -stu -vv -o . \n\n",
    taskname, helptxt, taskname);
}

int main(int argc, char** argv) {
    char opt;
    socklen_t sd;
    char* dpath = "record";

    /* set default options
     */
    par.host = NULL;
    par.port = 4740;
    par.path = dpath;
    par.prot = NULL;
    par.size = 10*1024;

    while ((opt = getopt(argc, argv, "f:p:h:s:P")) != EOF) {
        switch (opt) {
            case 's': /*"j"unksize*/
                par.size = atoi(optarg) *1024*1024;
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
            case 'h':
                break;
            case 'P':
                break;
            case '?':
                break;
            default:
                usage(par.progname);
                exit(1);
                break;
        }
    }

    sd = initCollector();

    while(1)
        startCollector((void*)&sd);

    return 0;
}

