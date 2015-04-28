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

char origin_path[PATH_LEN];
char path[OPEN_MAX][PATH_LEN];
int fdmap[OPEN_MAX], busyio[OPEN_MAX];

int exit_err(char* str)
{
    perror(str);
    exit(1);
}

void initial()
{
    int i;
    system("mkdir -p Upload");
    for(i = 0; i < OPEN_MAX; ++i) busyio[i] = fdmap[i] = -1;
    getcwd(origin_path, sizeof(origin_path));
}

void print(int sockfd, char* str)
{
    struct sockaddr client;
    socklen_t clilen = sizeof(client);
    getpeername(sockfd, &client, &clilen);
    struct sockaddr_in* clientaddr = (struct sockaddr_in*) &client;
    printf("%s:%d %s\n", inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port), str);
}

int findfd(int sockfd)
{
    int i;
    for (i = 0; i < OPEN_MAX; ++i)
        if (fdmap[i] == sockfd) break;
    return i == OPEN_MAX ? -1 : i;
}

void change_folder(int sockfd, char* targ_path)
{
    int i, fd;
    
    chdir(origin_path);

    if ((fd = findfd(sockfd)) == -1) {
        for (i = 0; i < OPEN_MAX; ++i)
            if (fdmap[i] < 0) {
                fdmap[i] = sockfd;
                getcwd(path[i], sizeof(path[i]));
                fd = i;
                printf("[DEBUG] %d use fd=%d[path=%s]\n", sockfd, fd, path[fd]);
                break;
            }
    } else {
        printf("[DEBUG] restore dir to: %s\n", path[fd]);
        chdir(path[fd]);
    }
    if (targ_path[2] != '\0') {
        chdir(targ_path);
        getcwd(path[fd], sizeof(path[fd]));
    }

}

int busyIO(int sockfd)
{
    int i;
    for (i = 0; i < OPEN_MAX; ++i)
        if (busyio[i] == sockfd)
            return 1;
    return 0;
}

void manipulate_file(int sockfd, char* filename, char* mode)
{
    char file[PATH_LEN];
    char buffer[MAXLINE];
    size_t n;
    
    sprintf(file, "./Upload/%s", filename);
    printf("[DEBUG] file@ %s\n", file);
    FILE* f = fopen(file, mode);

    if (mode[0] == 'w') {
        while((n = read(sockfd, buffer, MAXLINE)) > 0) {
            printf("[DEBUG] recieve data: %s", buffer);
            //fwrite(buffer, sizeof(char), n, f);
            fprintf(f, "%s", buffer);
            bzero(buffer, MAXLINE);
        }

        sprintf(buffer, "Ok, uploaded");
        printf("[DEBUG] (send %s)\n", buffer);
        write(sockfd, buffer, strlen(buffer));
    } else {
        while((n = read(sockfd, buffer,  MAXLINE)) > 0) {
            buffer[n] = '\0';
            printf("[DEBUG] recieve data: %s", buffer);
            fwrite(buffer, 1, n, f);
        }
    }
}

void serve_for(int sockfd, char *line)
{
    pid_t pid;
    int i = 0;
    char instruction = line[0];
    char* buffer = line+2;
    
    change_folder(sockfd, buffer);
    if (instruction == 'U' || instruction == 'D') {
        for (i = 0; i < OPEN_MAX; ++i)
            if (busyio[i] < 0) {
                busyio[i] = sockfd;
                printf("[DEBUG] %d is in busy(i=%d)\n", sockfd, i);
                break;
            }
    } else if (instruction == 'C')
        // hacky clear line
        line[2] = '\0';

    if (instruction == 'U') manipulate_file(sockfd, buffer, "wb");

    if ((pid = fork()) == 0) {
        dup2(sockfd, STDOUT_FILENO);
        close(sockfd);

        switch(instruction) {
            case 'L': 
                execlp("ls", "ls", "-al", NULL); break;
            case 'C':
                execlp("pwd", "pwd", NULL); break;
            case 'U':
                // manipulate_file(sockfd, buffer, "w");
                execlp("echo", "echo", "Upload Complete!", NULL); break;
            case 'D':
                sleep(10);
                manipulate_file(sockfd, buffer, "r");
                execlp("echo", "echo", "Download Complete!", NULL); break;
            default:
                execlp("echo", "echo", "No instruction", NULL); break;
        }
        fflush(stdout);
        return;
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
    servaddr.sin_port = htons(atoi(argv[1]));

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
            if ((sockfd = client[i].fd) < 0 || busyIO(sockfd)) continue;

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
                    printf("[DEBUG] recv %s(%d)\n", line, (int)n);

                    serve_for(sockfd, line);
                }
                if (--nready <= 0) break;
            }
        }
    }

    return 0;
}
;
