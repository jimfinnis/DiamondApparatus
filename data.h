/**
 * @file data.h
 * @brief  Brief description of file.
 *
 */

#ifndef __DATA_H
#define __DATA_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>

#define DT_FLOAT 0
#define DT_STRING 1

namespace diamondapparatus {

/// the base type, which is either a string or a float.
struct Datum {
    uint32_t t;
    union {
        float f;
        char *s;  // can't use string in plain C++. C'est la vie.
    } d;
    
    /// default value is 0.0f
    Datum(){
        t = DT_FLOAT;
        d.f=0;
    }
    
    // copy ctor
    Datum(const Datum& q){
        t = q.t;
        switch(t){
        case DT_FLOAT:
            d.f = q.d.f;
            break;
        case DT_STRING:
            d.s = strdup(q.d.s);
            break;
        }
    }
    
    Datum(float f){
        t = DT_FLOAT;
        d.f=f;
    }
    Datum(const char *s){
        t = DT_STRING;
        d.s = strdup(s);
    }
    
    float f() const{
        if(t!=DT_FLOAT)return 0.0;
        else return d.f;
    }
    
    const char *s() const{
        if(t!=DT_STRING)return "?notstring?";
        else return d.s;
    }
    
    void dump() const{
        switch(t){
        case DT_FLOAT:
            printf("%f\n",d.f);
            break;
        case DT_STRING:
            printf("%s\n",d.s);
            break;
        }
    }
    
    void clr(){
        if(t == DT_STRING)free(d.s);
        t = DT_FLOAT;
        d.f=0;
    }
    
    ~Datum(){
        clr();
    }
};



/// Collection of datums is data. Clearly.

class Topic {
    friend class MyClient;
private:
    std::vector<Datum> d;
    double timeLastSet; // time of last update (used on client only)
public:
    
    Topic(){
        state = TOPIC_NODATA;
        timeLastSet=-1;
    }  
    
    int state;
    static Datum zeroDat;
    
    const Datum &operator[] (int n) const {
        if(n<0 || n>=d.size())
            return zeroDat;
        else
            return d[n];
    }
    
    void add(const Datum &q){
        d.push_back(q);
    }
    
    int size(){
        return d.size();
    }
    
    void clear(){
        d.clear();
    }
    
    void dump(){
        for(int i=0;i<size();i++){
            d[i].dump();
        }
    }
                  
    
    /// does the topic actually contain valid data?
    bool isValid(){
        return (state == TOPIC_UNCHANGED || state == TOPIC_CHANGED);
    }
    
    /// convert to a message where the first uint32_t is the type,
    /// allocating and returning a buffer. Set the size of the allocated
    /// buffer in the integer ptr provided.
    /// Convert all floats/ints to network order.
    /// Also requires a name for the data.
    const char *toMessage(int *size,int msgtype,const char *name);
    
    /// load the contents of the message into this object, discarding
    /// previous data. Returns message type. Can have a buffer
    /// to write the data name into (but see getNameFromMsg).
    /// Convert all floats/ints from network order.
    uint32_t fromMessage(const char *p,char *namebuf=NULL);
    /// this reads the name from the message without doing
    /// anything else
    static const char *getNameFromMsg(const char *p);
    
    /// return the age - i.e. the time since we last got data
    /// on this topic - or -1 if no data has ever been received.
    double age();
};

}
#endif /* __DATA_H */
