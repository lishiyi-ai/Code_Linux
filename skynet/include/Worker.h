#ifndef WORKER_H
#define WORKER_H
#include <thread>
#include "Sunnet.h"
#include "Service.h"
#include <memory>
using namespace std;
class Sunnet;
class Service;
class Worker{
public:
    int id; // 编号
    int eachNum; // 每次处理多少条信息
    void operator()(); // 线程函数
    void checkAndPutGlobal(std::shared_ptr<Service> srv);
    
};


#endif