/**
 * @file mainc.c
 * @brief This is a version of the main Diamond Apparatus program
 * written in C and using the C linkage wrappers.
 * It compiles to diamondc.
 *
 */

#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "diamondapparatus.h"

void usagepanic(){
    fprintf(stderr,"Diamond Apparatus %d (%s)\n",VERSION,VERSIONNAME);
    fprintf(stderr,"Usage: diamond server | pub <topic> <types> <val>... | listen <topic> | show <topic> | kill | version\n");
    fprintf(stderr,"       types is a string of chars, f=float, s=string.\n");
    exit(1);
}

void handler(int sig){
    printf("sig %d caught\n",sig);
    diamondapparatus_destroy();
    exit(1); // no safe way to continue
}

void panic(){
    fprintf(stderr,"Diamond error: %s\n",diamondapparatus_error());
    diamondapparatus_destroy();
    exit(1);
}


int main(int argc,char *argv[]){
    
    if(argc<2){
        usagepanic();
    }
    
    if(!strcmp(argv[1],"server")){
        if(diamondapparatus_server())panic();
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
        if(argc<3)
            usagepanic();
        if(diamondapparatus_init())panic();
        if(diamondapparatus_subscribe(argv[2]))panic();
        while(diamondapparatus_isrunning()){
            if(diamondapparatus_get(argv[2],GET_WAITNEW)<0)panic();
            int i;
            for(i=0;i<diamondapparatus_fetchsize();i++){
                if(diamondapparatus_fetchtype(i) == DT_FLOAT)
                    printf("%f\n",diamondapparatus_fetchfloat(i));
                else
                    printf("%s\n",diamondapparatus_fetchstring(i));
            }
        }
    } else if(!strcmp(argv[1],"show")){
        if(argc<3)
            usagepanic();
        if(diamondapparatus_init())panic();
        if(diamondapparatus_subscribe(argv[2]))panic();
        if(diamondapparatus_get(argv[2],GET_WAITNEW)<0)panic();
        int i;
        for(i=0;i<diamondapparatus_fetchsize();i++){
            if(diamondapparatus_fetchtype(i) == DT_FLOAT)
                printf("%f\n",diamondapparatus_fetchfloat(i));
            else
                printf("%s\n",diamondapparatus_fetchstring(i));
        }
    } else if(!strcmp(argv[1],"pub")){
        if(diamondapparatus_init())panic();
        if(argc<5)
            usagepanic();
        char *name = argv[2];
        char *types = argv[3];
        int argidx=4;
        
        diamondapparatus_newtopic();
        
        char c;
        while(c = *types++){
            if(argc==argidx)
                usagepanic();
            switch(c){
            case 'f':
                diamondapparatus_addfloat(atof(argv[argidx++]));
                break;
            case 's':
                diamondapparatus_addstring(argv[argidx++]);
                break;
            }
        }
        
        if(diamondapparatus_publish(name))panic();
    } else if(!strcmp(argv[1],"kill")){
        if(diamondapparatus_init())panic();
        if(diamondapparatus_killserver())panic();
    } else
        usagepanic();
    
    // destroy the client which logically must have been init()ed 
    // at this point.
    diamondapparatus_destroy();
}
