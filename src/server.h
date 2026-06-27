#ifndef SERVER_H
#define SERVER_H

#include "global.h"

using namespace std;

class Server{
    private:
        int server_fd; //服务器套接字
        vector<int> client_fds; //客户端套接字列表
        struct sockaddr_in server_addr; //服务器地址结构体
        const char* ip; //服务器ip
        int port;  //服务器端口
    public:
        Server(const char* ip,int port); //带参构造函数，使用指定ip和端口
        ~Server();  //析构函数，关闭服务器套接字
        int run(); //监听客户端连接并处理消息
        void handle_client(int client_fd); //处理客户端消息的函数
};

#endif // SERVER_H