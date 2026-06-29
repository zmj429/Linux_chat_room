#include "client.h"
#include <signal.h>

// 构造函数：初始化服务器端口和IP
client::client(int port,string ip):server_port(port),server_ip(ip){}
// 析构函数：关闭套接字
client::~client(){
    close(sock);
}

// === 主运行入口：建立连接并启动业务处理 ===
void client::run(){
    // 忽略 SIGPIPE，避免发送到已关闭连接时进程被内核直接杀掉
    signal(SIGPIPE, SIG_IGN);

    // === 连接服务器 ===
    // 创建TCP套接字
    sock = socket(AF_INET,SOCK_STREAM, 0);

    // 构造服务器地址结构体
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;              // 使用TCP/IP协议族
    servaddr.sin_port = htons(server_port);     // 设置服务器端口（htons转网络字节序）
    servaddr.sin_addr.s_addr = inet_addr(server_ip.c_str()); // 设置服务器IP

    // 主动连接服务器，成功返回0，失败返回-1
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    cout<<"连接服务器成功\n";

    // 进入业务处理主循环
    HandleClient(sock);

    return;
}

// === 发送消息线程（成员函数） ===
// conn > 0 视为私聊，conn < 0 视为群聊（用 abs(conn) 作为真实 fd）
void client::SendMsg(int conn){
    int sockfd = abs(conn);
    while (1)
    {
        string str;
        cin>>str;                              // 读取控制台输入
        if(conn > 0){
            // 私聊消息前缀
            str="content:"+str;
        }
        else if(conn < 0){
            // 群聊消息前缀
            str="gr_message:"+str;
        }
        int ret=send(sockfd, str.c_str(), str.length(), 0);
        if(ret < 0){
            perror("send");
            break;
        }
        if(str == "content:exit" || str == "gr_message:exit")
            break;
    }
}

// === 接收消息线程（成员函数） ===
void client::RecvMsg(int conn){
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    char recvbuf[1000];                       // 接收缓冲区
    while(1)
    {
        memset(recvbuf, 0, sizeof(recvbuf));
        int len = recv(conn, recvbuf, sizeof(recvbuf), 0);
        if(len<=0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                continue;
            }
            break;
        }
        string msg(recvbuf);
        if(msg == "chat_end"){
            break;
        }
        cout<<recvbuf<<endl;
    }
}

// === 登录后开始聊天（私聊 / 群聊通用） ===
// chat_type: 'p' 私聊, 'g' 群聊
// target: 私聊时是对方用户名；群聊时是群号（字符串形式）
void client::StartChat(int conn, char chat_type, const string& target){
    int sign = (chat_type=='g') ? -1 : 1;
    cout<<"请输入你想说的话(输入exit退出):\n";
    thread t1(client::SendMsg, conn*sign); // 负数表示群聊
    thread t2(client::RecvMsg, conn);
    t1.join();
    t2.join();
}

// === 业务处理主控：登录/注册/菜单 ===
void client::HandleClient(int conn){
    int choice;                               // 用户菜单选择
    string name,pass,pass1;                   // 用户名、密码、确认密码
    bool if_login=false;                      // 是否已登录成功
    string login_name;                        // 登录后的用户名

    // === Cookie自动登录 ===
    // 读取本地cookie文件，存在则发送到服务器验证
    ifstream f("cookie.txt");
    string cookie_str;
    if(f.good()){
        f>>cookie_str;
        f.close();
        cookie_str="cookie:"+cookie_str;     // 加上cookie前缀
        send(sock,cookie_str.c_str(),cookie_str.length()+1,0); // 发送cookie
        char cookie_ans[100];
        memset(cookie_ans,0,sizeof(cookie_ans));
        recv(sock,cookie_ans,sizeof(cookie_ans),0);           // 接收服务器应答
        string ans_str(cookie_ans);
        if(ans_str!="NULL"){                 // Redis中查到此cookie，自动登录成功
            if_login=true;
            login_name=ans_str;              // 服务器返回对应用户名
        }
    }

    // === 登录/注册菜单 ===
    cout<<" ------------------\n";
    cout<<"|                  |\n";
    cout<<"| 请输入你要的选项:|\n";
    cout<<"|    0:退出        |\n";
    cout<<"|    1:登录        |\n";
    cout<<"|    2:注册        |\n";
    cout<<"|                  |\n";
    cout<<" ------------------ \n\n";

    // === 处理注册、登录事件 ===
    while(1){
        if(if_login)
           break;                            // 已登录则跳出菜单循环
        cin>>choice;
        if(choice==0)
            break;                            // 0表示退出
        // === 注册 ===
        else if(choice==2){
            cout<<"注册的用户名:";
            cin>>name;
            while(1){
                cout<<"密码:";
                cin>>pass;
                cout<<"确认密码:";
                cin>>pass1;
                if(pass==pass1)
                    break;                   // 两次密码一致才继续
                else
                    cout<<"两次密码不一致!\n\n";
            }
            name="name:"+name;               // 用户名前缀
            pass="pass:"+pass;               // 密码前缀
            string str=name+pass;            // 拼接为注册请求
            if(send(conn,str.c_str(),str.length(),0) < 0){
                perror("send");
                cout<<"注册请求发送失败\n";
                break;
            }

            // 等待服务器响应
            char buffer[1000];
            memset(buffer,0,sizeof(buffer));
            if(recv(sock,buffer,sizeof(buffer),0) <= 0){
                cout<<"注册响应接收失败\n";
                break;
            }

            string recv_str(buffer);
            if(recv_str == "ok"){
                cout<<"注册成功！\n";
            }
            else if(recv_str == "exist"){
                cout<<"注册失败：用户名已存在！\n";
            }
            else if(recv_str == "db_error"){
                cout<<"注册失败：数据库错误！\n";
            }
            else{
                cout<<"注册失败：未知错误！\n";
            }
            cout<<"\n继续输入你要的选项:";
        }
        // === 登录 ===
        else if(choice == 1 && !if_login){
            while(1){
                cout<<"用户名:";
                cin>>name;
                cout<<"密码:";
                cin>>pass;
                string str="login:"+name;    // 拼接登录请求字符串
                str+="pass:";
                str+=pass;
                if(send(sock,str.c_str(),str.length(),0) < 0){
                    perror("send");
                    cout<<"登录请求发送失败\n";
                    break;
                }

                char buffer[1000];
                memset(buffer,0,sizeof(buffer));
                if(recv(sock,buffer,sizeof(buffer),0) <= 0){
                    cout<<"登录响应接收失败\n";
                    break;
                }
                string recv_str(buffer);
                // 以"ok"开头表示登录成功
                if(recv_str.substr(0,2)=="ok"){
                    if_login=true;
                    login_name=name;

                    // 服务器返回的sessionid写入本地cookie文件
                    string tmpstr=recv_str.substr(2);           // 去掉前缀"ok"
                    tmpstr="cat > cookie.txt <<end \n"+tmpstr+"\nend";
                    system(tmpstr.c_str());                    // 通过shell写文件

                    cout<<"登录成功\n\n";
                    break;
                }
                // 登录失败，提示重试
                else
                    cout<<"密码或用户名错误！\n\n";
            }
        }
    }

    // === 登录成功后的业务处理菜单 ===
    while(if_login && 1){
        if(if_login){
            system("clear");                // 清空终端屏幕
            cout<<"        欢迎回来,"<<login_name<<endl;
            cout<<" -------------------------------------------\n";
            cout<<"|                                           |\n";
            cout<<"|          请选择你要的选项：               |\n";
            cout<<"|              0:退出                       |\n";
            cout<<"|              1:发起单独聊天               |\n";
            cout<<"|              2:发起群聊                   |\n";
            cout<<"|                                           |\n";
            cout<<" ------------------------------------------- \n\n";
        }
        cin>>choice;
        // 退出聊天系统
        if(choice==0)
            break;
        // === 私聊 ===
        else if(choice==1){
            cout<<"请输入对方的用户名:";
            string target_name;
            cin>>target_name;
            string sendstr("target:"+target_name+"from:"+login_name); // 私聊目标与源
            if(send(sock,sendstr.c_str(),sendstr.length(),0) < 0){
                perror("send");
                cout<<"私聊目标设置失败\n";
                continue;
            }
            StartChat(conn, 'p', target_name);
        }
        // === 群聊 ===
        else if(choice == 2){
            cout<<"请输入群号:";
            int num;
            cin>>num;
            string sendstr("group:"+to_string(num)); // 群聊请求
            if(send(sock,sendstr.c_str(),sendstr.length(),0) < 0){
                perror("send");
                cout<<"群聊目标设置失败\n";
                continue;
            }
            StartChat(conn, 'g', to_string(num));
        }

    }

}
