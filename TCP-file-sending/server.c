#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "const.h"

#define OPEN_MAX 1024
#define LISTENQ 1024

int saved_stdout;

char origin_path[PATH_LEN];
char path[OPEN_MAX][PATH_LEN];
int fdmap[OPEN_MAX], busyio[OPEN_MAX];

int exit_err(char* str)
{
    perror(str);
    exit(1);
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WCONTINUED)) > 0);
    return;
}

void initial()
{
    saved_stdout = dup(1);
    system("mkdir -p Upload");
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
    dup2(sockfd, 1);

    char instruction = line[0];
    char filename[255];
    char* buffer = line+2;
    size_t n, c, recv_bytes, filesize;
    FILE* f;
    struct stat fst;

    switch(instruction) {
        case 'L':
            system("ls -al");
            break;
        case 'C':
            chdir(buffer);
            system("pwd");
            break;
        case 'U':
                  dup2(saved_stdout, 1);
                  f = fopen(buffer, "wb");

                  bzero(buffer, MAXLINE);
                  recv_bytes = 0;
                  recv(sockfd, buffer, MAXLINE, 0);
                  sscanf(buffer, "%zd", &filesize);

                  while ((n = recv(sockfd, buffer, MAXLINE, 0)) > 0) {
                      if ((c = fwrite(buffer, sizeof(char), n, f)) < n) {
                          printf("write error c=%zd < n=%zd\n", c, n);
                          exit(1);
                      }
                      recv_bytes += n;
                      bzero(buffer, MAXLINE);

                      if (recv_bytes >= filesize) break;
                  }
                  fclose(f);

                  sprintf(buffer, "Uploaded sucess!\n");
                  write(sockfd, buffer, MAXLINE);
                  break;
        case 'D':
                  dup2(saved_stdout, 1);
                  f = fopen(buffer, "rb");
                  if (f == NULL) {
                      printf("[DEBUG] No such file: %s\n", filename);
                      return;
                  }
                  fstat(fileno(f), &fst);
                  filesize = fst.st_size;

                  bzero(buffer, MAXLINE);
                  sprintf(buffer, "%zd", filesize);
                  send(sockfd, buffer, MAXLINE, 0);

                  while((n = fread(buffer, sizeof(char), MAXLINE, f)) > 0) {
                      if (send(sockfd, buffer, n, 0) < 0) exit_err("send error");
                      bzero(buffer, MAXLINE);
                  }
                  fclose(f);
                  break;
        case 'E': break;
        default: execlp("echo", "echo", "No instruction", NULL); break;
    }
    dup2(saved_stdout, 1);
}

int main(int argc, char **argv)
{
    int listenfd, connfd, sockfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    socklen_t clilen;
    ssize_t n;
    pid_t pid;
    int i;
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

    signal (SIGCHLD, sig_chld);

    for(;;) {
        clilen = sizeof(clientaddr);
        connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clilen);
        print(connfd, "connected");

        if ((pid = fork()) == 0) {
            close(listenfd);
            while(1) {
                if ((n = read(connfd, line, MAXLINE)) > 0) {
                    line[n-1] = '\0';
                    serve_for(connfd, line);
                } else break;
            }
            print(connfd, "disconnected");
            close(connfd);
            exit(0);
        } else {
            close(connfd);
        }
    }

    return 0;
}

