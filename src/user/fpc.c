/*
 * fpc.c
 *
 *  Created on: Aug 28, 2015
 *      Author: Rick Xiong
 */

#include "nid_log.h"

#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include "list.h"
#include "fpc_if.h"
#include "fpn_if.h"
#include "nid.h"
#include "hash_if.h"

struct fpc_private {
	fpc_algorithm fp_algorithm;
	uint8_t	fp_len;
	uint8_t	fp_wgt;	// used for dsreclaim lru quque. 0: lowest weight(default) 15: highest weight
	struct hash_interface hash;
};

static uint8_t fpc_lens[] = {32, 28, 16};

typedef unsigned char* (*fpc_algorithm_fun_p)(const unsigned char *, size_t, unsigned char *);
static fpc_algorithm_fun_p fpc_algrm_p[] ={
		SHA256,
		SHA224,
		MD5,
};

void
fpc_bcalculate(struct fpc_interface *fpc, struct fpn_interface *fpn, void *buf, size_t count, size_t blocksz, struct list_head *fp_head)
{
    char *blkbuf, *fpbuf;
    struct fp_node *fp_nd;
    size_t buf_offset = 0;
    blkbuf = (char*)buf;
    INIT_LIST_HEAD(fp_head);

    while (buf_offset < count){
        blkbuf = buf + buf_offset;
        buf_offset += blocksz;
        fp_nd = fpn->fp_op->fp_get_node(fpn);
        fpbuf = fp_nd->fp;
        fpc->fpc_op->fpc_calculate(fpc, blkbuf, blocksz, (unsigned char*)fpbuf);
        // Add new finger print node for return to read content cache.
        list_add_tail(&fp_nd->fp_list, fp_head);
    }
}

void
fpc_bcalculatev(struct fpc_interface *fpc, struct fpn_interface *fpn, struct iovec *iovs, int iovcnt, size_t blocksz, struct list_head *fp_head)
{
    int i;
    size_t buf_offset;
    char *blkbuf, *fpbuf;
    size_t count;
    struct fp_node *fp_nd;

    INIT_LIST_HEAD(fp_head);

    for (i = 0; i < iovcnt; i++) {
        buf_offset = 0;
        count = iovs[i].iov_len;
        blkbuf = (char*)iovs[i].iov_base;
        while (buf_offset < count){
            blkbuf = iovs[i].iov_base + buf_offset;
            buf_offset += blocksz;
            fp_nd = fpn->fp_op->fp_get_node(fpn);
            fpbuf = fp_nd->fp;
            fpc->fpc_op->fpc_calculate(fpc, blkbuf, blocksz, (unsigned char*)fpbuf);
            // Add new finger print node for return to read content cache.
            list_add_tail(&fp_nd->fp_list, fp_head);
        }
    }
}

uint8_t
fpc_get_fp_len(struct fpc_interface *fpc)
{
	struct fpc_private *priv_p = (struct fpc_private*)fpc->fpc_private;
	return priv_p->fp_len;
}

void
fpc_set_wgt(struct fpc_interface *fpc, uint8_t wgt) {
	struct fpc_private *priv_p = (struct fpc_private*)fpc->fpc_private;
	assert(wgt < 16);

	if (priv_p->fp_len < NID_SIZE_FP) {
		priv_p->fp_wgt = wgt;
	} else {
		priv_p->fp_wgt = 0;
	}
}

uint8_t
fpc_get_wgt_from_fp(struct fpc_interface *fpc, const char *fp)
{
	struct fpc_private *priv_p = (struct fpc_private*)fpc->fpc_private;
	uint8_t wgt = 0;

	if (priv_p->fp_len < NID_SIZE_FP) {
		wgt = fp[priv_p->fp_len] & (char)0x0f;
	}

	return wgt;
}

void
fpc_set_wgt_to_fp(struct fpc_interface *fpc, char *fp, uint8_t fp_wgt)
{
	struct fpc_private *priv_p = (struct fpc_private*)fpc->fpc_private;
	assert(fp_wgt < 16);

	if (priv_p->fp_len < NID_SIZE_FP) {
		fp[priv_p->fp_len] = (fp[priv_p->fp_len] & 0xf0) + fp_wgt;
	}
}

int
fpc_cmp(struct fpc_interface *fpc, const void *fp1, const void *fp2) {
	struct fpc_private *priv_p = (struct fpc_private*)fpc->fpc_private;
	return memcmp(fp1, fp2, priv_p->fp_len);
}

#if 0
uint64_t
fpc_hvcal(struct fpc_interface *fpc, const char *fp) {
	struct fpc_private *priv_p = (struct fpc_private*) fpc->fpc_private;

	int counter = priv_p->fp_len;
	uint64_t val = 0;
	uint64_t n0, n1, n2, n3;

	while (counter) {
		n0 = *fp++;
		n1 = *fp++;
		n2 = *fp++;
		n3 = *fp++;
		val += n0++ + (n1 << 8) + (n2 << 16) + (n3 << 24);
		counter -= 4;
	}
	return val;
}
#endif

uint64_t
fpc_hvcal(struct fpc_interface *fpc, const char *fp) {
	struct fpc_private *priv_p = (struct fpc_private*) fpc->fpc_private;
	struct hash_interface *hash_p = &priv_p->hash;
	int counter = priv_p->fp_len;

	return hash_p->s_op->hash_calc(hash_p, fp, counter);
}


void
fpc_destroy(struct fpc_interface * fpc) {
	struct fpc_private *priv = (struct fpc_private*) fpc->fpc_private;
	free(priv);
}

void
fpc_cal(struct fpc_interface *fpc, const void *buf, size_t len, unsigned char *fp) {
	struct fpc_private *priv_p = (struct fpc_private*) fpc->fpc_private;
	memset(fp, 0, NID_SIZE_FP);
	fpc_algrm_p[priv_p->fp_algorithm](buf, len, fp);

	if (priv_p->fp_len < NID_SIZE_FP) {
		fp[priv_p->fp_len] = (fp[priv_p->fp_len] & 0xf0) + priv_p->fp_wgt;
	}
}

struct fpc_operations op = {
	.fpc_calculate = fpc_cal,
	.fpc_hvcalculate = fpc_hvcal,
	.fpc_cmp = fpc_cmp,
	.fpc_get_fp_len = fpc_get_fp_len,
	.fpc_set_wgt = fpc_set_wgt,
	.fpc_get_wgt_from_fp = fpc_get_wgt_from_fp,
	.fpc_set_wgt_to_fp = fpc_set_wgt_to_fp,
	.fpc_bcalculate = fpc_bcalculate,
	.fpc_bcalculatev = fpc_bcalculatev,
	.fpc_destroy = fpc_destroy,
};

int
fpc_initialization(struct fpc_interface *fpc, struct fpc_setup *setup) {
	struct fpc_private *priv_p = x_malloc(sizeof(struct fpc_private));
	fpc->fpc_private = priv_p;
	priv_p->fp_algorithm = setup->fpc_algrm;
	priv_p->fp_len = fpc_lens[priv_p->fp_algorithm];
	priv_p->fp_wgt = 0;
	fpc->fpc_op = &op;

	struct hash_setup hash_setup;
	memset(&hash_setup, 0, sizeof(hash_setup));
	hash_initialization(&priv_p->hash, &hash_setup);

	return 0;
}
