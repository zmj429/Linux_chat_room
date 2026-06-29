// 引入客户端头文件
#include "client.h"

// 客户端主程序入口
int main(){
    // 创建客户端实例，连接本机 8023 端口
    client clnt(8023,"127.0.0.1");
    // 启动客户端运行
    clnt.run();
}
