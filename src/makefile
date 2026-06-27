# Makefile for online chat room

CXX = g++
CXXFLAGS = -std=c++11 -Wall
LDFLAGS = -lpthread

# 目标文件
all: client server

# 编译客户端
client: client.cpp client.h global.h
	$(CXX) $(CXXFLAGS) -o client client.cpp $(LDFLAGS)

# 编译服务端
server: server.cpp server.h global.h
	$(CXX) $(CXXFLAGS) -o server server.cpp $(LDFLAGS)

# 清理编译生成的文件
.PHONY: clean
clean:
	rm -f client server

# 单独编译客户端
.PHONY: client
client: client.cpp client.h global.h
	$(CXX) $(CXXFLAGS) -o client client.cpp $(LDFLAGS)

# 单独编译服务端
.PHONY: server
server: server.cpp server.h global.h
	$(CXX) $(CXXFLAGS) -o server server.cpp $(LDFLAGS)