/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-09 15:45:03
 */

#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>    // open
#include <unistd.h>   // close
#include <sys/stat.h> // stat
#include <sys/mman.h> // mmap, munmap

#include "log.h"
#include "buffer.h"

class HttpResponse
{
public:
    // 构造函数
    HttpResponse();
    // 析构函数
    ~HttpResponse();
    // 响应初始化
    void init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);
    // 生成HTTP响应
    void makeResponse(Buffer &buff);
    // 消除文件在内存的映射
    void unmapFile();
    // 获取文件的内存映射地址
    char *file();
    // 获取文件大小
    size_t fileLen() const;
    // 添加错误内容
    void errorContent(Buffer &buff, std::string message);
    // 获取状态码
    int code() const;

private:
    // 添加状态行
    void addStateLine_(Buffer &buff);
    // 添加响应头
    void addHeader_(Buffer &buff);
    // 打开响应文件，将文件映射到内存中，并添加Content-length首部字段
    void addContent_(Buffer &buff);
    // 保存错误码为400，403，404的文件路径，将文件信息存入mmFileStat_变量中
    void errorHtml_();
    // 返回文件类型
    std::string getFileType_();

    int code_;               // 返回码
    bool isKeepAlive_;       // 是否保持长连接
    char *mmFile_;           // 发送文件的内存映射地址
    struct stat mmFileStat_; // 发送文件的信息
    std::string path_;       // 发送文件的路径
    std::string srcDir_;     // 资源目录
    // 静态变量
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE; // 文件类型和返回类型键值对
    static const std::unordered_map<int, std::string> CODE_STATUS;         // 状态码和信息键值对
    static const std::unordered_map<int, std::string> CODE_PATH;           // 错误码与页面对应关系
};

#endif // HTTP_RESPONSE_H
