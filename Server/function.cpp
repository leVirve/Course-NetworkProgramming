#include "function.h"
using namespace std;

bool volatile keepRunning = true;
std::set<std::string> online_users;
std::map<std::string, sockaddr_in> usersmap;
std::map<string, TFile> fileManager;
int max_retry = 100;

static int callback(void *num, int argc, char **argv, char **azColName){
    for(int i = 0; i < argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

char ct_user[] = "CREATE TABLE User("  \
         "account CHAR(80) PRIMARY KEY NOT NULL," \
         "password CHAR(80));";

char ct_article[] = "CREATE TABLE Article("  \
         "aid INTEGER PRIMARY KEY," \
         "title CHAR(80)," \
         "author CHAR(80)," \
         "timestamp CHAR(80)," \
         "content TEXT," \
         "ip CHAR(80)," \
         "count INTEGER);";

char ct_response[] = "CREATE TABLE Response("  \
         "rid INTEGER PRIMARY KEY," \
         "account CHAR(80)," \
         "article_id INTEGER," \
         "content CHAR(160));";

char ct_blacklist[] = "CREATE TABLE Blacklist("  \
         "blid INTEGER PRIMARY KEY," \
         "account CHAR(80)," \
         "article_id INTEGER);";

bool sql_true(char* sql)
{
    char *err = NULL;
    int rows, cols;
    char **result;
    sqlite3_get_table(db , sql, &result , &rows, &cols, &err);
    return rows ? true : false;
}

char* sql_exec(char* sql)
{
    char* err;
    sqlite3_exec(db, sql, 0, 0, &err);
    if (err) DEBUG("%s\n", err);
}

void startup()
{
    system("mkdir -p Upload");
    signal(SIGINT, sig_dump);

    char* zErrMsg;
    int res = sqlite3_open_v2("sqlite.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (res){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    } else fprintf(stdout, "Opened database successfully\n");

    sqlite3_exec(db, ct_user, callback, 0, &zErrMsg);
    sqlite3_exec(db, ct_article, callback, 0, &zErrMsg);
    sqlite3_exec(db, ct_response, callback, 0, &zErrMsg);
    sqlite3_exec(db, ct_blacklist, callback, 0, &zErrMsg);
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


void sig_dump(int n)
{
    keepRunning = false;
    sqlite3_close(db);
    printf("exit!\n");
    exit(0);
}

// Ensure data sent
void send_datagram(struct sockaddr_in client, char* raw)
{
    char buff[MAX_DATA];
    int times = 0;

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
    char instr[MAXLINE];
    sscanf(raw, "%*s %s", instr);
    if (instr[0] == 'U' || instr[0] == 'D') return true;

    // ACK
    int n = sendto(sockfd, raw, MAX_DATA, 0, (struct sockaddr*) &client, len);
    if (n < 0) perror(strerror(errno));
    return true;
}


////////////////////////////////////////////////////////////////////////
// Article Model
////////////////////////////////////////////////////////////////////////

void new_article(char* mesg, struct sockaddr_in client)
{
    time_t ticks = time(NULL);
    char userinfo[MAXLINE];
    char author[MAXLINE], title[MAXLINE], content[MAX_DATA];
    sscanf(mesg, "%s %*s %[^\n]%[^$]", author, title, content);
    snprintf(userinfo, MAXLINE, "來自: %s:%d\n",
        inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    char sql[MAX_DATA];
    snprintf(sql, MAX_DATA,
        "insert into Article values(NULL, '%s', '%s', '%.24s', '%s', '%s', '%d')",
        title, author, ctime(&ticks), content, userinfo, 0);
    sql_exec(sql);

    sprintf(mesg, "Article has been posted!\n");
}

void del_article(char* mesg)
{
    int index;
    char username[MAXLINE], sql[MAX_DATA];
    sscanf(mesg, "%s %*s %d\n", username, &index);

    sprintf(sql, "select 1 from Article Where aid = '%d' and author = '%s';"
        , index, username);
    if (sql_true(sql)) {
        sprintf(sql, "delete from Article Where aid = '%d';", index);
        sql_exec(sql);
        snprintf(mesg, MAXLINE, "Article deleted\n");
    } else {
        snprintf(mesg, MAXLINE, "You have no permission\n");
    }
}

void browse_article(char* mesg)
{
    int index;
    string resp;
    char username[MAXLINE];
    char sql[MAXLINE], *err = NULL, **result;
    sscanf(mesg, "%s %*s %d\n", username, &index);

    sprintf(sql, "select * from Blacklist where account = '%s' and article_id = '%d';", username, index);
    if (sql_true(sql)) {
        snprintf(mesg, MAXLINE, "You can't read this!!! QQ\n");
        return;
    }

    int rows, cols;
    string count;
    sprintf(sql, "select * from Article Where aid = '%d'", index);
    if (!sql_true(sql)) sprintf(mesg, "Not existed!\n");
    else {
        sqlite3_get_table(db , sql, &result , &rows, &cols, &err);

        string title = result[1 * cols + 1];
        string author = result[1 * cols + 2];
        string timestamp = result[1 * cols + 3];
        string content = result[1 * cols + 4];
        content += result[1 * cols + 5];
        count = result[1 * cols + 6];
        snprintf(mesg, MAXLINE, article_header,
            title.c_str(), author.c_str(), timestamp.c_str(), content.c_str());
        resp = mesg;
        sqlite3_free_table(result);
    }

    sprintf(sql, "select * from Response Where article_id = '%d'", index);
    sqlite3_get_table(db , sql, &result , &rows, &cols, &err);

    for (int i = 1; i <= rows; ++i) {
        string account = result[i * cols + 1];
        string content = result[i * cols + 3];
        resp += account + content;
    }
    sqlite3_free_table(result);

    int cnt;
    sscanf(count.c_str(), "%d", &cnt);
    sprintf(sql, "update Article set count = '%d' where aid = '%d';",
        ++cnt, index);
    sql_exec(sql);
    snprintf(mesg, MAXLINE, "%s", resp.c_str());
}

////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// Article manupulation
////////////////////////////////////////////////////////////////////////

void list_article(char* mesg)
{
    int rows, cols;
    char sql[MAXLINE], *err = NULL, **result;
    string resp = "**** Articles: *****\n";
    sprintf(sql, "select * from Article");
    sqlite3_get_table(db , sql, &result , &rows, &cols, &err);

    for (int i = 0; i < rows; i++) {
        string id = result[i * cols];
        string title = result[i * cols + 1];
        string author = result[i * cols + 2];
        string count = result[i * cols + 6];

        char tmp[MAXLINE];
        snprintf(tmp, MAXLINE, "%s\t%s\t\t\t\t%s\t(Read: %s)\n",
            id.c_str(), title.c_str(), author.c_str(), count.c_str());
        resp += tmp;
    }
    snprintf(mesg, MAXLINE, "%s", resp.c_str());
}

void push_article(char* mesg, struct sockaddr_in client)
{
    int index;
    char username[MAXLINE], content[MAXLINE];
    char sql[MAXLINE];
    sscanf(mesg, "%s %*s %d %[^\n]", username, &index, content);

    sprintf(sql, "select * from Blacklist where account = '%s' and article_id = '%d';", username, index);
    if (sql_true(sql)) {
        snprintf(mesg, MAXLINE, "You can't push this!!! QQ\n");
        return;
    }

    char push[MAXLINE];
    snprintf(push, MAXLINE, ": %s " KGRN "@%s:%d\n" KNRM,
        content,
        inet_ntoa(client.sin_addr),
        ntohs(client.sin_port)
        );
    sprintf(sql, "insert into Response values(NULL, '%s', '%d', '%s')",
        username, index, push);
    sql_exec(sql);
    sprintf(mesg, "Push !\n");
}

void manage_blacklist(char* mesg)
{
    int index;
    char username[MAXLINE], instr[MAXLINE], targ[MAXLINE];
    sscanf(mesg, "%s %s %d %s\n", username, instr, &index, targ);

    char sql[MAXLINE];

    sprintf(sql, "select * from Article where author = '%s';", username);
    if (sql_true(sql)) {
        if (instr[1] == 'A') {
            sprintf(sql, "select * from User where account = '%s';", targ);
            if (!sql_true(sql)) {
                sprintf(mesg, "No such user\n");
                return;
            }
            sprintf(sql, "insert into Blacklist values(NULL, '%s', '%d');",
                targ, index);
            sql_exec(sql);
            snprintf(mesg, MAXLINE, "You've blacked %s!\n", targ);
        } else {
            sprintf(sql, "select * from Blacklist where account = '%s' and article_id = '%d';",
                targ, index);
            if (!sql_true(sql)) {
                sprintf(mesg, "User not in blacklist\n");
                return;
            }
            sprintf(sql, "delete from Blacklist where account = '%s' and article_id = '%d';",
                targ, index);
            sql_exec(sql);
            snprintf(mesg, MAXLINE, "You've unblacked %s!\n", targ);
        }
    } else {
        snprintf(mesg, MAXLINE, "You have no permission\n");
    }
}

////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// User communicate
////////////////////////////////////////////////////////////////////////

void list_users(char* mesg)
{
    int offset = 0;
    for (string user : online_users) {
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
        if (c.first == name) continue;
        send_datagram(c.second, mesg);
        DEBUG("%s:%d\n", inet_ntoa(c.second.sin_addr), ntohs(c.second.sin_port));
    }
    sprintf(mesg, "Message has been broadcast\n");
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

////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// User Model
////////////////////////////////////////////////////////////////////////

void new_account(char* mesg)
{
    char account[MAXLINE], password[MAXLINE];
    sscanf(mesg, "%*s %*s %s %s\n", account, password);

    char sql[MAXLINE];
    sprintf(sql, "select * from User Where account = '%s';", account);
    if (sql_true(sql)) sprintf(mesg, "Account has been used\n");
    else {
        sprintf(sql, "INSERT INTO User VALUES('%s', '%s');", account, password);
        sql_exec(sql);
        sprintf(mesg, "Register successfully\n");
    }
}

void login(char* mesg, struct sockaddr_in client)
{
    char account[MAXLINE], password[MAXLINE];
    sscanf(mesg, "%*s %*s %s %s\n", account, password);

    char sql[MAXLINE], *err;
    sprintf(sql, "select * from User Where account = '%s' and password = '%s';", account, password);
    if (!sql_true(sql)) {
        sprintf(sql, "select * from User Where account = '%s';", account);
        if (sql_true(sql)) sprintf(mesg, KRED "Password error\n" KNRM);
        else sprintf(mesg, "Account not existed!\n");
    } else {
        online_users.insert(account);
        usersmap.insert({account, client});
        sprintf(mesg, login_ok);
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
    char account[MAXLINE];
    sscanf(mesg, "%s %*s", account);

    char sql[MAXLINE], *err;
    sprintf(sql, "select * from User Where account = '%s';", account);
    if (!sql_true(sql)) sprintf(mesg, "Account not existed!\n");
    else {
        sprintf(sql, "delete from User Where account = '%s';", account);
        sql_exec(sql);
        online_users.erase(account);
        usersmap.erase(account);
        sprintf(mesg, "Account is deleted!\n");
    }
}

////////////////////////////////////////////////////////////////////////


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

    while ((n = fread(raw, sizeof(char), RAW_DATA, f)) > 0) {
        snprintf(sendline, MAX_DATA,
            "%s %s %zd %d ",
            username, filename, filesize, i++);
        memcpy(sendline + strlen(sendline), raw, RAW_DATA);
        s = sendto(sockfd, sendline, MAX_DATA, 0, (struct sockaddr*) &client, len);
        DEBUG("sendout: %d\n", s);
        sleep(1);
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
        char cmd[MAXLINE];
        sprintf(cmd, "rm %s", path);
        system(cmd);
        fileManager.erase(fm);
        sprintf(mesg, "Error: download\n");
        // send_datagram(client, mesg);
        return;
    }

    if (fm->second.write_byte + RAW_DATA <= filesize) {
        fm->second.write_byte += fwrite(raw, sizeof(char), RAW_DATA, f);
    } else {
        fm->second.write_byte += fwrite(raw, sizeof(char), filesize - fm->second.write_byte, f);
    }
    fclose(f);

    if (fm->second.write_byte >= filesize) {
        char push[MAXLINE];
        char sql[MAXLINE];
        snprintf(push, MAXLINE, ": " KMAG "[File] %s-%s " KGRN "@%s:%d\n" KNRM,
            username, filename,
            inet_ntoa(client.sin_addr),
            ntohs(client.sin_port)
            );
        sprintf(sql, "insert into Response values (NULL, '%s', '%d', '%s');",
            username, index, push);
        sql_exec(sql);

        sprintf(mesg, "Upload fin.\n");
        send_datagram(client, mesg);
    }
}
