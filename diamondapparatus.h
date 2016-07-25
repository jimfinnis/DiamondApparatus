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

#include "data.h"

#define VERSION 100
#define VERSIONNAME "Malachite Territory"

namespace diamondapparatus {

struct DiamondException {
    int err;
    char str[256];
    DiamondException(const char *s){
        strncpy(str,s,256);str[255]=0;
        err=errno;
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

/// close down the client politely
void destroy();

/// subscribe to a topic - we will receive msgs from the server
/// when it changes. Calling gettopic will get the latest value.
void subscribe(const char *n);

/// publish a topic
void publish(const char *name,Topic& d);

// wait for NEW data
static const int GetWaitNew=1;
// wait for ANY data
static const int GetWaitAny=2;
/// get the a copy of a topic as it currently is.
Topic get(const char *n,int wait=0);

/// returns false when the client loop has quit (i.e. the server
/// has died)
bool isRunning();

/// kill the server
void killServer();

/// destroy all data on the server
void clearServer();


}
#endif /* __DIAMONDAPPARATUS_H */
