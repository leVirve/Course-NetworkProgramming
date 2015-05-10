#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <map>
#include <set>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>

#include "../const.h"
#define MAXLINE 4096

using namespace std;

struct Article {
    string title;
    string author;
    string content;
    int readcount;
};

bool volatile keepRunning = true;
map<string, string> users;
set<string> online_users;
map<string, sockaddr_in> usersmap;
vector<Article> articles;

void setup_udpsock(int, int&);
void response(int, struct sockaddr *, socklen_t);
void recv_article(string, string, int sockfd, struct sockaddr *, socklen_t, char*);
void dump();
void startup();

bool new_account(string&, string&, char*);
void del_account(string&, string&, char*);
void login(string&, string&, char*, struct sockaddr_in);
void logout(string&, char*);

void sig_dump(int n)
{
    keepRunning = false;
    DEBUG("-----Dumping------: %d\n", n);
    dump();
    printf("exit!\n");
    exit(0);
}

int main(int argc, char **argv)
{
    int udpfd;
    signal(SIGINT, sig_dump);
    setup_udpsock(atoi(argv[1]), udpfd);
    startup();

    struct sockaddr_in cliaddr;
    int maxfd = udpfd + 1;

    char line[MAXLINE];

    fd_set readset;
    FD_ZERO(&readset);
    while(keepRunning) {
        FD_SET(udpfd, &readset);
        if (select(maxfd, &readset, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            else printf("select error");
        } else if (FD_ISSET(udpfd, &readset)) {
            response(udpfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
        }
    }

    return 0;
}

void startup()
{
    users.clear();
    char acc[MAXLINE], pwd[MAXLINE];
    FILE* fp = fopen("users", "r");
    while (fscanf(fp, "%s %s\n", acc, pwd) != EOF) {
        users.insert({acc, pwd});
    }

    // Anonymous, haha
    users.insert({ANONYMOUS, ANONYMOUS});
}

void dump()
{
    FILE* fp = fopen("users", "w");
    for (auto &user : users) {
        fprintf(fp, "%s %s\n", user.first.c_str(), user.second.c_str());
    }
    fclose(fp);
}

void response(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
    socklen_t len = clilen;
    char mesg[MAXLINE];
    char buffer1[MAX_DATA], buffer2[MAX_DATA], buffer3[MAX_DATA];
    char instr[MAXLINE];

    int n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
    // !!!!!!!!! To-DO : diff strtok split
    int args = sscanf(mesg, "%s %s %s %s\n", buffer1, instr, buffer2, buffer3);
    struct sockaddr_in client = *(struct sockaddr_in *)pcliaddr;
    printf("from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    DEBUG("User: %s - %c\n", buffer1, instr[0]);

    switch(instr[0]) {
    case 'n':
        sprintf(mesg, "%s", "");
        break;
    case 'R': {
        if (args == 4) {
            string account = buffer2, password = buffer3;
            if (new_account(account, password, mesg))
                login(account, password, mesg + strlen(mesg), client);
        } else printf("%s: %s %s\n", buffer1, buffer2, buffer3);
        break;
    }
    case 'L': {
        if (args == 4) {
            string account = buffer2, password = buffer3;
            login(account, password, mesg, client);
        }
        break;
    }
    case 'E': {
        if (args == 2) {
            string account = buffer1;
            logout(account, mesg);
        }
        break;
    }
    case 'D': {
        if (args == 2) {
            string cur_user = buffer1, account = buffer2;
            del_account(account, cur_user, mesg);
        }
        break;
    }
    case 'U': {
        int offset = 0;
        for (string user : online_users) {
            if (offset >= THRESHOLD) { // send & refill
                mesg[strlen(mesg)] = '@';
                sendto(sockfd, mesg, strlen(mesg), 0, pcliaddr, len);
                bzero(mesg, sizeof(mesg));
                offset = 0;
            }
            sprintf(mesg + offset, "%s\n", user.c_str());
            offset += (user.size() + 1);
        }
        break;
    }
    case 'Y': {
        if (args == 3) {
            string name = buffer1, content = buffer2;
            sprintf(mesg, "[Broadcast] %s: %s\n", name.c_str(), content.c_str());
            for (auto c : usersmap) {
                sendto(sockfd, mesg, sizeof(mesg), 0, (struct sockaddr*) &c.second, len);
                DEBUG("%s:%d\n", inet_ntoa(c.second.sin_addr), ntohs(c.second.sin_port));
            }
        }
        break;
    }
    case 'T': {
        if (args == 4) {
            string name = buffer1, targ = buffer2, content = buffer3;
            if (online_users.count(targ)) {
                auto t = usersmap.find(targ);
                sprintf(mesg, "[Private] %s: %s\n", name.c_str(), content.c_str());
                sendto(sockfd, mesg, sizeof(mesg), 0, (struct sockaddr*) &t->second, len);
            } else sprintf(mesg, "User is not online\n");
        }
    }
    case 'A': {
            sscanf(mesg, "%*s %*s %[^\n]%[^$]", buffer2, buffer3);
            string name = buffer1, title = buffer2, content = buffer3;
            Article article = {title, name, content, 0};
            articles.push_back(article);

            for (auto a : articles) {
                printf("title:%s\nauthor:%s\ncontent:%s\n", a.title.c_str(), a.author.c_str(), a.content.c_str());
            }
        break;
    }
    default:
        break;
    }

    sendto(sockfd, mesg, strlen(mesg), 0, pcliaddr, len);
    bzero(mesg, sizeof(mesg));

}

void recv_article(string title, string content, int sockfd, struct sockaddr *pcliaddr, socklen_t len, char* mesg)
{
    int n;
    char buff[MAXLINE];
    FILE* fp = fopen("Test1", "w");
    fprintf(fp, "%s\n%s\n", title.c_str(), content.c_str());
    // while ((n = recvfrom(sockfd, buff, MAXLINE, 0, pcliaddr, &len)) > 0) {
        // if (string(buff).find("END") == string::npos) break;
    // }
    fclose(fp);
}

bool new_account(string& account, string& password, char* mesg)
{
    if (users.count(account)) {
        sprintf(mesg, "Account has been used\n");
        return false;
    } else {
        users.insert({account, password});
        sprintf(mesg, "Register successfully\n");
        DEBUG("Register %s %s\n", account.c_str(), users.find(account)->second.c_str());
    }
    return true;
}

void login(string& account, string& password, char* mesg, struct sockaddr_in client)
{
    if (!users.count(account)) sprintf(mesg, "Account not existed!\n");
    else {
        if (password != users.find(account)->second) sprintf(mesg, "Password error\n");
        else {
            online_users.insert(account);
            usersmap.insert({account, client});
            sprintf(mesg, "Login successfully\n");
        }
    }
}

void logout(string& account, char* mesg)
{
    if (online_users.find(account) == online_users.end()) sprintf(mesg, "You've logged out\n");
    else {
        online_users.erase(account);
        usersmap.erase(account);
        sprintf(mesg, "Logout successfully\n");
    }
}

void del_account(string& account, string& cur_user, char* mesg)
{
    DEBUG("del: cur=%s targ=%s\n", cur_user.c_str(), account.c_str());
    if (cur_user != account) {
        sprintf(mesg, "Permission denied!\n");
        return;
    }
    logout(account, mesg);
    if (!users.count(account)) sprintf(mesg, "Account not existed!\n");
    else {
        users.erase(account);
        online_users.erase(account);
        sprintf(mesg, "Account is deleted!\n");
    }
}

void setup_udpsock(int port, int& sockfd)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family         = AF_INET;
    servaddr.sin_addr.s_addr    = htonl(INADDR_ANY);
    servaddr.sin_port           = htons(port);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
}
