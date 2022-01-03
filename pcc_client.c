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
    unsigned int readable = 0;
    int16_t port;
    unsigned char buff[sizeof(struct in_addr)];
    FILE * fd;
    int sockfd;
    long fileLen;
    int bytes_write;
    uint32_t NSize;
    char * fileValue;
    int written=0;


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
        bytes_write = (int)write(sockfd,
                            (char*)&NSize + written,
                          sizeof(NSize));
        if( bytes_write < 0 ){
            perror("Can't write to server");
            exit(1);
        }
            written += bytes_write;
        if (written == NSize) {
            bytes_write = 0;
            written = 0;
            break;
        }
        bytes_write = 0;
    }
    while( 1 )
    {
        bytes_write = (int)write(sockfd,
                                 fileValue + written,
                                 sizeof(NSize));
        if( bytes_write < 0 ){
            perror("Can't write to server");
            exit(1);
        }
        written += bytes_write;
        if (written == NSize) {
            break;
        }
    }


    close(sockfd);
    close(fd);
    printf("# of printable characters: %u\n",readable);
    exit(0);
}