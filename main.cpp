/**
 * @file main.cpp
 * @brief The main Diamond Apparatus program, C++ version.
 * A C version can be found in mainc.c
 *
 */

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "diamondapparatus.h"
#include "time.h"

using namespace diamondapparatus;

static bool daemonizeServer=false;
static bool listenHeaders=true;
static bool replaceSlash=false;
static bool exitListenLoop=false;
static bool listening=false;

void usagepanic(){
    fprintf(stderr,"Diamond Apparatus %s (%s)\n",VERSION,VERSIONNAME);
    fprintf(stderr,"Usage: diamond server [-d] | pub <topic> <types> <val>... | listen [-hs] <topic> | show <topic> | kill | version\n");
    fprintf(stderr,"       types is a string of chars, f=float, s=string.\n");
    fprintf(stderr,"       Specify -ve numbers with leading n, not -.\n");
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"    -d : (server) start server as daemon\n");
    fprintf(stderr,"    -h : (listen) suppress header line\n");
    fprintf(stderr,"    -s : (server) replace / with . (for R)\n");
    
    exit(1);
}

void handler(int sig){
    if(listening){
        exitListenLoop=true;
    } else {
        printf("sig %d caught\n",sig);
        destroy(); // shut down client
        exit(1); // no safe way to continue
    }
}


// the listen command
void cmdListen(const char *topic){
    bool headerDone=false;
    init();
    subscribe(topic);
    char *printedname=strdup(topic);
    
    if(replaceSlash && listenHeaders){
        for(char *p = printedname;*p;p++)
            if(*p=='/')*p='.';
    }
    
    listening=true;
    while(isRunning() && !exitListenLoop){
        Topic t = get(topic,GET_WAITNEW);
        if(!headerDone){
            headerDone=true;
            if(listenHeaders){
                for(int i=0;i<t.size();i++){
                    printf("%s%d%s",printedname,i,i<t.size()-1?",":"\n");
                }
            }
        }
        if(t.isValid()){
           t.dump();
       }
    }
    free(printedname);
    listening=false;
}


int main(int argc,char *argv[]){
    char c;
    
    Time::init();
    
    while((c=getopt(argc,argv,"dhs"))!=-1){
        switch(c){
        case 'd':
            daemonizeServer=true;
            break;
        case 'h':
            listenHeaders=false;
            break;
        case 's':
            replaceSlash=true;
            break;
        }
    }
    
    if(optind>=argc){
        try {
            init();
            destroy();
            printf("Server is running.\n");
        } catch(DiamondException e){
            printf("Server is not running.\n");
        }
        usagepanic();
    }
    
    // consume args
    argv+=optind;
    argc-=optind;
    
    
    if(!strcmp(argv[0],"server")){
        // first, try to connect, expecting an exception: there
        // should be no server
        try {
            init();
            // only get here if there is a server. Rude, this code.
            fprintf(stderr,"Server already running.\n");
            destroy();
            exit(1);
        } catch(DiamondException e){
            // ignore exception
        }
        // now start the server, daemonizing first if required.
        try {
            if(daemonizeServer){
                if(daemon(0,0)){
                    fprintf(stderr,"Failed to daemonize.\n");
                    exit(1);
                }
            }
            server();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
            exit(1);
        }
        exit(0);
    }
    
    if(!strcmp(argv[0],"version")){
        try {
            printf("Diamond Apparatus %s (%s)\n",VERSION,VERSIONNAME);
            init();
            subscribe("version");
            Topic t = get("version",GET_WAITANY);
            printf("Server version: %s (%s)\n",
                   t[0].s(),t[1].s());
            destroy();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
            exit(1);
        }
        exit(0);
    }
    
    // if not server. We separately init each one, to allow
    // for the "unknown command" case: we wouldn't want to put
    // init here and then have it connect even if the command
    // were unknown.
    
    // we set up signals to make sure we destroy the client.
    signal(SIGINT,handler);
    signal(SIGQUIT,handler);
    
    try {
        if(!strcmp(argv[0],"check")){
            // just try to connect
            init();
        } else if(!strcmp(argv[0],"listen")){
            if(argc<2)
                usagepanic();
            cmdListen(argv[1]);
        } else if(!strcmp(argv[0],"show")){
            if(argc<2)
                usagepanic();
            init();
            subscribe(argv[1]);
            Topic t = get(argv[1],GET_WAITANY);
            t.dump();
        } else if(!strcmp(argv[0],"pub")){
            init();
            if(argc<4)
                usagepanic();
            char *name = argv[1];
            char *types = argv[2];
            int argidx=3;
            Topic t;
            while(char c = *types++){
                if(argc==argidx)
                    usagepanic();
                switch(c){
                case 'f':
                    if(argv[argidx][0]=='n')
                        argv[argidx][0]='-';
                    t.add(Datum(atof(argv[argidx++])));
                    break;
                case 's':
                    t.add(Datum(argv[argidx++]));
                    break;
                }
            }
            
            publish(name,t);
        } else if(!strcmp(argv[0],"kill")){
            init();
            killServer();
        } else
            usagepanic();
    } catch(DiamondException e){
        fprintf(stderr,"Failed: %s\n",e.what());
        destroy();
        exit(1);
    }
    
    
    // destroy the client which logically must have been init()ed 
    // at this point.
    destroy();
    return 0;
}
