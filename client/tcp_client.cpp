#include "tcp_client.h"
using namespace std;

extern bool stdin_p2p;

void* tcp_p2p_client(void* arg)
{
    char* recv = (char*) arg;
    char host[SHORTINFO];

    int p2p_servfd;

    sscanf(recv, "%*s %[^:]:%*s", host);
    DEBUG("host %s\n", host);
    p2p_servfd = tcp_connect(host, P2P_PORT);
    DEBUG("p2p_servfd %d\n", p2p_servfd);

    p2p_chat(p2p_servfd);
    return NULL;
}

void* tcp_p2p_server(void* arg)
{
    int listenfd, connfd;
    struct sockaddr p2p_client;
    socklen_t len;
    listenfd = tcp_listen(NULL, P2P_PORT, &len);
    DEBUG("listenfd %d\n", listenfd);
    connfd = accept(listenfd, &p2p_client, &len);
    DEBUG("accept %d\n", connfd);

    p2p_chat(connfd);
    return NULL;
}

void* request(void* arg)
{
    char send[MAXLINE];
    while (!stdin_p2p && fgets(send, MAXLINE, stdin) != NULL) {
        request_processing(send);
        write(sockfd, send, strlen(send));
    }
    shutdown(sockfd, SHUT_WR);

    return NULL;
}

void request_processing(char* send)
{
    char* tmp = strdup(send);
    snprintf(send, MAXLINE, "%s %s", "user", tmp);
    free(tmp);
}
