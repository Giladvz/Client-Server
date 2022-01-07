#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

uint32_t pcc_total[95];
uint32_t addToStatistics[95];
// Variables that can be changed by handler
int Exit = 1;
int connfd = -1;

// Function that is used to quit main loop
void quits() {
    int i;
    Exit = 0;
    // If server is listening can quit right now
    if (connfd == -1) {
        for (i=0;i<95;i++) {
            printf("char '%c' : %u times\n", i+32, pcc_total[i]);
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
    int exbool = 1;
    int tryAgain = 1;
    uint32_t count = 0;
    uint32_t used;
    uint32_t bytes_rw;
    struct sockaddr_in serv_addr;
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
        perror("Could not create socket \n");
        exit(1);
    }

    // Got help from
    // https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
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

    // Set listen queue to 10 like requested.
    if( 0 != listen( sockfd, 10 ) )
    {
        perror("Listen Failed\n");
        exit(1);
    }

    while( Exit ) {
        connfd = -1;
        if ((connfd = accept(sockfd,
                        NULL,
                        NULL)) < 0) {
            perror("Accept Failed\n");
            exit(1);
        }

        used = 0;
        // Don't know why using pointer doesn't work
        // used https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c
        size = (char*)&num;
        // Keep looping until nothing left to write
        while (exbool) {
            bytes_rw = read(connfd,
                          size + used,
                          sizeof(uint32_t) - used);
            // Problem reading from client
            if (bytes_rw <= 0) {
                // Connection closed, connect to a new request
                if (errno == ETIMEDOUT || errno == ECONNRESET ||
                    errno == EPIPE){
                    perror("Problem reading from client");
                    exbool = 0;
                    break;
                }
                // Read return 0 for multiple reasons. Makes sure its 0 because
                // reaching EOF too soon. Goes again to make sure
                else if(bytes_rw == 0) {
                    if (tryAgain) {
                        tryAgain = 0;
                        continue;
                    }
                    else {
                        perror("Problem reading from client");
                        exbool = 0;
                        break;
                    }
                }
                // Some other problem. Closes server
                else {
                    perror("Problem reading from client");
                    exit(1);
                }
            }
            used += bytes_rw;

            if (used == sizeof(uint32_t)) {
                used = 0;
                bytes_rw = 0;
                break;
            }
            bytes_rw = 0;
        }
        tryAgain = 1;
        // Reallocs txt size according to N passed
        if ((txt = realloc(txt,ntohl(num))) == NULL) {
            perror("Error with malloc");
            exit(1);
        }

        while (exbool) {
            bytes_rw = read(connfd,
                            txt + used,
                            ntohl(num) - used);
            // Problem reading from client
            if (bytes_rw <= 0) {
                // Connection closed, connect to a new request
                if (errno == ETIMEDOUT || errno == ECONNRESET ||
                    errno == EPIPE){
                    perror("Problem reading from client");
                    exbool = 0;
                    break;
                }
                // Read return 0 for multiple reasons. Makes sure its 0 because
                // reaching EOF too soon. Goes again to make sure
                else if(bytes_rw == 0) {
                    if (tryAgain) {
                        tryAgain = 0;
                        continue;
                    }
                    else {
                        perror("Problem reading from client");
                        exbool = 0;
                        break;
                    }
                }
                // Some other problem. Closes server
                else {
                    perror("Problem reading from client");
                    exit(1);
                }
            }
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
        tryAgain = 1;
        count = htonl(count);
        while( exbool )
        {
            bytes_rw = write(connfd,
                             &count + used,
                             sizeof(uint32_t)-used);
            // Problem reading from client
            if (bytes_rw <= 0) {
                // Connection closed, connect to a new request
                if (errno == ETIMEDOUT || errno == ECONNRESET ||
                    errno == EPIPE){
                    perror("Problem writing from client");
                    exbool = 0;
                    break;
                }
                // Read return 0 for multiple reasons. Makes sure its 0 because
                // reaching EOF too soon. Goes again to make sure
                else if(bytes_rw == 0) {
                    if (tryAgain) {
                        tryAgain = 0;
                        continue;
                    }
                    else {
                        perror("Problem writing from client");
                        exbool = 0;
                        break;
                    }
                }
                // Some other problem. Closes server
                else {
                    perror("Problem writing from client");
                    exit(1);
                }
            }

            used += bytes_rw;
            if (used == sizeof(uint32_t)) {
                for (i = 0; i < 95;i++){
                    pcc_total[i] += addToStatistics[i];
                }
                break;
            }
        }
        count = 0;
        for (i = 0; i < 95;i++){
            addToStatistics[i] = 0;
        }
        exbool = 1;
        tryAgain = 1;
    }
    for (i=0;i<95;i++) {
        printf("char '%c' : %u times\n", i+32, pcc_total[i]);
    }
    exit(0);
}