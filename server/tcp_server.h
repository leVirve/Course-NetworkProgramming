#ifndef __TCP_SERVER__
#define __TCP_SERVER__

#ifndef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <map>
#include <vector>
#include <string>
#include <sstream>

#define MAXLINE 4096
#define MAXDATA 40960
#define LISTENQ 1024

struct Clinet {
    struct sockaddr_in addr;
    std::vector<std::string> files;
};

extern std::map<std::string, std::string> accounts;
extern std::map<std::string, Clinet> online_users;

int exit_err(std::string str);
void print(int sockfd, char* str);

int tcp_listen(const char* host, const char* service, socklen_t* addrlen);
void service(int sockfd);

void account_processing(int fd, char* mesg);
void list_infomation(int fd, char* mesg);
void p2p_chat_system(char* mesg);
void file_proccesing(char* mesg);

void str_echo(int sockfd);

#endif
