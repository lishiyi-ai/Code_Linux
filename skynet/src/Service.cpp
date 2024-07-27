#include "../include/Service.h"
#include "../include/Sunnet.h"
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include "../include/LuaAPI.h"
using namespace std;
// 构造函数
Service::Service(){
    // 初始化锁
}

// 析构函数
Service::~Service(){
    // 销毁锁
}

// 插入消息
void Service::pushMsg(std::shared_ptr<BaseMsg> msg){
    std::unique_lock<mutex> lock(locker);
    msgQueue.push(msg);
}

// 取出消息
std::shared_ptr<BaseMsg> Service::popMsg(){
    shared_ptr<BaseMsg> msg = NULL;
    std::unique_lock<mutex> mlock(locker);
    if(!msgQueue.empty()){
        msg = msgQueue.front();
        msgQueue.pop();
    }
    mlock.unlock();
    return msg;
}

// 创建服务后触发
void Service::onInit(){
    cout << "[" << id  << "] onInit" << endl;
    // 开启监听
    if( *type == "main"){
        Sunnet::inst->listento(8080, id);
    }
    //新建lua虚拟机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);
    // 注册Sunnet系统API
    LuaAPI::Register(luaState);
    // 执行lua文件
    string filename = "./service/" + *type + "/init.lua";
    int isok = luaL_dofile(luaState, filename.data());
    if(isok != 0){
        cout << "run lua fail" << lua_tostring(luaState, -1) << endl;
    }
    // 调用lua函数
    lua_getglobal(luaState, "OnInit");
    lua_pushinteger(luaState, id);
    isok = lua_pcall(luaState, 1, 0, 0);
    if(isok != 0){
        cout << "call lua OnInit fail" << lua_tostring(luaState, -1);
    }
}
// 收到消息后触发
void Service::onMsg(std::shared_ptr<BaseMsg> msg){
    cout << "[" << id  << "] onMsg" << endl;
    // SERVICE
    if(msg->type == BaseMsg::TYPE::SERVICE){
        auto m = dynamic_pointer_cast<ServiceMsg>(msg);
        onServiceMsg(m);
    }
    // SOCKET_ACCEPT
    if(msg->type == BaseMsg::TYPE::SOCKET_ACCEPT){
        auto m = dynamic_pointer_cast<SocketpAcceptMsg>(msg);
        cout << "new conn" << m->clientfd << endl;
        onAcceptMsg(m);
    }
    // SOCKET_RW
    if(msg->type == BaseMsg::TYPE::SOCKET_RW){
        auto m = dynamic_pointer_cast<SocketRWMsg>(msg);
        onRWMsg(m);
    }
}
// 退出服务时触发
void Service::onExit(){
    cout << "[" << id  << "] onExit" << endl;
    // 调用lua函数
    lua_getglobal(luaState, "OnExit");
    int isok = lua_pcall(luaState, 0, 0, 0);
    if(isok != 0){
        cout << "call lua OnExit fail" << lua_tostring(luaState, -1) << endl;
    }
    // 关闭lua虚拟机
    lua_close(luaState);
}
// 处理一条消息，返回值代表是否处理
bool Service::processMsg(){
    std::shared_ptr<BaseMsg> msg = popMsg();
    if(msg){
        onMsg(msg);
        return true;
    }else{
        return false; // 返回值预示着队列是否为空
    }
}
// 处理N条信息，返回值代表是否处理
void Service::processMsg(int max){
    for(int i = 0; i < max; ++i){
        bool succ = processMsg();
        if(!succ){
            break;
        }
    }
}
void Service::setInGlobal(bool isIn){
    std::unique_lock lock(inGloballocker);
    inGlobal = isIn;
}
// 发送消息
void Sunnet::send(unsigned int toId, std::shared_ptr<BaseMsg> msg){
    std::shared_ptr<Service> toSrv = getService(toId);
    if(!toSrv){
        cout << "Send fail, toSrv not exist toId:" << toId << endl;
        return;
    }
    // 插入目标服务地消息队列
    toSrv->pushMsg(msg);
    // 检查并放入全局队列
    bool hasPush = false;
    std::unique_lock<mutex> inlock(toSrv->inGloballocker);
    if(!toSrv->inGlobal){
        pushGlobalQueue(toSrv);
        toSrv->inGlobal = true;
        hasPush = true;
    }
    if(hasPush){
        // 唤醒进程
        checkAndWeakUp();
    }
    inlock.unlock();
}
// 收到其他服务发来的消息
void Service::onServiceMsg(shared_ptr<ServiceMsg> msg){
    cout << "onServiceMsg" << endl;
    // 调用lua函数
    lua_getglobal(luaState, "OnServiceMsg");
    lua_pushinteger(luaState, msg->source);
    lua_pushlstring(luaState, (*msg->buff).c_str(), msg->size);
    int isok = lua_pcall(luaState, 2, 0, 0);
    if(isok != 0){
        cout << "call lua OnserviceMsg fail" << lua_tostring(luaState, -1) 
        << endl;
    }
}
// 新连接
void Service::onAcceptMsg(shared_ptr<SocketpAcceptMsg> msg){
    cout << "onAcceptMsg" << msg->clientfd << endl;
    //调用Lua函数
    lua_getglobal(luaState, "OnAcceptMsg"); 
    lua_pushinteger(luaState, msg->listenfd); 
    lua_pushinteger(luaState, msg->clientfd); 
    int isok = lua_pcall(luaState, 2, 0, 0);
    if(isok != 0){ //成功返回值为0，否则代表失败.
         cout << "call lua OnAcceptMsg fail " << lua_tostring(luaState, -1) << endl;
    }

}
// 套接字可读可写
void Service::onRWMsg(shared_ptr<SocketRWMsg> msg){
    int fd = msg->fd;
    // 可读
    if(msg->isRead){
        const int BUFFSIZE = 512;
        char buff[BUFFSIZE];
        int len = 0;
        do{
            len = read(fd, &buff, BUFFSIZE);
            if(len > 0){
                onSocketData(fd, buff, len);
            }
        }while(len == BUFFSIZE);
        cout << "read finish" << endl;

        if(len <= 0&& errno != EAGAIN){
            if(Sunnet::inst->getConn(fd)){
                onSocketClose(fd);
                Sunnet::inst->closeConn(fd);
            }
        }
    }
    // 可写
    if(msg->isWrite){
        if(Sunnet::inst->getConn(fd)){
            onSocketWritable(fd);
        }
    }
}
// 收到客户端数据
void Service::onSocketData(int fd, const char* buff, int len){
    cout << "onSocketData" << fd << "buff:" << buff << endl;
    //echo;
    string tmp = "HTTP/1.1 200 OK\r\nConnection:keep-alive\r\nContent-Length:6\r\n\r\n123456";
    struct iovec iov;
    char *ch = (char *)tmp.c_str();
    iov.iov_base = ch;
    iov.iov_len = tmp.size();
    int has_size = 0;
    while(iov.iov_len > 0){
        cout << "onSocketData" << endl;
        int len = writev(fd, &iov, 1);
        iov.iov_base =ch + len;
        iov.iov_len -=len;
        cout << iov.iov_len << endl;
    }
/*
    //调用Lua函数
    lua_getglobal(luaState, "OnSocketData"); 
    lua_pushinteger(luaState, fd); 
    lua_pushlstring(luaState, buff,len); 
    int isok = lua_pcall(luaState, 2, 0, 0);
    if(isok != 0){ //成功返回值为0，否则代表失败.
         cout << "call lua OnSocketData fail " << lua_tostring(luaState, -1) << endl;
    }
*/
}
// 套接字可写
void Service::onSocketWritable(int fd){
    cout << "onSocketWriteable" << fd << endl;
}    
// 关闭连接前
void Service::onSocketClose(int fd){
    cout << "onSocketClose" << fd << endl;
    //调用Lua函数
    lua_getglobal(luaState, "OnSocketClose"); 
    lua_pushinteger(luaState, fd); 
    int isok = lua_pcall(luaState, 1, 0, 0);
    if(isok != 0){ //成功返回值为0，否则代表失败.
         cout << "call lua OnSocketClose fail " << lua_tostring(luaState, -1) << endl;
    }
}
