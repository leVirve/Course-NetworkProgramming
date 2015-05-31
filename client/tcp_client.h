#include "../lib/tcp_lib.h"

extern int sockfd;
extern FILE* fp;

void* request(void* arg);
void request_processing(char* send);
void tcp_client(FILE* fp_arg, int sockfd_arg);

