#ifndef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define THRESHOLD 2048
#define MAX_DATA 4096
#define ANONYMOUS "anonymous"

#define CREATE_ARTICLE sendline[0] == 'A'

char welcome_info[] =
    "###########################\n" \
    "   Welcome to Simple BBS\n"    \
    "      [R]註冊 [L]登入\n" \
    "###########################\n";

char choice_info[] =
    "###################################################################\n" \
    "\t哈囉, %s\n" \
    "  [S]文章列表    [A]新增文章    [R]閱讀文章        [D]刪除帳號\n" \
    "  [U]使用者列表  [Y]廣播所有人  [T]私訊使用者      [E]離開\n" \
    "###################################################################\n";

char login_ok[] =
    "Login successfully\n";

char logout_ok[] =
    "Logout successfully\n";

char article_mode_resp[] =
    "ARTICLLE MODE\n";

