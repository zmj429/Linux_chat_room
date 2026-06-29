// 引入标准输入输出流
#include <iostream>
// 引入 hiredis C 客户端库头文件
#include <hiredis/hiredis.h>
// 使用标准命名空间
using namespace std;

// Redis 连接测试主程序入口
int main(){
    // 连接本机 Redis 服务（默认端口 6379）
    redisContext *c = redisConnect("127.0.0.1",6379);
    // 判断连接是否出错
    if(c->err){
       // 释放 Redis 上下文资源
       redisFree(c);
       // 打印连接失败信息
       cout<<"连接失败"<<endl;
       // 返回非 0 表示异常退出
       return 1;
    }
    // 打印连接成功信息
    cout<<"连接成功"<<endl;
    // 发送 PING 命令验证连通性，强转为 redisReply 类型
    redisReply *r = (redisReply*)redisCommand(c,"PING");
    // 打印 PING 的返回内容（通常为 "PONG"）
    cout<<r->str<<endl;
    // 正常退出
    return 0;
}
