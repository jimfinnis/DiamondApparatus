/**
 * @file main.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdlib.h>
#include <unistd.h>

#include "diamondapparatus.h"

using namespace diamondapparatus;

void usagepanic(){
    fprintf(stderr,"Usage: diamond server | pub <topic> <types> <val>... | listen <topic> | kill\n");
    fprintf(stderr,"       types is a string of chars, f=float, s=string.\n");
    exit(1);
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
    } else if(!strcmp(argv[1],"listen")){
        try {
            if(argc<3)
                usagepanic();
            init();
            subscribe(argv[2]);
            while(isRunning()){
                usleep(100000);
                Topic t = get(argv[2]);
                if(t.state == Topic::Changed){
                    for(int i=0;i<t.size();i++)
                        t[i].dump();
                }
            }
            destroy();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
            destroy();
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
            
            publish(name,&t);
            destroy();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
            destroy();
        }
    } else if(!strcmp(argv[1],"kill")){
        try{
            init();
            killServer();
        } catch(DiamondException e){
            fprintf(stderr,"Failed: %s\n",e.what());
            destroy();
        }
        
    } else
        usagepanic();
}
