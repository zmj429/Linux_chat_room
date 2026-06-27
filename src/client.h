#ifndef CLIENT_H
#define CLIENT_H

#include "global.h"

class Client{
    private:
        int client_fd; //客户端套接字
        const char* server_ip; //服务器ip
        int server_port; //服务器端口
        struct sockaddr_in server_addr; //服务器地址结构体
    public:
        Client(); //无参构造函数
        ~Client();  //析构函数，关闭客户端套接字
        int connect(const char* ip, int port); //连接服务器，成功返回0，错误返回-1
        void receive_messages(); //接收并打印信息的函数
        void send_messages(); //输入并发送信息的函数
};

#endif // CLIENT_H