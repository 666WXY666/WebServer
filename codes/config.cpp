/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-06-08 15:13:16
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-12 14:22:24
 */
#include <stdio.h>
#include "../headers/config.h"

Config::Config()
{
    // 端口
    port = 9006;
    // epoll事件触发模式，默认ET+ET模式
    trigMode = 3;
    // timeoutMs，默认1min
    timeoutMS = 60000;
    // 优雅退出，默认关闭
    OptLinger = false;
    // Mysql配置
    sqlPort = 3306;
    sqlUser = "wxy";
    sqlPwd = "123456";
    dbName = "webserver";
    // 连接池数量
    connPoolNum = 12;
    // 线程池数量
    threadNum = 6;
    // 日志开关
    openLog = true;
    // 日志等级
    logLevel = 1;
    // 阻塞队列容量
    logQueSize = 1024;
    // 事件处理模式：Reactor(0)/Proactor(1)，默认Reactor
    actor = 0;
    // 守护进程，默认不开启
    is_daemon = false;
}

// 处理命令行参数
void Config::ParseCmd(int argc, char *argv[])
{
    int opt;
    const char *str = "p:l:m:o:s:t:e:a:d:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;
        case 'l':
            openLog = atoi(optarg);
            break;
        case 'm':
            timeoutMS = atoi(optarg);
            break;
        case 'o':
            OptLinger = atoi(optarg);
            break;
        case 's':
            connPoolNum = atoi(optarg);
            break;
        case 't':
            threadNum = atoi(optarg);
            break;
        case 'e':
            logLevel = atoi(optarg);
            break;
        case 'a':
            actor = atoi(optarg);
            break;
        case 'd':
            is_daemon = atoi(optarg);
            break;
        default:
            break;
        }
    }
}