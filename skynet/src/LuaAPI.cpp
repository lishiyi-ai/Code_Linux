#include "../include/LuaAPI.h"
#include "stdint.h"
#include "../include/Sunnet.h"
#include <unistd.h>
#include <string.h>
#include <iostream>

// 开启服务
int LuaAPI::NewService(lua_State *luaState){
    // 参数个数
    int num = lua_gettop(luaState); // 获取参数个数
    // 参数1:服务类型
    if(lua_isstring(luaState, 1) == 0){
        lua_pushinteger(luaState, -1);
        return 1;
    }
    size_t len = 0;
    const char *type = lua_tolstring(luaState, 1, &len);
    char *ns = new char[len + 1]; // 后面加 \0
    ns[len] = '\0';
    memcpy(ns, type, len);
    auto t = make_shared<string>(ns);
    // 处理
    uint32_t id = Sunnet::inst->newService(t);
    // 返回值
    lua_pushinteger(luaState, id);
    return 1;
}
// 注册lua模块
void LuaAPI::Register(lua_State *luaState){
    static luaL_Reg lualibs[] = {
        { "NewService", NewService },
        { "KillService", KillService },
        { "Send", Send },
        { "Listen", Listen },
        { "CloseConn", CloseConn },
        { "Write", Write },
        { NULL, NULL }
    };
    luaL_newlib (luaState, lualibs);
    lua_setglobal(luaState, "sunnet");
}
int LuaAPI::KillService(lua_State *luaState){
    // 参数
    int num = lua_gettop(luaState); // 获取参数个数
    if(lua_isinteger(luaState, 1) == 0){
        return 0;
    }int id = lua_tointeger(luaState, 1);
    // 处理
    Sunnet::inst->killService(id);
    // 返回值
    return 0;
}
// 发送消息
int LuaAPI::Send(lua_State *luaState){
    // 参数总数
    int num = lua_gettop(luaState);
    if(num != 3){
        cout << "Send fail, num err" << endl;
        return 0;
    }
    // 参数1
    if(lua_isinteger(luaState, 1) == 0){
        cout << "send fail arg1 err" << endl;
        return 0;
    }
    int source = lua_tointeger(luaState, 1);
    // 参数2
    if(lua_isinteger(luaState, 2) == 0){
        cout << "send fail arg2 err" << endl;
        return 0;
    }
    int toId = lua_tointeger(luaState, 2);
    // 参数3
    if(lua_isstring(luaState, 3) == 0){
        cout << "send fail arg3 err" << endl;
        return 0;
    }
    size_t len = 0;
    const char *buff = lua_tolstring(luaState, 3, &len);
    string *ns = new string(buff);
    // 处理
    auto msg = make_shared<ServiceMsg>();
    msg->type = BaseMsg::TYPE::SERVICE;
    msg->source = source;
    msg->buff = shared_ptr<string>(ns);
    msg->size = len;
    Sunnet::inst->send(toId, msg);
    //返回值
    return 0;
}
//开启网络监听
int LuaAPI::Listen(lua_State *luaState){
    //参数个数
    int num = lua_gettop(luaState);
    //参数1：端口
    if(lua_isinteger(luaState, 1) == 0) {
        lua_pushinteger(luaState, -1);
        return 1;
    }
    int port = lua_tointeger(luaState, 1);
    //参数2：服务Id
    if(lua_isinteger(luaState, 2) == 0) {
        lua_pushinteger(luaState, -1);
        return 1;
    }
    int id = lua_tointeger(luaState, 2);
    //处理
    int fd = Sunnet::inst->listento(port, id);
    //返回值
    lua_pushinteger(luaState, fd);
    return 1;
}

//关闭连接
int LuaAPI::CloseConn(lua_State *luaState){
    //参数个数
    int num = lua_gettop(luaState);
    //参数1：fd
    if(lua_isinteger(luaState, 1) == 0) {
        return 0;
    }
    int fd = lua_tointeger(luaState, 1);
    //处理
    Sunnet::inst->closeConn(fd);
    //返回值
    //（无）
    return 0;
}

//写套接字
int LuaAPI::Write(lua_State *luaState){
    //参数个数
    int num = lua_gettop(luaState);
    //参数1：fd
    if(lua_isinteger(luaState, 1) == 0) {
        lua_pushinteger(luaState, -1);
        return 1;
    }
    int fd = lua_tointeger(luaState, 1);
    //参数2：buff
    if(lua_isstring(luaState, 2) == 0){
        lua_pushinteger(luaState, -1);
        return 1;
    }
    size_t len = 0;
    const char *buff = lua_tolstring(luaState, 2, &len);
    //处理
    int r = write(fd, buff, len);
    //返回值
    lua_pushinteger(luaState, r);
    return 1;
}