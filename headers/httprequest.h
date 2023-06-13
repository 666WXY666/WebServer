/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-12 20:08:25
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <errno.h>
#include <mysql/mysql.h> //mysql

#include "log.h"
#include "buffer.h"
#include "sqlconnpoll.h"
#include "sqlconnRAII.h"

class HttpRequest
{
public:
    // 指示解析到请求头的哪一部分的枚举变量
    enum PARSE_STATE
    {
        REQUEST_LINE, // 正在解析请求首行
        HEADER,       // 正在解析请求头
        BODY,         // 正在解析请求体
        FINISH        // 解析完成
    };

    // 指示解析结果的枚举变量
    enum HTTP_CODE
    {
        NO_REQUEST = 0,    // 请求不完整，继续读取请求数据，epoll继续监测读事件
        GET_REQUEST,       // 获取了完整请求，处理请求，完成资源映射
        BAD_REQUEST,       // 请求报文语法错误，返回错误报文
        NO_RESOURCE,       // 没有资源，返回错误报文
        FORBIDDEN_REQUEST, // 禁止访问，返回错误报文
        FILE_REQUEST,      // 请求文件可以正常访问，返回正常响应报文
        INTERNAL_ERROR,    // 内部错误（switch的default，一般不会触发）
        CLOSED_CONNECTION  // 关闭连接
    };

    // 构造函数
    HttpRequest();
    // 生成默认析构函数
    ~HttpRequest() = default;
    // 请求初始化
    void init();
    // 解析请求（数据包存在buff）
    HTTP_CODE parse(Buffer &buff);
    // 获取请求的基本信息
    PARSE_STATE state() const;
    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    // 返回请求头中指定key对应的数据
    std::string getPost(const std::string &key) const;
    std::string getPost(const char *key) const;
    // 是否是长连接
    bool isKeepAlive() const;

    // 静态常量
    // note: 注意，这里的需要是静态的，并且需要在在全局定义，在外层调用构造的时候初始化
    static std::string uploadDir; // 上传文件目录

private:
    // 16进制转10进制
    static int convertHex(char ch);
    // 用户验证（登陆或注册）
    static bool userVerify(const std::string &name, const std::string &pwd, bool isLogin);
    // 解析HTTP请求首行
    bool parseRequestLine_(const std::string &line);
    // 解析HTTP请求头
    void parseHeader_(const std::string &line);
    // 解析HTTP消息体
    bool parseBody_(const std::string &line);
    // 解析资源路径，并将路径添加完整
    void parsePath_();
    // 解析POST消息体
    bool parsePost_();
    // 解析form-urlencoded格式，获取POST的数据
    void parseFromUrlencoded_();
    // 解析multipart/form-data格式，获取POST的数据（上传文件）
    bool parseFormData_();

    PARSE_STATE state_;                                   // 状态机解析状态
    std::string method_, path_, version_, body_;          // 请求首行：方法、URL、版本，消息体
    std::unordered_map<std::string, std::string> header_; // 请求头字段hashmap，key:value对形式
    // POST请求表单中的信息，以key:value对的形式存储POST的参数（用户名&密码）
    // 因为本服务器接受的是application/x-www-form-urlencoded这种表单形式
    std::unordered_map<std::string, std::string> post_;

    int parseBodyCnt_;           // 解析了几行请求内容
    std::string uploadFilename_; // 上传的文件名
    FILE *fp_;                   // 文件指针
    bool upload_;                // 是否上传文件
    bool upload_error_;          // 上传文件错误指示

    // 静态常量
    static const std::unordered_set<std::string> DEFAULT_HTML;          // 默认的返回页面的地址
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG; // 保存默认的HTML标签
};

#endif // HTTP_REQUEST_H
