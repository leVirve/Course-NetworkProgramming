#include "../lib/tcp_lib.h"

extern int sockfd;
extern FILE* fp;
extern std::string username;
extern pthread_mutex_t std_input;
extern pthread_mutex_t std_input_s;

void request_processing(char* send);
void* user_input(void* arg);
void* tcp_p2p_server(void*);
void tcp_p2p_client(char*);

void tcp_p2p_init(char*);
std::string update_assets(std::string);
void update_peers(std::string, char);

void download_from(char*);
void* download_from_thread(void* arg);
void p2p_download(char* raw);
