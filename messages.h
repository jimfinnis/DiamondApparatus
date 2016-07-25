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
// data - NoDataMsg
#define CS_CLEARSERVER 4

// server to client
// data - Data
#define SC_NOTIFY 20
// data - NoDataMsg
#define SC_KILLCLIENT 30

struct NoDataMsg {
    uint32_t type;
};

struct StrMsg {
    uint32_t type;
    char msg[256];
};

}          

#endif /* __MESSAGES_H */
