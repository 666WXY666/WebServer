/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-13 17:02:53
 */
#include "../headers/httprequest.h"

// 保存默认界面名字的静态变量，所有对以下界面的请求都会加上 .html 后缀
const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
    "/upload",
    "/success"};

// 静态变量，保存默认的HTML标签
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1}};

// 上传文件目录
std::string HttpRequest::uploadDir;

/*
 * 构造函数中初始化
 */
HttpRequest::HttpRequest()
{
    init();
}

/*
 * 初始化相关变量
 */
void HttpRequest::init()
{
    // 所有变量清空
    method_ = path_ = version_ = body_ = "";
    // 状态重置到解析请求首行状态
    state_ = REQUEST_LINE;
    // 清空以下变量
    header_.clear();
    post_.clear();
    // 重置是否上传状态，这里必须重置，因为每次请求都会重新init
    upload_ = false;
    upload_error_ = false;
    // 重置记录解析请求体的行数，一定要在这里初始化
    // 否则如果在parse里初始化，不完整数据下一半来的时候就不知道前面读了几行了
    parseBodyCnt_ = 0;
}

/*
 * 返回客户端是否有长连接请求
 */
bool HttpRequest::isKeepAlive() const
{
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

/*
 * 将16进制数转为十进制数
 */
int HttpRequest::convertHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return ch - '0';
}

/*
 * 返回解析请求首行得到的state_变量
 */
HttpRequest::PARSE_STATE HttpRequest::state() const
{
    return state_;
}

/*
 * 返回解析请求首行得到的path_变量
 */
std::string &HttpRequest::path()
{
    return path_;
}

/*
 * 上面函数的重载版本
 */
std::string HttpRequest::path() const
{
    return path_;
}

/*
 * 返回解析请求首行得到的method_变量
 */
std::string HttpRequest::method() const
{
    return method_;
}

/*
 * 返回解析请求首行得到的version_变量
 */
std::string HttpRequest::version() const
{
    return version_;
}

/*
 * 返回请求头中指定键对应的数据
 */
std::string HttpRequest::getPost(const std::string &key) const
{
    assert(key != "");
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

/*
 * 返回请求头中指定键对应的数据，重载版本
 */
std::string HttpRequest::getPost(const char *key) const
{
    assert(key != nullptr);
    if (post_.count(key) == 1)
    {
        return post_.find(key)->second;
    }
    return "";
}

/*
 * 使用正则库解析请求首行
 * GET请求的请求首行示例
 * GET /test.jpg HTTP/1.1
 * POST请求的请求首行示例
 * POST / HTTP1.1
 */
bool HttpRequest::parseRequestLine_(const std::string &line)
{
    // note: 正则表达式匹配
    // 指定匹配规则，^表示行开始，$表示行尾，[^ ]表示匹配非空格
    // 括号()括住的代表我们需要得到的字符串，最后会送入submatch中
    // subMatch[0]代表整个字符串的完整匹配结果，subMatch[1...]等代表()中各组匹配结果
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern))
    {
        // 解析请求首行获取需要的信息
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        // 切换到下一个状态，即解析请求头
        state_ = HEADER;
        return true;
    }
    // 没有匹配到说明请求首行格式错误，打印错误日志，返回false
    LOG_ERROR("RequestLine Error! %s", line.c_str());

    return false;
}

/*
 * 使用正则库解析请求头
 * 请求头由如下键值对组成，由 : 分割键值对
 * Host:test.baidu.com
 */
void HttpRequest::parseHeader_(const std::string &line)
{
    // 指定匹配规则，^表示行开始，[^:]表示匹配非冒号(:)
    // ?代表前面的（这里是空格）1次或0次
    // 括号()括住的代表我们需要得到的字符串，最后会送入submatch中
    // subMatch[0]代表整个字符串的完整匹配结果，subMatch[1...]等代表()中各组匹配结果
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern))
    {
        // 将解析出来的信息以键值对的形式存入unordered_map里面
        header_[subMatch[1]] = subMatch[2];
    }
    else
    {
        // 如果没有匹配到，说明进入到了请求头后的空行
        // 这里先默认设为解析请求体BODY状态，在调用此函数的函数外可判断method_来判断是POST还是GET请求
        // 从而转换状态为BODY（POST）还是FINISH（GET）
        state_ = BODY;
    }
}

/*
 * 解析请求体
 */
bool HttpRequest::parseBody_(const std::string &line)
{
    // 判断是否是上传文件，只进一次，upload_设为true后就不会再进if
    if (!upload_ && header_["Content-Type"].find("multipart/form-data") != std::string::npos)
    {
        upload_ = true;
        LOG_DEBUG("Upload!");
        int p = header_["Content-Type"].find("--");
        header_["boundary"] = header_["Content-Type"].substr(p);
    }
    // 将line赋值给类变量body_
    body_ = line;
    // 解析请求体行数++
    parseBodyCnt_++;
    // 打印读到的请求体日志信息
    if (!upload_)
    {
        LOG_DEBUG("UserInfo Body: Usr&Pwd(%s), len(%d)", line.c_str(), line.size());
    }
    else
    {
        LOG_DEBUG("File UpLoad Body: line%d(%s), len(%d)", parseBodyCnt_, line.c_str(), line.size());
    }
    // 调用ParsePost_函数解析POST中带的请求体数据
    // parsePost_()函数返回false代表请求体不完整，继续请求
    if (!parsePost_())
    {
        return false;
    }
    // 请求体完整，解析成功，将状态置为FINISH
    state_ = FINISH;
    return true;
}

/*
 * 解析请求体中的内容
 * 目前只能解析application/x-www-form-urlencoded此种格式，以后可以根据需要添加格式，比如json
 * 如果请求方式时POST Content-Type为application/x-www-form-urlencoded就可以解析，表示以键值对的数据格式提交
 * 使用POST的形式进行登录信息的传输
 */
bool HttpRequest::parsePost_()
{
    // 以后可以添加其他种类的Content-Type的支持
    // 登录业务
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        // 判断POST数据是否接受完整，未接收完则返回false，表示继续请求
        if (body_.size() < atol(header_["Content-Length"].c_str()))
        {
            return false;
        }
        // 将请求体中的内容解析到post_变量中
        parseFromUrlencoded_();
        // 用户是否请求的默认DEFAULT_HTML_TAG（登录与注册）网页
        if (DEFAULT_HTML_TAG.count(path_))
        {
            // 获取登录与注册对应的标识
            // 注意：const的map没有重载[]运算符，所以这里需要用find
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag: %d", tag);
            if (tag == 0 || tag == 1)
            {
                // 通过标识确定用户请求的是登录还是注册（0注册，1登陆）
                bool isLogin = (tag == 1);
                // 进行用户验证，数据库相应操作
                // 验证成功，进入下一步，设置为成功页面
                if (userVerify(post_["username"], post_["password"], isLogin))
                {
                    path_ = "/welcome.html";
                }
                // 登录验证失败，设置返回登录错误页面
                else if (isLogin)
                {
                    path_ = "/login_error.html";
                }
                // 注册验证失败，设置返回注册错误页面
                else
                {
                    path_ = "/register_error.html";
                }
            }
        }
    }
    // 上传文件业务
    else if (method_ == "POST" && upload_)
    {
        // 判断文件大小（Content-Length），超过30M直接返回错误
        if (strtoll(header_["Content-Length"].c_str(), nullptr, 10) > 30 * 1024 * 1024)
        {
            // 记录错误标志
            upload_error_ = true;
        }
        bool ret = parseFormData_();
        // 上传成功
        if (ret)
        {
            // 跳转失败页面
            if (upload_error_)
            {
                path_ = "/upload_error.html";
            }
            // 跳转成功页面
            else
            {
                path_ = "/success.html";
            }
            return true;
        }
        // 上传不完整
        else
        {
            return false;
        }
    }
    return true;
}

/*
 * 从请求体中的application/x-www-form-urlencoded类型信息提取信息
 * 示例：username=test&password=test
 */
void HttpRequest::parseFromUrlencoded_()
{
    // 请求体大小=0，直接返回
    int n = body_.size();
    if (n == 0)
    {
        return;
    }

    std::string key, value, temp;
    int num = 0;
    // 遍历整个请求体大小
    for (int i = 0; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            // 等号之前是key
            key = temp;
            temp.clear();
            break;
        case '+':
            // + 号改为 空格 ，因为浏览器会将空格编码为+号
            temp += ' ';
            break;
        case '%':
            // 浏览器会将非字母字符，encode成百分号+其ASCII码的十六进制
            // %后面跟的是十六进制码，将十六进制转化为10进制
            // 这里应该是转成ASCII码对应的字符，而不是转换成对应的十进制字符，况且转化后的十进制也只能局限在0-99范围内
            num = convertHex(body_[i + 1]) * 16 + convertHex(body_[i + 2]);
            // 根据ascii码转换为字符
            temp += static_cast<char>(num);
            // 向后移动两个位置
            i += 2;
            break;
        case '&':
            // &号前是val
            value = temp;
            temp.clear();
            // 添加键值对
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            temp += ch;
            break;
        }
    }
    // 获取最后一个键值对
    if (post_.count(key) == 0)
    {
        value = temp;
        post_[key] = value;
    }
}

/*
 * 从请求体中的multipart/form-data类型信息提取信息，用类似有限状态自动机，根据parseBodyCnt_和body头和body结尾判断结束
 * 示例：
 * ------WebKitFormBoundaryT2H3ppmKTEsin6D1
 * Content-Disposition: form-data; name="file"; filename="test.txt"
 * Content-Type: text/plain
 * 空行
 * 文件内容
 * ------WebKitFormBoundaryT2H3ppmKTEsin6D1--
 */
bool HttpRequest::parseFormData_()
{
    // 第2行是文件信息
    if (parseBodyCnt_ == 2)
    {
        // 取文件名
        auto p = body_.find("filename");
        // 取从"开始
        uploadFilename_ = body_.substr(p + 10);
        // 去掉后面的"
        uploadFilename_ = uploadFilename_.substr(0, uploadFilename_.size() - 1);
        // 取文件后缀
        p = uploadFilename_.find(".");
        // 未找到后缀，直接返回格式错误
        if (p == uploadFilename_.npos)
        {
            upload_error_ = true;
            return false;
        }
        std::string suffix = uploadFilename_.substr(p + 1);
        // 后缀不是txt
        if (suffix != "txt")
        {
            upload_error_ = true;
            return false;
        }
        // 文件格式验证通过
        if (!upload_error_)
        {
            // 增加路径前缀
            uploadFilename_ = uploadDir + uploadFilename_;
            // 打开文件
            // note: fopen和open有区别
            fp_ = fopen(uploadFilename_.c_str(), "w");
        }
        return false;
    }
    // 第5行以后都是文件内容，后面body_的判断是防止空文件
    else if (parseBodyCnt_ >= 5 && body_ != "--" + header_["boundary"] + "--")
    {
        if (!upload_error_)
        {
            // 将请求体的上传文件数据写入文件
            fputs(body_.c_str(), fp_);
            // 刷新缓冲区（若没有及时刷新，内核可能会延迟一段时间才将缓存区数据写到磁盘）
            fflush(fp_);
        }
        return false;
    }
    // 最后一行是结尾
    else if (body_ == "--" + header_["boundary"] + "--")
    {
        if (!upload_error_)
        {
            // 在关闭时fopen和open也有区别
            fclose(fp_);
        }
        return true;
    }
    // 其他情况继续读取，可能是空行等
    return false;
}

/*
 * 验证用户
 */
bool HttpRequest::userVerify(const std::string &name, const std::string &pwd, bool isLogin)
{
    // 密码或用户名为空，直接错误
    if (name == "" || pwd == "")
    {
        return false;
    }
    LOG_INFO("Verify Name:%s Password:%s", name.c_str(), pwd.c_str());

    // note: 用RAII获取一个MySQL连接
    MYSQL *sql;
    // SqlConnPool::instance()单例获取数据库连接池实例
    // RAII自动管理资源，析构自动释放
    SqlConnRAII sqlConnRaii(&sql, SqlConnPool::instance());
    assert(sql);

    // 初始化一系列相关变量
    bool flag = false;             // 最终返回值，密码是否通过验证（如果是注册，直接通过）
    unsigned int j = 0;            // 结果集中的列数（未使用）
    char order[256] = {0};         // 存储sql语句
    MYSQL_FIELD *fields = nullptr; // 结果集中的字段结构数组（未使用）
    MYSQL_RES *res = nullptr;      // sql查询结果

    // 如果是注册，那么将flag置为true
    if (!isLogin)
    {
        flag = true;
    }
    // 查询用户及密码
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    // 运行sql语句
    // 注意：mysql_query()成功返回0，失败返回非0
    if (mysql_query(sql, order))
    {
        // 释放所有关联的结果内存
        mysql_free_result(res);
        return false;
    }
    // 从表中检索完整的结果集
    // 如果mysql_store_result()查询未返回结果集，返回nullptr，比如INSERT
    res = mysql_store_result(sql);
    // 返回结果集中的列数
    j = mysql_num_fields(res);
    // 返回所有字段结构的数组
    fields = mysql_fetch_fields(res);

    // 从结果集中获取下一行
    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        // 登录验证
        if (isLogin)
        {
            // 密码正确
            if (pwd == password)
            {
                flag = true;
            }
            // 密码错误
            else
            {
                flag = false;
                LOG_DEBUG("Password Error!");
            }
        }
        // 注册，能查找到用户名，说明用户名重复
        else
        {
            flag = false;
            LOG_DEBUG("User Used!");
        }
    }
    mysql_free_result(res);

    // 注册行为 且 用户名未被使用
    if (!isLogin && flag)
    {
        LOG_DEBUG("Register!");
        // 重置sql语句
        bzero(order, 256);
        // 插入sql语句
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        // 插入数据库
        // 插入失败
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert Error!");
            flag = false;
        }
        // 插入成功，用户注册成功
        flag = true;
    }

    LOG_DEBUG("User Verify Success!");
    return flag;
}

/*
 * 将客户端传来的path变量添加完整，以目录结束的路径添加上默认页面
 * 在DEFAULT_HTML中的界面添加上文件后缀名
 */
void HttpRequest::parsePath_()
{
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

/*
 * 解析收到的http请求内容
 * 作者实现有点问题，需要改进，可以按照tinywebserver的解析方式进行改进
 */
HttpRequest::HTTP_CODE HttpRequest::parse(Buffer &buff)
{
    // 在请求头的每一行结尾都有\r\n
    const char CRLF[] = "\r\n";
    // note: 若没有内容可以被读取，不能直接返回false
    // 应该返回一个NO_REQUEST状态，调用函数接受到该状态之后重新修改epoll事件注册一个EPOLLIN事件继续读
    if (buff.readableBytes() <= 0)
    {
        return NO_REQUEST;
    }
    // 状态机方式解析http请求头，只要可读并且没有解析完成就继续循环
    // 在BODY状态，如果可读，说明在缓冲区中有多行数据
    // 如果不可读说明网络传输了一部分，此时跳出while循环，返回NO_REQUEST，重新注册EPOLLIN事件，等待接收剩余数据
    while (buff.readableBytes() && state_ != FINISH)
    {
        // 首先通过查找CRLF标志找到一行的结尾
        // note: search用于在序列A中查找序列B第一次出现的位置，如果未找到，返回last迭代器，也就是buff.beginWriteConst()
        // lineEnd指针会指向\r位置，也就是有效字符串的后一个位置
        // 当序列A中没有序列B中的字符时，会返回序列A的尾后指针
        // 这里用变量记录，方便switch后判断是否完整，即是否查到了CRLF
        const char *rdp = buff.peek();
        const char *wdp = buff.beginWriteConst();
        const char *lineEnd = std::search(rdp, wdp, CRLF, CRLF + 2);
        // 根据查找到的行尾的位置初始化一个行字符串
        std::string line(rdp, lineEnd);
        // 若解析状态停留在HEADER且没有CRLF作为结尾（未找到CRLF）
        // 直接退出循环，返回NO_REQUEST请求不完整，直到接收完整数据
        // 因为是header，如果不完整，无法解析，而body可以先接受一部分
        if (lineEnd == wdp && state_ == HEADER)
        {
            break;
        }
        // 开始解析
        switch (state_)
        {
        case REQUEST_LINE:
            // 首先解析请求首行
            if (!parseRequestLine_(line))
            {
                // 如果失败的话，返回BAD_REQUEST
                return BAD_REQUEST;
            }
            // 将客户端传来的path变量添加完整,目录加上默认页面，没有后缀的指定文件加上后缀
            parsePath_();
            // 这里break是退出switch，不是退出循环，要继续解析
            break;
        case HEADER:
            // 解析完请求首行之后，state_变量会被置为HEADERS，所以会跳到此处解析请求头部
            parseHeader_(line);
            // note: 因为可能会存在请求未接受完整的情况，所以后面可以改成根据method_判断是否跳转到FINISH阶段
            // 因为在ParseHeader_中请求头解析到空行时会将state_置为BODY
            // 所以这里的判断条件可以改为 if(state_==body && method_==GET){state_=FINISH;}
            // 这样就可以实现GET与POST的对应处理
            // 如果是POST请求，则将state_设置为BODY进入解析请求体的流程
            // 如果是GET请求，则将state_设置为FINISH状态结束解析
            if (state_ == BODY && method_ == "GET")
            {
                // 状态改为完成
                state_ = FINISH;
                // 读完所有，回收空间
                buff.retrieveAll();
                // 返回GET_REQUEST获取完整请求
                return GET_REQUEST;
            }
            // 同上，继续循环解析
            break;
        case BODY:
            // 解析完请求首行之后，若是POST请求则会进入解析请求体这一步
            // 如果请求体数据不完整，parseBody_()函数返回false
            if (!parseBody_(line))
            {
                // 请求体没有读完整，可能还在缓冲区中有多行数据，也可能网络传输了一部分，break出switch到while循环判断
                break;
            }
            // 完整请求体，回收空间
            buff.retrieveAll();
            // 返回GET_REQUEST获取完整请求
            return GET_REQUEST;
        default:
            // 默认返回INTERNAL_ERROR服务器内部错误，一般不会进
            return INTERNAL_ERROR;
        }
        // 移动读指针到下一行，跳过上一行的\r\n
        // 这里一定要注意，因为可能没找到\r\n，就不能跳过\r\n
        // 若解析状态停留在BODY且没有CRLF作为结尾（未找到CRLF）
        if (lineEnd == wdp && state_ == BODY)
        {
            buff.retrieveUntil(lineEnd);
        }
        // 读到了完整body，找打了\r\n
        else
        {
            buff.retrieveUntil(lineEnd + 2);
        }
    }
    // 打印请求首行日志信息
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    // note: 最后直接返回NO_REQUEST状态，表示如果执行到这一部分，说明请求没有接受完整，需要继续接受请求，若是请求完整的在之前就会return出while
    return NO_REQUEST;
}