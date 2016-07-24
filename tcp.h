/**
 * @file tcp.h
 * @brief  Brief description of file.
 *  binary tcp, variable size recv blocks and send blocks,
 *  each starting with a count.
 * 
 * Weirdly slow - about 12 msgs/sec. ???
 */

#ifndef __TCP_H
#define __TCP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include <map>

#define DEFAULTPORT 29921

//#define dprintf printf
#define dprintf if(0)printf

#include "diamondapparatus.h"
using namespace diamondapparatus;

/// call to do stuff to the socket
inline void setsock(int fd){
    int v;
    // larger buffer, reuseaddr
//    v=8192;
//    if(setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(char *)&v,sizeof(v))<0)
//        throw DiamondException("set recv buf size failed");
    v=1;
    if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&v,sizeof(v))<0)
        throw DiamondException("set reuse failed");
    
//    // set to nonblocking
//    v = fcntl(fd,F_GETFL);
//    v |= O_NONBLOCK;
//    fcntl(fd,F_SETFL,v);
}


/// call to send data
inline void basesend(int fd,void *data,uint32_t size){
    dprintf("Sending %d bytes\n",size);
    uint32_t sizetosend = htonl(size);
    // retry some
    for(int i=0;i<10;i++){
        // write the packet size
        int rv = write(fd,&sizetosend,sizeof(sizetosend));
        dprintf("send size: %d\n",rv);
        int rv2 = write(fd,data,size);
        dprintf("send dsize: %d\n",rv2);
        
        if(rv>0 && rv2>0)return;
        if(errno==EPIPE){
            printf("Broken pipe, giving up\n");
            return;
        }
        
        // if either failed, retry.
        usleep(100000);
        printf("retry...\n");
    }
    throw DiamondException("cannot send");
}

/// interface for things which process stuff.
class TCPProcessor {
public:
    virtual void process(uint32_t packetsize,void *packet)=0;
};

/// this handles receiving data from a socket. The server has one of these
/// for each client, the client has only one.
class TCPReceiver {
    int fd;
public:
    uint32_t recvct; // number of bytes received of the message so far.
    
    char *recvdata; // the buffer we're going to fill with data
    uint32_t recvsize; // the size of the data (recvdata null if we don't know yet)
    bool portClosed;
    bool isForceExit;
    
    TCPReceiver(){
        recvct=0;
        // initially we're getting the size
        recvdata=NULL;
        recvsize=0; // not relevant until receiving data
        portClosed = false;
        isForceExit = false;
    }
    
    void setfd(int i){fd=i;}
    
    bool isClosed(){
        return portClosed;
    }
    
    void forceExit(){
        isForceExit=true;
    }
          
    
    /// call to receive any data, calls process and returns if
    /// is a complete message.
    /// Returns false if port closed or no data received.
    /// Check isClosed() afterwards.
    bool update(TCPProcessor *p){
        dprintf("ENTERING UPDATE LOOP\n");
        bool rv=false;
        for(;;){
            if(!recvdata){
                dprintf("receiving size int\n");
                // first phase - get the message size
                char *sizeptr = (char *)&recvsize;
                int size = read(fd,sizeptr+recvct,
                                    sizeof(uint32_t)-recvct);
                dprintf("Size %d\n",size);
                if(size<0)portClosed=true;
                if(size<=0)break;
                rv=true;
                recvct+=size;
                dprintf("Size received now %d, waiting for %ld\n",
                        recvct,sizeof(uint32_t));
                if(recvct==sizeof(uint32_t)){
                    // got the whole size, allocate and prep to receive
                    // payload
                    if(recvdata)
                        free(recvdata);
                    recvsize = ntohl(recvsize);
                    recvdata = (char *)malloc(recvsize);
                    dprintf("Size received - allocated buffer of %d at %p\n",recvsize,recvdata);
                    recvct=0;
                }
            }
            if(recvdata){
                dprintf("receiving data\n");
                // second phase - have a buffer to get the actual
                // data into
                int size = read(fd,recvdata+recvct,
                                recvsize-recvct);
                dprintf("Size2 %d\n",size);
                if(size<0)portClosed=true;
                if(size<=0)break;
                rv=true;
                recvct+=size;
                dprintf("Size2 received now %d, waiting for %d\n",
                        recvct,recvsize);
                if(recvct==recvsize){
                    dprintf("Message received on %d, size %d\n",fd,recvsize);
                    // got the whole packet
                    // process and reset
                    if(p)
                        p->process(recvsize,recvdata);
                    else
                        dprintf("No processor installed\n");
                    recvct=0;
                    free(recvdata);
                    recvdata=NULL;
                    recvsize=0;
                    return true; // must be 
                }
            }
            if(isForceExit)return false;
        }
        return rv;
    }
};



/// To send a request, fill in "req" and call request().
/// Call update() to poll, which will call process() when data
/// arrives. The response will be in "resp." 

class TCPClient : public TCPProcessor {
    int fd;
    TCPReceiver r;
public:
    TCPClient(const char *hostname,int port) {
        printf("Starting client\n");
        struct sockaddr_in addr;
        struct hostent *server;
        fd = socket(AF_INET,SOCK_STREAM,0);
        if(fd<0)throw DiamondException("cannot open socket");
        server = gethostbyname(hostname);
        if(!server)throw DiamondException("cannot find hostname");
        memset(&addr,0,sizeof(addr));
        addr.sin_family=AF_INET;
        memcpy(&addr.sin_addr.s_addr,server->h_addr,
               server->h_length);
        addr.sin_port = htons(port);
        
        if(connect(fd,(struct sockaddr *)&addr,sizeof(addr))<0)
            throw DiamondException("error connecting");
        
        setsock(fd);
        r.setfd(fd);
    }
    
    ~TCPClient(){
        close(fd);
    }
    
    void request(void *data,uint32_t size){
        basesend(fd,data,size);
    }
    
    // returns false if no data
    bool update(){
        return r.update(this);
    }
    // returns true if port closed by peer
    bool isClosed(){
        return r.isClosed();
    }
};


struct ClientInServer {
    int fd;
    TCPReceiver r;
};

/// server object - override process() 
class TCPServer : TCPProcessor {
    int listenfd;
    fd_set active_fd_set;
    ClientInServer *curClient;
protected:    
    std::map<int,ClientInServer *> clients;
public:
    TCPServer(int port) {
        printf("Starting server\n");
        sockaddr_in servaddr;
        
        listenfd = socket(AF_INET,SOCK_STREAM,0);
        if(listenfd<0)throw DiamondException("cannot open socket");
        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_addr.s_addr=INADDR_ANY;
        servaddr.sin_port=htons(port);
        if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
            throw DiamondException("cannot bind");
        listen(listenfd,5);
        // initialise active fd set with listening socket
        FD_ZERO(&active_fd_set);
        FD_SET(listenfd,&active_fd_set);
        
        setsock(listenfd);
    }
    
    void update(){
        sockaddr_in cliaddr;
        fd_set read_fd_set;
        read_fd_set=active_fd_set;
        int srv = select(FD_SETSIZE,&read_fd_set,NULL,NULL,NULL);
        
        if(srv<0)
            throw DiamondException("select failed");
        if(srv>0){
            for(int i=0;i<FD_SETSIZE;i++){
                if(FD_ISSET(i,&read_fd_set)){
                    if(i==listenfd){
                        // it's on the listen socket; accept new connection
                        socklen_t clilen = sizeof(cliaddr);
                        int fd = accept(listenfd,
                                        (struct sockaddr *)&cliaddr,&clilen);
                        if(fd<0)
                            throw DiamondException("accept failed");
                        fprintf(stderr,"New connection from %s, port %d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
                        ClientInServer *c = new ClientInServer();
                        c->fd = fd;
                        c->r.setfd(fd);
                        clients[fd]=c;
                        FD_SET(fd,&active_fd_set);
                    } else {
                        // receive data, clear the socket if failed.
                        curClient = clients[i];
                        if(!curClient->r.update(this)){
                            printf("Closing client %d\n",i);
                            // no data received
                            close(i);
                            FD_CLR(i,&active_fd_set);
                            delete curClient;
                            clients.erase(i);
                            onClose(i);
                        }
                        curClient=NULL;
                    }
                }
            }
        }
    }
    
    virtual void onClose(int fd){}
    
    virtual ~TCPServer(){
        printf("Closing down server\n");
        close(listenfd);
        
        for(std::map<int,ClientInServer*>::iterator it=clients.begin();it!=clients.end();++it){
            close(it->first);
            delete it->second;
        }
    }
    
    
    /// only call from inside process()
    void respond(void *data,uint32_t size){
        if(!curClient)
            throw DiamondException("cannot call respond() outside process()");
        basesend(curClient->fd,data,size);
    }
    
    /// return FD for current client in process()
    int getfd(){
        if(!curClient)
            throw DiamondException("cannot call getfd() outside process()");
        return curClient->fd;
    }
        
        
};

#endif /* __TCP_H */
