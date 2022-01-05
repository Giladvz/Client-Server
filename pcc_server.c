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

int main(int argc,char* argv[]) {
    int i;
    int sockfd;
    uint32_t statistics[95];
    uint32_t addToStatistics[95];
    struct sockaddr_in serv_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

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
                   sizeof(struct sockaddr_in) ) )
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
        connfd = accept(listenfd,
                        (struct sockaddr *) &peer_addr,
                        &addrsize);

        if (connfd < 0) {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

        getsockname(connfd, (struct sockaddr *) &my_addr, &addrsize);
        getpeername(connfd, (struct sockaddr *) &peer_addr, &addrsize);
        printf("Server: Client connected.\n"
               "\t\tClient IP: %s Client Port: %d\n"
               "\t\tServer IP: %s Server Port: %d\n",
               inet_ntoa(peer_addr.sin_addr),
               ntohs(peer_addr.sin_port),
               inet_ntoa(my_addr.sin_addr),
               ntohs(my_addr.sin_port));

        // write time
        ticks = time(NULL);
        snprintf(data_buff, sizeof(data_buff),
                 "%.24s\r\n", ctime(&ticks));

        totalsent = 0;
        int notwritten = strlen(data_buff);

        // keep looping until nothing left to write
        while (notwritten > 0) {
            // notwritten = how much we have left to write
            // totalsent  = how much we've written so far
            // nsent = how much we've written in last write() call */
            nsent = write(connfd,
                          data_buff + totalsent,
                          notwritten);
            // check if error occured (client closed connection?)
            assert(nsent >= 0);
            printf("Server: wrote %d bytes\n", nsent);

            totalsent += nsent;
            notwritten -= nsent;
        }


    }
    for (i=0;i<95;i++) {
        printf("char '%c' : %u times\n");
    }
    exit(0);
}