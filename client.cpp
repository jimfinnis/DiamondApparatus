/**
 * @file clitest.cpp
 * @brief  Brief description of file.
 *
 */

#include <pthread.h>
#include <string>
#include <map>
#include <queue>

#include "tcp.h"
#include "messages.h"
#include "data.h"
#include "time.h"

namespace diamondapparatus {

/// this is the database the client writes stuff to from its thread

typedef std::map<std::string,Topic *> TopicMap;
static volatile TopicMap topics;

/// primary mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
/// condition used in get-wait
static pthread_cond_t getcond = PTHREAD_COND_INITIALIZER;

static bool volatile running=false;

inline void lock(const char *n){
    dprintf("+++ %p Attemping to lock: %s\n",(void *)pthread_self(),n);
    pthread_mutex_lock(&mutex);
    dprintf("    %p Locked\n",(void *)pthread_self());
}
inline void unlock(const char *n){
    dprintf("--- %p Unlocking: %s\n",(void *)pthread_self(),n);
    pthread_mutex_unlock(&mutex);
}


// the states the client can be in
enum ClientState {
    ST_IDLE // only one state at the moment, but may add more.
};
class MyClient : public TCPClient {
    ClientState state;
public:
    MyClient(const char *host,int port) : TCPClient(host,port){
        state = ST_IDLE;
        waittopic=NULL;
    }
    
    void setState(ClientState s){
        dprintf("====> state = %d\n",s);
        state = s;
    }
    
    // the topic I'm waiting for in get(), if any.
    // If I'm waiting for any topic, this will be the empty string.
    const char *waittopic;
    
    void notify(const char *d){
        dprintf("notify at %p\n",d);
        const char *name = Topic::getNameFromMsg(d);
        TopicMap& tm = (TopicMap &)topics; // lose the volatile
        if(tm.find(std::string(name)) == tm.end()){
            fprintf(stderr,"Topic %s not in client set, ignoring\n",name);
            return;
        }
        Topic *t = tm[name];
        t->fromMessage(d);
        t->state =TOPIC_CHANGED;
        t->timeLastSet = Time::now();
        
        // if we were waiting for this topic, or for any topic, signal.
        if(waittopic && (!*waittopic || !strcmp(name,waittopic))){
            pthread_cond_signal(&getcond);
            waittopic=NULL; // and zero the wait topic.
        }
    }
    
    virtual void process(uint32_t packetsize,void *packet){
        lock("msgrecv");
        char *p = (char *)packet;
        uint32_t type = ntohl(*(uint32_t *)p);
        dprintf("Packet type %d at %p\n",type,packet);
        
        if(type == SC_KILLCLIENT){
            if(waittopic){
                dprintf("Waiting on %s so signalling\n",waittopic);
                pthread_cond_signal(&getcond);
                waittopic=NULL;
            }
            fprintf(stderr,"force exit\n");
            running=false;
        }
        
        switch(state){
        case ST_IDLE:
            if(type == SC_NOTIFY){
                notify(p);
            }
            break;
        }
        unlock("msgrecv");
    }
    
    void subscribe(const char *name){
        lock("subscribe");
        
        Topic *t = new Topic();
        TopicMap& tm = (TopicMap &)topics; // lose the volatile
        tm[name]=t;
        
        StrMsg p;
        p.type = htonl(CS_SUBSCRIBE);
        strcpy(p.msg,name);
        request(&p,sizeof(StrMsg));
        unlock("subscribe");
    }
    
    void publish(const char *name,Topic& d){
        lock("publish");
        int size;
        const char *p = d.toMessage(&size,CS_PUBLISH,name);
        request(p,size);
        free((void *)p);
        unlock("publish");
    }
    
    void simpleMsg(int msg){
        lock("smlmsg");
        NoDataMsg p;
        p.type = htonl(msg);
        request(&p,sizeof(NoDataMsg));
        unlock("smlmsg");
    }
    
    bool isIdle(){
        lock("isidle");
        bool e = (state == ST_IDLE);
        dprintf("Is idle? state=%d, so %s\n",state,e?"true":"false");
        unlock("isidle");
        return e;
    }
    
};


/*
 * 
 * 
 * Threading stuff
 * 
 * 
 */


static pthread_t thread;

static MyClient *client;

static void *threadfunc(void *parameter){
    running = true;
    dprintf("STARTED THREAD\n");
    while(running){
        // deal with requests
        if(!client->update())break;
    }
    dprintf("LOOP EXITING\n");
    delete client;
    running = false;
};
static void waitForIdle(){
    while(running && !client->isIdle()){
        usleep(100000);
    }
    dprintf("Wait done, running=%s, isidle=%s\n",running?"T":"F",
            client->isIdle()?"T":"F");
}


/*
 * 
 * 
 * Interface code
 * 
 * 
 */

static int initCount=0;

void init(){
    // get environment data or defaults
    if(!initCount){
        const char *hn = getenv("DIAMOND_HOST");
        char *hostname = NULL;
        if(hn)hostname = strdup(hn);
        const char *pr = getenv("DIAMOND_PORT");
        int port = pr?atoi(pr):DEFAULTPORT;
        client = new MyClient(hostname?hostname:"localhost",port);
        pthread_create(&thread,NULL,threadfunc,NULL);
        if(hostname)free(hostname);
        while(!running){} // wait for thread
    }
    initCount++;
}    

int destroy(){
    if(initCount && --initCount){
        running=false;// assume atomic :)
        pthread_cond_destroy(&getcond);
        pthread_mutex_destroy(&mutex);
    }
    return initCount;
}

void subscribe(const char *n){
    if(!running)throw DiamondException("not connected");
    client->subscribe(n);
}

void publish(const char *name,Topic& t){
    if(!running)throw DiamondException("not connected");
    client->publish(name,t);
}

void killServer(){
    if(!running)throw DiamondException("not connected");
    client->simpleMsg(CS_KILLSERVER);
}
void clearServer(){
    if(!running)throw DiamondException("not connected");
    client->simpleMsg(CS_CLEARSERVER);
}

bool isRunning(){
    return running;
}

Topic get(const char *n,int wait){
    Topic rv;
    if(!running){
        rv.state = TOPIC_NOTCONNECTED;
        return rv;
    }
    
    lock("gettopic");
    TopicMap& tm = (TopicMap &)topics; // lose the volatile
    if(tm.find(n) == tm.end()){
        // topic not subscribed to
        rv.state = TOPIC_NOTFOUND;
        unlock("gettopic");
        return rv;
    }
    
    // we are connected and subscribed to this topic
    
    Topic *t = tm[n];
    client->waittopic=NULL;
    dprintf("Topic state: %d\n",t->state);
    if(t->state == TOPIC_NODATA){
        // if WaitAny, wait for data
        if(wait == GET_WAITANY || wait == GET_WAITNEW){
            dprintf("No data present : entering wait\n");
            while(t->state == TOPIC_NODATA){
                client->waittopic = n;
                // stalls until new data arrives, but unlocks mutex
                // during the wait and relocks it after.
                pthread_cond_wait(&getcond,&mutex);
                client->waittopic=NULL;
                if(!running){
                    // in this case, forced exit killed the thread
                    rv.state = TOPIC_NOTCONNECTED;
                    unlock("gettopic");
                    return rv;
                }
                dprintf("Data apparently arrived!\n");
            }
            // should now have data and mutex locked again
        } else {
            unlock("gettopic");
            rv.state = TOPIC_NOTFOUND;
            return rv;
        }
    } else if(wait == GET_WAITNEW){
        dprintf("Entering waitnew\n");
        while(t->state != TOPIC_CHANGED){
            client->waittopic = n;
            // stalls until new data arrives, but unlocks mutex
            // during the wait and relocks it after.
            pthread_cond_wait(&getcond,&mutex);
            client->waittopic=NULL;
            if(!running){
                // in this case, forced exit killed the thread
                rv.state = TOPIC_NOTCONNECTED;
                unlock("gettopic");
                return rv;
            }
        }
        // should now have data and mutex locked again
    }
    dprintf("Data unchanged\n");
    rv = *t;
    t->state = TOPIC_UNCHANGED;
    unlock("gettopic");
    return rv;
}

/// wait for any subscribed topic
void waitForAny(){
    lock("wait");
    client->waittopic = ""; // empty string means any
    // stalls until new data arrives, but unlocks mutex
    // during the wait and relocks it after.
    pthread_cond_wait(&getcond,&mutex);
    client->waittopic=NULL;
    unlock("wait");
}

}
    


