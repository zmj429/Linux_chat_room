# Linux Chat Room Project

## Project Introduction

A network chat room system implemented in C++ with object-oriented design.

This project is a simple chat room based on TCP protocol, supporting multiple clients to connect to the server simultaneously for message sending and receiving.

### Features

- Multi-threading: The server uses multiple threads to handle multiple client connections
- Message sending/receiving: Clients can send and receive messages
- Easy to use: Clear code structure, suitable for learning and extension

### Project Source

This project is based on a practical course from Lanqiao Cloud Class (formerly Lab Building). URL: `https://www.lanqiao.cn/courses/3573`

## Project Structure

```
online-chat-room/
├── src/                    # Source code directory
│   ├── client.cpp         # Client implementation (includes main function)
│   ├── client.h           # Client class definition
│   ├── server.cpp         # Server implementation (includes main function)
│   ├── server.h           # Server class definition
│   ├── global.h           # Common header file
│   └── makefile           # Build script
├── README.md              # Project documentation (Chinese)
├── README.en.md           # Project documentation (English)
├── LICENSE                # License file
└── .gitignore             # Git ignore file
```

## Dependencies

This project requires the following environment:

- **Compiler**: g++ compiler supporting C++11 standard
- **Operating System**: Linux/Unix system (uses POSIX thread library)

## Compilation and Running

### Compile the Project

Navigate to the src directory and execute the make command:

```bash
cd src
make
```

This will compile both client and server programs. You can also compile them separately:

```bash
make client  # Compile client only
make server  # Compile server only
```

### Run the Server

```bash
./server              # Use default configuration (IP: 0.0.0.0, Port: 8080)
./server 192.168.1.100 8080  # Specify IP and port
```

### Run the Client

```bash
./client              # Use default configuration (IP: 127.0.0.1, Port: 8080)
./client 192.168.1.100 8080  # Specify server IP and port
```

### Clean Build Files

```bash
make clean
```

## Code Description

### Server

- Listens on specified port, waiting for client connections
- Creates a separate thread for each client connection
- Receives client messages and displays them on the server side
- Supports multiple client connections simultaneously

### Client

- Connects to the server
- Uses two threads: one for sending messages, one for receiving messages
- Type `exit` to exit the program

## Common Issues

1. **Thread library not found during compilation**

   Make sure the pthread library is installed:
   ```bash
   sudo apt-get install pthread
   ```

2. **Port already in use**

   Check if the port is occupied by another program, or use a different port number.

3. **Connection refused**

   Make sure the server is running, and check if the IP address and port number are correct.

## License

This project is licensed under the MIT License, see the [LICENSE](LICENSE) file for details.
