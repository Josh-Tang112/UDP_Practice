#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <endian.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <math.h>

#include "../includes/optparser.h"

int main(int argc, char *argv[]) {
    if (argc != 9 && argc != 10){
        printf("You gave %d arguments. Wrong number of arguments.\n", argc);
        return 1;
    }

    /* get args */
    struct client_arguments args = client_parseopt(argc, argv);
    
    /* setting up address info */
    struct addrinfo addrCriteria;
    memset(&addrCriteria,0,sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_DGRAM;
    addrCriteria.ai_protocol = IPPROTO_UDP;
    char *servPort = (char *)malloc((int)(ceil(log10(args.port))+2));
    sprintf(servPort,"%d",args.port);
    /* get address */
    struct addrinfo *servAddr;
    int rntval = getaddrinfo(args.ip_address,servPort,&addrCriteria,&servAddr);
    if(rntval) {
        perror("getaddrinfo() failed.\n");
        return 1;
    }
    /* socket time */
    int sock = socket(servAddr->ai_family,servAddr->ai_socktype,servAddr->ai_protocol);
    if (sock < 0) {
        perror("socket() failed.\n");
        return 1;
    }

    char req[24], resp[40];
    struct timespec d;
    ssize_t numBytes;
    double *res = (double *)malloc(sizeof(double) * 2 * args.num);
    for (int i = 0; i < 2 * args.num; i++){
        res[i] = -100;
    }

    uint64_t *z;
    int *l;
    uint16_t *s;
    /* timerequest time */
    for(int i = 1; i <= args.num; i++){
        /* constructing timerequest */
        memset(req,0,sizeof(req));
        l = (int *)req;
        l[0] = htonl(i);
        if (!args.condense) {
            l[1] = htonl(7);
        }
        else {
            s = (uint16_t *)(l+1);
            s[0] = htons(7);
        }
        clock_gettime(CLOCK_REALTIME,&d);
        z = (uint64_t *)(req + 8 - args.condense);
        z[0] = htobe64((int)d.tv_sec);
        z[1] = htobe64(d.tv_nsec);
        /* send timerequest */
        numBytes = sendto(sock,req,sizeof(req)-args.condense,0,servAddr->ai_addr,servAddr->ai_addrlen);
        if (numBytes < 0) {
            printf("sendto() failed.\n");
            return 1;
        }
        else if ((long unsigned int)numBytes != sizeof(req)-args.condense) {
            perror("sendto() sent unexpected number of bytes.\n");
            return 1;
        }
    }

    /* receive timeresponse */
    struct pollfd fds[1];
    memset(fds,0,sizeof(fds));
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    int timeout = args.timeout?(args.timeout * 1000) : -1, rc, nfds = 1;
    int num_received = 0;
    while(num_received < args.num){
        /* setting up timeout */
        rc = poll(fds,nfds,timeout);
        if (rc < 0) {
            perror("poll() failed.\n");
            return 1;
        }
        else if(!rc){
            break;
        }
        /* receives datagram */
        struct sockaddr_storage fromAddr;
        socklen_t fromAddrLen = sizeof(fromAddr);
        numBytes = recvfrom(sock,resp,sizeof(resp)-args.condense,0,(struct sockaddr *) &fromAddr,&fromAddrLen);
        if (!numBytes) {
            printf("No bytes were received for time response.\n");
            return 1;
        }
        else if (numBytes < 0){
            printf("recvfrom() failed.\n");
            return 1;
        }
        else if ((long unsigned int)numBytes != sizeof(resp)-args.condense) {
            perror("recvfrom() received unexpected number of bytes.\n");
            return 1;
        }
        /* calculation */
        clock_gettime(CLOCK_REALTIME,&d); // T2
        if ((!args.condense && ntohl(((int *)resp)[1]) != 7) || 
            (args.condense && ntohs(((uint16_t *)resp)[2]) != 7)) {
            printf("Received wrong datagram.\n");
            return 1;
        }
        z = (uint64_t *)(resp + 8 - args.condense);
        uint64_t t0_sec = be64toh(z[0]), t0_nsec = be64toh(z[1]); // T0
        uint64_t t1_sec = be64toh(z[2]), t1_nsec = be64toh(z[3]); // T1
        char *str0 = (char *)malloc((int)(ceil(log10(t0_sec))+ceil(log10(t0_nsec))+4));
        char *str1 = (char *)malloc((int)(ceil(log10(t1_sec))+ceil(log10(t1_nsec))+4));
        char *str2 = (char *)malloc((int)(ceil(log10((int)d.tv_sec))+ceil(log10(d.tv_nsec))+4));
        sprintf(str0,"%lu.%lu",t0_sec,t0_nsec);
        sprintf(str1,"%lu.%lu",t1_sec,t1_nsec);
        sprintf(str2,"%d.%ld",(int)d.tv_sec,d.tv_nsec);
        long double t0 = strtod(str0, NULL), t1 = strtod(str1, NULL), t2 = strtod(str2, NULL);
        if(!(t1&&t2&&t0)){
            printf("t0: %Lf, t1: %Lf, t2: %Lf\n",t0,t1,t2);
            return 1;
        }
        double theta = ((t1 - t0) + (t1 - t2)) / 2;
        double delta = t2 - t0;
        int pos = ntohl(((int *)resp)[0]) - 1;
        if (res[2 * pos] == -100 && res[2 * pos + 1] == -100) {
            num_received++;
        }
        res[2 * pos] = theta;
        res[2 * pos + 1] = delta;
    }
    for (int i = 0; i < 2 * args.num; i += 2) {
        if(res[i] == -100 && res[i + 1] == -100) {
            printf("%d: Dropped\n", (i/2)+1);
        }
        else {
            printf("%d: %.4f %.4f\n",(i/2)+1,res[i],res[i+1]);
        }
    }

    freeaddrinfo(servAddr);
    close(sock);
    free(res);
    free(servPort);
    return 0;
}