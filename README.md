# Linux 聊天室项目

## 项目介绍

基于 C++ / TCP 的网络聊天室系统，使用面向对象封装 + 多线程 + MySQL + Redis 实现。

### 功能特性

- 多线程：服务器为每个客户端连接创建独立线程
- 注册 / 登录：账号密码存储在 MySQL 数据库
- Cookie 自动登录：session 信息存 Redis，过期自动失效
- 私聊：一对一定向发送
- 群聊：按群号广播，支持多个群并存


## 项目结构

```
online-chat-room/
├── src/
│   ├── global.h          # 公共头文件（POSIX 系统头、STL、MySQL、hiredis）
│   ├── client.h          # 客户端类声明
│   ├── client.cpp        # 客户端实现（连接 / 登录注册 / 私聊 / 群聊）
│   ├── server.h          # 服务端类声明
│   ├── server.cpp        # 服务端实现（accept / recv / MySQL / Redis 分发）
│   ├── test_client.cpp   # 客户端 main 入口
│   ├── test_server.cpp   # 服务端 main 入口
│   └── makefile          # 编译脚本
├── README.md
├── README.en.md
└── LICENSE
```

## 依赖项

- **编译器**：g++（支持 C++11）
- **操作系统**：Linux（依赖 POSIX 线程）
- **MySQL**：用户名密码默认 `root / 123456`，数据库名 `ChatProject`，需要建表：

```sql
CREATE DATABASE IF NOT EXISTS ChatProject;
USE ChatProject;
CREATE TABLE IF NOT EXISTS USER (
    NAME    VARCHAR(64) PRIMARY KEY,
    PASSWORD VARCHAR(64) NOT NULL
);
```

- **Redis**（可选）：默认 `127.0.0.1:6379`，用于保存登录 session。Redis 未启动时程序仍可运行，只是 Cookie 自动登录会被跳过。

## 编译

```bash
cd src
make          # 同时生成 client 和 server
make client   # 只编译客户端
make server   # 只编译服务端
make clean    # 清理生成的二进制
```

链接库路径写在 `src/makefile` 第 7 行：`-L/usr/lib64/mysql`。如果你的 MySQL 库装在别的目录，需要改成对应路径。

## 运行

### 1) 启动数据库

```bash
# MySQL
systemctl start mysqld  

# Redis
redis-server              # 或 systemctl start redis
```

### 2) 启动服务端

```bash
cd src
./server
# 默认监听 127.0.0.1:8023
# 可在 test_server.cpp 中修改端口 / IP
```

### 3) 启动客户端

开一个新终端：

```bash
cd src
./client
```

## 交互流程

### 客户端菜单

连接成功后客户端会打印：

```
 ------------------
|                  |
| 请输入你要的选项:|
|    0:退出        |
|    1:登录        |
|    2:注册        |
|                  |
 ------------------
```

- `0` — 退出客户端
- `1` — 登录已有账号
- `2` — 注册新账号（用户名重复会提示"已存在"）

### 注册示例

```
请输入你要的选项:
2
注册的用户名:用户1
密码:password
确认密码:password
注册成功！
```

### 登录示例

```
请输入你要的选项:
1
用户名:用户1
密码:password
登录成功
```

如果之前登录过，客户端目录下会生成一个 `cookie.txt` 文件；下次启动会自动用 Cookie 向服务器请求登录，无需再次输入用户名密码（Redis 不可用时会自动跳过）。

## 聊天输入格式

登录成功后，客户端会显示：

```
        欢迎回来,用户1
 -------------------------------------------
|          请选择你要的选项：               |
|              0:退出                       |
|              1:发起单独聊天               |
|              2:发起群聊                   |
 -------------------------------------------
```

### 私聊（选项 1）

```
请输入对方的用户名:用户2          # 假设目标用户是 用户2
请输入你想说的话(输入exit退出):
你好，用户2                        # 回车发送
在吗？                           # 继续发
exit                             # 退出聊天
```

服务端收到后会把消息拼成 `[用户1]:你好，用户2` 转发给 用户2。

### 群聊（选项 2）

```
请输入群号:100                   # 任意整数
请输入你想说的话(输入exit退出):
大家好！
有人在吗？
exit
```

同一群号的其他用户都能收到形如 `[用户1]:大家好！` 的广播。

## 常见问题

1. **编译报错 `mysql.h: No such file or directory`**

   需要安装 MySQL/MariaDB 开发包。例如 Fedora/RHEL：

   ```bash
   sudo dnf install mariadb-devel hiredis-devel
   ```

   或 Ubuntu/Debian：

   ```bash
   sudo apt install libmariadb-dev libmysqlclient-dev libhiredis-dev
   ```

2. **编译报错 `找不到 -lmysqlclient`**

   MySQL 可能装在非标准目录。先找一下真实位置：

   ```bash
   find /usr/lib64 -name "libmysqlclient*"
   ```

   把 `src/makefile` 里的 `-L/usr/lib64/mysql` 改成对应路径即可。

3. **运行时报 `连接redis失败`**

   Redis 没启动。可以忽略，登录 / 注册 / 私聊 / 群聊仍然可用，只是 Cookie 自动登录被跳过。要开启自动登录请先：

   ```bash
   redis-server
   ```

4. **运行时报 `Duplicate entry 'xxx' for key 'USER.PRIMARY'`**

   数据库里已经有这个用户名了。先用已有的用户名密码 **登录**（菜单选 1），不要再选注册。或者手动删一条：

   ```bash
   mysql -uroot -p123456 -e "USE ChatProject; DELETE FROM USER WHERE NAME='xxx';"
   ```

5. **端口被占用**

   两个客户端进程不能同时监听同一端口。或者换一个端口号：改 `test_server.cpp` 里的 `8023`，再 `make`。

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。
