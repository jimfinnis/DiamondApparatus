/**
 * @file time.h
 * @brief  Brief description of file.
 *
 */

#ifndef __UTIME_H
#define __UTIME_H

namespace Time {
void init();
double now();
uint32_t ticks();
void tick();
};

#endif /* __UTIME_H */
