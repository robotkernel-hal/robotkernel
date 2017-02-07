//
// Created by crem_ja on 2/2/17.
//

#ifndef PROJECT_CLISERVER_H
#define PROJECT_CLISERVER_H

#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <list>
#include <deque>
#include <mutex>
#include <string>
#include <functional>

namespace cli_bridge {
    
    class CliServer;
    
    class CliConnection {
    private:
        int connFD;
        struct sockaddr_in addr;
        bool stopRequested;
        pthread_t connectionThread;
        pthread_mutex_t lock;
        CliServer* cliServer;

        static void* run(void* args);


    public:
        std::function<void(CliConnection*, char*, ssize_t)> messageHandler;
        typedef std::list<CliConnection*> List;
        static CliConnection::List all;
        static pthread_mutex_t allConnectionsLock;

        CliConnection(int socketFD, CliServer* cliServer);
        ~CliConnection();
        void close();
        std::string getRemoteName();
        bool write(const char* msg, size_t len);
    };    
    
    class CliServer {
        
    private:
        int socketFD;
        struct sockaddr_in addr;
        bool stopRequested;
        pthread_t serverThread;
        static void* run(void* args);

        
    public:
        std::function<void(CliConnection*)> onConnectHandler;
        std::function<void(CliConnection*)> onDisconnectHandler;
                
        CliServer(int port);
        ~CliServer();
        void start();
        void stop();
        void onConnect(CliConnection* client);
        void onDisconnect(CliConnection *client);

    };
}


#endif //PROJECT_CLISERVER_H
