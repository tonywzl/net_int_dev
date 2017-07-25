/*
 * util_bdev.h
 */
#ifndef NID_UTIL_BDEV_H
#define NID_UTIL_BDEV_H

#include <stdint.h>


extern int util_bdev_getinfo(int, uint64_t *, uint32_t *, uint8_t *);
extern int util_bdev_trim(int, off_t, size_t, uint64_t, uint32_t, uint8_t);
#endif

