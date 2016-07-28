/**
 * @file clinkage.cpp
 * @brief  Define C wrappers around stuff.
 *
 */

#include "diamondapparatus.h"

extern "C" {

using namespace diamondapparatus;

static DiamondException exception; // copy of last exception
static bool inerror=false;

static Topic topicToPublish,topicToAccess;

const char *diamondapparatus_error(){
    if(inerror)return exception.what();
    else return NULL;
}

int diamondapparatus_init(){
    try {
        inerror = false;
        init();
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
    return 0;
}

int diamondapparatus_server(){
    try {
        inerror = false;
        server();
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
    return 0;
}

// will return -1 in error, >0 if not last destroy, and 0 if did actually
// shutdown
int diamondapparatus_destroy(){
    try {
        inerror = false;
        return destroy();
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
}

int diamondapparatus_subscribe(const char *n){
    try {
        inerror = false;
        subscribe(n);
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
    return 0;
}

/// publish the topic built up by diamondapparatus_add..()
int diamondapparatus_publish(const char *n){
    try {
        inerror = false;
        publish(n,topicToPublish);
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
    return 0;
}
    
int diamondapparatus_killserver(){
    try {
        inerror = false;
        killServer();
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
}

int diamondapparatus_clearserver(){
    try {
        inerror = false;
        clearServer();
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
}

int diamondapparatus_isrunning(){
    return isRunning()?1:0;
}

/// read a topic, returning its state. The topic can be accessed
/// with diamondapparatus_fetch...

int diamondapparatus_get(const char *n,int wait){
    try {
        inerror = false;
        topicToAccess = get(n,wait);
        return topicToAccess.state;
    } catch (DiamondException e){
        exception = e;
        inerror = true;
        return -1;
    } 
}

/// clear the topic to publish
void diamondapparatus_newtopic(){
    topicToPublish = Topic();
}
/// add a float to the topic to publish
void diamondapparatus_addfloat(float f){
    topicToPublish.add(Datum(f));
}
/// add a string to the topic to publish
void diamondapparatus_addstring(const char *s){
    topicToPublish.add(Datum(s));
}

/// read a string from the topic got
const char *diamondapparatus_fetchstring(int n){
    return topicToAccess[n].s();
}
/// read a float from the topic got
float diamondapparatus_fetchfloat(int n){
    return topicToAccess[n].f();
}

uint32_t diamondapparatus_fetchtype(int n){
    return topicToAccess[n].t;
}

int diamondapparatus_fetchsize(){
    return topicToAccess.size();
}

int diamondapparatus_isfetchvalid(){
    return topicToAccess.isValid();
}

}
    






