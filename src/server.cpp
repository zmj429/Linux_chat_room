#include "global.h"
#include "server.h"

#define SERVER_DEFAULT_IP "0.0.0.0" // 服务器默认IP地址
#define SERVER_DEFAULT_PORT 8080    // 服务器默认端口号

using namespace std;

Server::Server(const char* ip, int port)
{
    this->ip = ip;
    this->port = port;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket error");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(this->port);
    server_addr.sin_addr.s_addr = inet_addr(this->ip);

    //bind，成功返回0，出错返回-1
    if(bind(server_fd,(struct sockaddr *)&server_addr,sizeof(server_addr))==-1)
    {
        perror("bind error");//输出错误原因
        return;//结束程序
    }

    //listen，成功返回0，出错返回-1
    if(listen(server_fd,20) == -1)
    {
        perror("listen error");//输出错误原因
        return;//结束程序
    }
}

Server::~Server(){
    for(auto fd:client_fds)  //关闭所有客户端套接字
        close(fd);
    close(server_fd);  //关闭服务器套接字
}

int Server::run(){
    //客户端套接字
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    while(1)
    {
        //accept，成功返会客户端套接字，出错返回-1
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &length);
        if(client_fd < 0)
        {
            perror("accept error");//输出错误原因
            return -1;//结束程序
        }
        printf("客户端成功连接\n,ip为%s,端口为%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        client_fds.push_back(client_fd); //将客户端套接字加入到列表中
        thread t(&Server::handle_client, this, client_fd); //创建线程处理客户端消息
        t.detach(); //分离线程，使其在完成后自动释放资源
    }

    return 0;
}

void Server::handle_client(int client_fd){
    //接收缓冲区
    char buffer[1000];

    //不断接收数据
    while(1)
    {
        memset(buffer,0,sizeof(buffer));  //清空缓冲区
        int len = recv(client_fd, buffer, sizeof(buffer),0);
        //客户端发送exit或者异常结束时，退出
        if(strcmp(buffer,"exit")==0 || len<=0){
            printf("客户端断开连接\n");
            break;
        }
        printf("收到客户端信息：%s\n", buffer);
    }

    close(client_fd); //关闭客户端套接字
    client_fds.erase(remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end()); //从列表中移除客户端套接字
}

int main(int argc, char *argv[])
{
    Server* server;

    // 初始化服务器
    // 判断是否手动传入IP地址和端口号
    if(argc==3){ // 手动传入IP地址和端口号
        const char* IP=argv[1];
        int port=atoi(argv[2]);
        printf("IP=%s port=%d\n",IP,port); // 打印手动传入的IP地址和端口号
        server = new Server(IP,port); //使用手动传入的IP地址和端口号初始化服务器
    }
    else{ // 没有手动传入IP地址和端口号，使用默认值
        printf("使用默认IP地址和端口号:%s:%d\n",SERVER_DEFAULT_IP,SERVER_DEFAULT_PORT); // 打印默认IP地址和端口号
        server = new Server(SERVER_DEFAULT_IP, SERVER_DEFAULT_PORT); //使用默认IP地址和端口号初始化服务器
    }

    if(server->run()==-1){ //监听客户端连接并处理消息
        printf("server run error\n");
        return -1;
    }

    return 0;

}