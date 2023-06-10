/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-10 13:23:01
 */
#ifndef SQL_CONN_POLL_H
#define SQL_CONN_POLL_H

#include <queue>
#include <mutex>
#include <string>
#include <thread>
#include <semaphore.h>
#include <mysql/mysql.h>

#include "log.h"

/*
 * 单例模式（懒汉模式）
 */
class SqlConnPool
{
public:
    // 单例懒汉，静态方法
    // 方法内静态初始化可以保证线程安全，调用该函数返回这一个静态实例的引用
    static SqlConnPool *instance();
    // 从连接池取走一个连接
    MYSQL *getConn();
    // 释放一个数据库连接，重新将连接入池
    void freeConn(MYSQL *sql);
    // 获取空闲连接的数量（connQue_队列长度）
    int getFreeConnCount();
    // 初始化数据库连接池，设置主机，端口，用户，密码，库名，连接池大小
    void init(const char *host, int port,
              const char *user, const char *pwd,
              const char *dbName, int connSize);
    // 关闭连接池
    void closePool();

private:
    // 私有构造函数，单例模式防止类外创建SqlConnPool实例
    SqlConnPool();
    // 私有析构函数，释放所有资源
    ~SqlConnPool();

    int MAX_CONN_;  // 最大连接数
    int useCount_;  // 当前已使用的连接数（未使用）
    int freeCount_; // 空闲连接数（未使用）

    std::mutex mtx_;              // 互斥量（锁空闲连接队列connQue_）
    sem_t semId_;                 // 信号量（代表当前可用连接数）
    std::queue<MYSQL *> connQue_; // 空闲连接队列
};

#endif // SQL_CONN_POLL_H
