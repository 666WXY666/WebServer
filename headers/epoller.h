/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-08 17:20:59
 */
#ifndef EPOLLER_H
#define EPOLLER_H

#include <vector>
#include <fcntl.h> // fcntl
#include <errno.h>
#include <unistd.h>    // close()
#include <assert.h>    // close()
#include <sys/epoll.h> //epoll_ctl()

class Epoller
{
public:
    // 构造函数，创建epoll内核事件表和初始化就绪事件数组
    explicit Epoller(int maxEvent = 1024);
    // 析构函数
    ~Epoller();
    // 注册事件
    bool addFd(int fd, uint32_t events);
    // 修改（重置）事件
    bool modFd(int fd, uint32_t events);
    // 删除事件
    bool delFd(int fd);
    // 开始监测
    int wait(int timeoutMS = -1);
    // 获取第i个事件的文件描述符
    int getEventFd(size_t i) const;
    // 获取第i个事件的事件类型
    uint32_t getEvents(size_t i) const;

private:
    int epollFd_;                            // epoll_create()创建的epoll文件描述符
    std::vector<struct epoll_event> events_; // epoll就绪事件数组
};

#endif // EPOLLER_H
