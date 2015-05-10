#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <sstream>

#include "../const.h"

#define MAXLINE 4096

using namespace std;

int sockfd;
struct sockaddr_in servaddr;
socklen_t servlen = sizeof(servaddr);

string username;
stringstream article;
bool isLogin = false;
bool keepRunning = true, article_mode = false;

void init(char*, char*, int);
void sig_handle(int);
void data_send(char*);
void data_recv(char*);

int main(int argc, char **argv)
{
    if (argc != 3) exit(1);
    char sendline[MAXLINE];
    init(sendline, argv[1], atoi(argv[2]));

    int maxfd = sockfd + 1;
    fd_set readset;

    FD_ZERO(&readset);
    while(keepRunning) {
        if (!article_mode) FD_SET(fileno(stdin), &readset);
        FD_SET(sockfd, &readset);

        if (select(maxfd, &readset, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            else printf("select error");
        } else if (FD_ISSET(fileno(stdin), &readset)) {
            data_send(sendline);
        } else if (FD_ISSET(sockfd, &readset)) {
            data_recv(sendline);
        }
    }
    exit(0);
}

void init(char* sendline, char* servip, int port)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    inet_pton(AF_INET, servip, &servaddr.sin_addr);

    signal(SIGINT, sig_handle);

    sprintf(sendline, "%s new\n", ANONYMOUS);
    sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *) &servaddr, servlen);
}

void sig_handle(int n)
{
    if (article_mode) {
        article_mode = false;
    } else exit(n);
}

void data_send(char* sendline)
{
    char buffer[MAX_DATA], tmp[MAXLINE];
    bzero(sendline, sizeof(sendline));
    fgets(sendline, MAXLINE, stdin);
    sprintf(buffer, "%s %s", username.empty() ? ANONYMOUS : username.c_str(), sendline);
    if (CREATE_ARTICLE) {
        system("clear");
        article_mode = true;
        article.str("");
        article.clear();
        printf("Enter article title: ");
        fflush(stdout);

        fgets(tmp, MAXLINE, stdin);
        article << buffer << tmp;
        while (article_mode) {
            fgets(tmp, MAXLINE, stdin);
            article << tmp;
        }
        article.read(buffer, MAX_DATA);
        printf("Send article!\n");
    }
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*) &servaddr, servlen);
    strcpy(sendline, buffer);
}

void data_recv(char* sendline)
{
    char buffer[MAXLINE];
    bool append;
    stringstream response;

    while(1) {
        int n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
        if((append = buffer[n - 1] == '@')) n--;
        buffer[n] = '\0';
        response << buffer;
        string resp = buffer;
        if (!isLogin && resp.find(login_ok) != string::npos) {
            sscanf(sendline, "%*s %*c %s %*s\n", buffer);
            username = buffer;
            isLogin = true;
        } else if (isLogin && resp.find(logout_ok) != string::npos) {
            username = "";
            isLogin = false;
        }
        if (!append) break;
    }

    system("clear");
    if (isLogin) printf(choice_info, username.c_str());
    else printf("%s\n", welcome_info);
    printf("%s\n", response.str().c_str());

    if (!article_mode) printf("> ");
    fflush(stdout);
}

