#include "tcp_client.h"
using namespace std;

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

void exit_err(string str)
{
    perror(str.c_str());
    exit(1);
}

void request_processing(char* send)
{
    char* tmp = strdup(send);
    snprintf(send, MAXLINE, "%s %s", "user", tmp);
    free(tmp);
}
