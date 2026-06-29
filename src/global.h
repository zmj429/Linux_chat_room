// ============================================================
// global.h  ——  全局头文件与通用依赖
// 统一引入套接字、线程、数据库(MariaDB)、Redis、STL 等常用头文件
// ============================================================
#ifndef _GLOBAL_H              // 防止头文件被重复包含
#define _GLOBAL_H

// ---------- 系统 / POSIX 头文件 ----------
#include <sys/types.h>          // 基础类型定义（如 ssize_t, pid_t）
#include <sys/socket.h>         // 套接字编程基础（socket/bind/listen/accept）
#include <stdio.h>              // 标准 I/O（printf/scanf/fopen 等）
#include <netinet/in.h>         // IPv4 网络地址结构 sockaddr_in/htons/ntohs
#include <arpa/inet.h>          // IP 地址字符串与二进制转换 inet_pton/inet_ntop
#include <unistd.h>             // close/read/write 等 POSIX 接口
#include <string.h>             // C 风格字符串操作 memcpy/memset/strcmp
#include <stdlib.h>             // 内存分配、程序退出、随机数等
#include <fcntl.h>              // 文件描述符控制 F_SETFL/F_NONBLOCK
#include <sys/shm.h>            // System V 共享内存 shmget/shmat 等

// ---------- C++ 标准库 ----------
#include <iostream>             // 标准输入输出流 cout/cin
#include <thread>               // 多线程支持 std::thread
#include <vector>               // 动态数组容器
#include <string>               // 字符串类 std::string

// ---------- 第三方数据库 / 缓存 ----------
#include <mysql/mysql.h>        // MySQL客户端接口
#include <unordered_map>        // 哈希表容器（KV 查找 O(1)）
#include <mutex>                // 互斥锁 std::mutex（多线程同步）
#include <set>                  // 有序集合 std::set
#include <hiredis/hiredis.h>    // Redis C 客户端 hiredis
#include <fstream>              // 文件流 ifstream/ofstream

using namespace std;            // 使用 std 命名空间，避免书写 std:: 前缀

#endif                          // 结束头文件保护宏
