#include "function.h"
using namespace std;

bool volatile keepRunning = true;
std::map<std::string, std::string> users;
std::set<std::string> online_users;
std::map<std::string, sockaddr_in> usersmap;
std::vector<Article> articles;
std::map<string, TFile> fileManager;
std::list<std::string> requests;
int max_retry = 100;

void startup(sqlite3* db)
{
    system("mkdir -p Upload");
    signal(SIGINT, sig_dump);

    users.clear();
    char acc[MAXLINE], pwd[MAXLINE];
    FILE* fp = fopen("users", "r");
    while (fscanf(fp, "%s %s\n", acc, pwd) != EOF) {
        users.insert({acc, pwd});
    }

    // Anonymous, haha
    users.insert({ANONYMOUS, ANONYMOUS});
}

void setup_udpsock(int port)
{
    struct sockaddr_in servaddr;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    len = sizeof(servaddr);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family         = AF_INET;
    servaddr.sin_addr.s_addr    = htonl(INADDR_ANY);
    servaddr.sin_port           = htons(port);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }
    bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
}

// Ensure data sent
void send_datagram(struct sockaddr_in client, char* raw)
{
    char buff[MAX_DATA];
    int times = 0;

    DEBUG("Response to:%d\n", ntohs(client.sin_port));
    do {
        times++;
        bzero(buff, sizeof(buff));
        sendto(sockfd, raw, MAX_DATA, 0, (struct sockaddr*) &client, len);
        recvfrom(sockfd, buff, MAX_DATA, 0, (sockaddr*) &client, &len);
        if (times >= max_retry) break;
    } while (string(raw) != string(buff));
    DEBUG("Response in %d retry! (to:%d)\n", times, ntohs(client.sin_port));
}

// Ensure data sent
bool recv_datagram(struct sockaddr_in& client, char* raw)
{
    recvfrom(sockfd, raw, MAX_DATA, 0, (sockaddr*) &client, &len);
    // ACK
    int n = sendto(sockfd, raw, MAX_DATA, 0, (struct sockaddr*) &client, len);
    if (n < 0) perror(strerror(errno));
    return true;
}

void new_article(char* mesg, struct sockaddr_in client)
{
    char userinfo[MAXLINE];
    char buff1[MAXLINE], buff2[MAXLINE], buff3[MAXLINE];
    sscanf(mesg, "%s %*s %[^\n]%[^$]", buff1, buff2, buff3);
    snprintf(userinfo, MAXLINE, "來自: %s:%d\n",
        inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    string title = buff2, author = buff1, content = buff3;
    content += userinfo;
    Article article = {title, author, content, {}, {}, {}, 0};
    articles.push_back(article);
}

void del_article(char* mesg)
{
    int index;
    char username[MAXLINE];
    sscanf(mesg, "%s %*s %d\n", username, &index);

    Article a = articles.at(--index);
    if (string(username) == a.author) {
        articles.erase(articles.begin() + index);
        snprintf(mesg, MAXLINE, "Article deleted\n");
    } else
        snprintf(mesg, MAXLINE, "You have no permission\n");
}


void browse_article(char* mesg)
{
    int index;
    char username[MAXLINE];
    sscanf(mesg, "%s %*s %d\n", username, &index);
    time_t ticks = time(NULL);
    try {
        Article& a = articles.at(--index);
        a.readcount++;
        if (find(a.blacklist.begin(), a.blacklist.end(),
            string(username)) != a.blacklist.end()) {
            snprintf(mesg, MAXLINE, "You can't read this!!! QQ\n");
            return;
        }
        snprintf(mesg, MAXLINE, article_header,
            a.title.c_str(), a.author.c_str(), ctime(&ticks), a.content.c_str());
        string tmp = mesg;
        for (auto r : a.files) tmp += r + '\n';
        for (auto r : a.responses) tmp += r;
        snprintf(mesg, MAXLINE, "%s", tmp.c_str());
    } catch (const out_of_range& e) {
        snprintf(mesg, MAXLINE, "Wrong article number\n");
    }
}

void list_article(char* mesg)
{
    int i = 1;
    char tmp[MAXLINE];
    string result = "\t\tArticles:\n";
    for (auto a : articles) {
        snprintf(tmp, MAXLINE, "%d\t%s\t\t\t\t%s\t(Read: %d)\n",
            i++, a.title.c_str(), a.author.c_str(), a.readcount);
        result += tmp;
    }
    snprintf(mesg, MAXLINE, "%s", result.c_str());
}

void push_article(char* mesg, struct sockaddr_in client)
{
    int index;
    char username[MAXLINE], content[MAXLINE];
    sscanf(mesg, "%s %*s %d %[^\n]", username, &index, content);

    Article& a = articles.at(--index);
    if (find(a.blacklist.begin(), a.blacklist.end(),
        string(username)) != a.blacklist.end()) {
        snprintf(mesg, MAXLINE, "You can't leave message! QQ\n");
        return;
    }
    char push[MAXLINE];
    snprintf(push, MAXLINE, "%s: %s @%s:%d\n",
        username,
        content,
        inet_ntoa(client.sin_addr),
        ntohs(client.sin_port)
        );
    a.responses.push_back(string(push));
    sprintf(mesg, "Push !\n");
}

void manage_blacklist(char* mesg)
{
    int index;
    char username[MAXLINE], instr[MAXLINE], targ[MAXLINE];
    sscanf(mesg, "%s %s %d %s\n", username, instr, &index, targ);

    Article& a = articles.at(--index);
    if (string(username) == a.author) {
        if (instr[1] == 'A') {
            a.blacklist.push_back(string(targ));
            snprintf(mesg, MAXLINE, "You've blacked %s!\n", targ);
        } else {
            auto itor = find(a.blacklist.begin(), a.blacklist.end(), string(targ));
            a.blacklist.erase(itor);
            snprintf(mesg, MAXLINE, "You've unblacked %s!\n", targ);
        }
    } else
        snprintf(mesg, MAXLINE, "You have no permission\n");
}

void sig_dump(int n)
{
    keepRunning = false;
    DEBUG("-----Dumping------: %d\n", n);
    dump();
    printf("exit!\n");
    exit(0);
}


void dump()
{
    FILE* fp = fopen("users", "w");
    for (auto &user : users) {
        fprintf(fp, "%s %s\n", user.first.c_str(), user.second.c_str());
    }
    fclose(fp);
}

void new_account(char* mesg)
{
    char buffer1[MAXLINE], buffer2[MAXLINE];
    sscanf(mesg, "%*s %*s %s %s\n", buffer1, buffer2);
    string account = buffer1, password = buffer2;
    if (users.count(account)) {
        sprintf(mesg, "Account has been used\n");
    } else {
        users.insert({account, password});
        sprintf(mesg, "Register successfully\n");
        DEBUG("Register %s %s\n", account.c_str(), users.find(account)->second.c_str());
    }
}

void login(char* mesg, struct sockaddr_in client)
{
    char buffer1[MAXLINE], buffer2[MAXLINE];
    sscanf(mesg, "%*s %*s %s %s\n", buffer1, buffer2);
    string account = buffer1, password = buffer2;
    if (!users.count(account)) sprintf(mesg, "Account not existed!\n");
    else {
        if (password != users.find(account)->second) sprintf(mesg, KRED "Password error\n" KNRM);
        else {
            online_users.insert(account);
            usersmap.insert({account, client});
            sprintf(mesg, login_ok);
        }
    }
}

void logout(char* mesg)
{
    char buffer1[MAXLINE];
    sscanf(mesg, "%s %*s", buffer1);
    string account = buffer1;
    if (online_users.find(account) == online_users.end()) sprintf(mesg, logout_ok);
    else {
        online_users.erase(account);
        usersmap.erase(account);
        sprintf(mesg, logout_ok);
    }
}

void del_account(char* mesg)
{
    char buffer1[MAXLINE];
    sscanf(mesg, "%s %*s", buffer1);
    string account = buffer1;
    if (!users.count(account)) sprintf(mesg, "Account not existed!\n");
    else {
        users.erase(account);
        online_users.erase(account);
        usersmap.erase(account);
        sprintf(mesg, "Account is deleted!\n");
    }
}

void list_users(char* mesg)
{
    int offset = 0;
    for (string user : online_users) {
        // if (offset >= THRESHOLD) {
        //     mesg[strlen(mesg)] = '@';
        //     sendto(sockfd, mesg, strlen(mesg), 0, pcliaddr, len);
        //     bzero(mesg, sizeof(mesg));
        //     offset = 0;
        // }
        sprintf(mesg + offset, "%s\n", user.c_str());
        offset += (user.size() + 1);
    }
}


void broadcast(char* mesg, int& sockfd)
{
    char buffer1[MAXLINE], buffer2[MAXLINE];
    sscanf(mesg, "%s %*s %s\n", buffer1, buffer2);
    string name = buffer1, content = buffer2;
    sprintf(mesg, "[Broadcast] %s: %s\n", name.c_str(), content.c_str());
    for (auto c : usersmap) {
        sendto(sockfd, mesg, MAXLINE, 0, (struct sockaddr*) &c.second, len);
        DEBUG("%s:%d\n", inet_ntoa(c.second.sin_addr), ntohs(c.second.sin_port));
    }
}

void direct_mesg(char* mesg, int& sockfd)
{
    char buffer1[MAXLINE], buffer2[MAXLINE], buffer3[MAXLINE];
    sscanf(mesg, "%s %*s %s %s\n", buffer1, buffer2, buffer3);
    string name = buffer1, targ = buffer2, content = buffer3;
    if (online_users.count(targ)) {
        auto t = usersmap.find(targ);
        sprintf(mesg, "[Private] %s: %s\n", name.c_str(), content.c_str());
        DEBUG("Private msg to %d\n", ntohs(t->second.sin_port));
        send_datagram(t->second, mesg);
        sprintf(mesg, "Message has been sent to %s\n", targ.c_str());
    } else sprintf(mesg, "User is not online\n");
}

void send_file(char* mesg, int sockfd, struct sockaddr_in client)
{
    int i = 0;
    int n, s;
    char raw[RAW_DATA], sendline[MAX_DATA];
    char username[MAXLINE], filename[MAXLINE], path[MAXLINE];
    sscanf(mesg, "%s %*s %s\n", username, filename);
    snprintf(path, MAXLINE, "./Upload/%s", filename);

    FILE* f = fopen(path, "rb");
    struct stat fst;
    fstat(fileno(f), &fst);
    size_t filesize = fst.st_size;

    DEBUG("%s\n", path);

    while ((n = fread(raw, sizeof(char), RAW_DATA, f)) > 0) {
        snprintf(sendline, MAX_DATA,
            "%s %s %zd %d ",
            username, filename, filesize, i++);
        memcpy(sendline + strlen(sendline), raw, RAW_DATA);
        s = sendto(sockfd, sendline, MAX_DATA, 0, (struct sockaddr*) &client, len);
        DEBUG("sendout: %d\n", s);
        sleep(  1);
    }
    fclose(f);

    sprintf(mesg, "Download fin.\n");
    send_datagram(client, mesg);
}

void recv_file(char* mesg, struct sockaddr_in client)
{
    size_t n, filesize;
    int expect_number, cur_number, index;
    char username[MAXLINE], filename[MAXLINE], path[MAXLINE];
    char tmp[MAXLINE], raw[RAW_DATA];

    sscanf(mesg, "%s U %d %s %zd %d ", username, &index, filename, &filesize, &cur_number);
    sprintf(tmp, "%s U %d %s %zd %d ", username, index, filename, filesize, cur_number);
    memcpy(raw, mesg + strlen(tmp), RAW_DATA);

    snprintf(path, MAXLINE, "./Upload/%s-%s", username, filename);
    FILE* f = fopen(path, "ab");

    auto fm = fileManager.find(path);
    if (fm == fileManager.end()) {
        fileManager.insert({path, {cur_number + 1, 0}});
        fm = fileManager.find(path);
    } else {
        expect_number = fm->second.expected_num;
        fm->second.expected_num++;
    }
    if (cur_number != expect_number) {
        sprintf(mesg, "Error: download\n");
        sendto(sockfd, mesg, strlen(mesg), 0, (sockaddr*) &client, len);
        return;
    }

    if (fm->second.write_byte + RAW_DATA <= filesize) {
        fm->second.write_byte += fwrite(raw, sizeof(char), RAW_DATA, f);
    } else {
        fm->second.write_byte += fwrite(raw, sizeof(char), filesize - fm->second.write_byte, f);
    }
    fclose(f);

    if (fm->second.write_byte >= filesize) {
        char tmp[MAXLINE];
        snprintf(tmp, MAXLINE, "%s-%s", username, filename);
        Article& a = articles.at(--index);
        a.files.push_back(tmp);
        sprintf(mesg, "Upload fin.\n");
        send_datagram(client, mesg);
        // sendto(sockfd, mesg, strlen(mesg), 0, (sockaddr*) &client, len);
    }
}
