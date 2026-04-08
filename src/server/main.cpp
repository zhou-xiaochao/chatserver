#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>

void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv) {
    if(argc < 3) {
        std::cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000\n";
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);
    signal(SIGINT, resetHandler);
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server (&loop, addr, "ChatServer");
    server.start();
    loop.loop();
}