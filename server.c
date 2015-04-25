#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "const.h"

#define OPEN_MAX 1024
#define LISTENQ 1024
#define INFTIM -1

int exit_err(char* str)
{
    perror(str);
    exit(1);
}

void initial()
{
    system("mkdir Upload");
}

void print(int sockfd, char* str)
{
    struct sockaddr client;
    socklen_t clilen = sizeof(client);
    getpeername(sockfd, &client, &clilen);
    struct sockaddr_in* clientaddr = (struct sockaddr_in*) &client;
    printf("%s:%d %s\n", inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port), str);
}

void serve_for(int sockfd, char *line)
{
    pid_t pid;
    int state;

    if((pid = fork()) == 0) {
        dup2(sockfd, 2);
        dup2(sockfd, 1);
        dup2(sockfd, 0);
        close(sockfd);

        char instruction = line[0];
        char* buffer = line+2;
        switch(instruction) {
            case 'L': execlp("ls", "ls", "-al", NULL); break;
            case 'C': chdir(buffer); puts("Some"); exit(0); break;
                      //execl("/bin/sh", "-c", "cd", buffer, NULL); break;
            default: execlp("echo", "echo", "No instruction", NULL); break;
        }
    } else {
        wait(NULL);
    }
}

int main(int argc, char **argv)
{
    int maxfd, listenfd, connfd, sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    struct pollfd client[OPEN_MAX];
    socklen_t clilen;
    ssize_t n;
    int i, nready;
    char line[MAXLINE + 1];

    initial();

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        exit_err("Socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        exit_err("Binding error");

    if(listen(listenfd, LISTENQ) == -1)
        exit_err("Listen error");

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for(i = 1; i < OPEN_MAX; ++i) client[i].fd = -1;
    maxfd = 0;


    for(;;) {
        nready = poll(client, maxfd+1, INFTIM);

        if (client[0].revents & POLLRDNORM) {
            clilen = sizeof(clientaddr);
            connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clilen);
            print(connfd, "connected");

            for (i = 0; i < OPEN_MAX; ++i)
                if (client[i].fd < 0) {
                    client[i].fd = connfd;
                    break;
                }
            if (i == OPEN_MAX) printf("too many clients");

            client[i].events = POLLRDNORM;
            maxfd = i > maxfd ? i : maxfd;
            if (--nready <= 0) continue;
        }

        for (i = 1; i <= maxfd; ++i) {
            if ((sockfd = client[i].fd) < 0) continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) {
                if ((n = read(sockfd, line, MAXLINE)) < 0) {
                    if(errno == ECONNRESET) {
                        print(sockfd, "terminated");
                        close(sockfd);
                        client[i].fd = -1;
                    } else puts("read error");
                } else if (n == 0) {
                    print(sockfd, "terminated!");
                    close(sockfd);
                    client[i].fd = -1;
                } else {
                    
                    line[n-1] = '\0';
                    printf("[DEBUG] recv %s\n", line);
                    
                    serve_for(sockfd, line);
                }
                if (--nready <= 0) break;
            }
        }
    }

    return 0;
}
;
