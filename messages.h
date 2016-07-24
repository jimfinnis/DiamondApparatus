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

}          

#endif /* __MESSAGES_H */
