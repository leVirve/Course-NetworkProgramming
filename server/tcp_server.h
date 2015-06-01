#ifndef __TCP_SERVER__
#define __TCP_SERVER__

#include "../lib/tcp_lib.h"

struct Clinet {
    int sockfd;
    struct sockaddr_in addr;
    std::vector<std::string> files;
};

extern std::map<std::string, std::string> accounts;
extern std::map<std::string, Clinet> online_users;

void account_processing(int fd, char* mesg);
void list_infomation(char* mesg);
void p2p_chat_system(int fd, char* mesg);
void p2p_file_system(char* mesg);

#endif
