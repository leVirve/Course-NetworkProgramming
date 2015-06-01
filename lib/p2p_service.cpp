#include "tcp_lib.h"

void* p2p_send(void* arg)
{
    int fd = *((int*) arg);
    char send[MAXLINE];
    while (fgets(send, MAXLINE, stdin) != NULL)
        write(fd, send, strlen(send));
    shutdown(fd, SHUT_WR);
    return NULL;
}

void* p2p_recv(void* arg)
{
    int fd = *((int*) arg);
    ssize_t n;
    char recv[MAXLINE];
    while((n = read(fd, recv, MAXLINE)) > 0) {
        recv[n] = '\0';
        printf("%s\n", recv);
    }
    return NULL;
}

void p2p_chat(int chatfd)
{
    pthread_t stid, rtid;
    pthread_create(&stid, NULL, p2p_send, (void*) &chatfd);
    pthread_create(&rtid, NULL, p2p_send, (void*) &chatfd);
}
