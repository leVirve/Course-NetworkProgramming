#include "tcp_client.h"

void* p2p_send(void* arg)
{
    int fd = *((int*) arg);
    char send[MAXLINE];

    while (fgets(send, MAXLINE, stdin) != NULL) {
        write(fd, send, strlen(send));
    }

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
        printf("receive: %s", recv);
    }
    return NULL;
}

void* p2p_chat(void* chatfd)
{
    pthread_t stid, rtid;
    pthread_create(&stid, NULL, p2p_send, chatfd);
    pthread_create(&rtid, NULL, p2p_recv, chatfd);
    pthread_join(stid, NULL);
    pthread_join(rtid, NULL);
    // free(chatfd);
    return NULL;
}
