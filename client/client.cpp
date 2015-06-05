#include "tcp_client.h"

int sockfd;
bool stdin_p2p = false;
FILE* fp;

pthread_mutex_t std_input;

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

    // New thread for input handle
    pthread_create(&tid, NULL, user_input, (void*) send);

    // New thread for p2p_server
    pthread_create(&tid, NULL, tcp_p2p_server, NULL);

    while((n = read(sockfd, recv, MAXLINE)) > 0) {
        if (is_contained(recv, "Login")) tcp_p2p_init(recv);
        if (is_contained(recv, "@")) update_peers(recv, '\n');
        if (is_contained(recv, "connect")) {
            tcp_p2p_client(send, recv);
        } else {
            recv[n] = '\0';
            printf("%s\n", recv);
        }
    }
}

int main(int argc, char** argv)
{
    pthread_mutex_init(&std_input, NULL);
    client(argv[1], argv[2]);
    return 0;
}
