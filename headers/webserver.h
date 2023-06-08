//
// Created by zzh on 2022/4/19.
//

#ifndef MY_WEBSERVER_WEBSERVER_H
#define MY_WEBSERVER_WEBSERVER_H

#include <unordered_map>
#include <fcntl.h> // fcntl()
#include <errno.h>
#include <unistd.h> // close()
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "epoller.h"
#include "httpconn.h"
#include "heaptimer.h"
#include "threadpool.h"
#include "sqlconnpoll.h"
#include "sqlconnRAII.h"

class WebServer
{
public:
    WebServer(int port, int trigMode, int timeoutMS, bool optLinger,
              int sqlPort, const char *sqlUser, const char *sqlPwd,
              const char *dbName, int connPoolNum, int threadNum,
              bool openLog, int logLevel, int logQueSize, int actor, bool is_daemon);

    ~WebServer();
    // 运行server
    void start();

private:
    static int setFdNonblock(int fd);
    // 初始化监听socket
    bool initSocket_();
    // 初始化触发组合模式
    void initEventMode_(int trigMode);
    // 添加客户端
    void addClient_(int fd, sockaddr_in addr);
    // 获取新连接，初始化客户端数据

    void dealListen_();
    // 调用ExtentTime_，并将写任务加入线程池的工作队列
    void dealWrite_(HttpConn *client);
    // 调用ExtentTime_，并将读任务加入线程池的工作队列
    void dealRead_(HttpConn *client);
    // 发送错误信息给客户端并关闭连接
    void sendError_(int fd, const char *info);
    // 延长client的定时器的超时时长
    void extentTime_(HttpConn *client);
    // 关闭连接
    void closeConn_(HttpConn *client);

    // 读取数据，并调用OnProcess处理请求
    void onRead_(HttpConn *client);
    // 向客户端发送响应
    void onWrite_(HttpConn *client);
    // 调用process解析请求生成响应，然后修改监测事件：
    // 若生成了响应则改为监测写事件，否则说明没有解析请求，改为监测读事件
    void onProcess_(HttpConn *client);

    static const int MAX_FD = 65536; // 最大文件描述符数量

    int port_;      // 监听的端口
    int timeoutMS_; // 毫秒MS
    int listenFd_;  // 监听的文件描述符
    // note: SO_LINGER将决定系统如何处理残存在套接字发送队列中的数据
    // 处理方式无非两种：丢弃或者将数据继续发送至对端
    bool openLinger_; // 是否优雅关闭
    bool isClose_;    // 是否关闭服务器，指示InitSocket操作是否成功
    char *srcDir_;    // 资源文件目录
    int actor_;       // 事件处理模式：reactor(0)/proactor(1)
    bool is_daemon_;  // 是否以守护进程方式启动

    uint32_t listenEvent_; // 监听描述符上的epoll事件
    uint32_t connEvent_;   // 连接描述符上的epoll事件

    std::unique_ptr<HeapTimer> timer_;       // 基于小根堆的定时器
    std::unique_ptr<ThreadPool> threadPool_; // 线程池
    std::unique_ptr<Epoller> epoller_;       // 监听实例epoller变量
    // note: 使用hash实现的文件描述符，这样可以用一个实例化一个，不用一开始就初始化很多个
    std::unordered_map<int, HttpConn> users_; // 客户端连接集合
};

#endif // MY_WEBSERVER_WEBSERVER_H
