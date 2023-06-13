/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-12 20:51:45
 */
#ifndef BUFFER_H
#define BUFFER_H

#include <vector> // readv
#include <atomic>
#include <cstring> // perror
#include <iostream>
#include <assert.h>
#include <unistd.h>  // write
#include <sys/uio.h> // readv

class Buffer
{
public:
    // 初始化缓冲区大小为1024
    Buffer(int initBufferSize = 1024);
    // 默认析构函数
    ~Buffer() = default;

    // 可写的字节数
    size_t writableBytes() const;
    // 可读的字节数
    size_t readableBytes() const;
    // 已读的字节数
    size_t prependableBytes() const;

    // 返回当前readPos_所在字符地址的指针
    const char *peek() const;
    // 移动readPos_指针，读了len个字节的数据
    void retrieve(size_t len);
    // 移动readPos_指针，移动大小为readPos_所在字符地址的指针到end这一段
    void retrieveUntil(const char *end);
    // 读完了所有数据，回收所有空间，将读写指针还原，buffer清空
    void retrieveAll();
    // 以string类型返回被回收的数据，同时回收所有空间
    std::string retrieveAllToStr();

    // 确保还能写len个字节，如果不能，再开辟len大小的可写空间
    void ensureWritable(size_t len);
    // 写了len个字节的数据，移动writePos_位置
    void hasWritten(size_t len);
    // 返回可以写的缓冲区的第一个位置const
    const char *beginWriteConst() const;
    // 返回可以写的缓冲区的第一个位置
    char *beginWrite();

    // 往缓冲区写数据
    // string类型（主要用于日志buffer中）
    void append(const std::string &str);
    // char *类型
    void append(const char *str, size_t len);
    // void *类型
    void append(const void *data, size_t len);
    // Buffer类型，将传入的buff对象中的未读取数据写入本buff中
    void append(const Buffer &buff);

    // 读取socket的fd中的数据到buffer中
    ssize_t readFd(int fd, int *saveErrno);
    // 将buffer的数据写入socket的fd中，即向socket中发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    // 返回缓冲区的首字符的指针
    char *beginPtr_();
    // 返回缓冲区的首字符的const指针
    const char *beginPtr_() const;
    // 增加len大小的可写空间
    void makeSpace_(size_t len);

    std::vector<char> buffer_;          // vector实现的自增长缓冲区
    std::atomic<std::size_t> readPos_;  // 读指针所在的下标，标志可以读的起始点，原子
    std::atomic<std::size_t> writePos_; // 写指针所在的下标，标志下一个空闲可写位置，原子
};

#endif // BUFFER_H
