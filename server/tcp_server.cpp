#include "tcp_server.h"
using namespace std;

map<std::string, std::string> accounts;
map<std::string, Clinet> online_users;

int exit_err(string str)
{
    perror(str.c_str());
    exit(1);
}

string get_addr(struct sockaddr_in client)
{
    char str[MAXLINE];
    sprintf(str, "%s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    return string(str);
}

struct sockaddr_in get_clint(int sockfd)
{
    struct sockaddr client;
    socklen_t clilen = sizeof(client);
    getpeername(sockfd, &client, &clilen);
    struct sockaddr_in* clientaddr = (struct sockaddr_in*) &client;
    return *clientaddr;
}

int tcp_listen(const char* host, const char* service, socklen_t* addrlen)
{
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res, *ret;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, service, &hints, &res)) != 0)
        exit_err("tcp_listen error");
    ret = res;
    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0) continue;
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) break;
        close(listenfd);
    } while ((res = res->ai_next) != NULL);
    if (res == NULL) exit_err("tcp_listen error");

    listen(listenfd, LISTENQ);

    if (addrlen) *addrlen = res->ai_addrlen;
    freeaddrinfo(ret);

    return listenfd;
}

void service(int sockfd)
{
    ssize_t n;
    char mesg[MAXLINE];
    char user[MAXLINE], instr[MAXLINE];

again:
    while((n = read(sockfd, mesg, MAXLINE)) > 0) {
        mesg[n] = '\0';
        sscanf(mesg, "%s %s", user, instr);
        DEBUG("%s-%c\n", user, instr[0]);
        switch(instr[0]) {
            case 'L': case 'R': case 'E': case 'X':
                account_processing(sockfd, mesg);
                break;
            case 'I': case 'F':
                list_infomation(sockfd, mesg);
                break;
            case 'T': case 'Y':
                //p2p_chat_system(mesg);
                break;
            case 'D': case 'U':
                //file_proccesing(mesg);
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

bool is_existed(const string account)
{
    return accounts.count(account);
}

bool is_matched(const string account, const string password)
{
    return password == accounts.find(account)->second;
}

bool is_login(const string account)
{
    return online_users.find(account) != online_users.end();
}

void account_processing(int fd, char* mesg)
{
    char buff1[MAXLINE], buff2[MAXLINE], instr;
    sscanf(mesg, "%*s %c %s %s", &instr, buff1, buff2);
    string account = buff1, password = buff2;

    switch(instr) {
        case 'R':
            if (is_existed(account)) {
                sprintf(mesg, "Account has been used\n");
            } else {
                accounts.insert({account, password});
                sprintf(mesg, "Register successfully!\n");
            }
            break;
        case 'L':
            if (!is_existed(account)) {
                sprintf(mesg, "Account not existed...\n");
            } else {
                if (!is_matched(account, password)) {
                    sprintf(mesg, "Wrong password\n");
                } else {
                    Clinet client;
                    client.addr = get_clint(fd);
                    client.files = {};
                    online_users.insert({account, client});
                    sprintf(mesg, "Login successfully !\n");
                }
            }
            break;
        case 'E':
            if (!is_login(account)) {
                sprintf(mesg, "You're not logged in\n");
            } else {
                online_users.erase(account);
                sprintf(mesg, "Logout successfully\n");
            }
            break;
        case 'X':
            if (!is_existed(account)) {
                sprintf(mesg, "Account not existed...\n");
            } else {
                if (is_login(account)) {
                    online_users.erase(account);
                }
                accounts.erase(account);
                sprintf(mesg, "Account deleted\n");
            }
            break;
        default: break;
    }
}

void list_infomation(int fd, char* mesg)
{
    char instr;
    stringstream ss;
    sscanf(mesg, "%*s %c", &instr);
    switch(instr) {
        case 'I':
            for (auto user : online_users)
                ss << user.first << " : " << get_addr(user.second.addr) << endl;
            break;
        case 'F':
            for (auto user : online_users) {
                ss << user.first << " : ";
                for (auto file : user.second.files)
                    ss << file << ", ";
                ss << " ;" << endl;
            }
            break;
        default: break;
    }
    ss.read(mesg, MAXLINE);
    ss.clear();
    ss.str("");
}

void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];
again:
    while((n = read(sockfd, buf, MAXLINE)) > 0)
        write(sockfd, buf, n);
    if (n < 0 && errno == EINTR) /* interrupted by a signal before any data was read*/
        goto again; //ignore EINTR
    else if (n < 0)
        printf("str_echo: read error");
}
