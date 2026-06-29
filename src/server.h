// ============================================================
// server.h  ——  聊天室服务器类声明
// 负责监听 TCP 端口、接受客户端连接、管理在线用户与群组
// ============================================================
#ifndef SERVER_H                 // 防止头文件被重复包含
#define SERVER_H

#include "global.h"              // 引入公共依赖头文件

// 聊天室服务器类：封装一个 TCP 聊天室服务器的生命周期
class server{
    private:
        int server_port;                // 服务器监听端口号
        int server_sockfd;              // 处于 listen 状态的监听套接字描述符
        string server_ip;               // 服务器绑定的 IP 地址（通常 0.0.0.0）

        // 在线套接字状态表：下标为 fd，值 true 表示连接存活
        static vector<bool> sock_arr;
        // 用户昵称 → 套接字描述符映射，用于私聊快速查找
        static unordered_map<string,int> name_sock_map;
        // 保护 name_sock_map 的互斥锁（多个子线程并发修改）
        static mutex name_sock_mutx;
        // 群号 → 群成员套接字描述符集合，用于群聊转发
        static unordered_map<int,set<int>> group_map;
        // 保护 group_map 的互斥锁
        static mutex group_mutex;

    public:
        // 构造函数：传入监听端口与绑定 IP，初始化服务器对象
        server(int port,string ip);
        // 析构函数：关闭监听套接字、清理共享资源
        ~server();
        // 启动服务器：bind / listen / accept，进入主事件循环
        void run();
        // 子线程工作入口（静态成员）：处理单个连接的收发循环
        static void RecvMsg(int conn);
        // 请求分发器：根据协议解析结果处理登录、私聊、群聊等业务
        // info 为协议信息元组：(是否成功, 昵称, 群号或对象, 端口, 保留字段)
        static void HandleRequest(int conn,string str,tuple<bool,string,string,int,int> &info);
};

#endif                          // 结束头文件保护宏
