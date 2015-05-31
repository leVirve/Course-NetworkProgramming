#include "tcp_server.h"

static void* serve(void* arg)
{
    int connfd = *((int*) arg);
    free(arg);

    pthread_detach(pthread_self());

    service(connfd);

    close(connfd);
    return NULL;
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr* clientaddr;

    if (argc < 2) exit_err("usage: <server> [<host>] <service/port>");
    listenfd = tcp_listen(NULL, argv[1], &addrlen);
    clientaddr = (struct sockaddr*) malloc(addrlen);

    for(;;) {
        len = addrlen;
        connfdp = (int*) malloc(sizeof(int));
        *connfdp = accept(listenfd, clientaddr, &len);

        printf("%d\n", *connfdp);

        pthread_create(&tid, NULL, &serve, (void*) connfdp);
    }

    return 0;
}

