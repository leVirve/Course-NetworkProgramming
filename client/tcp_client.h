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

#include <string>

#ifndef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define MAXLINE 4096

void exit_err(std::string);
int tcp_connect(const char* host, const char* service);
void request_processing(char* send);
