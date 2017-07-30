#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <algorithm>

#include "../const.h"

struct TFile {
    int expected_num;
    int write_byte;
};

extern int sockfd;
extern socklen_t len;
extern sqlite3* db;
extern bool volatile keepRunning;
extern std::set<std::string> online_users;
extern std::map<std::string, sockaddr_in> usersmap;
extern std::map<std::string, TFile> fileManager;

void startup();
void sig_dump(int);
void setup_udpsock(int);
void send_datagram(struct sockaddr_in, char*);
bool recv_datagram(struct sockaddr_in&, char*);

void new_account(char*);
void del_account(char*);
void login(char*, struct sockaddr_in);
void logout(char*);

void list_users(char*);
void broadcast(char*, int&);
void direct_mesg(char*, int&);

void new_article(char*, struct sockaddr_in);
void del_article(char*);
void browse_article(char*);
void list_article(char*);
void push_article(char*, struct sockaddr_in);
void manage_blacklist(char*);

void recv_file(char*, struct sockaddr_in);
void send_file(char*, int, struct sockaddr_in);



#endif
