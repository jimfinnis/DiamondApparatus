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

struct Data {
    std::vector<Datum> d;
    
    const Datum &operator[] (int n) const {
        return d[n];
    }
    
    void add(const Datum &q){
        d.push_back(q);
    }
    
    int size(){
        return d.size();
    }
    
    /// convert to a message where the first uint32_t is the type,
    /// allocating and returning a buffer. Set the size of the allocated
    /// buffer in the integer ptr provided.
    /// Convert all floats/ints to network order.
    const char *toMessage(int *size,int msgtype);
    
    /// load the contents of the message into this object, discarding
    /// previous data. Returns message type.
    /// Convert all floats/ints from network order.
    uint32_t fromMessage(const char *msg);
};

}
#endif /* __DATA_H */
