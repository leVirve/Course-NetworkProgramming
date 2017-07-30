#include "tcp_lib.h"

int tcp_connect(const char* host, const char* service)
{
    sleep(1);
    int sockfd, n;
    struct addrinfo hints, *res, *ret;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, service, &hints, &res)) != 0)
        exit_err("tcp_connect error");
    ret = res;
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) continue;
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) break;
        close(sockfd);
    } while ((res = res->ai_next) != NULL);
    freeaddrinfo(ret);

    return sockfd;
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
    if (res == NULL) return -1;

    listen(listenfd, LISTENQ);

    if (addrlen) *addrlen = res->ai_addrlen;
    freeaddrinfo(ret);

    return listenfd;
}

std::string get_addr(struct sockaddr_in client)
{
    char str[MAXLINE];
    sprintf(str, "%s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    return std::string(str);
}

struct sockaddr_in get_client(int sockfd)
{
    struct sockaddr client;
    socklen_t clilen = sizeof(client);
    getpeername(sockfd, &client, &clilen);
    struct sockaddr_in* clientaddr = (struct sockaddr_in*) &client;
    return *clientaddr;
}

void exit_err(std::string str)
{
    perror(str.c_str());
    exit(1);
}

bool is_contained(std::string str, std::string targ)
{
    return str.find(targ) != std::string::npos;
}

std::vector<std::string> getdir(std::string dir)
{
    std::vector<std::string> files;
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        return files;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_name[0] == '.') continue;
        files.push_back(std::string(dirp->d_name));
    }
    closedir(dp);
    return files;
}

unsigned int get_filesize(FILE* fp)
{
    fseek(fp, 0L, SEEK_END);
    unsigned int sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return sz;
}

int cal_partial_size(int filesize, int total)
{
    return ceil((double)filesize / total);
}

void* send_file(void* arg)
{
    PeerInfo p = *((PeerInfo*) arg);
    DEBUG("part %d\n", p.part);
    char path[MAXLINE];
    snprintf(path, MAXLINE, "./assets/%s", p.filename.c_str());
    FILE* f = fopen(path, "rb");
    unsigned int filesize = get_filesize(f);

    int partial_size = 0, partial_start = 0;
    if (p.total > 0) {
        partial_size = cal_partial_size(filesize, p.total);
        partial_start = partial_size * (p.part - 1);
        if (partial_start + partial_start > filesize)
            partial_size = filesize - partial_start;
        // redifine
        filesize = partial_size;
        int s = fseek(f, partial_start, SEEK_SET);
    }
    DEBUG("part start: %d, size(%d)\n", partial_start, partial_size);

    int n, send_byte = 0;
    char buff[MAXDATA];
    sprintf(buff, "%u\n", filesize);
    send(p.fd, buff, sizeof(buff), 0);

    while ((n = fread(buff, sizeof(char), MAXDATA, f)) > 0) {
        if (send_byte + n > filesize) n = filesize;
        send_byte += n;
        if (n!=MAXDATA) DEBUG("%d\n", n);
        send(p.fd, buff, n, 0);
        bzero(buff, MAXDATA);
        if (send_byte >= filesize) break;
    }
    fclose(f);
    printf("Send fin!\n");
    return NULL;
}

void* recv_file(void* arg)
{
    PeerInfo p = *((PeerInfo*) arg);
    DEBUG("part %d\n", p.part);
    char path[MAXLINE];
    if (p.total > 0)
        snprintf(path, MAXLINE, "./assets/%s.part%d", p.filename.c_str(), p.part);
    else snprintf(path, MAXLINE, "./assets/nw-%s", p.filename.c_str());
    DEBUG("%s\n", path);
    FILE* f = fopen(path, "wb");
    unsigned int filesize, recv_size = 0;

    int n, c;
    char buff[MAXDATA];
    recv(p.fd, buff, MAXDATA, 0);
    sscanf(buff, "%u", &filesize);
    DEBUG("filesize: %u\n", filesize);

    while ((n = recv(p.fd, buff, MAXDATA, 0)) > 0) {
        if (recv_size + n > filesize) n = filesize - recv_size;
        recv_size += n;
        c = fwrite(buff, sizeof(char), n, f);
        bzero(buff, MAXDATA);
        if (recv_size >= filesize) break;
    }
    printf("Part%d Download fin!\n", p.part);
    fclose(f);
    return NULL;
}
