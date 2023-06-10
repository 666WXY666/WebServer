#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <chrono>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>

#include "log.h"

typedef std::function<void()> TimeoutCallBack;    // 定义的functional对象，接受一个bind函数绑定的函数对象
typedef std::chrono::high_resolution_clock Clock; // 获取时间的类
typedef std::chrono::milliseconds MS;             // 毫秒
typedef Clock::time_point TimeStamp;              // 时间戳

/*
 * 定义的绑定文件描述符，超时时间，删除函数的结构体
 * 同时还重载了 < 运算符
 */
struct TimerNode
{
    int id;             // 节点对应的文件描述符
    TimeStamp expires;  // 超时时间的时间戳，如果当前时间>=expires，说明超时
    TimeoutCallBack cb; // 回调函数，这里接受的是删除节点后的对应操作，即WebServer::CloseConn_
    // 重载的比较运算符，方便比较时间
    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
};

class HeapTimer
{
public:
    // 构造函数，预申请一些空间
    HeapTimer();
    // 析构函数，清空空间
    ~HeapTimer();
    // 调整文件描述符id关联的定时器的过期时间为：当前时间+timeout，并下移调整堆
    void adjust(int id, int newExpires);
    // 将指定文件描述符id添加一个定时器到堆中，超时时间为timeout，回调函数为cb
    void add(int id, int timeout, const TimeoutCallBack &cb);
    // 删除指定id结点，并触发回调函数（未使用）
    void doWork(int id);
    // 清空堆中的元素
    void clear();
    // 删除堆中所有的超时节点，并触发它们的回调函数
    void tick();
    // 删除堆中的第一个节点
    void pop();
    // 删除所有超时节点，返回最近一个超时节点的超时时间
    int getNextTick();

private:
    // 删除堆中的指定index节点
    void del_(size_t i);
    // 上移调整堆中元素
    void shiftup_(size_t i);
    // 下移调整堆中元素
    bool shiftdown_(size_t index, size_t n);
    // 交换堆中两个节点
    void swapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;         // 使用vector模拟实现的小根堆，每个元素为封装的TimerNode
    std::unordered_map<int, size_t> ref_; // 使用hashmap方便确定节点是否存在，O(1)
};

#endif // HEAP_TIMER_H
