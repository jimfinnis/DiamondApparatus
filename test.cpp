/**
 * @file test.cpp
 * @brief  Brief description of file.
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "diamondapparatus.h"
using namespace diamondapparatus;

void procparent(){
    init();
    clearServer();
    sleep(1);
    Topic t1,t2;
    t1.add(Datum("hello"));
    t1.add(Datum(1.0));
    
    for(int i=0;i<10;i++)
        t2.add(Datum(i*i));
    sleep(2);
    publish("test/1",t1);
    publish("test/2",t2);
    sleep(5);
    publish("test/2",t2);
    destroy();
    
    int st;
    wait(&st);
}


void procchild(){
    init();
    subscribe("test/1");
    subscribe("test/2");
    
    for(int i=0;i<10;i++){
        Topic t = get("test/2",GET_WAITANY);
        printf("st: %d\n",t.state);
        printf("Age %f - values:\n",t.age());
        t.dump();
        sleep(1);
    }
    destroy();
}

int main(int argc,char *argv[]){
    
    if(fork())
        procparent();
    else
        procchild();
    
}
