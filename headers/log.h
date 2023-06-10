#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <string.h>
#include <stdarg.h> // va_start va_end
#include <assert.h>
#include <sys/stat.h> // mkdir
#include <sys/time.h>

#include "buffer.h"
#include "blockqueue.h"

/*
 * 单例模式（懒汉模式）
 */
class Log
{
public:
    // 初始化日志，设置级别，路径，后缀，日志队列大小，创建日志文件
    void init(int level = 1, const char *path = "./log",
              const char *suffix = ".log", int maxQueueSize = 1024);
    // 单例懒汉，静态方法
    // 方法内静态初始化可以保证线程安全，调用该函数返回这一个静态实例的引用
    static Log *instance();
    // 线程的执行函数，调用AsyncWrite_()，必须为静态的（因为类内函数自带this参数）
    static void flushLogThread();
    // 向log文件中写入log信息，传入日志等级，格式format，可变参数va_list
    void write(int level, const char *format, ...);
    // 清空缓冲区，将当前的数据都写入到文件
    void flush();
    // 返回日志级别
    int getLevel();
    // 设置日志级别
    void setLevel(int level);
    // 返回日志是否打开
    bool isOpen();

private:
    // 私有构造函数，初始化一系列变量，单例模式防止类外创建Log实例
    Log();
    // 私有析构函数，将所有log信息写入文件，再关闭日志写入子线程
    virtual ~Log();
    // 调用Buffer的append函数，向缓冲区中加入log的级别信息
    void appendLogLevelTitle_(int level);
    // 异步取出队列中数据，写入文件中，循环写入，直到队列为空时阻塞等待
    void asyncWrite_();

    static const int LOG_PATH_LEN = 256; // 最大log文件路径长度
    static const int LOG_NAME_LEN = 256; // 最大log文件名长度
    static const int MAX_LINES = 50000;  // log文件最大行数，超过这个数量就要单独划分文件

    const char *path_;   // log文件路径
    const char *suffix_; // 文件后缀名

    int lineCount_; // 当前log文件行数
    int today_;     // 日期，用于日志文件名
    bool isOpen_;   // 是否开启日志
    int level_;     // 日志级别
    bool isAsync_;  // 是否异步模式写入log

    FILE *fp_;    // log文件指针
    Buffer buff_; // 缓冲区

    std::unique_ptr<BlockDeque<std::string>> deque_; // log阻塞队列
    // 主线程写入日志到buff_，日志子线程以单线程模式将buff_写入文件
    std::unique_ptr<std::thread> writeThread_; // 写入日志的线程
    std::mutex mtx_;                           // 互斥量
};

/*
 * 定义log日志相关的宏
 * 4种类型的日志，分别是LOG_DEBUG、LOG_INFO、LOG_WARN与LOG_ERROR
 * 它们共同使用LOG_BASE，以level来区分不同级别的日志
 * 只会像文件写入规定level以下的log信息 log->GetLevel() <= level
 * 日志类型必须小于等于系统的日志等级才能输出，比如日志等级设置为1，那么DEBUG类型的日志无法输出
 */
#define LOG_BASE(level, format, ...)                   \
    do                                                 \
    {                                                  \
        Log *log = Log::instance();                    \
        if (log->isOpen() && log->getLevel() <= level) \
        {                                              \
            log->write(level, format, ##__VA_ARGS__);  \
            log->flush();                              \
        }                                              \
    } while (0);

#define LOG_DEBUG(format, ...)             \
    do                                     \
    {                                      \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do                                     \
    {                                      \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do                                     \
    {                                      \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do                                     \
    {                                      \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);

#endif // LOG_H
