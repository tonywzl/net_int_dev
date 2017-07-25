#ifndef _NID_UMPK_H
#define _NID_UMPK_H

#include <stdint.h>

extern int umpk_encode_mrw(char *, uint32_t *, void *);
extern int umpk_decode_mrw(char *, uint32_t, void *);
extern int umpk_encode_wc(char *, uint32_t *, void *);
extern int umpk_decode_wc(char *, uint32_t, void *);
extern int umpk_encode_bwc(char *, uint32_t *, void *);
extern int umpk_decode_bwc(char *, uint32_t, void *);
extern int umpk_encode_twc(char *, uint32_t *, void *);
extern int umpk_decode_twc(char *, uint32_t, void *);
extern int umpk_encode_crc(char *, uint32_t *, void *);
extern int umpk_decode_crc(char *, uint32_t, void *);
extern int umpk_encode_doa(char *, uint32_t *, void *);
extern int umpk_decode_doa(char *, uint32_t, void *);
extern int umpk_encode_sac(char *, uint32_t *, void *);
extern int umpk_decode_sac(char *, uint32_t, void *);
extern int umpk_encode_pp(char *, uint32_t *, void *);
extern int umpk_decode_pp(char *, uint32_t, void *);
extern int umpk_encode_sds(char *, uint32_t *, void *);
extern int umpk_decode_sds(char *, uint32_t, void *);
extern int umpk_encode_drw(char *, uint32_t *, void *);
extern int umpk_decode_drw(char *, uint32_t, void *);
extern int umpk_encode_dxa(char *, uint32_t *, void *);
extern int umpk_decode_dxa(char *, uint32_t, void *);
extern int umpk_encode_dxp(char *, uint32_t *, void *);
extern int umpk_decode_dxp(char *, uint32_t, void *);
extern int umpk_encode_cxa(char *, uint32_t *, void *);
extern int umpk_decode_cxa(char *, uint32_t, void *);
extern int umpk_encode_cxp(char *, uint32_t *, void *);
extern int umpk_decode_cxp(char *, uint32_t, void *);
extern int umpk_encode_tp(char *, uint32_t *, void *);
extern int umpk_decode_tp(char *, uint32_t, void *);
extern int umpk_encode_ini(char *, uint32_t *, void*);
extern int umpk_decode_ini(char *, uint32_t, void *);
extern int umpk_encode_server(char *, uint32_t *, void*);
extern int umpk_decode_server(char *, uint32_t, void *);

#endif
