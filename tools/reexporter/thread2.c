#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "thread2.h"

#ifndef SINGLEFLOW
#include <ipfix.h>
#include <misc.h>
#include "utilities.h"
#endif

#define HEADER_SIZE 16
#define MSG_SIZE    4096

extern struct parameters par;
extern int bytecounter;

#ifndef SINGLEFLOW
extern pthread_barrier_t barrier;
extern pthread_barrierattr_t attr;
int first = 1;
#endif


int filec = 0;
char* timestring = "";


socklen_t initCollector() {
    socklen_t sd;
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof (sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(par.port);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket() failed\n");
        exit(1);
    }

    if (bind(sd, (struct sockaddr *) &sa, sizeof (sa)) < 0) {
        printf("bind() failed, %s\n", strerror(errno));
        close(sd);
        exit(1);
    }

    if (listen(sd, 5) < 0) {
        printf("listen() failed\n");
        close(sd);
        exit(1);
    }
    
    return sd;
}

void *startCollector(void* socket) {
    socklen_t *sd = (socklen_t*)socket;
    socklen_t client, cl;
    struct sockaddr_in ca;

#ifndef SINGLEFLOW
    /* Professionel...*/
    if (first) {
        pthread_barrier_wait(&barrier);
        first = 0;
    }
#endif

    cl = sizeof (ca);
    if ((client = accept(*sd, (struct sockaddr *) &ca, &cl)) < 0) {
        printf("accept() failed\n");
        close((int)*sd);
        exit(1);
    }

    fprintf(stdout, "Client Information (url: %s, port: %d)\n", inet_ntoa(ca.sin_addr), ca.sin_port);

    readLoop(client);
    shutdown(client, 2);
    

#ifndef SINGLEFLOW
    pthread_exit(NULL);
#else
    return NULL;
#endif
}

int fetchBytesFromSocket(socklen_t socket, uint8_t* data, int length) {
    int bytes = 0;
    int remaining = length;

    while (remaining) {
        bytes = read(socket, &data[length - remaining], remaining);
        if (bytes >= 0) {
            remaining -= bytes;
        }
        if(bytes < 0) {
            return bytes;
        }
    }
    return 0;
}

struct msgheader parseHeader(uint8_t* data) {
    struct msgheader header;
    memcpy(&header, (struct msgheader*)data, HEADER_SIZE);//= (struct msgheader*) data;

    if(ntohs(header.vnum) != 10) {
        fprintf(stderr, "Version nr wrong, probably out of sync\n");
    }
    return header;
}

int readHeader(socklen_t socket, uint8_t* data) {
    return fetchBytesFromSocket(socket, data, HEADER_SIZE);
}

int readMsg(socklen_t socket, uint8_t* data, int length) {
    return fetchBytesFromSocket(socket, data, length);
}

int writeToFile(FILE* fd, uint8_t* data, int length) {
    size_t written;

    while(1) {
        written = fwrite(data, 1, length, fd);
        if(written < 0) {
            fprintf(stderr, "Error writing data to file\n");
            return -1;
        }
        if(written<length) {
            length-=written;
            continue;
        }
        else
            return written;
    }
}

void readLoop(socklen_t socket) {
    uint8_t header[HEADER_SIZE];
    uint8_t msg[MSG_SIZE];
    uint16_t length = 0;
    struct msgheader headerstr;
    FILE* fd;

    fd = createNewFile(par.path);
    while(1) {
        if(length == 0) {
            if(readHeader(socket, header) < 0)
                return;
            writeToFile(fd, header, HEADER_SIZE);
            
            headerstr = parseHeader(header);
            bytecounter += ntohs(headerstr.length);
            length = ntohs(headerstr.length)-HEADER_SIZE;
        }

        if(length >= MSG_SIZE) {
            if(readMsg(socket, msg, MSG_SIZE) < 0)
                return;

            writeToFile(fd, msg, MSG_SIZE);
            length-=MSG_SIZE;
        }
        else {
            if(readMsg(socket, msg, length) < 0)
                return;
            writeToFile(fd, msg, length);
            length = 0;
        }

        if ((bytecounter >= par.size) && (length == 0)) {
            bytecounter = 0;
            fclose(fd);
            close(socket);
        }
    }
}

FILE* createNewFile(char* path) {
    char *pfile = malloc(255);

    if(strncmp(timestring, getTimeString(), 20) == 0)
        filec++;
    else
        filec = 0;

    timestring = getTimeString();
    sprintf(pfile, "%s%s_%d", path, timestring, filec);

    FILE* fd = fopen(pfile, "ab+");
    if(fd == NULL) {
        fprintf(stderr, "fopen() failed: %s\n", strerror(errno));
        exit(1);
    }

    return fd;
}

char* getTimeString() {
    char* timestr = malloc(20);
    struct tm *now = NULL;
    time_t timevalue = 0;

    timevalue = time(NULL);
    now = localtime(&timevalue);

    sprintf(timestr, "_%02d.%02d.%02d_%02d_%02d_%02d",
        now->tm_mday,
        now->tm_mon+1,
        now->tm_year-100,
        now->tm_hour,
        now->tm_min,
        now->tm_sec);
    return timestr;
}
