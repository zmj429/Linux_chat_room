#include "global.h"  //包含全局头文件，引入所需的标准库和网络库
#include "server.h"  //包含服务器头文件，引入Server类的定义

#define SERVER_DEFAULT_IP "0.0.0.0" // 服务器默认IP地址，0.0.0.0表示监听所有网络接口
#define SERVER_DEFAULT_PORT 8080    // 服务器默认端口号为8080

using namespace std;  //使用std命名空间，简化标准库的使用

Server::Server(const char* ip, int port)  //Server类的带参构造函数，参数为服务器IP和端口
{
    this->ip = ip;  //将传入的IP地址保存到成员变量
    this->port = port;  //将传入的端口号保存到成员变量

    //创建服务器套接字，AF_INET表示IPv4协议，SOCK_STREAM表示TCP协议，0表示使用默认协议
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)  //判断套接字是否创建成功，小于0表示失败
    {
        perror("socket error");  //打印错误信息
        return;  //返回构造函数，不继续执行
    }

    //使用memset函数清空服务器地址结构体，将所有字节设置为0
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;  //设置地址族为IPv4
    server_addr.sin_port = htons(this->port);  //将端口号转换为网络字节序
    server_addr.sin_addr.s_addr = inet_addr(this->ip);  //将IP地址字符串转换为网络字节序的整数

    //调用bind函数绑定套接字到指定的IP和端口，成功返回0，出错返回-1
    if(bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1)
    {
        perror("bind error");//输出绑定错误原因
        return;//结束构造函数
    }

    //调用listen函数开始监听客户端连接，第二个参数为最大等待队列长度，成功返回0，出错返回-1
    if(listen(server_fd,20) == -1)
    {
        perror("listen error");//输出监听错误原因
        return;//结束构造函数
    }
}

Server::~Server(){  //Server类的析构函数
    for(auto fd:client_fds)  //遍历客户端套接字列表
        close(fd);  //关闭每个客户端套接字
    close(server_fd);  //关闭服务器套接字
}

int Server::run(){  //服务器运行函数，监听客户端连接并处理消息
    //定义客户端地址结构体，用于存储连接客户端的地址信息
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);  //获取客户端地址结构体的大小

    while(1)  //无限循环，持续监听客户端连接
    {
        //调用accept函数接收客户端连接，成功返回客户端套接字，出错返回-1
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &length);
        if(client_fd < 0)  //判断是否成功接收客户端连接
        {
            perror("accept error");//输出接收错误原因
            return -1;//返回-1表示运行失败
        }
        //打印客户端成功连接的信息，包含客户端IP和端口
        printf("客户端成功连接\n,ip为%s,端口为%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        client_fds.push_back(client_fd); //将新连接的客户端套接字加入到客户端列表中
        //创建新线程处理客户端消息，绑定到handle_client成员函数，传入客户端套接字
        thread t(&Server::handle_client, this, client_fd);
        t.detach(); //分离线程，使其在完成后自动释放资源，不需要主线程等待
    }

    return 0;  //返回0表示正常运行（实际上不会执行到这里，因为上面是无限循环）
}

void Server::handle_client(int client_fd){  //处理单个客户端消息的函数，参数为客户端套接字
    //定义接收缓冲区，大小为1000字节，用于存储从客户端接收到的消息
    char buffer[1000];

    //不断接收客户端数据
    while(1)
    {
        memset(buffer,0,sizeof(buffer));  //清空接收缓冲区，避免残留数据
        //调用recv函数接收客户端消息，返回接收的字节数
        int len = recv(client_fd, buffer, sizeof(buffer),0);
        //如果客户端发送"exit"或者连接异常（len<=0），则退出循环
        if(strcmp(buffer,"exit")==0 || len<=0){
            printf("客户端断开连接\n");  //打印客户端断开连接的信息
            break;  //退出循环，结束对该客户端的处理
        }
        printf("收到客户端信息：%s\n", buffer);  //打印收到的客户端信息
    }

    close(client_fd); //关闭客户端套接字，释放资源
    //从客户端列表中移除该客户端套接字，使用remove-erase模式
    client_fds.erase(remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
}

int main(int argc, char *argv[])  //主函数入口，argc为命令行参数数量，argv为命令行参数数组
{
    Server* server;  //定义服务器指针

    // 初始化服务器
    // 判断是否手动传入IP地址和端口号
    if(argc==3){ // 如果命令行参数数量为3，表示手动传入了IP和端口
        const char* IP=argv[1];  //获取第一个参数作为服务器IP地址
        int port=atoi(argv[2]);  //获取第二个参数作为服务器端口号（字符串转整数）
        printf("IP=%s port=%d\n",IP,port); // 打印手动传入的IP地址和端口号
        server = new Server(IP,port); //使用手动传入的IP地址和端口号创建服务器对象
    }
    else{ // 如果命令行参数数量不为3，使用默认值
        //打印使用默认IP地址和端口号的信息
        printf("使用默认IP地址和端口号:%s:%d\n",SERVER_DEFAULT_IP,SERVER_DEFAULT_PORT);
        server = new Server(SERVER_DEFAULT_IP, SERVER_DEFAULT_PORT); //使用默认值创建服务器对象
    }

    if(server->run()==-1){ //调用服务器的run方法，监听客户端连接并处理消息
        printf("server run error\n");  //如果run方法返回-1，表示运行出错
        return -1;  //主函数返回-1，表示程序异常退出
    }

    return 0;  //主函数返回0，表示正常退出（实际上不会执行到这里）

}
