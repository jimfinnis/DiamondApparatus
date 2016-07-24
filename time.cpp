/**
 * @file time.cpp
 * @brief  Brief description of file.
 *
 */

#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "time.h"

namespace Time {

uint32_t ticksctr=0;
static timespec progstart;
inline double time_diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    
    double t = temp.tv_sec;
    double ns = temp.tv_nsec;
    t += ns*1e-9;
    return t;
}

void init(){
    clock_gettime(CLOCK_MONOTONIC,&progstart);
}

double now(){
    timespec t;
    clock_gettime(CLOCK_MONOTONIC,&t);
    return time_diff(progstart,t);
}

uint32_t ticks(){
    return ticksctr;
}

void tick(){
    ticksctr++;
}

}
