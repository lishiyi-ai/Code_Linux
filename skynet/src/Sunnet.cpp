#include <iostream>
#include "../include/Sunnet.h"
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
// 单例
Sunnet *Sunnet::inst;
Sunnet::Sunnet(){
    inst = this;
}
//开启系统
void Sunnet::start(){
    cout << "hello sunnet" << endl;
    // 忽略SIGPIEPE信号
    signal(SIGPIPE, SIG_IGN);
    // 锁

    // 开启Worker
    startWorker();
    startSocket();
}
void Sunnet::startWorker(){
    for(int i = 0; i < WORKER_NUM; ++i){
        // 创建线程对象
        Worker* worker = new Worker();
        worker->id = i;
        worker->eachNum = 2 << i;
        // 创建线程
        thread *th = new thread(*worker);
        workers.push_back(worker);
        workerThreads.push_back(th);
    }
}
void Sunnet::wait(){
    if(workerThreads[0]){
        workerThreads[0]->join();
    }
}
// 新建服务
unsigned int Sunnet::newService(std::shared_ptr<std::string> type){
    auto srv = make_shared<Service>();
    srv->type = type;
    std::unique_lock<mutex> qlock(locker);
    srv->id = maxId;
    ++maxId;
    services.emplace(srv->id, srv);
    qlock.unlock();
    srv->onInit(); // 初始化
    return srv->id;
}
// 由id查找服务
std::shared_ptr<Service> Sunnet::getService(unsigned int id){
    std::shared_ptr<Service> srv = NULL;
    std::unique_lock<mutex> qlock(locker);
    auto iter = services.find(id);
    if(iter != services.end()){
        srv = iter->second;
    }
    qlock.unlock();
    return srv;
}
// 删除服务
// 只能service自己调自己，因为会调用不加锁的onExit 和 isExiting
void Sunnet::killService(unsigned id){
    std::shared_ptr srv = getService(id);
    if(!srv){
        return;
    }
    // 退出前
    srv->onExit();
    srv->isExiting = true;
    // 删列表
    std::unique_lock<mutex> lock(locker);
    services.erase(id);
}
//弹出全局队列
shared_ptr<Service> Sunnet::popGlobalQueue(){
    shared_ptr<Service> srv = NULL;
    std::unique_lock<mutex> glock(globallocker);
    if(!globalQueue.empty()){
        srv = globalQueue.front();
        globalQueue.pop();
        globalLen--;
    }
    glock.unlock();
    return srv;
}
// 插入全局队列
void Sunnet::pushGlobalQueue(std::shared_ptr<Service> srv){
    std::unique_lock<mutex> glock(globallocker);
    globalQueue.push(srv);
    globalLen++;
    glock.unlock();
}

std::shared_ptr<BaseMsg> Sunnet::makeMsg(unsigned int source, std::string buff, int len){
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    // 基本类型的对象没有析构
    // 所以delete或delete[]都可以销毁基本类型数组;
    // 智能指针默认使用delete销毁对象。
    // 所以无须重写智能指针地销毁方法
    msg->buff = std::shared_ptr<string> (new string(buff));
    msg->size = len ;
    return msg;
}
// Worker线程调用，进入休眠
void Sunnet::workWait(){
    std::unique_lock<mutex> slock(sleeplocker);
    sleepcount++;
    sleepcond.wait(slock);
    sleepcount--;
}
// 检测并唤醒线程
void Sunnet::checkAndWeakUp(){
    if(sleepcount == 0){
        return;
    }
    if(WORKER_NUM - sleepcount <= globalLen){
        cout << "weakup" << endl;
        sleepcond.notify_one();
    }
}
// 开启Socket线程
void Sunnet::startSocket(){
    // 创建线程对象
    socketWorker = new SocketWorker();
    // 初始化
    socketWorker->init();
    // 创建线程
    socketThread = new thread(*socketWorker);
}
// 添加链接
int Sunnet::addConn(int fd, unsigned int id, Conn::TYPE type){
    auto conn = make_shared<Conn>();
    conn->fd = fd;
    conn->serviceId = id; 
    conn->type = type;
    unique_lock clock(connlocker);
    conns.emplace(fd, conn);
    clock.unlock();
    return fd;
}
// 通过id查找连接
shared_ptr<Conn> Sunnet::getConn(int fd){
    shared_ptr<Conn> conn = NULL;
    unique_lock clock(connlocker);
    auto iter = conns.find(fd);
    if(iter != conns.end()){
        conn = iter->second;
    }
    clock.unlock();
    return conn;
}
//删除连接
bool Sunnet::removeConn(int fd){
    int res;
    unique_lock clock(connlocker);
    res = conns.erase(fd);
    clock.unlock();
    return res == 1;
}
int Sunnet::listento(unsigned int port, unsigned int serviceId){
    // 步骤1:创建socket;
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        cout << "listen error, listendfd <= 0" << endl;
        return -1;
    }
    // 步骤2:设置为非阻塞
    fcntl(listenfd, F_SETFL, O_NONBLOCK);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int r = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr));
    if(r < 0){
        cout << "listen error, bind fail" << endl;
        return -1;
    }
    // 步骤4:listen
    r = listen(listenfd, 64);
    if(r < 0){
        return -1;
    }
    // 步骤5:添加到管理结构
    addConn(listenfd, serviceId, Conn::TYPE::LISTEN);
    // 步骤6:epoll事件,跨线程
    socketWorker->addEvent(listenfd);
    return listenfd;
}

void Sunnet::closeConn(unsigned int fd){
    // 删除conn对象
    bool succ = removeConn(fd);
    // 关闭套接字
    close(fd);
    // 删除epoll对象对套接字的监听(跨线程)
    if(succ){
        socketWorker->removeEvent(fd);
    }
}