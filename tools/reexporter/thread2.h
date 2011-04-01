#ifndef THREAD2_H
#define	THREAD2_H

struct msgheader {
    uint16_t vnum;
    uint16_t length;
    uint32_t expt;
    uint32_t seqnr;
    uint32_t odid;
};

struct parameters {
    char* path;
    int size;
    /*For redirecting on a socket*/
    char* host;
    int port;
    socklen_t sock;
    char* prot;
};

socklen_t initCollector();
void* startCollector();
int fetchBytesFromSocket(socklen_t, uint8_t*, int);
uint16_t fetchMsgSize(socklen_t);
int readHeader(socklen_t, uint8_t*);
void readLoop(socklen_t);
FILE* createNewFile(char*);
char* getTimeString();

#endif

