#ifndef CLIENT_H  //防止头文件重复包含的宏定义开始
#define CLIENT_H  //定义头文件保护宏

#include "global.h"  //包含全局头文件，引入所需的标准库和网络库

class Client{  //定义客户端类
    private:  //私有成员变量区域
        int client_fd; //客户端套接字文件描述符，用于与服务器通信
        const char* server_ip; //服务器IP地址字符串指针
        int server_port; //服务器端口号
        struct sockaddr_in server_addr; //服务器地址结构体，存储服务器网络地址信息
    public:  //公共成员函数区域
        Client(); //无参构造函数，初始化客户端套接字
        ~Client();  //析构函数，关闭客户端套接字
        int connect(const char* ip, int port); //连接服务器，成功返回0，错误返回-1
        void receive_messages(); //接收并打印服务器发送信息的函数
        void send_messages(); //从标准输入读取数据并发送给服务器的函数
};

#endif // CLIENT_H  //防止头文件重复包含的宏定义结束
