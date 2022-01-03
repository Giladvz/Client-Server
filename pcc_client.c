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
    struct sockaddr_in serv_addr;
    FILE * fd;
    int sockfd;
    long fileLen;
    int bytes_rw;
    uint32_t NSize;
    char * fileValue;
    int used =0;


    if (argc != 4) {
        errno = EINVAL;
        perror("There should be 3 paramaters\n");
        //exit(1);
    }

    if ((fd = fopen(argv[3], "rb")) < 0) {
        perror("Could not open the file\n");
        exit(1);
    }
    // Get file length
    if (fseek(fd, 0L, SEEK_END)<0) {
        perror("Can't get file length");
        exit(1);
    }
    if ((fileLen = ftell(fd)) < 0) {
        perror("Can't get file length");
        exit(1);
    }
    // Restarts the file buffer
    rewind(fd);

    NSize = htonl(fileLen);

    // Create send buffer
    fileValue=malloc(fileLen);

   if (fread(fileValue,NSize,1,fd) != NSize) {
       perror("Can't write file to buffer");
       exit(1);
   }

    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Could not create socket \n");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t)atoi(argv[2])); // Note: htons for endiannes
    if (inet_pton(AF_INET,argv[1],&serv_addr.sin_addr) <= 0) {
        perror("Could not use the ip given.\n");
        exit(1);
    }

    if( connect(sockfd,
                (struct sockaddr*) &serv_addr,
                sizeof(serv_addr)) < 0)
    {
        perror("Connect Failed.\n");
        exit(1);
    }

    while( 1 )
    {
        bytes_rw = (int)write(sockfd,
                              (char*)&NSize + used,
                              sizeof(NSize));
        if(bytes_rw < 0 ){
            perror("Can't write to server");
            exit(1);
        }
        used += bytes_rw;
        if (used == sizeof(NSize)) {
            bytes_rw = 0;
            used = 0;
            break;
        }
        bytes_rw = 0;
    }
    while( 1 )
    {
        bytes_rw = (int)write(sockfd,
                              fileValue + used,
                              NSize);
        if(bytes_rw < 0 ){
            perror("Can't write to server");
            exit(1);
        }
        used += bytes_rw;
        if (used == NSize) {
            bytes_rw = 0;
            used = 0;
            break;
        }
    }
    realloc(fileValue,sizeof(uint32_t));
    while( 1 )
    {
        bytes_rw = read(sockfd,
                          fileValue + used,
                          sizeof(uint32_t));
        if( bytes_rw < 0 ) {
            perror("Can't read from server\n");
            exit(1);
        }
        used += bytes_rw;
        if (used == NSize){
            break;
        }
    }

    close(sockfd);
    close(fd);
    printf("# of printable characters: %u\n",*fileValue);
    exit(0);
}