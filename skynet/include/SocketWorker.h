#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H
#include <memory>
#include <sys/epoll.h>
#include "Sunnet.h"
#include "Conn.h"
using namespace std;
class SocketWorker{
private:
    // epoll描述符
    int epollfd;
public:
    void init(); // 初始化
    void operator()(); // 线程函数
public:
    void addEvent(int fd);
    void removeEvent(int fd);
    void modfifyEvent(int fd, bool epollout);
private:
    void onEvent(epoll_event ev);
    void onAccept(shared_ptr<Conn> conn);
    void onRW(shared_ptr<Conn> conn, bool r, bool w);
};

#endif