/**
 * @file messages.h
 * @brief  Brief description of file.
 *
 */

#ifndef __MESSAGES_H
#define __MESSAGES_H

namespace diamondapparatus {

// client to server

// data - Data
#define CS_PUBLISH 1
// data - StrMsg
#define CS_SUBSCRIBE 2
// data - NoDataMsg
#define CS_KILLSERVER 3

// server to client
// data - SCAck
#define SC_ACK 10
// data - Data
#define SC_NOTIFY 20
// data - NoDataMsg
#define SC_KILLCLIENT 30

struct NoDataMsg {
    uint32_t type;
};

struct SCAck {
    uint32_t type; // SC_ACK
    uint16_t code;
    char msg[256];
};

struct StrMsg {
    uint32_t type;
    char msg[256];
};

// for both CS_PUBLISH and SC_NOTIFY
struct DataMsg {
    uint32_t type;
    char name[256];
    uint32_t count;
    // .. followed by float[count] - IEEE 32-bit!
    
    // convert to network
    void hton(){
        count = htons(count);
        uint32_t *p = (uint32_t *)(this+1);
        for(int i=0;i<count;i++){
            p[i] = htonl(p[i]);
        }
    }
    // convert from network
    void ntoh(){
        uint32_t *p = (uint32_t *)(this+1);
        count = ntohs(count);
        for(int i=0;i<count;i++){
            p[i] = ntohl(p[i]);
        }
    }
    
    // get the actual size
    uint32_t size(){
        return sizeof(*this)+sizeof(float)*count;
    }
    
    float *getfloats(){
        return (float *)(this+1);
    }
    
    void dump(){
        printf("name : %s, count : %d\n",name,count);
        float *f = getfloats();
        for(int i=0;i<count;i++)printf("%d : %f\n",i,f[i]);
    }
};


}          

#endif /* __MESSAGES_H */
