// ============================================================
// client.h  ——  聊天室客户端类声明
// 负责与服务器建立 TCP 连接，启动发送/接收线程并处理用户交互
// ============================================================
#ifndef CLIENT_H                 // 防止头文件被重复包含
#define CLIENT_H

#include "global.h"              // 引入公共依赖头文件

// 聊天室客户端类：封装一个客户端实例的所有状态与行为
class client{
    private:
        int server_port;        // 服务器监听端口号
        string server_ip;       // 服务器 IP 地址（IPv4 字符串）
        int sock;               // 与服务器建立连接的套接字描述符（连接成功后由 socket() 获得）
    public:
        // 构造函数：传入服务器端口与 IP，初始化客户端对象
        client(int port,string ip);
        // 析构函数：关闭套接字、释放资源
        ~client();
        // 启动客户端主服务：连接服务器并启动收发线程
        void run();
        // 发送线程入口（静态成员，便于被 std::thread 直接绑定为函数指针）
        static void SendMsg(int conn);
        // 接收线程入口（静态成员，监听服务器下发消息并输出）
        static void RecvMsg(int conn);
        // 业务处理函数：分发各类请求 / 响应协议
        void HandleClient(int conn);
        // 开始一次聊天会话（封装 SendMsg / RecvMsg 线程的启动与回收）
        void StartChat(int conn, char chat_type, const string& target);
};
#endif                          // 结束头文件保护宏
