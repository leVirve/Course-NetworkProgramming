#include "tcp_client.h"
using namespace std;

void tcp_client(FILE* fp_arg, int sockfd_arg)
{
    ssize_t n;
    char recv[MAXLINE];
    pthread_t tid;

    sockfd = sockfd_arg;
    fp = fp_arg;

    pthread_create(&tid, NULL, request, NULL);

    while ((n = read(sockfd, recv, MAXLINE)) > 0) {
        recv[n] = '\0';
        printf("%s\n", recv);
    }
}

void* request(void* arg)
{
    char send[MAXLINE];

    while (fgets(send, MAXLINE, fp) != NULL) {
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
