#ifndef SERVER_H  //防止头文件重复包含的宏定义开始
#define SERVER_H  //定义头文件保护宏

#include "global.h"  //包含全局头文件，引入所需的标准库和网络库

using namespace std;  //使用std命名空间，简化标准库的使用

class Server{  //定义服务器类
    private:  //私有成员变量区域
        int server_fd; //服务器套接字文件描述符，用于监听客户端连接
        vector<int> client_fds; //客户端套接字列表，存储所有已连接客户端的文件描述符
        struct sockaddr_in server_addr; //服务器地址结构体，存储服务器网络地址信息
        const char* ip; //服务器IP地址字符串指针
        int port;  //服务器端口号
    public:  //公共成员函数区域
        Server(const char* ip,int port); //带参构造函数，使用指定ip和端口初始化服务器
        ~Server();  //析构函数，关闭服务器和所有客户端套接字
        int run(); //监听客户端连接并处理消息，成功返回0，错误返回-1
        void handle_client(int client_fd); //处理单个客户端消息的函数，参数为客户端套接字
};

#endif // SERVER_H  //防止头文件重复包含的宏定义结束
