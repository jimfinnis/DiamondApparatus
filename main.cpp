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
    fprintf(stderr,"Usage: diamond server | pub <topic> <val> | sub <topic> | kill\n");
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
                float f;
                if(get(argv[2],&f,1)>0){
                    printf("%f\n",f);
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
            if(argc<4)
                usagepanic();
            float val = atof(argv[3]);
            publish(argv[2],&val,1);
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
