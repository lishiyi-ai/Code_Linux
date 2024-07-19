#include "include/mysqlpool.h"

mysqlpool::mysqlpool(){
    _Curconn = 0;
    _Freeconn = 0;
}

mysqlpool *mysqlpool::getInstance(){
    static mysqlpool mysqlPool;
    return &mysqlPool;
}

void mysqlpool::init(std::string host, 
                    std::string user, 
                    std::string pwd,
                    std::string db,
                    unsigned int port,
                    std::string socket,
                    unsigned long client_flag,
                    unsigned int Maxconn){
    _host = host;
    _user = user;
    _pwd = pwd;
    _db = db;
    _port = port;
    _socket = socket;
    _client_flag = client_flag;
    _Maxconn = Maxconn;

    for(int i = 0; i < _Maxconn; ++i){
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con == NULL){
            std::cout << "MySql Error";
            exit(1);
        }
        con = mysql_real_connect(con, _host.c_str(), _user.c_str(), _pwd.c_str(), 
        _db.c_str(), _port, NULL, _client_flag);
        if (con == NULL)
		{
			std::cout << "MySQL Error";
			exit(1);
		}
        pool_push(con);
        ++_Freeconn;
    }          
}

MYSQL *mysqlpool::poolFront(){
    return _connList.front();
}

void mysqlpool::poolPop(){
    _connList.pop_front();
}

bool mysqlpool::isEmpty(){
    return _connList.size() == 0;
}

MYSQL *mysqlpool::getConnect(){
    MYSQL *con = NULL;

    if(isEmpty()){
        return NULL;
    }

    std::unique_lock<std::mutex> lock(_poollock);
    if(0 == _Freeconn) return NULL;
    con = poolFront();
    poolPop();
    ++_Curconn;
    --_Freeconn;
    return con;
}

void mysqlpool::pool_push(MYSQL *con){
    _connList.push_back(con);
}

bool mysqlpool::releaseConnect(MYSQL *con){
    if(NULL == con){
        return false;
    }
    
    std::unique_lock<std::mutex> lock(_poollock);

    pool_push(con);
    ++_Freeconn;
    --_Curconn;

    return true;
}

void mysqlpool::destroyPool(){
    std::unique_lock<std::mutex> lock(_poollock);

    if(!isEmpty()){
        std::list<MYSQL *>::iterator it;
        for(it = _connList.begin(); it != _connList.end(); ++it){
            MYSQL *con = *it;
            mysql_close(con);
        }
        _Curconn = 0;
        _Freeconn = 0;
        _connList.clear();
    }
}

mysqlpool::~mysqlpool(){
    destroyPool();
}