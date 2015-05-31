#include "tcp_client.h"

int sockfd;
FILE* fp;

int main(int argc, char** argv)
{
    sockfd = tcp_connect(argv[1], argv[2]);
    tcp_client(stdin, sockfd);
    return 0;
}
