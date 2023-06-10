/*
 * @Copyright: Copyright (c) 2022 WangXingyu All Rights Reserved.
 * @Description:
 * @Version:
 * @Author: WangXingyu
 * @Date: 2023-05-30 18:15:26
 * @LastEditors: WangXingyu
 * @LastEditTime: 2023-06-10 14:11:26
 */
#ifndef SQL_CONN_RAII_H
#define SQL_CONN_RAII_H

#include "sqlconnpoll.h"

/*
 * RAII：资源在对象构造初始化 资源在对象析构时释放
 */
class SqlConnRAII
{
public:
    /*
     * 构造函数，获取数据库连接池单例对象，从数据库连接池获取MySQL连接
     * 因为这里需要修改指针的值，所以需要传入二级指针，其实这里可以用指针的引用接收，这样只需要传入指针即可
     */
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool)
    {
        assert(connpool);
        *sql = connpool->getConn();
        sql_ = *sql;
        connpool_ = connpool;
    }

    /*
     * 析构函数，释放MySQL连接，重新放回数据库连接池
     */
    ~SqlConnRAII()
    {
        if (sql_)
        {
            connpool_->freeConn(sql_);
        }
    }

private:
    MYSQL *sql_;            // MySQL连接
    SqlConnPool *connpool_; // 数据库连接池（单例，只有一个对象）
};

#endif // SQL_CONN_RAII_H
