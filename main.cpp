/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-12 13:13:29
 */
#include <unistd.h>
#include "headers/webserver.h"
#include "headers/config.h"

int main(int argc, char *argv[])
{
    // 导入配置
    Config config;
    config.ParseCmd(argc, argv);
    // 守护进程
    if (config.is_daemon)
    {
        if (daemon(1, 0) < 0)
        {
            LOG_ERROR("Start Daemon Error!");
            return 0;
        }
    }
    // WebServer初始化
    WebServer server(
        config.port, config.trigMode, config.timeoutMS, config.OptLinger,                         // 端口 ET模式 timeoutMs 优雅退出
        config.sqlPort, config.sqlUser, config.sqlPwd, config.dbName,                             // Mysql配置
        config.connPoolNum, config.threadNum, config.openLog, config.logLevel, config.logQueSize, // 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量
        config.actor, config.is_daemon                                                            // 事件模式 守护进程
    );
    // WebServer启动
    server.start();
    // 打印退出成功日志
    LOG_INFO("=========================Server Quit=========================\n");
    return 0;
}