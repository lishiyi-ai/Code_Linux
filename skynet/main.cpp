#include "./include/Msg.h"
#include "./include/Service.h"
#include "./include/Sunnet.h"
#include "./include/Worker.h"
#include <iostream>
#include <unistd.h>
using namespace std;
int main(){
    new Sunnet();
    Sunnet::inst->start();
    // 启动main服务
    auto t = make_shared<string>("main");
    Sunnet::inst->newService(t);
    // 等待
    Sunnet::inst->wait();
    return 0;
}