#ifndef MSG_H
#define MSG_H

#include<memory>
#include<string>
// 消息基类
class BaseMsg{
public:
    enum TYPE{
        SERVICE  = 1,
        SOCKET_ACCEPT = 2, 
        SOCKET_RW = 3,
    };
    unsigned int type; // 消息类型
    // char load[999999]{}; // 用于检测内存泄漏，仅用于调试
    virtual ~BaseMsg(){}; // 
};

class ServiceMsg : public BaseMsg{
public:
    unsigned source; // 消息发送方
    std::shared_ptr<std::string> buff; // 消息内容
    size_t size;
};
class SocketpAcceptMsg : public BaseMsg{
public:
    int listenfd;
    int clientfd;

};
class SocketRWMsg : public BaseMsg{
public:
    int fd;
    bool isRead = false;
    bool isWrite = false;
};

#endif