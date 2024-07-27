#ifndef SUNNET_H
#define SUNNET_H
#include <vector>
#include "Worker.h"
#include <unordered_map>
#include "Service.h"
#include <condition_variable>
#include "SocketWorker.h"
#include "Conn.h"
class Worker;
class Conn;
class SocketWorker;
class Sunnet{
public:
    // 单例
    static Sunnet* inst;
public:
    // 构造函数
    Sunnet();
    // 初始化并开始
    void start();
    // 等待运行
    void wait();
private:
    // 工作线程
    int WORKER_NUM = 8; // 工作线程数
    std::vector<Worker *> workers; // 工作对象
    std::vector<std::thread *> workerThreads; // 线程
    // 开启工作线程
    void startWorker();
public:
    // 服务列表
    std::unordered_map<unsigned int, std::shared_ptr<Service> > services;
    unsigned int maxId = 0; // 最大id
    std::mutex locker;
    // 增删服务
    unsigned int newService(std::shared_ptr<std::string> type);
    void killService(unsigned int id); // 仅限服务自己跳动
private:
    // 获取服务
    std::shared_ptr<Service> getService(unsigned int id);
private:
    // 全局队列
    std::queue<std::shared_ptr<Service> > globalQueue;
    int globalLen = 0;
    std::mutex globallocker;
public:
    // 发送消息
    void send(unsigned int toId, std::shared_ptr<BaseMsg> msg);
    // 全局队列操作
    std::shared_ptr<Service> popGlobalQueue();
    void pushGlobalQueue(std::shared_ptr<Service> srv); 
public:
    // 产生消息
    std::shared_ptr<BaseMsg> makeMsg(unsigned source, std::string buff, int len);
private:
    // 休眠和唤醒
    std::mutex sleeplocker;
    std::condition_variable sleepcond;
    int sleepcount = 0;
public:
    // 唤醒工作线程
    void checkAndWeakUp();
    // 让工作现场等待
    void workWait();
private:
    // Socket 线程
    SocketWorker* socketWorker;
    thread* socketThread;
private:
    // 开启Socket线程
    void startSocket();
public:
    // 增删查Conn
    int addConn(int fd, unsigned int id ,Conn::TYPE type);
    shared_ptr<Conn> getConn(int fd);
    bool removeConn(int fd);
private:
    // Conn列表
    unordered_map<unsigned int, shared_ptr<Conn> > conns;
    mutex connlocker;
public:
    // 网络连接操作接口
    int listento(unsigned int port, unsigned int serviceId);
    void closeConn(unsigned fd);
};


#endif