/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-06-08 15:13:16
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-08 16:03:47
 */
#ifndef MY_WEBSERVER_CONFIG_H
#define MY_WEBSERVER_CONFIG_H

#include <sys/types.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"

class Config
{
public:
    // 服务器默认配置
    Config();
    ~Config() {}

    // 解析命令行，实现个性化运行
    void ParseCmd(int argc, char *argv[]);
    // 后台化
    int SetDaemon();

    int port;            // 端口
    int trigMode;        // LT+ET模式
    int timeoutMS;       // timeoutMs
    bool OptLinger;      // 优雅退出
    int sqlPort;         // Mysql配置-端口
    const char *sqlUser; // Mysql配置-用户名
    const char *sqlPwd;  // Mysql配置-密码
    const char *dbName;  // Mysql配置-数据库名
    int connPoolNum;     // 连接池数量
    int threadNum;       // 线程池数量
    bool openLog;        // 日志开关
    int logLevel;        // 日志等级
    int logQueSize;      // 阻塞队列容量
    int actor;           // 事件处理模式默认为reactor
    bool is_daemon;      // 是否开启守护进程
};

#endif