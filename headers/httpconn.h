#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <errno.h>
#include <stdlib.h>    // atoi()
#include <sys/uio.h>   // readv/writev
#include <arpa/inet.h> // sockaddr_in
#include <sys/types.h>

#include "log.h"
#include "buffer.h"
#include "sqlconnRAII.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
public:
    // 构造函数
    HttpConn();
    // 析构函数
    ~HttpConn();
    // 初始化
    void init(int sockfd, const sockaddr_in &addr);
    // 读写数据
    ssize_t read(int *saveErrno);
    ssize_t write(int *saveErrno);
    // 关闭该连接
    void close();
    // 获取该连接的信息
    int getFd() const;
    int getPort() const;
    const char *getIP() const;
    sockaddr_in getAddr() const;
    // 解析请求并生成响应
    bool process();
    // 返回需要写的数据长度
    int toWriteBytes();
    // 返回是否长连接
    bool isKeepAlive() const;
    // 静态成员
    static bool isET;                  // 指示工作模式
    static const char *srcDir;         // 资源文件目录
    static const char *uploadDir;      // 上传文件目录
    static std::atomic<int> userCount; // 指示用户连接个数，原子变量，各连接共享

private:
    int fd_;                  // socket对应的文件描述符
    bool isClose_;            // 指示工作状态，该连接是否关闭
    struct sockaddr_in addr_; // 客户端socket对应的地址

    // 缓冲区块
    int iovCnt_;          // 输出数据的个数，不在连续区域
    struct iovec iov_[2]; // 代表输出哪些数据的结构体

    Buffer readBuff_;  // 读缓冲区，保存请求数据
    Buffer writeBuff_; // 写缓冲区，保存相应数据

    HttpRequest request_;   // 包装的处理http请求的类
    HttpResponse response_; // 包装的处理http响应的类
};

#endif // HTTP_CONN_H
