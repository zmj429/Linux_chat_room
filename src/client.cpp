#include "global.h"
#include "client.h"

#define SERVER_DEFAULT_IP "127.0.0.1"
#define SERVER_DEFAULT_PORT 8080

using namespace std;

Client::Client(){
    //定义客户端的sockfd
    client_fd = socket(AF_INET,SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("socket error");
    }
}

Client::~Client(){
    close(client_fd);
}

int Client::connect(const char* ip, int port){
    this->server_ip = ip;
    this->server_port = port;
    //定义sockaddr_in
    server_addr.sin_family = AF_INET;//TCP/IP协议族
    server_addr.sin_port = htons(port);//服务器端口
    server_addr.sin_addr.s_addr = inet_addr(ip);  //服务器ip

    if (::connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect error");
        return -1;
    }
    printf("连接服务器成功,ip为%s,端口为%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    thread send_thread(&Client::send_messages, this); //创建线程输入并发送信息
    thread recv_thread(&Client::receive_messages, this); //创建线程接收并打印信息

    send_thread.join();
    recv_thread.join();

    return 0;
}

void Client::receive_messages(){
    char recvbuf[100];
    while(1){
        memset(recvbuf,0,sizeof(recvbuf));
        int n=recv(client_fd,recvbuf,sizeof(recvbuf),0);
        if(n>0)
            printf("收到消息:%s\n",recvbuf);
        else if(n==0){
            printf("服务器关闭连接\n");
            break;
        }
        else{
            perror("recv error");
            break;
        }
    }
}

void Client::send_messages(){
    char sendbuf[100];
    while(1){
        memset(sendbuf,0,sizeof(sendbuf));
        cin>>sendbuf;
        send(client_fd, sendbuf, strlen(sendbuf),0);
        if(strcmp(sendbuf,"exit")==0)
            break;
    }
}

//多线程客户端（一个线程用于接收并打印信息、一个线程用于输入并发送信息）
int main(int argc, char *argv[])
{
    Client client; //创建客户端对象

    //判断是否手动传入服务器IP与端口
    if(argc==3){ // 手动传入服务器IP地址和端口号
        const char* IP=argv[1];
        int port=atoi(argv[2]);
        printf("IP=%s port=%d\n",IP,port); // 打印手动传入的IP地址和端口号
        client.connect(IP,port); //连接服务器，成功返回0，错误返回-1
    }
    else{ // 没有手动传入IP地址和端口号，使用默认值
        printf("使用默认IP地址和端口号:%s:%d\n",SERVER_DEFAULT_IP,SERVER_DEFAULT_PORT); // 打印默认IP地址和端口号
        client.connect(SERVER_DEFAULT_IP, SERVER_DEFAULT_PORT); //连接服务器，成功返回0，错误返回-1
    }

    char sendbuf[100]; //发送缓冲区
    char recvbuf[100]; //接收缓冲区
    while (1)
    {
        memset(sendbuf, 0, sizeof(sendbuf));
        cin>>sendbuf;
        send(client_fd, sendbuf, strlen(sendbuf),0); //发送
        if(strcmp(sendbuf,"exit")==0)
            break;
    }
    close(client_fd);
    return 0;
}