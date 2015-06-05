#include "tcp_server.h"
using namespace std;

map<std::string, std::string> accounts;
map<std::string, Clinet> online_users;

bool is_existed(const string account)
{
    return accounts.count(account);
}

bool is_matched(const string account, const string password)
{
    return password == accounts.find(account)->second;
}

bool is_login(const string account)
{
    return online_users.find(account) != online_users.end();
}

void account_processing(int fd, char* mesg)
{
    char buff1[SHORTINFO], buff2[SHORTINFO], instr;
    sscanf(mesg, "%*s %c %s %s", &instr, buff1, buff2);
    string account = buff1, password = buff2;

    switch(instr) {
        case 'R':
            if (is_existed(account)) {
                sprintf(mesg, "Account has been used\n");
            } else {
                accounts.insert({account, password});
                sprintf(mesg, "Register successfully!\n");
            }
            break;
        case 'L':
            if (!is_existed(account)) {
                sprintf(mesg, "Account not existed...\n");
            } else {
                if (!is_matched(account, password)) {
                    sprintf(mesg, "Wrong password\n");
                } else {
                    Clinet client;
                    client.sockfd = fd;
                    client.addr = get_client(fd);
                    sprintf(mesg, "%s. Login successfully !\n", account.c_str());
                    write(fd, mesg, strlen(mesg));

                    // Read client's assets
                    char buff[MAXDATA];
                    read(fd, buff, MAXDATA);
                    client.serv_port = buff;
                    bzero(buff, MAXDATA);

                    // Read client's assets
                    read(fd, buff, MAXDATA);
                    client.files = buff;
                    online_users.insert({account, client});

                    // Send peers' info
                    string raw = "";
                    for (auto peer : online_users) {
                        if (peer.first == account) continue;
                        raw += peer.first + "@" +
                            get_addr(peer.second.addr) + ":" +
                            peer.second.serv_port + ",";
                        DEBUG("%s\n", raw.c_str());
                    }
                    if (raw.length() == 0) raw = "None";
                    write(fd, raw.c_str(), raw.length());

                    sprintf(mesg, "%s, initialized !\n", account.c_str());
                }
            }
            break;
        case 'E':
            if (!is_login(account)) {
                sprintf(mesg, "You're not logged in\n");
            } else {
                online_users.erase(account);
                sprintf(mesg, "Logout successfully\n");
            }
            break;
        case 'X':
            if (!is_existed(account)) {
                sprintf(mesg, "Account not existed...\n");
            } else {
                if (is_login(account)) {
                    online_users.erase(account);
                }
                accounts.erase(account);
                sprintf(mesg, "Account deleted\n");
            }
            break;
        default: break;
    }
}

void list_infomation(char* mesg)
{
    char instr;
    stringstream ss;
    vector<string> files;
    sscanf(mesg, "%*s %c", &instr);
    switch(instr) {
        case 'I':
            for (auto user : online_users)
                ss << user.first << "@" << get_addr(user.second.addr) << endl;
            break;
        case 'F':
            files = getdir("./Assets");
            ss << "Server: ";
            for (auto f : files)
                ss << f + ", ";
            ss << " ;" << endl;
            for (auto user : online_users) {
                ss << user.first << " : ";
                ss << user.second.files << endl;
            }
            break;
        case 'u':
            char user[SHORTINFO], assets[MAXDATA];
            sscanf(mesg, "%s %*c %[^$]", user, assets);
            online_users[user].files = assets;
            ss << "Update list!" << endl << "\0";
            break;
        default: break;
    }
    ss.read(mesg, MAXLINE);
    ss.str("");
    ss.clear();
}

void p2p_chat_system(int fd, char* mesg)
{
    char instr;
    char send[MAXLINE], user[SHORTINFO];
    sscanf(mesg, "%*s %c %s", &instr, user);

    if (!is_login(user)) {
        sprintf(mesg, "%s is not online!\n", user);
    } else {
        string p2p_server = get_addr(get_client(fd));
        Clinet p2p_client = online_users.find(user)->second;
        sprintf(send, "connect %s", p2p_server.c_str());
        sprintf(mesg, "new connection\n");
        write(p2p_client.sockfd, send, MAXLINE);
    }
}

void p2p_file_system(int fd, char* mesg)
{
    char instr, buff1[SHORTINFO], buff2[SHORTINFO];
    int argc = sscanf("%*s %c %s %s", &instr, buff1, buff2);

    switch(instr) {
    case 'D':
        // if (argc == 2) {
        // // from server
        //     send_file(sockfd, filename);
        // } else {
        // // from peers
        //     for (auto client : online_users) {
        //         if (client.files.find(string(filename
        //             )) != client.files.end()) {
        //             write("wait for connection #%d");
        //             write("connect to #%d");
        //         }
        //     }
        // }
        break;
    case 'U':
        break;
        default:break;
    }
}
