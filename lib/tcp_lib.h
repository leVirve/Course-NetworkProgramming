#ifndef __TCP_LIB__
#define __TCP_LIB__

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

#ifndef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define P2P_PORT "7777"

#define SHORTINFO 256
#define MAXLINE 4096
#define MAXDATA 40960
#define LISTENQ 1024

void exit_err(std::string);

int tcp_connect(const char* host, const char* service);
int tcp_listen(const char* host, const char* service, socklen_t* addrlen);
std::string get_addr(struct sockaddr_in client);
struct sockaddr_in get_client(int sockfd);

void p2p_chat(int);

#endif
