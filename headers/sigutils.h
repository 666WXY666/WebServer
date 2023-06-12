#ifndef SIG_UTILS_H
#define SIG_UTILS_H

#include <sys/types.h>
#include <string.h> // memset
#include <assert.h>
#include <sys/socket.h>

#include <signal.h>

class SigUtils
{
public:
    // 构造函数
    SigUtils() {}
    // 析构函数
    ~SigUtils() {}

    // 信号处理函数
    static void sigHandler_(int sig);
    // 设置信号函数
    void addSig_(int sig, void(handler)(int), bool restart = true);

    // 静态成员，传递信号到主循环的管道
    static int *u_pipefd;
};

#endif // SIG_UTILS_H