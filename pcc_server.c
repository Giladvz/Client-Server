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
#include <signal.h>
#include <string.h>

uint32_t statistics[95];
uint32_t addToStatistics[95];
int Exit = 1;
int connfd = -1;

void quits() {
    int i;
    Exit = 0;
    if (connfd == -1) {
        for (i=0;i<95;i++) {
            printf("char '%c' : %u times\n", i+32,statistics[i]);
        }
        exit(0);
    }
}

void setSig() {
    struct sigaction quit;
    memset(&quit,0,sizeof(quit));
    quit.sa_handler = quits;
    quit.sa_flags = SA_RESTART;
    sigaction(SIGINT,&quit,NULL);
}

int main(int argc,char* argv[]) {
    setSig();
    int i;
    int sockfd = -1;

    uint32_t count = 0;
    uint32_t used;
    uint32_t bytes_rw;
    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );
    int32_t num =0;
    char * size;
    char* txt = NULL;



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

    while( Exit ) {
        // Accept a connection.
        // Can use NULL in 2nd and 3rd arguments
        // but we want to print the client socket details
        connfd = -1;
        if ((connfd = accept(sockfd,
                        (struct sockaddr *) &peer_addr,
                        &addrsize)) < 0) {
            perror("Accept Failed\n");
            exit(1);
        }

        used = 0;
        size = (char*)&num;
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
        if ((txt = realloc(txt,ntohl(num))) == NULL) {
            perror("Error with malloc");
            exit(1);
        }

        while (1) {
            bytes_rw = read(connfd,
                            txt + used,
                            ntohl(num) - used);
            used += bytes_rw;
            if (used == ntohl(num)) {
                bytes_rw = 0;
                break;
            }
            bytes_rw = 0;
        }
        for (i = 0; i < used;i++){
            if (txt[i] <= 126 && txt[i] >= 32) {
                addToStatistics[txt[i] - 32] += 1;
                count++;
            }
        }
        used = 0;
        count = htonl(count);
        while( 1 )
        {
            bytes_rw = write(connfd,
                             &count + used,
                             sizeof(uint32_t)-used);
            if(bytes_rw < 0 ){
                perror("Can't write to server");
                exit(1);
            }
            used += bytes_rw;
            if (used == sizeof(uint32_t)) {
                for (i = 0; i < 95;i++){
                    statistics[i] += addToStatistics[i];
                }
                break;
            }
            fprintf(stderr,"used:%d",used);
        }
        count = 0;
        for (i = 0; i < 95;i++){
            addToStatistics[i] = 0;
        }
    }
    for (i=0;i<95;i++) {
        printf("char '%c' : %u times\n", i+32,statistics[i]);
    }
    exit(0);
}