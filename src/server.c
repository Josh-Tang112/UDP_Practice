#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <time.h>

#include "../includes/optparser.h"

struct state {
    int max_seq;
    int port;
    long double last_time;
    char ip[16];
};

/* used to find states */
int state_finder(struct state *ptr, char *ip, int port, int state_count){
    for(int i = 0; i < state_count; i ++) {
        if(!strcmp(ptr[i].ip,ip) && port == ptr[i].port){
            return i;
        }
    }
    return -1; //default return value
}

int main(int argc, char *argv[]) {
    if (argc != 5 && argc != 3 && argc != 6){
        printf("wrong # of arguments: %d\n", argc);
        return 1;
    }

    /* get args and parse port*/
    struct server_arguments args = server_parseopt(argc, argv);
    if(args.droprate < 0 || args.droprate > 100) {
        printf("-d n, n must in [0,100]\n");
        return 1;
    }
    char *servPort = (char *)malloc((int)(ceil(log10(args.port))+2));
    sprintf(servPort,"%d",args.port);

    /* construct server address structure */
    struct addrinfo addrCriteria;
    memset(&addrCriteria,0,sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_flags = AI_PASSIVE;
    addrCriteria.ai_socktype = SOCK_DGRAM;
    addrCriteria.ai_protocol = IPPROTO_UDP;

    struct addrinfo *servAddr;
    int rtnval = getaddrinfo(NULL,servPort,&addrCriteria,&servAddr);
    if (rtnval) {
        perror("geraddrinfo() failed.\n");
        return 1;
    }

    /* socket for connection */
    int sock = socket(servAddr->ai_family,servAddr->ai_socktype,servAddr->ai_protocol);
    if (sock < 0) {
        perror("socket() failed.\n");
        return 1;
    }

    /* binding */
    if(bind(sock,servAddr->ai_addr,servAddr->ai_addrlen) < 0){
        perror("bind() failed.\n");
        return 1;
    }

    /* real meat */
    int point;
    char req[24], resp[40];
    ssize_t numBytes;
    uint64_t *z;
    struct timespec d;
    int state_count = 0; // keep track of # of state
    struct state state_lst[1024];
    while(1) {
        struct sockaddr_storage clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);
        memset(resp,0,sizeof(resp));
        /* receives request */
        numBytes = recvfrom(sock,req,sizeof(req)-args.condense,0,(struct sockaddr *)&clntAddr,&clntAddrLen);
        if (numBytes < 0) {
            perror("recvfrom() failed.\n");
            continue;
        }
        /* converting client info into ip and port*/
        char clntIP[INET_ADDRSTRLEN];
        struct sockaddr_in *clnt = (struct sockaddr_in*)&clntAddr;
        inet_ntop(AF_INET,&clnt->sin_addr.s_addr,clntIP,sizeof(clntIP));
        /* get clock time */
        clock_gettime(CLOCK_REALTIME,&d);
        /* decide whether to drop */
        point = random() % 101;
        if(point <= args.droprate){
            continue;
        }
        /* crafting response */
        memcpy(resp,req,sizeof(req));
        z = (uint64_t *)(resp + 8 - args.condense);
        z[2] = htobe64((int)d.tv_sec);
        z[3] = htobe64(d.tv_nsec);
        /* converting to floating point num */
        char *str1 = (char *)malloc((int)(ceil(log10((int)d.tv_sec))+ceil(log10(d.tv_nsec))+4));
        sprintf(str1,"%d.%ld",(int)d.tv_sec,d.tv_nsec);
        long double t1 = strtod(str1, NULL);
        /* keep track of seq # */
        int *i = (int *)req;
        int seq = i[0];
        if(!state_count || state_count >= 100) { // make sure state list isn't empty
            strcpy(state_lst[0].ip,clntIP);
            state_lst[0].last_time = t1;
            state_lst[0].max_seq = seq;
            state_lst[0].port = clnt->sin_port;
            state_count = 1;
        }
        else { // try to find the state
            int pos = state_finder(state_lst,clntIP,clnt->sin_port, state_count);
            if (pos == -1) {
                state_lst[state_count].last_time = t1;
                state_lst[state_count].max_seq = seq;
                state_lst[state_count].port = clnt->sin_port;
                strcpy(state_lst[state_count].ip,clntIP);
                state_count += 1;
            }
            else if (t1 - state_lst[pos].last_time > 120) {
                state_lst[pos].last_time = t1;
                state_lst[pos].max_seq = seq;
            }
            else if (state_lst[pos].max_seq > seq) {
                printf("%s:%d %d %d",clntIP,clnt->sin_port,seq,state_lst[pos].max_seq);
            }
            else {
                state_lst[pos].last_time = t1;
                state_lst[pos].max_seq = seq;
            }
        }
        /* send response */
        numBytes = sendto(sock,resp,sizeof(resp)-args.condense,0,(struct sockaddr *)&clntAddr,sizeof(clntAddr));
        if(numBytes<0){
            perror("sendto() failed.\n");
            continue;
        }
        else if((long unsigned int)numBytes!=sizeof(resp)-args.condense){
            printf("sendto() sends unexpected number of bytes.\n");
            continue;
        }
        free(str1);
    }
    freeaddrinfo(servAddr);
    free(servPort);
}