/**
 * @file diamondd.cpp
 * @brief Diamond Apparatus message broker daemon.
 *
 */

#include <signal.h>
#include <set>
#include <string>

#include "tcp.h"
#include "messages.h"
#include "data.h"

namespace diamondapparatus {

static bool serverKill=false;


class MyServer : public TCPServer {
    // lists of subscribers to each topic
    std::map<std::string,std::set<int> > subscribers;
    // lists of topics for each subscriber
    std::map<int,std::set<std::string> > subtopics;
    std::map<std::string,Topic *> topics;
    
    Topic *findOrCreateTopic(const char *n){
        Topic *t;
        if(topics.find(n)!=topics.end())
            t = topics[n];
        else {
            t = new Topic();
            topics[n]=t;
        }
        return t;
    }
    
    void rebuildTopicsTopic(){
        Topic *t = findOrCreateTopic("topics");
        t->clear();
        std::map<std::string,Topic *>::iterator i;
        for(i=topics.begin();i!=topics.end();++i){
            t->add(Datum(i->first.c_str()));
        }
        // and publish it.
        int pktsize;
        const char *p = t->toMessage(&pktsize,SC_NOTIFY,"topics");
        
        // get the list of subscribers to this topic
        std::set<int>& subs = subscribers["topics"];
        std::set<int>::iterator i2;
        dprintf("--- publish loop for topics\n");
        for(i2=subs.begin();i2!=subs.end();++i2){
            printf("----Sending to %d\n",*i2);
            basesend(*i2,p,pktsize);
        }
        dprintf("--- publish loop done\n");
    }
    
    void subscribe(int fd,const char *n){
        std::string name(n);
        subscribers[name].insert(fd);
        
        std::set<std::string>& subtoplist = subtopics[fd];
        subtoplist.insert(name);
        
        printf("--- added %d to subs for %s\n",fd,n);
        
        // now send any data we already have for that topic
        if(topics.find(n)!=topics.end()){
            Topic *t = topics[n];
            int size;
            const char *p = t->toMessage(&size,SC_NOTIFY,n);
            respond((void *)p,size);
        }
        
    }
    
    void unsubscribe(int fd){
        printf("---- removing %d from subs\n",fd);
        // get the list of topics this FD subscribes to
        std::set<std::string> &topics = subtopics[fd];
        // iterate over them
        std::set<std::string>::iterator i;
        for(i=topics.begin();i!=topics.end();++i){
            // for each one, remove this FD from its subscriber list
            subscribers[*i].erase(fd);
        }
        // finally, remove me from the subtopics map
        subtopics.erase(fd);
    }
    
    void deletetopic(const char *n){
        std::string str(n);
        // get the list of subscribers to this topic
        std::set<int>& subs = subscribers[str];
        std::set<int>::iterator i;
        for(i=subs.begin();i!=subs.end();++i){
            // remove it from the list of subscribed topics
            // for each subscriber
            subtopics[*i].erase(str);
        }
        // erase me...
        subscribers.erase(str);
    }
    
    
    void publish(char *p,uint32_t pktsize){
        // store the data for the topic
        const char *name = Topic::getNameFromMsg(p);
        Topic *t;
        bool isnew=false;
        if(topics.find(name)!=topics.end())
            t = topics[name];
        else {
            t = new Topic();
            topics[name]=t;
            isnew=true;
        }
        t->fromMessage(p);
        
        // data received and stored - now just push the data
        // back to the subscribers.
        
        NoDataMsg *d = (NoDataMsg *)p;
        d->type = htonl(SC_NOTIFY); // overwrite pkt type
        // get the list of subscribers to this topic
        std::set<int>& subs = subscribers[name];
        std::set<int>::iterator i;
        dprintf("--- publish loop for %s\n",name);
        for(i=subs.begin();i!=subs.end();++i){
            printf("----Sending to %d\n",*i);
            basesend(*i,p,pktsize);
        }
        dprintf("--- publish loop done\n");
        
        // rebuild the topics topic if we made a new one
        if(isnew)
           rebuildTopicsTopic();
    }
    
public:
    MyServer(int port) : TCPServer(port){
        rebuildTopicsTopic();
    }
    virtual void process(uint32_t packetsize,void *packet){
        const char *name;
        char *p = (char *)packet;
        StrMsg *sm;
        
        
        uint32_t type = ntohl(*(uint32_t *)p);
//        printf("Packet type %d\n",type);
        
        switch(type){
        case CS_PUBLISH:
            publish(p,packetsize);
            break;
        case CS_SUBSCRIBE:
            sm = (StrMsg *)p;
            subscribe(getfd(),sm->msg);
            break;
        case CS_KILLSERVER:
            serverKill=true;
            break;
        case CS_CLEARSERVER:{
            std::map<std::string,Topic *>::iterator i;
            for(i=topics.begin();i!=topics.end();++i){
                delete i->second;
            }
            topics.clear();
            break;}
        }
    }
    virtual void onClose(int fd){
        unsubscribe(fd);
    }
    
    void shutdown(){
        // send kill to all clients
        NoDataMsg m;
        m.type = htonl(SC_KILLCLIENT);
        for(std::map<int,ClientInServer*>::iterator it=clients.begin();it!=clients.end();++it){
            basesend(it->first,&m,sizeof(m));
        }
        // wait for client death
        sleep(2);
    }
};





MyServer *serv=NULL;
void handler(int sig){
    printf("sig %d caught\n",sig);
    serv->shutdown();
    delete serv;
    exit(1); // no safe way to continue
}

/*
 * 
 * Server entry point (never exits except on failure)
 * 
 * 
 */

void server() {
    const char *pr = getenv("DIAMOND_PORT");
    int port = pr?atoi(pr):DEFAULTPORT;
    
    signal(SIGINT,handler);
    signal(SIGQUIT,handler);
    signal(SIGPIPE,SIG_IGN); // ignore closed socket at far end
    
    serv = new MyServer(port);
    while(serv){
        serv->update();
        if(serverKill){
            serv->shutdown();
            delete serv;
            serv=NULL;
        }
    }
}


}
