/**
 * @file data.cpp
 * @brief  Brief description of file.
 *
 */

#include "data.h"
#include "diamondapparatus.h"
#include <arpa/inet.h>

/*
 * Message format for a datum:
 * uint32_t msgtype
 * uint32_t numitems
 * byte namelen+1
 * char name[namelen+1] (null term)
 * for each item:
 *   byte type
 *   union {
 *     float f
 *     struct {
 *      uint32_t len
 *      char s[len];
 *     }
 *   }
 */


using namespace diamondapparatus;

Datum Topic::zeroDat;

const char *Topic::getNameFromMsg(const char *p){
    p+=sizeof(uint32_t)*2+1;
    return p;
}


const char *Topic::toMessage(int *size,int msgtype,const char *name){
    int nll = strlen(name);
    if(nll>128)nll=128;
    uint8_t namelen = nll+1;
    
    // first task - calculate the size.
    *size = sizeof(uint32_t)*2; // msgtype and item count
    *size += namelen+1; // namelen and name
    
    std::vector<Datum>::iterator i;
    for(i=d.begin();i!=d.end();++i){
        (*size)++; // type byte
        switch(i->t){
        case DT_FLOAT:
            *size+=sizeof(float);
            break;
        case DT_STRING:
            *size+=sizeof(uint32_t); // length
            *size+=strlen(i->d.s); // NOT nullterminated
            break;
        default:
            throw DiamondException("bad data");
        }
    }
    // now we have the size, alloc the buffer
    char *buf = (char*)malloc(*size);
    char *p = buf;
    
    // and fill it in, remembering to perform conversions
    *(uint32_t *)p = htonl(msgtype); p+=sizeof(uint32_t);
    *(uint32_t *)p = htonl(d.size()); p+=sizeof(uint32_t);
    *p++ = namelen;
    strcpy(p,name); // this one is null terminated
    p+=namelen;
    
    for(i=d.begin();i!=d.end();++i){
        int len;
        *p++ = i->t; // type
        switch(i->t){
        case DT_FLOAT:
            memcpy(p,&i->d.f,sizeof(float));
            *(uint32_t *)p = htonl(*(uint32_t *)p);  
            p+=sizeof(float);
            break;
        case DT_STRING:
            len = strlen(i->d.s);
            *(uint32_t *)p = htonl(len);
            p+= sizeof(uint32_t);
            memcpy(p,i->d.s,len);
            p+=len; // NOT nullterminated
            break;
        default:
            throw DiamondException("bad data");
        }
    }
    return buf;
}

uint32_t Topic::fromMessage(const char *p,char *namebuf){
    d.clear();
    uint32_t msg = ntohl(*(uint32_t *)p);p+=sizeof(uint32_t);
    uint32_t count = ntohl(*(uint32_t *)p);p+=sizeof(uint32_t);
    uint8_t namelen = *p++;
    
    if(namebuf){
        memcpy(namebuf,p,namelen);
        namebuf[namelen]=0;
    }
    p+=namelen;
    
    for(int i=0;i<count;i++){
        int len;
        Datum t; // temporary datum
        switch(*p++){
        case DT_FLOAT:
            t.t = DT_FLOAT;
            memcpy(&t.d.f,p,sizeof(float));
            *(uint32_t *)(&t.d.f) = htonl(*(uint32_t *)(&t.d.f));  
            p+=sizeof(float);
            break;
        case DT_STRING:
            t.t = DT_STRING;
            len = ntohl(*(uint32_t *)p);
            p+=sizeof(uint32_t);
            t.d.s = (char *)malloc(len+1);
            memcpy(t.d.s,p,len);
            t.d.s[len]=0;
            p+=len;
            break;
        default:
            throw DiamondException("bad data in fromMessage");
        }
        d.push_back(t);
    }
}
