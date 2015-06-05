#include "tcp_client.h"
using namespace std;

string username = "USER";
string p2p_serv_port;
map<string, pair<string, string>> peers;

void tcp_p2p_client(char* send, char* recv)
{
    printf("New message! [Y/n]\n");
    // auto peer = peers[user];
    // DEBUG("send to %s %s\n", peer.first.c_str(), peer.second.c_str());
    // int p2p_servfd = tcp_connect(peer.first.c_str(), peer.second.c_str());

    char host[SHORTINFO], port[SHORTINFO];
    int p2p_servfd;
    sscanf(recv, "%*s %[^:]:%s", host, port);
    p2p_servfd = tcp_connect(host, port);

    pthread_t tid;
    pthread_create(&tid, NULL, p2p_chat, (void*) &p2p_servfd);
    pthread_join(tid, NULL);
    pthread_mutex_unlock(&std_input);
}

void p2p_download(char* raw)
{
    char filename[SHORTINFO], ps[MAXDATA], *message;
    stringstream ss;
    string name;
    int i = 0, cnt = 0;
    int n = sscanf(raw, "%*s %s %[^$]", filename, ps);
    pthread_t tids[MAXPEERS];
    string s = ps;
    ss << ps;
    cnt = (int) count(s.begin(), s.end(), ',');
    while (getline(ss, name, ',')) {
        auto p = peers[name];
        message = (char*) malloc(MAXLINE);
        sprintf(message, "D %s %s %d %d", name.c_str(), filename, ++i, cnt);
        pthread_create(&tids[i - 1], NULL, download_from_thread, (void*) message);
    }
    for (int j = 0; j < cnt; j++) pthread_join(tids[j], NULL);

    DEBUG("Concate file\n");
    char path[MAXLINE], buff[MAXDATA];
    snprintf(path, MAXLINE, "./assets/nw-%s", filename);
    FILE* out = fopen(path, "wb");
    for (int j = 1; j <= cnt; j++) {
        snprintf(path, MAXLINE, "./assets/%s.part%d", filename, j);
        FILE* f = fopen(path, "rb");
        while ((n = fread(buff, sizeof(char), MAXDATA, f)) > 0)
            fwrite(buff, sizeof(char), n, out);
        fclose(f);
        DEBUG("fin %s\n", path);
        remove(path);
    }
    fclose(out);
    printf("Finished!\n");
}

void* download_from_thread(void* arg)
{
    download_from((char*) arg);
    free((char*) arg);
    return NULL;
}

void download_from(char* raw)
{
    DEBUG("download_from RAW: %s\n", raw);
    char filename[SHORTINFO], user[SHORTINFO];
    int sn = -1, total = -1;
    int n = sscanf(raw, "%*s %s %s %d %d", user, filename, &sn, &total);
    DEBUG("read %d\n", n);

    auto peer = peers[user];
    DEBUG("send to %s %s\n", peer.first.c_str(), peer.second.c_str());
    int p2p_servfd = tcp_connect(peer.first.c_str(), peer.second.c_str());

    // Tell peer: D <name> <filename>
    write(p2p_servfd, raw, strlen(raw));

    PeerInfo p;
    p.fd = p2p_servfd;
    p.filename = filename;
    p.part = sn;
    p.total = total;
    pthread_t tid;
    pthread_create(&tid, NULL, recv_file, (void*) &p);
    pthread_join(tid, NULL);

    // send self.files to server
    string s = username + " u ";
    s = s + update_assets("./assets");
    write(sockfd, s.c_str(), s.length());
}

void* tcp_p2p_server(void* arg)
{
    int listenfd, *connfd;
    PeerInfo* p;
    struct sockaddr p2p_client;
    socklen_t addrlen, len;
    char test_port[SHORTINFO], buff[MAXDATA];
    char filename[SHORTINFO];
    for (int i = 0; listenfd <= 0; ++i) {
        sprintf(test_port, "%d", atoi(P2P_PORT) + i);
        listenfd = tcp_listen(NULL, test_port, &addrlen);
        p2p_serv_port = test_port;
    }

    for (;;) {
        len = addrlen;
        connfd = (int*) malloc(sizeof(int));
        *connfd = accept(listenfd, &p2p_client, &addrlen);

        read(*connfd, buff, MAXDATA);
        DEBUG("%s\n", buff);

        pthread_t tid;

        if (buff[0] == 'D') {
            int part = -1, total = -1;
            int n = sscanf(buff, "D %*s %s %d %d", filename, &part, &total);
            DEBUG("%s %d\n", filename, *connfd);
            PeerInfo p;
            p.fd = *connfd;
            p.filename = filename;
            p.part = part;
            p.total = total;
            pthread_create(&tid, NULL, send_file, (void*) &p);
            pthread_join(tid, NULL);
            free(connfd);
        } else {
            pthread_create(&tid, NULL, p2p_chat, (void*) &connfd);
            pthread_mutex_unlock(&std_input);
            pthread_join(tid, NULL);
            free(connfd);
        }
    }
    return NULL;
}

void* user_input(void* arg)
{
    char send[MAXLINE];
    char buff1[SHORTINFO], buff2[SHORTINFO];
    char* input = (char*) arg;

    while (1) {
        pthread_mutex_lock(&std_input);
        char *p = fgets(input, MAXLINE, stdin);
        pthread_mutex_unlock(&std_input);

        if (p == NULL) break;
        if (input[0] == 'T' || input[0] == 'Y')
            pthread_mutex_lock(&std_input);
        if (input[0] == 'D') {
            if (sscanf(input, "D %s %s", buff1, buff2) == 2) {
                download_from(input);
                continue;
            }
        }

        strcpy(send, input);
        request_processing(send);
        write(sockfd, send, strlen(send));
    }
    shutdown(sockfd, SHUT_WR);

    return NULL;
}

void request_processing(char* send)
{
    char* tmp = strdup(send);
    snprintf(send, MAXLINE, "%s %s", username.c_str(), tmp);
    free(tmp);
}

void tcp_p2p_init(char* recv)
{
    char buff[MAXLINE];
    sscanf(recv, "%[^.]", buff);
    username = buff;

    // send self.files to server
    DEBUG("p2p serv@%s\n", p2p_serv_port.c_str());
    write(sockfd, p2p_serv_port.c_str(), p2p_serv_port.length());

    // send self.files to server
    string s = update_assets("./assets");
    write(sockfd, s.c_str(), s.length());

    // get other clients info
    read(sockfd, buff, MAXLINE);
    update_peers(buff, ',');
}

string update_assets(string dir)
{
    vector<string> files = getdir(dir.c_str());
    string s;
    for (auto f : files) s += f + ",";
    return s;
}

// (pattern) <user>:<IP>,<user>:<IP>,...
void update_peers(string data, char delim)
{
    stringstream ss(data);
    string item;
    char name[SHORTINFO], serv_port[SHORTINFO], ip[SHORTINFO];
    if (data == "None") return;
    while (getline(ss, item, delim)) {
        if (sscanf(item.c_str(), "%[^@]@%[^:]:%*[^:]:%s", name, ip, serv_port) != 3) return;
        peers[name] = make_pair(ip, serv_port);
        printf("%s-%s::%s\n", name, peers[name].first.c_str(), peers[name].second.c_str());
    }
}
