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
    char buff1[MAXLINE], buff2[MAXLINE], instr;
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
                    client.addr = get_clint(fd);
                    client.files = {};
                    online_users.insert({account, client});
                    sprintf(mesg, "Login successfully !\n");
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

void list_infomation(int fd, char* mesg)
{
    char instr;
    stringstream ss;
    sscanf(mesg, "%*s %c", &instr);
    switch(instr) {
        case 'I':
            for (auto user : online_users)
                ss << user.first << " : " << get_addr(user.second.addr) << endl;
            break;
        case 'F':
            for (auto user : online_users) {
                ss << user.first << " : ";
                for (auto file : user.second.files)
                    ss << file << ", ";
                ss << " ;" << endl;
            }
            break;
        default: break;
    }
    ss.read(mesg, MAXLINE);
    ss.clear();
    ss.str("");
}
