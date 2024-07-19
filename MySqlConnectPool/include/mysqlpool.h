#ifndef mysqlpool_H
#define mysqlpool_h

#include <iostream>
#include <mysql/mysql.h>
#include <queue>
#include <map>
#include <vector>
#include <utility>
#include <string>
#include <mutex>
#include <thread>
#include <list>

class mysqlpool{
    public:
        
        bool releaseConnect(MYSQL *conn); // 释放连接
        MYSQL *getConnect(); // 获取连接
        void destroyPool(); // 销毁所有连接。
        void init(std::string host, 
                    std::string user, 
                    std::string pwd,
                    std::string db,
                    unsigned int port,
                    std::string socket,
                    unsigned long client_flag,
                    unsigned int Maxconn);
        int getFreeconn(){
            return this->_Freeconn;
        }
        static mysqlpool *getInstance(); // 单例模式,懒汉模式
    private:
        mysqlpool();
        virtual ~mysqlpool();
        void pool_push(MYSQL* conn);
        bool isEmpty();
        MYSQL* poolFront();
        unsigned int poolSize();
        void poolPop();

        unsigned int _Maxconn; // 最大连接数
        unsigned int _Curconn; // 当前连结数
        unsigned int _Freeconn; // 空闲连结数
        std::mutex _poollock; // 连接池锁
        std::list<MYSQL *> _connList; // 
    private:
        std::string _host; // 主机地址
        std::string _user; // 用户名
        std::string _pwd; // 密码
        std::string _db; // 数据库
        unsigned int _port; // 端口号
        std::string _socket; //可以设置成Socket or Pipeline，通常设置为NULL
        unsigned long _client_flag; // 设置为0
}; 


#endif