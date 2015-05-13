#include "function.h"
using namespace std;

int sockfd;
socklen_t len;

void response();

int main(int argc, char **argv)
{
    setup_udpsock(atoi(argv[1]));
    startup();
    fd_set readset;
    FD_ZERO(&readset);
    int maxfd = sockfd + 1;

    while(keepRunning) {
        FD_SET(sockfd, &readset);
        if (select(maxfd, &readset, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            else printf("select error");
        } else if (FD_ISSET(sockfd, &readset)) {
            response();
        }
    }
    return 0;
}

void response()
{
    bool resp = true;
    struct sockaddr_in client;
    char mesg[MAX_DATA], buffer1[MAX_DATA], instr[MAXLINE];
    int n = recvfrom(sockfd, mesg, MAX_DATA, 0, (sockaddr*) &client, &len);
    sscanf(mesg, "%s %s", buffer1, instr);
    printf("User: %s- %s from %s:%d\n",
        buffer1, instr, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    switch(instr[0]) {
    case 'n': sprintf(mesg, "%s", ""); break;
    case 'R': new_account(mesg); break;
    case 'L': login(mesg, client); break;
    case 'E': logout(mesg); break;
    case 'C': del_account(mesg); break;
    case 'I': list_users(mesg); break;
    case 'Y': broadcast(mesg, sockfd); break;
    case 'T': direct_mesg(mesg, sockfd); break;
    case 'A': new_article(mesg, client); break;
    case 'S': list_article(mesg); break;
    case 'B': browse_article(mesg); break;
    case 'P': push_article(mesg, client); break;
    case 'X': del_article(mesg); break;
    case 'b': manage_blacklist(mesg); break;
    case 'D': send_file(mesg, sockfd, client, len); break;
    case 'U': recv_file(mesg, client); resp = false; break;
    default: break;
    }

    if(resp) sendto(sockfd, mesg, strlen(mesg), 0, (sockaddr*) &client, len);
    bzero(mesg, sizeof(mesg));
}
