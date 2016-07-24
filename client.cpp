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

#include "time.h"

// a topic we can subscribe to

struct Topic {
#define TSTATE_NONE 0
#define TSTATE_UNCHANGED 1
#define TSTATE_CHANGED 2
    
    Topic(){
        count = -1;
        f = NULL;
        state = TSTATE_NONE;
    }
    
    void set(int ct, float *ff){
        if(f)delete [] f;
        count = ct;
        f = new float[ct];
        for(int i=0;i<count;i++)f[i]=ff[i];
    }
    
    Topic(Topic *t){
        f=NULL;
        set(t->count,t->f);
    }
    ~Topic(){
        delete [] f;
    }
    
    int state;
    int count;
    float *f;
    
};


/// this is the database the client writes stuff to from its thread
static std::map<std::string,Topic *> topics;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
static bool running=false;

inline void lock(const char *n){
    dprintf("+++Attemping to lock: %s\n",n);
    pthread_mutex_lock(&mutex);
}
inline void unlock(const char *n){
    dprintf("---Unlocking: %s\n",n);
    pthread_mutex_unlock(&mutex);
}


// the states the client can be in
enum ClientState {
    ST_IDLE,
    ST_AWAITACK
};
class MyClient : public TCPClient {
    ClientState state;
public:
    MyClient(const char *host,int port) : TCPClient(host,port){
        state = ST_IDLE;
    }
    
    void setState(ClientState s){
        dprintf("====> state = %d\n",s);
        state = s;
    }
    
    void notify(Data *d){
        dprintf("notify at %p\n",d);
        d->ntoh();
        if(topics.find(std::string(d->name)) == topics.end()){
            fprintf(stderr,"Topic %s not in client set, ignoring\n",d->name);
            return;
        }
        Topic *t = topics[d->name];
        float *f = d->getfloats();
        t->set(d->count,f);
        t->state = TSTATE_CHANGED;
    }
    
    virtual void process(uint32_t packetsize,void *packet){
        lock("msgrecv");
        char *p = (char *)packet;
        SCAck *ack;
        uint32_t type = ntohl(*(uint32_t *)p);
        dprintf("Packet type %d at %p\n",type,packet);
        
        if(type == SC_KILLCLIENT){
            fprintf(stderr,"force exit\n");
            running=false;
        }
        
        switch(state){
        case ST_IDLE:
            if(type == SC_NOTIFY){
                notify((Data *)p);
            }
            break;
        case ST_AWAITACK:
            ack = (SCAck *)p;
            if(type != SC_ACK)
                throw "unexpected packet in awaiting ack";
            else if(ack->code){
                dprintf("Ack code %d: %s\n",ntohs(ack->code),ack->msg);
                throw "bad code from ack";
            } else
                dprintf("Acknowledged\n");
            setState(ST_IDLE);
            break;
        }
        unlock("msgrecv");
    }
    
    void subscribe(const char *name){
        lock("subscribe");
        StrMsg p;
        p.type = htonl(CS_SUBSCRIBE);
        strcpy(p.msg,name);
        request(&p,sizeof(StrMsg));
        //TODO - ADD TIMEOUT
        setState(ST_AWAITACK);
        unlock("subscribe");
    }
    
    void publish(const char *name,float *f,int size){
        lock("publish");
        Data dummy; // just for sizing
        dummy.count = size;
        Data *d = (Data *)malloc(dummy.size());
        d->type = htonl(CS_PUBLISH);
        d->count = size;
        strcpy(d->name,name);
        memcpy(d->getfloats(),f,sizeof(*f)*size);
//        d->dump();
        d->hton();
        request(d,dummy.size());
        //TODO - ADD TIMEOUT
        setState(ST_AWAITACK);
        unlock("publish");
    }
    
    void killServer(){
        lock("kill");
        NoDataMsg p;
        p.type = htonl(CS_KILLSERVER);
        request(&p,sizeof(StrMsg));
        unlock("kill");
    }
    
    bool isIdle(){
        lock("isidle");
        bool e = (state == ST_IDLE);
        dprintf("Is idle? state=%d, so %s\n",state,e?"true":"false");
        pthread_mutex_unlock(&mutex);
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
    Topic *t;
    while(running){
        // deal with requests
        if(!client->update())break;
//        if(client->isClosed())break;
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

namespace diamondapparatus {
void init(){
    // get environment data or defaults
    const char *hn = getenv("DIAMOND_HOST");
    char *hostname = NULL;
    if(hn)hostname = strdup(hn);
    const char *pr = getenv("DIAMOND_PORT");
    int port = pr?atoi(pr):0;
    
    client = new MyClient(hostname?hostname:"localhost",port);
    pthread_mutex_init(&mutex,NULL);
    pthread_create(&thread,NULL,threadfunc,NULL);
    if(hostname)free(hostname);
    while(!running){} // wait for thread
}    

void destroy(){
    running=false;// assume atomic :)
}

void subscribe(const char *n){
    if(!running)throw DiamondException("not connected");
    Topic *t = new Topic();
    topics[n]=t;
    client->subscribe(n);
    waitForIdle(); // wait for the ack
}

void publish(const char *n,float *f,int ct){
    if(!running)throw DiamondException("not connected");
    client->publish(n,f,ct);
    waitForIdle(); // wait for the ack
}

void killServer(){
    if(!running)throw DiamondException("not connected");
    client->killServer();
}

bool isRunning(){
    return running;
}

int get(const char *n,float *out,int maxsize){
    if(!running)return -3;
    if(topics.find(n) == topics.end()){
        return -1; // no such topic subscribed to
    }
    Topic *t = topics[n];
    lock("gettopic");
    
    int rv,i;
    
    switch(t->state){
    case TSTATE_NONE:
        rv = -2;
        break;
    case TSTATE_CHANGED:
        for(i=0;i<t->count && i<maxsize;i++)
            out[i]=t->f[i];
        t->state = TSTATE_UNCHANGED;
        rv = i;
        break;
    case TSTATE_UNCHANGED:
        rv = 0;
        break;
    }
    unlock("gettopic");
    return rv;
}

}

