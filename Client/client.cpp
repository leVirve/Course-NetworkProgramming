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

bool acked = false;

void init(char*, char*, int);
void send_datagram(char*);
bool recv_datagram(char*);
void sig_handle(int);
void data_send(char*);
void data_recv(char*);
void download(char*);
void upload(char*);

int main(int argc, char **argv)
{
    if (argc != 3) exit(1);
    char sendline[MAX_DATA];
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
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);
    inet_pton(AF_INET, servip, &servaddr.sin_addr);

    signal(SIGINT, sig_handle);
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sprintf(sendline, "%s new\n", ANONYMOUS);
    send_datagram(sendline);
}

void sig_handle(int n)
{
    if (article_mode) {
        article_mode = false;
    } else exit(n);
}

// Ensure data sent
void send_datagram(char* raw)
{
    acked = false;
    char buff[MAX_DATA];
    int times = 0;
    do {
        times++;
        bzero(buff, sizeof(buff));
        sendto(sockfd, raw, MAX_DATA, 0, (struct sockaddr*) &servaddr, servlen);
        recvfrom(sockfd, buff, MAX_DATA, 0, (sockaddr*) &servaddr, &servlen);
    } while (string(raw) != string(buff));
    DEBUG("Sent after %d try !\n", times);
}

// Ensure data sent
bool recv_datagram(char* raw)
{
    recvfrom(sockfd, raw, MAX_DATA, 0, (sockaddr*) &servaddr, &servlen);
    // ACK
    sendto(sockfd, raw, MAX_DATA, 0, (struct sockaddr*) &servaddr, servlen);
    return true;
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
        bzero(buffer, sizeof(buffer));
        article.read(buffer, MAX_DATA); // ?
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
    } else if (sendline[0] == 'D') {
        download(buffer);
        return;
    }

    send_datagram(buffer);
    strcpy(sendline, buffer);
}

void data_recv(char* sendline)
{
    char buffer[MAX_DATA];
    stringstream response;
    bool clean = true;

    // int n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
    recv_datagram(buffer);

    string resp = buffer;
    if (!isLogin && resp.find(login_ok) != string::npos) {
        sscanf(sendline, "%*s %*c %s %*s\n", buffer);
        username = buffer;
        isLogin = true;
    } else if (isLogin && resp.find(logout_ok) != string::npos) {
        username = "";
        isLogin = false;
    }


    if (clean) system("clear");
    if (isLogin) printf(choice_info, username.c_str());
    else printf("%s\n", welcome_info);
    printf("%s\n", resp.c_str());

    if (read_mode) printf("%s", reading_info);
    if (!article_mode) printf("> ");
    fflush(stdout);

    acked = true;
}

void upload(char* filename)
{
    int i = 0;
    int sendout = 0, n, s;
    char sendline[MAX_DATA], raw[RAW_DATA];
    FILE* f = fopen(filename, "rb");
    struct stat fst;
    size_t filesize;
    fstat(fileno(f), &fst);
    filesize = fst.st_size;

    while ((n = fread(raw, sizeof(char), RAW_DATA, f)) > 0) {
        snprintf(sendline, MAX_DATA,
            "%s U %d %s %zd %d ",
            username.c_str(), articleNumber, filename, filesize, i++);
        memcpy(sendline + strlen(sendline), raw, RAW_DATA);
        s = sendto(sockfd, sendline, MAX_DATA, 0, (struct sockaddr*) &servaddr, servlen);
        sleep(1);
        sendout += n;
        // DEBUG("#%d| read: %d, send: %d\n", i-1, n, s);
    }
    fclose(f);
    DEBUG("%d\n", sendout);
    // recv_datagram(sendline);
    printf("> ");
}

void download(char* mesg)
{
    char filename[MAXLINE];
    sscanf(mesg, "%*s %*s %[^\n]", filename);
    sendto(sockfd, mesg, strlen(mesg), 0, (struct sockaddr*) &servaddr, servlen);

    int expect_number = 0, cur_number;
    size_t n, filesize, rcev_byte = 0;
    char buffer[MAX_DATA], tmp[MAXLINE], username[MAXLINE];
    char path[MAXLINE];
    snprintf(path, MAXLINE, "./Download/%s", filename);
    FILE* f = fopen(path, "ab");

    while ((n = recvfrom(sockfd, buffer, MAX_DATA, 0, NULL, NULL)) > 0) {
        int write_size = RAW_DATA;
        char raw[RAW_DATA];
        sscanf(buffer, "%s %*s %zd %d ", username, &filesize, &cur_number);
        sprintf(tmp, "%s %s %zd %d ", username, filename, filesize, cur_number);
        memcpy(raw, buffer + strlen(tmp), RAW_DATA);
        if (cur_number != expect_number++) {
            printf("Error: download\n");
            break;
        }
        if (rcev_byte + RAW_DATA >= filesize) write_size = filesize - rcev_byte;
        rcev_byte += fwrite(raw, sizeof(char), write_size, f);
        if (rcev_byte >= filesize) break;
    }
    fclose(f);
    printf("> ");
    // DEBUG("Download finish!\n");
}
