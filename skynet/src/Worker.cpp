#include <iostream>
#include <unistd.h>
#include "../include/Worker.h"
#include "../include/Service.h"
void Worker::operator()(){
    while(true){
        cout << "working id:" << id << endl;
        std::shared_ptr<Service> srv = Sunnet::inst->popGlobalQueue();
        if(!srv){
            cout << "wait id:" << id << endl;
            Sunnet::inst->workWait();
        }else{
            srv->processMsg(eachNum);
            checkAndPutGlobal(srv);
        }
    }
}
void Worker::checkAndPutGlobal(std::shared_ptr<Service> srv){
    // 退出中
    if(srv->isExiting){
        return;
    }
    std::unique_lock<mutex> lock(srv->locker);
    // 重新放回全局队列
    if(!srv->msgQueue.empty()){
        // 此时srv->inGlobal一定是true
        Sunnet::inst->pushGlobalQueue(srv);
    }else{
        srv->setInGlobal(false);
    }
}