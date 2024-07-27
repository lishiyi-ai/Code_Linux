#ifndef SERVICE_H
#define SERVICE_H
extern "C"{
    #include "../3rd/lua-5.4.7/src/lua.h"
    #include "../3rd/lua-5.4.7/src/lauxlib.h"
    #include "../3rd/lua-5.4.7/src/lualib.h"
}
#include <queue>
#include <thread>
#include <mutex>
#include <memory>
#include "Msg.h"
using namespace std;
class Service{
public:
    // 唯一id
    unsigned int id;
    // 类型
    std::shared_ptr<std::string> type;
    //是否正在推出
    bool isExiting  = false;
    // 消息队列和锁
    std::queue<std::shared_ptr<BaseMsg> > msgQueue;
    std::mutex locker;
public:
    // 构造函数和析构函数
    Service();
    virtual ~Service();
    //回调函数(编写服务逻辑)
    void onInit();
    void onMsg(std::shared_ptr<BaseMsg> msg);
    void onExit();
    // 插入消息
    void pushMsg(std::shared_ptr<BaseMsg> msg);
    // 执行消息
    bool processMsg();
    void processMsg(int max);
private:
    // 取出消息
    std::shared_ptr<BaseMsg> popMsg();
public:
    // 标记是否在全局队列，true:表示在队列中，或争财叔
    bool inGlobal = false;
    std::mutex inGloballocker;
    // 线程安全地设置
    void setInGlobal(bool isIn);
private:
    // 消息处理方法
    void onServiceMsg(shared_ptr<ServiceMsg> msg);
    void onAcceptMsg(shared_ptr<SocketpAcceptMsg> msg);
    void onRWMsg(shared_ptr<SocketRWMsg> msg);
    void onSocketData(int fd, const char* buff, int len);
    void onSocketWritable(int fd);
    void onSocketClose(int fd);
private:
    // lua虚拟机
    lua_State *luaState;
};

#endif