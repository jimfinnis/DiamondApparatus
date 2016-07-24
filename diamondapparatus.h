/**
 * @file diamondapparatus.h
 * @brief Message format and API
 *
 */

#ifndef __DIAMONDAPPARATUS_H
#define __DIAMONDAPPARATUS_H

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
// the diamond app will do this.

void server();

// client calls

/// initialise the system and connect to the server
void init(); 

/// close down the client politely
void destroy();

/// subscribe to a topic - we will receive msgs from the server
/// when it changes. Calling gettopic will get the latest value.
void subscribe(const char *n);

/// publish an array of floats to a topic
void publish(const char *n,float *f,int ct);

/// get the latest value of a topic into an array of floats of
/// appropriate size. Returns the number of floats actually
/// in the array. If this would have been >maxsize, the array
/// will be truncated.
/// Return values:
/// 0    - value has not been changed so no copying was done
/// -1   - topic has not been subscribed to
/// -2   - topic has not yet received data
/// -3   - not connected (thread not running)
/// n    - number of floats copied (will be <= maxsize)
int get(const char *n,float *out,int maxsize);

/// returns false when the client loop has quit (i.e. the server
/// has died)
bool isRunning();

/// kill the server
void killServer();


}
#endif /* __DIAMONDAPPARATUS_H */
