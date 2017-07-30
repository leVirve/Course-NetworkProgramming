#include "tcp_client.h"

int sockfd;
bool stdin_p2p = false;
FILE* fp;

pthread_mutex_t std_input;
pthread_mutex_t std_input_s;

void client(char* host, char* port)
{
    ssize_t n;
    char send[MAXLINE], recv[MAXLINE];
    pthread_t tid, sid;

    sockfd = tcp_connect(host, port);

    // New thread for input handle
    pthread_create(&tid, NULL, user_input, (void*) send);

    // New thread for p2p_server
    pthread_create(&sid, NULL, tcp_p2p_server, NULL);

    while((n = read(sockfd, recv, MAXLINE)) > 0) {
        if (is_contained(recv, "Login")) tcp_p2p_init(recv);
        if (is_contained(recv, "@")) update_peers(recv, '\n');
        if (is_contained(recv, "new")) {
            // tcp_p2p_client(recv);
        } else if (is_contained(recv, "download")) {
            p2p_download(recv);
        } else {
            recv[n] = '\0';
            printf("%s\n", recv);
        }
    }
    pthread_join(sid, NULL);
    pthread_join(tid, NULL);
}

int main(int argc, char** argv)
{
    pthread_mutex_init(&std_input, NULL);
    pthread_mutex_init(&std_input_s, NULL);
    client(argv[1], argv[2]);
    return 0;
}
