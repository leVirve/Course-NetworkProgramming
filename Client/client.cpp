#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>
#include <sstream>

#include "../const.h"

using namespace std;

int sockfd;
struct sockaddr_in servaddr;
socklen_t servlen = sizeof(servaddr);

string username;
stringstream article;
int articleNumber;
bool isLogin = false, article_mode = false, read_mode = false;
bool keepRunning = true;

void init(char*, char*, int);
void sig_handle(int);
void data_send(char*);
void data_recv(char*);
void download(char*);
void upload(char*);

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
    system("mkdir -p Download");

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

    read_mode = false;
    if (CREATE_ARTICLE) {
        system("clear");
        // article clean ?????
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
    } else if (READ_ARTICLE) {
        read_mode = true;
        sscanf(buffer, "%*s %*s %d\n", &articleNumber);
    } else if (RESPONSE_ARTICLE || MANAGE_ARTICLE) {
        char instr[MAXLINE], content[MAXLINE];
        sscanf(sendline, "%s %[^\n]", instr, content);
        sprintf(buffer, "%s %s %d %s",
            username.empty() ? ANONYMOUS : username.c_str(),
            instr,
            articleNumber,
            content);
    } else if (sendline[0] == 'U') {
        char filename[MAXLINE];
        sscanf(sendline, "%*s %[^\n]", filename);
        upload(filename);
        return;
    }
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*) &servaddr, servlen);
    strcpy(sendline, buffer);
}

void data_recv(char* sendline)
{
    char buffer[MAXLINE];
    bool append;
    stringstream response;
    bool clean = true;

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
        } else if (resp.find("Broadcast") != string::npos) {
            printf("\x1B[%d;%df", 5, 2);
            fflush(stdout);
            printf("%s\n", response.str().c_str());
            printf("\x1B[%d;%df", 5, 2);
            return;
            clean = false;
        } else if (resp.find("U") != string::npos) {
            // return;
        }

        if (!append) break;
    }

    if (clean) system("clear");
    if (isLogin) printf(choice_info, username.c_str());
    else printf("%s\n", welcome_info);
    printf("%s\n", response.str().c_str());

    if (read_mode) printf("%s", reading_info);
    if (!article_mode) printf("> ");
    fflush(stdout);
}

void upload(char* filename)
{
    int i = 0;
    char sendline[MAX_DATA], raw[RAW_DATA];
    FILE* f = fopen(filename, "rb");
    struct stat fst;
    size_t filesize;
    fstat(fileno(f), &fst);
    filesize = fst.st_size;

    int sendout = 0, n, s;

    while ((n = fread(raw, sizeof(char), RAW_DATA, f)) > 0) {
        snprintf(sendline, MAX_DATA,
            "%s U %s %zd %d ",
            username.c_str(), filename, filesize, i++);
        memcpy(sendline + strlen(sendline), raw, RAW_DATA);

        s = sendto(sockfd, sendline, MAX_DATA, 0, (struct sockaddr*) &servaddr, servlen);
        sleep(1);
        sendout += n;
        DEBUG("#%d| read: %d, send: %d\n", i-1, n, s);
    }
    fclose(f);
    DEBUG("%d\n", sendout);
}

void download(char* filename)
{
    int expect_number = 0, cur_number;
    size_t n, filesize, rcev_byte = 0;
    char buffer[MAX_DATA], raw[RAW_DATA], path[MAXLINE];
    snprintf(path, MAXLINE, "./Download/%s", filename);
    FILE* f = fopen(path, "wb");

    while ((n = recvfrom(sockfd, buffer, MAX_DATA, 0, NULL, NULL)) > 0) {
        sscanf(buffer, "%*s %*s %zd %d %s", &filesize, &cur_number, raw);
        if (cur_number != expect_number++) {
            printf("Error: download\n");
            break;
        }
        fwrite(buffer, sizeof(char), n, f);
        rcev_byte += n;
        if (rcev_byte >= filesize) break;
    }
    fclose(f);
    DEBUG("Download finish!\n");
}
