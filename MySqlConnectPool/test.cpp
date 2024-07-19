#include "include/mysqlpool.h"

int main(){
    mysqlpool *pool = mysqlpool::getInstance();
    pool->init("localhost", "root","123456", "webuserdb", 3306,
    "", 0, 8);
    MYSQL *con = pool->getConnect();
    if (con == NULL){
        std::cout << "bad";
    }
    mysql_query(con, "SELECT username,passwd FROM user");
    // 从表中检索万在的完整的结果集
    MYSQL_RES *result= mysql_store_result(con);
    // 返回结果集中的列数
    int num_fields = mysql_num_fields(result);
    // 返回所有字段结构数组
    MYSQL_FIELD *fields = mysql_fetch_field(result);
    // 从结果集中获取下一行，将对应的用户名和密码存入map中
    while(MYSQL_ROW row = mysql_fetch_row(result)){
        std::string tmp1(row[0]);
        std::string tmp2(row[1]);
        std::cout << tmp1 << " " << tmp2 << std::endl;
    }
    std::cout << pool->getFreeconn() << std::endl;
    pool->releaseConnect(con);
    std::cout << pool->getFreeconn();
    return 0;
}
