#include "global.h"  //包含全局头文件，引入所需的标准库和网络库
#include "client.h"  //包含客户端头文件，引入Client类的定义

#define SERVER_DEFAULT_IP "127.0.0.1"  //定义服务器默认IP地址为本地回环地址
#define SERVER_DEFAULT_PORT 8080  //定义服务器默认端口号为8080

using namespace std;  //使用std命名空间，简化标准库的使用

Client::Client(){  //Client类的无参构造函数
    //创建客户端套接字，AF_INET表示IPv4协议，SOCK_STREAM表示TCP协议，0表示使用默认协议
    client_fd = socket(AF_INET,SOCK_STREAM, 0);
    if (client_fd < 0)  //判断套接字是否创建成功，小于0表示失败
    {
        perror("socket error");  //打印错误信息
    }
}

Client::~Client(){  //Client类的析构函数
    close(client_fd);  //关闭客户端套接字，释放资源
}

int Client::connect(const char* ip, int port){  //连接服务器的方法，参数为服务器IP和端口
    this->server_ip = ip;  //将传入的IP地址保存到成员变量
    this->server_port = port;  //将传入的端口号保存到成员变量
    //设置服务器地址结构体
    server_addr.sin_family = AF_INET;  //设置地址族为IPv4
    server_addr.sin_port = htons(port);  //将端口号转换为网络字节序
    server_addr.sin_addr.s_addr = inet_addr(ip);  //将IP地址字符串转换为网络字节序的整数

    //调用connect函数连接服务器，成功返回0，失败返回-1
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect error");  //打印连接错误信息
        return -1;  //返回-1表示连接失败
    }
    //打印连接成功信息，包含服务器IP和端口
    printf("连接服务器成功,ip为%s,端口为%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    //创建发送消息的线程，绑定到send_messages成员函数
    thread send_thread(&Client::send_messages, this);
    //创建接收消息的线程，绑定到receive_messages成员函数
    thread recv_thread(&Client::receive_messages, this);

    send_thread.join();  //等待发送线程结束
    recv_thread.join();  //等待接收线程结束

    return 0;  //返回0表示连接和通信成功
}

void Client::receive_messages(){  //接收并打印服务器消息的函数
    char recvbuf[100];  //定义接收缓冲区，大小为100字节
    while(1){  //无限循环，持续接收消息
        memset(recvbuf,0,sizeof(recvbuf));  //清空接收缓冲区
        //调用recv函数接收服务器消息，返回接收的字节数
        int n=recv(client_fd,recvbuf,sizeof(recvbuf),0);
        if(n>0)  //如果接收成功（字节数大于0）
            printf("收到消息:%s\n",recvbuf);  //打印收到的消息
        else if(n==0){  //如果接收的字节数为0，表示服务器关闭连接
            printf("服务器关闭连接\n");  //打印服务器关闭信息
            break;  //退出循环
        }
        else{  //如果接收失败（返回值小于0）
            perror("recv error");  //打印接收错误信息
            break;  //退出循环
        }
    }
}

void Client::send_messages(){  //从标准输入读取数据并发送给服务器的函数
    char sendbuf[100];  //定义发送缓冲区，大小为100字节
    while(1){  //无限循环，持续发送消息
        memset(sendbuf,0,sizeof(sendbuf));  //清空发送缓冲区
        cin>>sendbuf;  //从标准输入读取用户输入的字符串
        //调用send函数发送消息给服务器
        send(client_fd, sendbuf, strlen(sendbuf),0);
        if(strcmp(sendbuf,"exit")==0)  //判断用户是否输入了"exit"命令
            break;  //如果是，退出循环，结束发送线程
    }
}

//多线程客户端主函数（一个线程用于接收并打印信息、一个线程用于输入并发送信息）
int main(int argc, char *argv[])  //主函数入口，argc为命令行参数数量，argv为命令行参数数组
{
    Client client;  //创建客户端对象

    //判断是否手动传入服务器IP与端口
    if(argc==3){  //如果命令行参数数量为3，表示手动传入了IP和端口
        const char* IP=argv[1];  //获取第一个参数作为服务器IP地址
        int port=atoi(argv[2]);  //获取第二个参数作为服务器端口号（字符串转整数）
        printf("IP=%s port=%d\n",IP,port);  //打印手动传入的IP地址和端口号
        client.connect(IP,port);  //调用connect方法连接服务器
    }
    else{  //如果命令行参数数量不为3，使用默认值
        //打印使用默认IP地址和端口号的信息
        printf("使用默认IP地址和端口号:%s:%d\n",SERVER_DEFAULT_IP,SERVER_DEFAULT_PORT);
        client.connect(SERVER_DEFAULT_IP, SERVER_DEFAULT_PORT);  //使用默认值连接服务器
    }

    char sendbuf[100];  //发送缓冲区（主函数中的备用缓冲区，实际已在线程中使用）
    char recvbuf[100];  //接收缓冲区（主函数中的备用缓冲区，实际已在线程中使用）
    while (1)  //主函数中的消息循环（实际已在线程中处理，此部分为冗余代码）
    {
        memset(sendbuf, 0, sizeof(sendbuf));  //清空发送缓冲区
        cin>>sendbuf;  //从标准输入读取用户输入
        send(client_fd, sendbuf, strlen(sendbuf),0);  //发送消息给服务器
        if(strcmp(sendbuf,"exit")==0)  //判断是否输入"exit"
            break;  //退出循环
    }
    close(client_fd);  //关闭客户端套接字
    return 0;  //主函数返回0，表示正常退出
}
