#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "const.h"

int exit_err(char* str);
void service(int sockfd);
void send_file(int sockfd, char* file);
void recv_file(int sockfd, char* file);

int main (int argc, char **argv)
{
    int sockfd, n, i;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;

    if(argc != 3) exit_err("usage: a.out <IPaddress> <Port>");
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) exit_err("socket error");

    system("mkdir -p Download");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        exit_err("inet_ption error");
    if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
        exit_err("connect error");

    service(sockfd);

    exit(0);
}

void service(int sockfd)
{
    char send[MAXLINE + 1];
    char recv[MAXLINE + 1];
    int n;

    printf("[C]hange folder [L]ist [U]pload [D]ownload [E]xit\n");
    while(fgets(send, MAXLINE, stdin) != NULL) {
        send[strlen(send) - 1] = '\0';
        write(sockfd, send, MAXLINE);

        if (send[0] == 'U') send_file(sockfd, send+2);
        if (send[0] == 'D') recv_file(sockfd, send+2);
        else {
            if (send[0] == 'E') break;
            n = read(sockfd, recv, MAXLINE);
            recv[n] = '\0';
            printf("%s", recv);
        }
        printf("[C]hange folder [L]ist [U]pload [D]ownload [E]xit\n");
    }
    //close(sockfd);
    puts("Exit");
}

void recv_file(int sockfd, char* file)
{
    file[strlen(file)] = '\0';
    size_t n, c, recv_bytes, filesize;
    char buffer[MAXLINE];
    char filename[255];
    sprintf(filename, "./Download/%s", file); // No need
    FILE* f = fopen(filename, "wb");

    bzero(buffer, MAXLINE);
    recv_bytes = 0;
    recv(sockfd, buffer, MAXLINE, 0);
    sscanf(buffer, "%zd", &filesize);
    printf("[DEBUG] Downloading... filesize=%zd\n", filesize);

    while((n = recv(sockfd, buffer, MAXLINE, 0)) > 0) {
        if ((c = fwrite(buffer, sizeof(char), n, f)) < n) {
            printf("[DEBUG] c=%zd < n=%zd\n", c, n);
            exit_err("fwrite error");
        }
        recv_bytes += n;
        bzero(buffer, MAXLINE);

        if (recv_bytes >= filesize) break;
    }
    fclose(f);
    printf("[DEBUG] download finished (recv: %zd)\n", recv_bytes);
    puts("Download sucess!");
}

void send_file(int sockfd, char* file)
{
    size_t n, filesize;
    char buffer[MAXLINE];
    struct stat fst;

    file[strlen(file)] = '\0';
    FILE* f = fopen(file, "rb");

    if (f == NULL) {
        printf("No such file: %s\n", file);
        return;
    }
    fstat(fileno(f), &fst);
    filesize = fst.st_size;
    printf("[DEBUG] sending...(size: %zd)\n", filesize);

    sprintf(buffer, "%zd", filesize);
    send(sockfd, buffer, MAXLINE, 0);

    while((n = fread(buffer, sizeof(char), MAXLINE, f)) > 0) {
        if (send(sockfd, buffer, n, 0) < 0) exit_err("send error");
        bzero(buffer, MAXLINE);
    }
    fclose(f);
    puts("[DEBUG] send finished");
}

int exit_err(char* str)
{
    perror(str);
    exit(1);
}
