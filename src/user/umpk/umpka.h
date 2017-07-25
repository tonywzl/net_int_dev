#ifndef _NID_UMPKA_H
#define _NID_UMPKA_H

#include <stdint.h>

extern int umpk_encode_agent(char *, uint32_t *, void*);
extern int umpk_decode_agent(char *, uint32_t, void *);
extern int umpk_encode_driver(char *, uint32_t *, void*);
extern int umpk_decode_driver(char *, uint32_t, void *);

#endif
