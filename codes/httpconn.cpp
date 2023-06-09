#include "../headers/httpconn.h"

// 静态变量
const char *HttpConn::srcDir;         // 资源目录
std::atomic<int> HttpConn::userCount; // 连接数
bool HttpConn::isET;                  // 工作模式

/*
 * 构造函数中赋初值
 */
HttpConn::HttpConn() : fd_(-1), isClose_(true)
{
    addr_ = {0};
}

/*
 * 为了确保资源被释放，所以在析构函数里调用close释放所有资源
 */
HttpConn::~HttpConn()
{
    close();
}

/*
 * 根据文件描述符与socket地址初始化httpconn类实例
 */
void HttpConn::init(int sockfd, const sockaddr_in &addr)
{
    assert(sockfd > 0);
    // 用户数量++
    userCount++;
    addr_ = addr;
    fd_ = sockfd;

    // 初始化读写缓冲区以及标志httpconn是否开启的变量
    writeBuff_.retrieveAll();
    readBuff_.retrieveAll();
    isClose_ = false;

    LOG_INFO("Client[%d](%s:%d) In, UserCount: %d", sockfd, getIP(), getPort(), (int)userCount);
}

/*
 * 关闭该socket的连接，取消文件映射，关闭文件描述符
 */
void HttpConn::close()
{
    // 解除内存映射
    response_.unmapFile();
    if (!isClose_)
    {
        isClose_ = true;
        userCount--;
        // 使用全局作用域下的close函数，而不是自己类中这个
        ::close(fd_);

        LOG_INFO("Client[%d](%s:%d) Quit, UserCount: %d", fd_, getIP(), getPort(), (int)userCount)
    }
}

/*
 * 返回还需要写多少字节的数据
 */
int HttpConn::toWriteBytes()
{
    return iov_[0].iov_len + iov_[1].iov_len;
}

/*
 * 返回连接状态是否为长连接
 */
bool HttpConn::isKeepAlive() const
{
    return request_.isKeepAlive();
}

/*
 * 获取该socket对应的文件描述符的函数
 */
int HttpConn::getFd() const
{
    return fd_;
}

/*
 * 获取该socket对应的地址的函数
 */
struct sockaddr_in HttpConn::getAddr() const
{
    return addr_;
}

/*
 * 获取该socket对应的IP地址的函数
 */
const char *HttpConn::getIP() const
{
    return inet_ntoa(addr_.sin_addr);
}

/*
 * 获取该socket对应的端口的函数
 */
int HttpConn::getPort() const
{
    return addr_.sin_port;
}

/*
 * 读取socket中的数据，保存到读缓冲区中
 */
ssize_t HttpConn::read(int *saveErrno)
{
    ssize_t len = -1;
    // 如果是LT模式，那么只读取一次，如果是ET模式，会一直读取，直到读不出数据
    do
    {
        len = readBuff_.readFd(fd_, saveErrno);
        if (len <= 0)
        {
            break;
        }
    } while (isET);

    return len;
}

/*
 * 使用writev方法将数据发送到指定socket中
 * writev函数用于在一次函数调用中写多个非连续缓冲区，有时也将这该函数称为聚集写
 */
ssize_t HttpConn::write(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        // note: 聚集写：写多个非连续缓冲区
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0)
        {
            // 记录信号返回给调用函数
            *saveErrno = errno;
            break;
        }
        // 两个传输位置长度都为0，表示传输结束
        if (iov_[0].iov_len + iov_[1].iov_len == 0)
        {
            break;
        }
        // 如果发送的数据长度大于iov_[0].iov_len，说明第一块区域的数据是发送完毕
        // 再根据多发送了多少调整iov_[1]的取值
        else if (static_cast<size_t>(len) > iov_[0].iov_len)
        {
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            // 区域一的数据发送完毕，回收缓冲空间，并将长度归0
            if (iov_[0].iov_len)
            {
                writeBuff_.retrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        // 没有传输完区域一的内容那么就继续传区域一的
        else
        {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            // 回收对应长度的空间
            writeBuff_.retrieve(len);
        }
        // 任需发送的数据大于10240的情况下会继续发送 或 若在LT模式下，只会发送一次，ET模式下会一直发送，直到完毕
        // 这里有点问题，如果是LT模式，且需要写的数据小于等于10240，那么连接会被之间关闭，这里可以选择在调用函数处修改判断条件
        // 或者直接将这里改为ToWriteBytes()>0
    } while (toWriteBytes() > 0);

    return len;
}

/*
 * 解析http请求数据
 * 本函数实际上时有bug的，不具备处理非完整请求的能力
 * 因为request_.parse(readBuff_)返回false可能是因为请求不完整，需要继续读取http请求数据
 * 但在此函数中却直接返回了400
 * 修改方案：request_.parse()修改为返回解析状态，然后本函数根据解析状态返回true or false
 */
bool HttpConn::process()
{
    // 首先初始化一个httprequest类对象，负责处理请求的事务
    // 这里不需要每一次都进行初始化，需要保存上次连接的状态，用以指示是否为新的http请求
    // 所以只有当上一次的请求为完成状态时，才回去重新初始化request_成员，以重从开始解析一个http请求
    if (request_.state() == HttpRequest::FINISH)
    {
        request_.init();
    }
    // 检查读缓存区中是否存在可读数据
    if (readBuff_.readableBytes() <= 0)
    {
        // 小于等于0表示没有数据可读，直接返回false
        return false;
    }
    // 解析readBuff_中读到的HTTP请求
    HttpRequest::HTTP_CODE processStatus = request_.parse(readBuff_);
    // 解析结果为GET_REQUEST获取了完整请求，解析完成，进入回复请求阶段
    if (processStatus == HttpRequest::GET_REQUEST)
    {
        // 打印解析的请求路径日志
        LOG_DEBUG("request path %s", request_.path().c_str());
        // 初始化一个200 OK的httpresponse对象，包含请求文件路径等信息，负责http应答阶段
        response_.init(srcDir, request_.path(), request_.isKeepAlive(), 200);
    }
    // 解析结果为解析结果为GET_REQUEST请求不完整，应该继续读取请求
    // 返回false通知调用者继续使用epoll监听该连接上的EPOLLIN读事件
    else if (processStatus == HttpRequest::NO_REQUEST)
    {
        return false;
    }
    // 其他情况表示解析失败，初始化一个400错误的httpresponse对象
    else
    {
        response_.init(srcDir, request_.path(), false, 400);
    }
    // httpresponse负责拼装返回的头部以及需要发送的文件
    // 注意这里响应数据要存在writeBuff_中，供后续写事件使用，而不是在readBuff_
    response_.makeResponse(writeBuff_);

    // 响应头（写缓冲区writeBuff_）
    // 将写缓冲区赋值给iov_，后面使用writev函数发送至客户端
    iov_[0].iov_base = const_cast<char *>(writeBuff_.peek());
    iov_[0].iov_len = writeBuff_.readableBytes();
    iovCnt_ = 1;

    // 响应体（文件内存映射）
    // 如果需要返回服务器的文件内容，且文件内容不为空
    if (response_.fileLen() > 0 && response_.file())
    {
        // 将文件映射到内存后的地址以及文件长度赋值给iov_变量
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    // 打印响应文件信息日志
    LOG_DEBUG("filesize: %d, %d to %d", response_.fileLen(), iovCnt_, toWriteBytes());

    return true;
}
