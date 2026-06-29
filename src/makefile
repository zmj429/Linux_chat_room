# Makefile for online chat room

CXX = g++
CXXFLAGS = -std=c++11 -Wall
LDFLAGS = -lpthread
# MySQL/MariaDB + Redis 链接库
LIBS_SERVER = -lpthread -L/usr/lib64/mysql -lmysqlclient -lhiredis

# 默认目标：生成客户端和服务端可执行文件
all: client server

# 编译客户端（需要 test_client.cpp 中的 main）
client: client.cpp client.h global.h test_client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp test_client.cpp $(LDFLAGS)

# 编译服务端（需要 test_server.cpp 中的 main，并且链接数据库库）
server: server.cpp server.h global.h test_server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp test_server.cpp $(LIBS_SERVER)

# 清理编译生成的文件
.PHONY: clean
clean:
	rm -f client server
