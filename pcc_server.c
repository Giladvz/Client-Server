#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

uint32_t statistics[95];
uint32_t addToStatistics[95];
int main(int argc,char* argv[]) {
    int i;
    int sockfd = -1;
    int connfd = -1;

    uint32_t used;
    uint32_t bytes_rw;
    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    uint32_t size[sizeof(uint32_t)];
    char* txt;

    if (argc != 2) {
        errno = EINVAL;
        perror("There should be 1 paramaters\n");
        exit(1);
    }

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Could not create socket \n");
        exit(1);
    }

    // Got help from https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed\n");
        exit(1);
    }


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if( 0 != bind( sockfd,
                   (struct sockaddr*) &serv_addr,
                   addrsize))
    {
        perror("Bind failed");
        exit(1);
    }

    if( 0 != listen( sockfd, 10 ) )
    {
        perror("Listen Failed\n");
        exit(1);
    }

    while( 1 ) {
        // Accept a connection.
        // Can use NULL in 2nd and 3rd arguments
        // but we want to print the client socket details
        if ((connfd = accept(sockfd,
                        (struct sockaddr *) &peer_addr,
                        &addrsize)) < 0) {
            perror("Accept Failed\n");
            exit(1);
        }

        used = 0;

        // keep looping until nothing left to write
        while (1) {
            bytes_rw = read(connfd,
                          size + used,
                          sizeof(uint32_t) - used);
            used += bytes_rw;
            if (used == sizeof(uint32_t)) {
                used = 0;
                bytes_rw = 0;
                break;
            }
            bytes_rw = 0;
        }
        txt = malloc(ntohl(size));
        while (1) {
            bytes_rw = read(connfd,
                            txt + used,
                            sizeof(uint32_t) - used);
            used += bytes_rw;
            if (used == size) {
                bytes_rw = 0;
                break;
            }
            bytes_rw = 0;
        }
        for (i; i<size;i++){
            if (txt[i] <= 126 && txt[i] >= 32) {
                addToStatistics[txt[i] - 32] += 1;
            }
        }
    }
    for (i=0;i<95;i++) {
        printf("char '%c' : %u times\n");
    }
    exit(0);
}