// 引入服务器自定义头文件
#include "server.h"
#include <signal.h>

// 全局静态成员：套接字使用标记数组，下标为fd，值表示是否正在使用
vector<bool> server::sock_arr(10000,false);
// 全局静态成员：用户名 -> 套接字描述符 映射
unordered_map<string,int> server::name_sock_map;
// 全局静态成员：群号 -> 成员套接字fd集合 的映射
unordered_map<int,set<int>> server::group_map;
// 全局静态成员：保护 name_sock_map 的互斥锁
mutex server::name_sock_mutx;
// 全局静态成员：保护 group_map 的互斥锁
mutex server::group_mutex;

// === 构造函数 ===
// 初始化服务器的监听端口和IP
server::server(int port,string ip):server_port(port),server_ip(ip){
}

// === 析构函数 ===
// 遍历并关闭所有仍在使用的客户端套接字和监听套接字
server::~server(){
    // 遍历所有可能的fd，关闭仍在使用的套接字
    for(size_t i=0;i<sock_arr.size();i++){
        if(sock_arr[i])
            close(i);
    }
    // 关闭服务器监听套接字
    close(server_sockfd);
}

// === 服务器主循环：创建监听套接字、绑定端口、接收新连接、为每个客户端启子线程 ===
void server::run(){
    // 忽略 SIGPIPE，否则 send 到已关闭连接时进程会被内核直接杀掉（闪退）
    signal(SIGPIPE, SIG_IGN);

    // 创建TCP流式套接字
    server_sockfd = socket(AF_INET,SOCK_STREAM, 0);

    // 准备服务器地址结构体
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET; // 指定使用 TCP/IP 协议族
    server_sockaddr.sin_port = htons(server_port); // 绑定端口（htons转网络字节序）
    server_sockaddr.sin_addr.s_addr = inet_addr(server_ip.c_str()); // 绑定IP，127.0.0.1 为本机环回地址

    // 开启 SO_REUSEADDR，允许 TIME_WAIT 状态的端口被立即复用，避免重启时 bind 失败
    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // bind 绑定地址到套接字，成功返回 0，出错返回 -1
    if(bind(server_sockfd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr))==-1)
    {
        perror("bind"); // 打印 bind 失败原因
        exit(1);        // 异常退出
    }

    // listen 将套接字设为监听状态，最大允许 20 个未完成连接排队
    if(listen(server_sockfd,20) == -1)
    {
        perror("listen"); // 打印 listen 失败原因
        exit(1);          // 异常退出
    }

    // 用于 accept 的客户端地址结构体
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    // 主循环：不断 accept 新连接并为每个客户端派发一个子线程
    while(1){
        // 从已完成连接队列中取一个连接，返回已连接套接字 conn
        int conn = accept(server_sockfd, (struct sockaddr*)&client_addr, &length);
        if(conn<0)
        {
            perror("connect"); // accept 失败则打印并退出
            exit(1);
        }
        cout<<"文件描述符为"<<conn<<"的客户端成功连接\n";
        // 标记该 fd 正在使用中
        sock_arr[conn]=true;
        // 为该客户端创建一个子线程，入口函数为 RecvMsg，参数为 conn
        thread t(server::RecvMsg,conn);
        t.detach(); // 设置为分离状态，防止主线程被 join 阻塞
    }
}

// === 子线程入口：持续接收当前客户端的数据并分发给业务处理函数 ===
// 注意：静态成员函数在类外定义时不需要再加 static 关键字
void server::RecvMsg(int conn){
    // 每个子线程维护一个元组 info，保存该连接的会话状态
    // tuple 共 5 个字段：
    // [0] bool if_login     当前用户是否已登录
    // [1] string login_name  当前登录用户名
    // [2] string target_name 私聊目标用户名
    // [3] int target_conn    私聊目标 fd，初始 -1 表示无
    // [4] int group_num      所在群聊号
    tuple<bool,string,string,int,int> info;
    get<0>(info)=false; // 初始化：if_login=false
    get<3>(info)=-1;    // 初始化：target_conn=-1

    // 接收缓冲区
    char buffer[1000];
    // 持续接收来自该客户端的数据
    while(1)
    {
        // 每次接收前清空缓冲区
        memset(buffer,0,sizeof(buffer));
        // 阻塞式接收，len 为实际读取字节数
        int len = recv(conn, buffer, sizeof(buffer),0);
        string msg(buffer);
        // 连接断开条件：recv返回<=0
        if(len<=0){
            close(conn);
            sock_arr[conn]=false;
            // 从 name_sock_map 和 group_map 中清理该连接
            {
                lock_guard<mutex> lock(name_sock_mutx);
                for(auto it=name_sock_map.begin(); it!=name_sock_map.end(); ++it){
                    if(it->second==conn){ name_sock_map.erase(it); break; }
                }
            }
            {
                lock_guard<mutex> lock(group_mutex);
                for(auto &kv : group_map) kv.second.erase(conn);
            }
            cout<<"文件描述符"<<conn<<" 断开连接"<<endl;
            break;
        }
        cout<<"收到套接字描述符为"<<conn<<"发来的信息："<<buffer<<endl;
        // 将 C 字符串包装为 std::string，交由业务函数处理
        string str(buffer);
        HandleRequest(conn,str,info);
    }
}

// === 核心业务分发函数：解析请求、读写 MySQL/Redis、支持注册/登录/私聊/群聊 ===
void server::HandleRequest(int conn,string str,tuple<bool,string,string,int,int> &info){
    string name,pass;

    // 把 tuple 中的各字段解包为局部变量，便于读写
    bool if_login=get<0>(info);        // 当前用户是否已登录
    string login_name=get<1>(info);    // 当前登录用户名
    string target_name=get<2>(info);   // 私聊目标用户名
    int target_conn=get<3>(info);      // 私聊目标 fd
    int group_num = get<4>(info);      // 当前绑定群聊号

    // === MYSQL 初始化 ===
    MYSQL *con=mysql_init(NULL); // 初始化 MySQL 句柄
    // 句柄初始化失败则报错退出
    if(con == nullptr){
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(EXIT_FAILURE);
    }
    // 连接 MySQL 数据库 ChatProject，root/123456
    if(!mysql_real_connect(con,"127.0.0.1","root","123456","ChatProject",0,NULL,CLIENT_MULTI_STATEMENTS)){
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(EXIT_FAILURE);
    }

    // === Redis 初始化 ===
    bool redis_ok = true;
    redisContext *redis_target = redisConnect("127.0.0.1",6379); // 连接本地 Redis
    if(redis_target == NULL || redis_target->err){
        if(redis_target) redisFree(redis_target);
        redis_ok = false;
        cout<<"连接redis失败，跳过Redis功能"<<endl;
    }

    // === 1. cookie 校验：客户端带上 cookie 时，从 Redis 查该 cookie 是否仍有效 ===
    if(str.find("cookie:")!=str.npos){
        // 提取 cookie 字符串（去掉前 7 字节 "cookie:"）
        string cookie=str.substr(7);
        string send_res="NULL";
        if(redis_ok){
            // 构造 Redis 命令：hget <cookie> name
            string redis_str="hget "+cookie+" name";
            redisReply *r = (redisReply*)redisCommand(redis_target,redis_str.c_str());
            // 查到对应 name，说明 cookie 有效
            if(r && r->str){
                cout<<"查询redis结果："<<r->str<<endl;
                send_res=r->str;
                // 更新会话状态
                if_login = true;
                login_name = r->str;
                // 注册到 name_sock_map
                {
                    lock_guard<mutex> lock(name_sock_mutx);
                    name_sock_map[login_name] = conn;
                }
            }
        }else{
            cout<<"Redis不可用，跳过cookie校验"<<endl;
        }
        // 把校验结果发回客户端
        send(conn,send_res.c_str(),send_res.length()+1,0);
    }

    // === 2. 注册：协议形如 name:xxxpass:xxx ===
    if(str.find("name:")!=str.npos){
        int p1=str.find("name:"),p2=str.find("pass:"); // 定位关键字位置
        int key1_len=strlen("name:"),key2_len=strlen("pass:"); // 关键字长度
        // 截取用户名子串：从 "name:" 后一位开始，长度为 p2-p1-key1_len
        name=str.substr(p1+key1_len,p2-p1-key1_len);
        // 截取密码子串
        pass=str.substr(p2+key2_len,str.length()-p2-key2_len);

        // 先 SELECT 检查用户名是否已存在
        string check_sql="SELECT * FROM USER WHERE NAME=\"";
        check_sql+=name;
        check_sql+="\";";
        if(mysql_query(con,check_sql.c_str())){
            fprintf(stderr, "%s\n", mysql_error(con));
            string err_msg="db_error";
            send(conn,err_msg.c_str(),err_msg.length(),0);
            if(redis_ok) redisFree(redis_target);
            mysql_close(con);
            return;
        }
        MYSQL_RES *check_res=mysql_store_result(con);
        if(check_res == nullptr){
            fprintf(stderr, "%s\n", mysql_error(con));
            string err_msg="db_error";
            send(conn,err_msg.c_str(),err_msg.length(),0);
            if(redis_ok) redisFree(redis_target);
            mysql_close(con);
            return;
        }
        int row_count=mysql_num_rows(check_res);
        mysql_free_result(check_res);

        if(row_count!=0){
            // 用户名已存在，告诉客户端注册失败
            cout<<"注册失败：用户名 "<<name<<" 已存在"<<endl;
            string err_msg="exist";
            send(conn,err_msg.c_str(),err_msg.length(),0);
        }else{
            // 用户名不存在，执行 INSERT
            string search="INSERT INTO USER VALUES (\"";
            search+=name;
            search+="\",\"";
            search+=pass;
            search+="\");";
            cout<<"sql语句:"<<search<<endl<<endl;
            if(mysql_query(con,search.c_str())){
                fprintf(stderr, "%s\n", mysql_error(con));
                cout<<"注册失败：数据库插入错误"<<endl;
                string err_msg="db_error";
                send(conn,err_msg.c_str(),err_msg.length(),0);
            }
            else{
                cout<<"注册成功："<<name<<endl;
                string ok_msg="ok";
                send(conn,ok_msg.c_str(),ok_msg.length(),0);
            }
        }
    }
    // === 3. 登录：协议形如 login:xxxpass:xxx ===
    else if(str.find("login:")!=str.npos){
        int p1=str.find("login:"),p2=str.find("pass:");
        int key1_len=strlen("login:"),key2_len=strlen("pass:");
        // 截取登录用户名
        name=str.substr(p1+key1_len,p2-p1-key1_len);
        // 截取密码
        pass=str.substr(p2+key2_len,str.length()-p2-key2_len);

        // 拼接 SELECT 语句，查询该用户名是否存在
        string search="SELECT * FROM USER WHERE NAME=\"";
        search+=name;
        search+="\";";
        cout<<"sql语句:"<<search<<endl;

        // 执行 SQL 并取回结果集
        auto search_res=mysql_query(con,search.c_str());
        if(search_res != 0){
            fprintf(stderr, "%s\n", mysql_error(con));
            cout<<"查询失败：数据库错误"<<endl;
            string str1="wrong";
            send(conn,str1.c_str(),str1.length(),0);
        }
        else{
            auto result=mysql_store_result(con);
            if(result == nullptr){
                fprintf(stderr, "%s\n", mysql_error(con));
                cout<<"查询失败：结果为空"<<endl;
                string str1="wrong";
                send(conn,str1.c_str(),str1.length(),0);
            }
            else{
                int row=mysql_num_rows(result);   // 结果集行数（0=用户不存在）

                // 查询成功且命中用户（row != 0）
                if(row!=0){
                    cout<<"查询成功\n";
                    auto info=mysql_fetch_row(result); // 读取首行（密码列 info[1]）
                    cout<<"查询到用户名:"<<info[0]<<" 密码:"<<info[1]<<endl;
                    // 密码匹配
                    if(info[1]==pass){
                        cout<<"登录密码正确\n\n";
                        string str1="ok";
                        if_login=true;
                        login_name=name; // 记录当前连接对应的登录用户名

                        // 多线程环境下，加锁维护 name_sock_map，记录 用户名 -> fd
                        {
                            lock_guard<mutex> lock(name_sock_mutx); // 自动加锁，作用域结束自动解锁
                            name_sock_map[login_name]=conn;         // 记录用户名到 fd 的映射
                        }

                        // 生成 10 位随机 sessionid（包含数字、大小写字母）
                        srand(time(nullptr)); // 用当前时间初始化随机种子
                        for(int i=0;i<10;i++){
                            int type=rand()%3; // type: 0 数字 / 1 小写 / 2 大写
                            if(type==0)
                                str1+='0'+rand()%9;
                            else if(type==1)
                                str1+='a'+rand()%26;
                            else if(type==2)
                                str1+='A'+rand()%26;
                        }
                        // Redis: 把 sessionid -> login_name 写入哈希表（Redis 不可用时跳过）
                        if(redis_ok){
                            string redis_str="hset "+str1.substr(2)+" name "+login_name; // 去掉前缀 "ok"
                            redisCommand(redis_target,redis_str.c_str());
                            // 设置 sessionid 生存时间 300 秒
                            redis_str="expire "+str1.substr(2)+" 300";
                            redisCommand(redis_target,redis_str.c_str());
                        }
                        cout<<"随机生成的sessionid为："<<str1.substr(2)<<endl;
                        // 把 "ok+sessionid" 发送给客户端
                        send(conn,str1.c_str(),str1.length(),0);
                    }
                    // 密码错误，返回 "wrong"
                    else{
                        cout<<"登录密码错误\n\n";
                        string str1="wrong";
                        send(conn,str1.c_str(),str1.length(),0);
                    }
                    mysql_free_result(result);
                }
                // 用户名不存在，返回 "wrong"
                else{
                    cout<<"查询失败：用户不存在\n\n";
                    string str1="wrong";
                    send(conn,str1.c_str(),str1.length(),0);
                    mysql_free_result(result);
                }
            }
        }
    }
    // === 4. 设定私聊目标：协议形如 target:<目标名>from:<来源名> ===
    else if(str.find("target:")!=str.npos){
        string strkey1("target:"),strkey2("from:");
        int p1=str.find(strkey1),p2=str.find(strkey2);
        // 截取目标用户名
        string target = str.substr(p1+strkey1.length(),p2-p1-strkey1.length());
        // 截取来源用户名
        string from = str.substr(p2+strkey2.length(),str.length()-p2-strkey2.length());

        target_name = target;
        // 目标用户还未登录
        {
            lock_guard<mutex> lock(name_sock_mutx);
            if(name_sock_map.find(target)==name_sock_map.end())
                cout<<"源用户为"<<login_name<<",目标用户"<<target_name<<"仍未登录，无法发起私聊\n";
            // 目标用户已登录，记录其 fd 用于后续转发
            else{
                cout<<"源用户"<<login_name<<"向目标用户"<<target_name<<"发起的私聊即将建立";
                cout<<",目标用户的套接字描述符为"<<name_sock_map[target_name]<<endl;
                target_conn=name_sock_map[target_name];
            }
        }
    }
    // === 5. 私聊消息转发：协议形如 content:<消息体> ===
    else if(str.find("content:")!=str.npos){
        // 私聊退出命令
        if(str == "content:exit"){
            cout<<"用户"<<login_name<<"退出私聊\n";
            target_conn = -1;
            target_name = "";
            send(conn,"chat_end",8,0);
            return;
        }

        // 如果当前没有记录到目标 fd，尝试重新从 name_sock_map 查找
        if(target_conn==-1){
            cout<<"找不到目标用户"<<target_name<<"的套接字，将尝试重新寻找目标用户的套接字\n";
            {
                lock_guard<mutex> lock(name_sock_mutx);
                if(name_sock_map.find(target_name)!=name_sock_map.end()){
                    target_conn=name_sock_map[target_name];
                    cout<<"重新查找目标用户套接字成功\n";
                }
                else{
                    cout<<"查找仍然失败，转发失败！\n";
                    return;
                }
            }
        }
        string recv_str(str);
        // 截取 content 后的实际消息内容（去掉前 8 字节 "content:"）
        string send_str=recv_str.substr(8);
        cout<<"用户"<<login_name<<"向"<<target_name<<"发送:"<<send_str<<endl;
        // 消息体前加上发送者昵称
        send_str="["+login_name+"]:"+send_str;
        // 发送到目标 fd
        int send_ret = send(target_conn,send_str.c_str(),send_str.length(),0);
        if(send_ret < 0){
            cout<<"向目标用户发送消息失败\n";
            target_conn = -1;
        }
    }
    // === 6. 绑定群聊号：协议形如 group:<群号> ===
    else if(str.find("group:")!=str.npos){
        string recv_str(str);
        string num_str = recv_str.substr(6);
        int new_group = stoi(num_str);

        // 先从旧群里移除，避免一条连接留在多个群
        if(group_num != new_group){
            lock_guard<mutex> lock(group_mutex);
            if(group_num != 0) group_map[group_num].erase(conn);
            group_map[new_group].insert(conn);
        }
        group_num = new_group;
        cout<<"用户"<<login_name<<"绑定群聊号为:"<<group_num<<endl;
    }
    // === 7. 群聊消息广播：协议形如 gr_message:<消息体> ===
    else if(str.find("gr_message:")!=str.npos){
        // 群聊退出命令
        if(str == "gr_message:exit"){
            cout<<"用户"<<login_name<<"退出群聊"<<group_num<<endl;
            {
                lock_guard<mutex> lock(group_mutex);
                if(group_num != 0) group_map[group_num].erase(conn);
            }
            group_num = 0;
            send(conn,"chat_end",8,0);
            return;
        }

        string send_str(str);
        // 截取实际消息体（去掉前 11 字节 "gr_message:"）
        send_str = send_str.substr(11);
        // 消息体前加发送者昵称
        send_str = "["+login_name+"]:"+send_str;
        cout<<"群聊信息:"<<send_str<<endl;

        // 检查群是否存在
        if(group_map.find(group_num) == group_map.end()){
            cout<<"错误：群聊号 "<<group_num<<" 不存在"<<endl;
            return;
        }

        // 加锁保护群成员遍历
        set<int> group_members;
        {
            lock_guard<mutex> lock(group_mutex);
            group_members = group_map[group_num];
        }

        // 遍历该群所有成员 fd，向除自己外的每个成员发送
        for(auto i:group_members){
            if(i!=conn){
                int send_ret = send(i,send_str.c_str(),send_str.length(),0);
                if(send_ret < 0){
                    cout<<"向fd "<<i<<" 发送消息失败"<<endl;
                }
            }
        }
    }


    // === 把更新后的会话状态写回 tuple，供下一轮 recv 循环使用 ===
    get<0>(info)=if_login;   // 记录当前用户是否已登录
    get<1>(info)=login_name;  // 当前登录用户名
    get<2>(info)=target_name; // 私聊目标用户名
    get<3>(info)=target_conn; // 私聊目标 fd
    get<4>(info)=group_num;   // 当前绑定群聊号

    // 释放数据库资源
    if(redis_ok) redisFree(redis_target);
    mysql_close(con);
}
