#include "../include/SocketWorker.h"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <assert.h>
using namespace std;
// 初始化
void SocketWorker::init(){
    cout << "SocketWorker init" << endl;
    // 创建epoll对象
    epollfd = epoll_create(1024); // 返回值:非负数表示成功创建的epoll对象的描述符,-1表示创建失败
    assert(epollfd > 0);
}
// 注意跨线程调用
void SocketWorker::addEvent(int fd){
    cout << "addEvent fd" << fd << endl;
    // 添加到epoll对象
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = fd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1){
        cout << "addEvent epoll_ctl fail:" << strerror(errno) << endl;
    }
}
// 跨线程调用
void SocketWorker::removeEvent(int fd){
    cout << "removeEvent fd " << fd << endl;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}
// 跨线程调用
void SocketWorker::modfifyEvent(int fd, bool epollout){
    cout << "modifyEvent fd" << fd << " " << epollout << endl;
    struct epoll_event ev;
    ev.data.fd = fd;
    if(epollout){
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    }else{
        ev.events = EPOLLIN | EPOLLET;
    }
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}
void SocketWorker::onEvent(epoll_event ev){
    cout << "onEvent" << endl;
    int fd = ev.data.fd;
    auto conn = Sunnet::inst->getConn(fd);
    if(conn == NULL){
        cout << "onEvent error, conn == NULL" << endl;
        return;
    }
    // 事件类
    bool isRead = ev.events & EPOLLIN;
    bool isWrite = ev.events & EPOLLOUT;
    bool isError = ev.events & EPOLLERR;
    // 监听socket
    if(conn->type == Conn::TYPE::LISTEN){
        if(isRead){
            cout << "accept link" << endl;
            onAccept(conn);
        }
    }
    // 普通socket
    else{
        if(isRead || isWrite){
            cout << "read or write" << endl;
            onRW(conn, isRead, isWrite);
        }
        if(isError){
            cout << "onError fd:" << conn->fd << endl;
        }
    }
}
void SocketWorker::operator()(){
    while(true){
        //阻塞等待
        const int EVENT_SIZE = 64;
        struct epoll_event events[EVENT_SIZE];
        int eventCount = epoll_wait(epollfd, events, EVENT_SIZE, -1);
        // 取得事件
        for(int i = 0; i < eventCount; ++i){
            epoll_event ev = events[i];
            onEvent(ev);
        }
    }
}
void SocketWorker::onAccept(shared_ptr<Conn> conn){
    cout << "onAccept fd:" << conn->fd << endl;
    // 步骤一:accept
    int clientfd = accept(conn->fd, NULL, NULL);
    if(clientfd < 0){
        cout << "accept error" << endl;
    }
    // 步骤二:设置非阻塞
    fcntl(clientfd, F_SETFL, O_NONBLOCK);
    //写缓冲区大小
    unsigned long buffsize = 4294967295;
    if(setsockopt(clientfd, SOL_SOCKET, SO_SNDBUFFORCE, &buffsize, sizeof(buffsize)));
    // 步骤三:添加连接对象
    Sunnet::inst->addConn(clientfd, conn->serviceId, Conn::TYPE::CLIENT);
    // 步骤四:添加到epoll监听列表
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = clientfd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev) == -1){
        cout << "onAccept epoll_ctl fail:" << strerror(errno) << endl;
    }
    // 步骤五:通知服务
    auto msg = make_shared<SocketpAcceptMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_ACCEPT;
    msg->listenfd = conn->fd;
    msg->clientfd = clientfd;
    Sunnet::inst->send(conn->serviceId, msg);
}

void SocketWorker::onRW(shared_ptr<Conn> conn, bool r, bool w){
    cout << "onRW fd:" << conn->fd << endl;
    auto msg = make_shared<SocketRWMsg>();
    msg->type = BaseMsg::TYPE::SOCKET_RW;
    msg->fd = conn->fd;
    msg->isRead = r;
    msg->isWrite = w;
    Sunnet::inst->send(conn->serviceId, msg);
}