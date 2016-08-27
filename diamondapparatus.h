/**
 * @file diamondapparatus.h
 * @brief Message format and API
 *
 */

#ifndef __DIAMONDAPPARATUS_H
#define __DIAMONDAPPARATUS_H

#include <errno.h>
#include <string.h>
#include <stdio.h>

////////////// wait flags for get()

// wait for NEW data
#define GET_WAITNEW 1
// wait for ANY data
#define GET_WAITANY 2

/////////////// topic states
#define TOPIC_NODATA 0
#define TOPIC_UNCHANGED 1
#define TOPIC_CHANGED 2
#define TOPIC_NOTFOUND 3
#define TOPIC_NOTCONNECTED 4

/////////////// data types
#define DT_FLOAT 0
#define DT_STRING 1


#define VERSION 103
#define VERSIONNAME "Red Tabletop"

#ifdef __cplusplus

#include "data.h"


namespace diamondapparatus {

struct DiamondException {
    int err;
    char str[256];
    DiamondException(const char *s){
        strncpy(str,s,256);str[255]=0;
        err=errno;
    }
    DiamondException(){
        str[0]=0;
        err=-1;
    }
    DiamondException(const DiamondException& e){
        strcpy(str,e.str);
        err = e.err;
    }
    const char *what(){
        static char buf[256];
        if(err)
            sprintf(buf,"%s: %s",str,strerror(err));
        else
            sprintf(buf,"%s",str);
        return buf;
    }
};


// start the server and never exit. Don't run this
// unless you really know what you're up do; typically
// the diamond app will do this. The port is either
// DEFAULT_PORT in tcp.h, or DIAMOND_PORT in the environment
// if it is set.
void server();

// client calls

/// initialise the system and connect to the server. The host
/// and port to connect to are the environment variables
/// DIAMOND_HOST and DIAMOND_PORT, or localhost and DEFAULT_PORT
/// (see tcp.h) if these are not set.
void init();

/// close down the client politely - returns >0 if we haven't
/// yet done the same number of destroys as inits, and 0 once
/// we have the same number and the actual shutdown was done.
/// Also returns zero if we do too many destroys!
int destroy();

/// subscribe to a topic - we will receive msgs from the server
/// when it changes. Calling gettopic will get the latest value.
void subscribe(const char *n);

/// publish a topic
void publish(const char *name,Topic& d);

/// get the a copy of a topic as it currently is.
Topic get(const char *n,int wait=0);

/// wait for a message on any topic we are subscribed to 
void waitForAny();

/// returns false when the client loop has quit (i.e. the server
/// has died)
bool isRunning();

/// kill the server
void killServer();

/// destroy all data on the server
void clearServer();


}

#endif

// now the C wrappers

#ifdef __cplusplus
extern "C" {
#endif

// return error if a function returned -1
const char *diamondapparatus_error();
// 0 if OK, -1 on error
int diamondapparatus_init();
// 0 if OK, -1 on error
int diamondapparatus_server();
// will return -1 in error, >0 if not last destroy, and 0 if did actually
// shutdown
int diamondapparatus_isrunning();
int diamondapparatus_destroy();
int diamondapparatus_killserver();
int diamondapparatus_clearserver();
/// publish the topic built up by diamondapparatus_add..()
int diamondapparatus_publish(const char *n);
/// clear the topic to publish
void diamondapparatus_newtopic();
/// add a float to the topic to publish
void diamondapparatus_addfloat(float f);
/// add a string to the topic to publish
void diamondapparatus_addstring(const char *s);


int diamondapparatus_subscribe(const char *n);
/// read a topic, returning its state or -1. The topic can be accessed
/// with diamondapparatus_fetch...
int diamondapparatus_get(const char *n,int wait);
/// is the last topic got a valid topic to fetch data from?
int diamondapparatus_isfetchvalid();
/// wait for a message on any topic we are subscribed to 
int waitforany();

/// read a string from the topic got
const char *diamondapparatus_fetchstring(int n);
/// read a float from the topic got
float diamondapparatus_fetchfloat(int n);
/// get the type of a datum in the topic got
uint32_t diamondapparatus_fetchtype(int n);
/// get the number of data in the topic got
int diamondapparatus_fetchsize();



#ifdef __cplusplus
}
#endif




#endif /* __DIAMONDAPPARATUS_H */
