#ifndef __CONST_H__
#define __CONST_H__

#ifndef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define THRESHOLD 2048
#define MAXLINE 8192
#define RAW_DATA 50000
#define MAX_DATA 65000

#define ANONYMOUS "anonymous"
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define CREATE_ARTICLE    sendline[0] == 'A'
#define READ_ARTICLE      sendline[0] == 'B'
#define RESPONSE_ARTICLE  sendline[0] == 'P'
#define MANAGE_ARTICLE    sendline[0] == 'X' || sendline[0] == 'b'

#define welcome_info \
    "###########################\n" \
    "   Welcome to Simple BBS\n"    \
    KRED "      [R]註冊 [L]登入\n" KNRM \
    "###########################\n"

#define choice_info \
    "###################################################################\n" \
    "\t哈囉, %s\n" \
    "  [S]文章列表    [A]新增文章    [B]閱讀文章        [C]刪除帳號\n" \
    "  [I]使用者列表  [Y]廣播所有人  [T]私訊使用者      [E]離開\n" \
    "###################################################################\n"

#define reading_info \
    "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n" \
    "  [X] 刪除文章 [U] 上傳檔案 [bA] 加入黑名單 [bB] 移出黑名單\n" \
    "  [P] 推文     [D] 下載檔案\n" \
    "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"

#define login_ok \
    KGRN "登入成功\n" KNRM

#define logout_ok \
    KGRN "已登出\n" KNRM

#define article_mode_resp \
    "ARTICLLE MODE\n"

#define KHEAD "\x1B[1;37;44m"
#define article_header \
    KHEAD \
    "標題: %-80s\n" \
    "作者: %-80s\n" \
    "時間: %-80.24s\n" KNRM \
    "%s-----------------------\n"

#define download_ok "download_start"
#define upload_ok   "upload_start"

#endif
