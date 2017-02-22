//
// Created by crem_ja on 2/2/17.
//

#include "cli_server.h"
#include <unistd.h>
#include <arpa/inet.h>
#include "robotkernel/rt_helper.h"
#include "robotkernel/kernel.h"

#define lockMutex(mtx) \
        if(pthread_mutex_lock(&mtx) != 0) {\
            printf("Fatal: Unable to lock mutex in %s:%d  ->  %s", __FILE__, __LINE__, strerror(errno));\
            exit(-1);\
        }

#define unlockMutex(mtx) \
        if(pthread_mutex_unlock(&mtx) != 0) {\
            printf("Fatal: Unable to unlock mutex in %s:%d  ->  %s", __FILE__, __LINE__, strerror(errno));\
            exit(-1);\
        }

using namespace robotkernel;

namespace cli_bridge {
    
        CliServer::CliServer(int port) : socketFD(-1), addr(), stopRequested(false), onConnectHandler(NULL), onDisconnectHandler(NULL) {
            if(port < 0 || port > 0xffff){
                throw str_exception("Invalid port number: %d", port);
            }
            socketFD = socket(PF_INET, SOCK_STREAM, 0);
            if (socketFD == -1) {
                throw str_exception("Unable zo create CLI interface socket (ERRNO: %d)", errno);
            }
            bzero(&addr, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons((uint16_t) port);
            for (int i = 1; i <= 5 && bind(socketFD, (const sockaddr *) &addr, sizeof(addr)) == -1; ++i) {
                klog(warning, "Unable to bind socket on port %d (ERRNO: %d)", ntohs(addr.sin_port), errno);
                addr.sin_port = htons((uint16_t) (port+i));
            }
        }

        CliServer::~CliServer() {
            if (socketFD != -1) {
                close(socketFD);
            }
        }

        void *CliServer::run(void *args) {
            CliServer *self = (CliServer *) args;
            setPriority(50, SCHED_OTHER);
            setAffinityMask(0xff);
            
            listen(self->socketFD, 3);

            klog(info, "CliServer: waiting for connections on port %d ...\n", ntohs(self->addr.sin_port));
            while (!self->stopRequested) {
                try {
                    new CliConnection(self->socketFD, self);
                } catch (str_exception &e) {
                    if (!self->stopRequested) {
                        klog(warning, "%s\n", e.what());
                        sleep(1);
                    }
                }
            }
            return NULL;
        }

        void CliServer::start() {
            pthread_create(&serverThread, NULL, CliServer::run, this);
        }

        void CliServer::stop() {
            this->stopRequested = true;
            pthread_cancel(serverThread);

            while (!CliConnection::all.empty()) {
                CliConnection *c = CliConnection::all.front();
                c->close();
                delete c;
            }
        }

    void CliServer::onConnect(CliConnection* client){
        if(this->onConnectHandler){
            this->onConnectHandler(client);
        }
    }
    
    void CliServer::onDisconnect(CliConnection *client){
        if(this->onDisconnectHandler){
            this->onDisconnectHandler(client);
        }
    }
    

    // ############# CliConnection
    
    CliConnection::List CliConnection::all;
    pthread_mutex_t CliConnection::allConnectionsLock = PTHREAD_MUTEX_INITIALIZER;

    CliConnection::CliConnection(int socketFD, CliServer* server) : connFD(-1), addr(), connectionThread(), lock(PTHREAD_MUTEX_INITIALIZER), cliServer(server), messageHandler(NULL) {
        socklen_t size = sizeof(addr);
        bzero(&addr, size);
        connFD = accept(socketFD, (struct sockaddr *) &addr, &size);
        if (connFD == -1)
            throw str_exception("CliConnection: accept() error -> %s", strerror(errno));
        klog(info, "CliConnection: established (%s)\n", getRemoteName().c_str());
        pthread_create(&connectionThread, NULL, CliConnection::run, this);
    }

    CliConnection::~CliConnection() {
        close();
    }
    
    void* CliConnection::run(void *args) {
        CliConnection* self = (CliConnection *) args;
        int N = 256*1; //currently max message size maybe not enough for future
        char* buf = new char[N];
        bzero(buf, N);

        lockMutex(allConnectionsLock);
        CliConnection::all.push_front(self);
        unlockMutex(allConnectionsLock);

        self->cliServer->onConnect(self);
        
        while(!self->stopRequested){
            ssize_t num = read(self->connFD, buf, N-1);
            if(num == N-1){
                klog(error, "Input buffer too small for message!");
                //TODO reallocate input buffer for messages greater than N
                //char* buf = realloc()
            }
            if (num <= 0){
                if(num == -1) {
                    switch (errno) {
                        case ETIMEDOUT:
                            klog(error, "CliConnection: Read timed out (%s)\n", strerror(errno));
                            continue;
                        case EAGAIN:
                        case EINTR:
                            continue;
                        case ECONNRESET:
                            klog(warning, "CliConnection: Connection reset by %s (%s)\n", self->getRemoteName().c_str(), strerror(errno));
                            break;
                        default:
                            klog(error, "CliConnection: Error reading data: %s -> closing connection\n",
                                 strerror(errno));
                            break;
                    }
                } else if(num == 0){
                    klog(info, "CliConnection: Connection closed by %s\n", self->getRemoteName().c_str());
                }
                self->close();
                delete self;
                goto FINALLY;
            }
            buf[num] = 0;
            klog(info, "CliConnection: READ: %s\n", buf);
            if(self->messageHandler){
                self->messageHandler(self, buf, num);
            }
        }
        FINALLY: {
            free(buf);
            return NULL;
        }
    }
    

    void CliConnection::close() {
        lockMutex(lock);
        if(connFD == -1) {
            return;
        }
        klog(info, "CliConnection: closing connection to %s\n", getRemoteName().c_str());
        this->stopRequested = true;
        
        cliServer->onDisconnect(this);
        lockMutex(allConnectionsLock);
        CliConnection::all.remove_if([this](CliConnection *c) {
            return this == c;
        });
        unlockMutex(allConnectionsLock);
        if(pthread_self() != connectionThread) {
            if (pthread_cancel(connectionThread) == 0) {
                if (pthread_join(connectionThread, NULL) != 0) {
                    klog(warning, "CliConnection: close() Unable to join reader thread: %s", strerror(errno));
                }
            } else {
                klog(warning, "CliConnection: close() Unable to cancel reader thread: %s", strerror(errno));
            }
        }
        ::close(connFD);
        connFD = -1;
        unlockMutex(lock);
        klog(info, "CliConnection: connection closed (%s)\n", getRemoteName().c_str());
    }

    std::string CliConnection::getRemoteName(){
        return std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(ntohs(addr.sin_port));
    }
    
    bool CliConnection::write(const char *msg, size_t len) {
        while(len > 0) {
            ssize_t num = ::write(connFD, msg, len);
            if(num == -1) {
                switch (errno) {
                    case ETIMEDOUT:
                        klog(error, "CliConnection: Read timed out (%s)\n", strerror(errno));
                        continue;
                    case EAGAIN:
                    case EINTR:
                        continue;
                    case ECONNRESET:
                        klog(warning, "CliConnection: Connection reset by %s (%s)\n", getRemoteName().c_str(), strerror(errno));
                        return false;
                    default:
                        klog(error, "CliConnection: Error writing data: %s -> closing connection\n", strerror(errno));
                        return false;
                }
            }
            msg += num;
            len -= num;
        }
        return true;
    }
}
