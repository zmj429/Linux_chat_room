#ifndef GLOBAL_H  //防止头文件重复包含的宏定义开始
#define GLOBAL_H  //定义头文件保护宏

#include <stdio.h>      //标准输入输出库，提供printf、scanf等函数
#include <stdlib.h>     //标准库，提供malloc、free、exit等函数
#include <string.h>     //字符串处理库，提供strcpy、strcmp、memset等函数

#include <iostream>     //C++标准输入输出流，提供cin、cout等对象
#include <thread>       //C++多线程库，提供std::thread类
#include <vector>       //C++容器库，提供std::vector动态数组

#include <sys/types.h>  //系统类型定义，提供基本数据类型如pid_t、size_t等
#include <sys/socket.h> //套接字编程库，提供socket、bind、listen、accept等函数
#include <netinet/in.h> //网络地址结构定义，提供sockaddr_in结构体
#include <arpa/inet.h>  //IP地址转换库，提供inet_addr、inet_ntoa等函数
#include <unistd.h>     //Unix标准库，提供close、read、write等函数
#include <fcntl.h>      //文件控制库，提供文件描述符相关操作

#endif // GLOBAL_H  //防止头文件重复包含的宏定义结束
