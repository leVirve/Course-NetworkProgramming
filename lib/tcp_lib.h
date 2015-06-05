#ifndef __TCP_LIB__
#define __TCP_LIB__

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
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
#include <utility>
#include <algorithm>

#ifndef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define P2P_PORT "7777"

#define MAXPEERS 100
#define SHORTINFO 256
#define MAXLINE 4096
#define MAXDATA 40960
#define LISTENQ 1024

struct PeerInfo {
    int fd;
    int part;
    int total;
    std::string filename;
};

void exit_err(std::string);
bool is_contained(std::string str, std::string targ);

int tcp_connect(const char* host, const char* service);
int tcp_listen(const char* host, const char* service, socklen_t* addrlen);
std::string get_addr(struct sockaddr_in client);
struct sockaddr_in get_client(int sockfd);

std::vector<std::string> getdir(std::string);

void* p2p_chat(void*);
void* send_file(void*);
void* recv_file(void*);

#endif
