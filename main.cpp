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
    
    if(argc<2){
        usagepanic();
    }
    
    if(!strcmp(argv[1],"server")){
        try {
            server();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
        }
        exit(0);
    }
    
    if(!strcmp(argv[1],"version")){
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
    
    if(!strcmp(argv[1],"listen")){
        try {
            if(argc<3)
                usagepanic();
            init();
            subscribe(argv[2]);
            while(isRunning()){
                Topic t = get(argv[2],GET_WAITNEW);
                for(int i=0;i<t.size();i++)
                    t[i].dump();
            }
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
        }
    } else if(!strcmp(argv[1],"show")){
        try {
            if(argc<3)
                usagepanic();
            init();
            subscribe(argv[2]);
            Topic t = get(argv[2],GET_WAITANY);
            for(int i=0;i<t.size();i++)
                t[i].dump();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
        }
        
    } else if(!strcmp(argv[1],"pub")){
        try{
            init();
            if(argc<5)
                usagepanic();
            char *name = argv[2];
            char *types = argv[3];
            int argidx=4;
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
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
        }
    } else if(!strcmp(argv[1],"kill")){
        try{
            init();
            killServer();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
        }
        
    } else
        usagepanic();
    
    // destroy the client which logically must have been init()ed 
    // at this point.
    destroy();
    
}
