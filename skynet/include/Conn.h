#ifndef CONN_H
#define CONN_H
using namespace std;

class Conn{
public:
    enum TYPE{
        LISTEN = 1,
        CLIENT = 2,
    };
    unsigned int type;
    int fd;
    unsigned int serviceId;
};

#endif