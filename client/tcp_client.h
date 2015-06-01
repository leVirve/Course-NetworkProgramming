#include "../lib/tcp_lib.h"

extern int sockfd;
extern FILE* fp;

void request_processing(char* send);
void* request(void* arg);
void* tcp_p2p_server(void* arg);
void* tcp_p2p_client(void* arg);

