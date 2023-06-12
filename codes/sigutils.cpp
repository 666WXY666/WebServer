/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-06-12 11:20:37
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-12 13:05:27
 */
#include "../headers/sigutils.h"
#include <iostream>

int *SigUtils::u_pipefd = 0; // 初始化管道

/*
 * 信号处理函数
 */
void SigUtils::sigHandler_(int sig)
{
    int msg = sig;
    // 发送信号到管道
    send(u_pipefd[1], (char *)&msg, 1, 0);
}

/*
 * 设置信号函数
 */
void SigUtils::addSig_(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    // 设置信号处理函数
    sa.sa_handler = handler;
    // 是否重启信号
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    // 设置临时阻塞信号集
    sigfillset(&sa.sa_mask);
    // 添加信号处理
    int ret = sigaction(sig, &sa, NULL);
    // note: 注意这里的assert，如果设置了NDEBUG宏，就不会运行assert
    // 在cmake中设置了NDEBUG宏，所以不要把逻辑代码写到assert里
    assert(ret != -1);
}