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

using namespace diamondapparatus;

void usagepanic(){
    fprintf(stderr,"Diamond Apparatus %d (%s)\n",VERSION,VERSIONNAME);
    fprintf(stderr,"Usage: diamond server | pub <topic> <types> <val>... | listen <topic> | show <topic> | kill | version\n");
    fprintf(stderr,"       types is a string of chars, f=float, s=string.\n");
    exit(1);
}

void handler(int sig){
    printf("sig %d caught\n",sig);
    destroy(); // shut down client
    exit(1); // no safe way to continue
}


int main(int argc,char *argv[]){
    char c;
    bool daemonizeServer=false;
    while((c=getopt(argc,argv,"d"))!=-1){
        switch(c){
        case 'd':
            daemonizeServer=true;
            break;
        }
    }
    
    if(optind>=argc){
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
        printf("Diamond Apparatus %d (%s)\n",VERSION,VERSIONNAME);
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
            init();
            subscribe(argv[1]);
            while(isRunning()){
                Topic t = get(argv[1],GET_WAITNEW);
                for(int i=0;i<t.size();i++)
                    t[i].dump();
            }
        } else if(!strcmp(argv[0],"show")){
            if(argc<2)
                usagepanic();
            init();
            subscribe(argv[1]);
            Topic t = get(argv[1],GET_WAITANY);
            for(int i=0;i<t.size();i++)
                t[i].dump();
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
