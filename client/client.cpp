#include "tcp_client.h"

int sockfd;
bool stdin_p2p = false;
FILE* fp;

bool is_contained(std::string str, std::string targ)
{
    return str.find(targ) != std::string::npos;
}

void client(char* host, char* port)
{
    ssize_t n;
    char send[MAXLINE], recv[MAXLINE];
    pthread_t tid;

    sockfd = tcp_connect(host, port);
    DEBUG("client %d=%s:%s\n", sockfd, host, port);

    pthread_create(&tid, NULL, request, (void*) send);

    while((n = read(sockfd, recv, MAXLINE)) > 0) {
        recv[n] = '\0';
        printf("%s\n", recv);
        if (is_contained(recv, "new connection")) {
            stdin_p2p = true;
            tcp_p2p_server(recv);
            // pthread_create(&tid, NULL, tcp_p2p_server, (void*) recv);
        } else if (is_contained(recv, "connect")) {
            stdin_p2p = true;
            tcp_p2p_client(recv);
            // pthread_create(&tid, NULL, tcp_p2p_client, (void*) recv);
        }
    }
}

int main(int argc, char** argv)
{
    client(argv[1], argv[2]);
    return 0;
}
