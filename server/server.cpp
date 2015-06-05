#include "tcp_server.h"

void service(int sockfd)
{
    ssize_t n;
    char mesg[MAXLINE];
    char user[MAXLINE], instr;
again:
    while((n = read(sockfd, mesg, MAXLINE)) > 0) {
        mesg[n] = '\0';
        sscanf(mesg, "%s %c", user, &instr);
        DEBUG("%s-%c\n", user, instr);

        switch(instr) {
        case 'L': case 'R': case 'E': case 'X': case 'f':
            account_processing(sockfd, mesg);
            break;
        case 'I': case 'F': case 'u':
            list_infomation(mesg);
            break;
        case 'T': case 'Y':
            p2p_chat_system(sockfd, mesg);
            break;
        case 'D': case 'U':
            DEBUG("What??\n");
            p2p_file_system(sockfd, mesg);
            break;
        default:
            break;
        }

        write(sockfd, mesg, MAXLINE);
        bzero(mesg, MAXLINE);
    }
    if (n < 0 && errno == EINTR) goto again;
    else if (n < 0) printf("str_echo: read error");
}

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

    for (;;) {
        len = addrlen;
        connfdp = (int*) malloc(sizeof(int));
        *connfdp = accept(listenfd, clientaddr, &len);

        DEBUG("%d %s\n", *connfdp, get_addr(*(struct sockaddr_in*)clientaddr).c_str());

        pthread_create(&tid, NULL, &serve, (void*) connfdp);
    }

    return 0;
}

