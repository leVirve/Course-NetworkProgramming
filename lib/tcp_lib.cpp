#include "tcp_lib.h"

int tcp_connect(const char* host, const char* service)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ret;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, service, &hints, &res)) != 0)
        exit_err("tcp_connect error");
    ret = res;
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) continue;
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) break;
        close(sockfd);
    } while ((res = res->ai_next) != NULL);
    freeaddrinfo(ret);

    return sockfd;
}

int tcp_listen(const char* host, const char* service, socklen_t* addrlen)
{
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res, *ret;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, service, &hints, &res)) != 0)
        exit_err("tcp_listen error");
    ret = res;
    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0) continue;
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) break;
        close(listenfd);
    } while ((res = res->ai_next) != NULL);
    if (res == NULL) exit_err("tcp_listen error");

    listen(listenfd, LISTENQ);

    if (addrlen) *addrlen = res->ai_addrlen;
    freeaddrinfo(ret);

    return listenfd;
}

std::string get_addr(struct sockaddr_in client)
{
    char str[MAXLINE];
    sprintf(str, "%s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    return std::string(str);
}

struct sockaddr_in get_clint(int sockfd)
{
    struct sockaddr client;
    socklen_t clilen = sizeof(client);
    getpeername(sockfd, &client, &clilen);
    struct sockaddr_in* clientaddr = (struct sockaddr_in*) &client;
    return *clientaddr;
}

void exit_err(std::string str)
{
    perror(str.c_str());
    exit(1);
}
