/*
 * bfp1.c
 * 	Implementation of Bio Flushing Policy Module
 */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <string.h>


#include "nid_log.h"
#include "wc_if.h"
#include "pp_if.h"
#include "bfp1_if.h"

#define THROTTLE_PSIZE	512
#define THROTTLE_BSIZE 512
#define AGGRES_FLUSH_SIZE 512
#define NORMAL_FLUSH_SIZE 1

struct bfp1_private {
	struct wc_interface	*p_wc;
	struct pp_interface	*p_pp;
	uint32_t	p_pagesz;
	uint32_t 	p_blocksz;
	uint32_t	p_page_avail;
	uint16_t	p_block_noccupied;
	uint32_t 	p_pagetip;			//tipping point for memory available page
	uint32_t 	p_buffertip;		//tipping point for buffer device occupied block
	uint32_t	p_rel_page_cnt;
	uint32_t	p_block_num;		//total block num for WC
	uint32_t	p_mark_range;		//the mark range between high and low
	int 	p_fastnum;			//flushing times for aggressive mode
	int 	p_normnum;			//flushing times for normal mode
	int 	p_flushnum;			//how many times go_flushing need to run
	int 	p_idlecnt;			//counter for normal flush mode
	int 	p_high_water_mark;
	int 	p_low_water_mark;
};

static int
bfp1_get_flush_num(struct bfp1_interface *bfp1_p)
{
	char *log_header = "bfp_flush_fuction";
	struct bfp1_private *priv_p = (struct bfp1_private *)bfp1_p->fp_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	uint32_t pool_sz, pool_len, pool_nfree;
	uint32_t page_available;
	struct wc_interface *wc_p = priv_p->p_wc;

	priv_p->p_rel_page_cnt = wc_p->wc_op->wc_get_release_page_counter(wc_p);

	pool_sz = pp_p->pp_op->pp_get_poolsz(pp_p);
	pool_nfree = pp_p->pp_op->pp_get_nfree(pp_p);
	pool_len = pp_p->pp_op->pp_get_poollen(pp_p);
	page_available = pool_nfree + pool_sz - pool_len + priv_p->p_rel_page_cnt;
	priv_p->p_page_avail = page_available;
	nid_log_debug("%s: page_stat: pool_sz: %u pool_nfree: %u pool_len: %u rel_page_cnt: %u", log_header, pool_sz, pool_nfree, pool_len, priv_p->p_rel_page_cnt);

	priv_p->p_block_noccupied = wc_p->wc_op->wc_get_block_occupied(wc_p);

	nid_log_debug("%s: page_avail is:%u, blocked_opccuied:%u ", log_header, priv_p->p_page_avail, priv_p->p_block_noccupied);
	if (priv_p->p_block_noccupied >= (priv_p->p_buffertip - 1) ||
		priv_p->p_page_avail < priv_p->p_pagetip) {
		priv_p->p_flushnum = priv_p->p_fastnum;
		nid_log_debug("%s: aggressive flush mode start, %u blocks need to flush", log_header, priv_p->p_flushnum);
	} else {
		priv_p->p_flushnum = 0;
	}

	if (priv_p->p_flushnum) {
		priv_p->p_idlecnt = 0;
	} else if (++priv_p->p_idlecnt >= 10) {
		nid_log_debug("%s: normal flush mode start, %u blocks need to flush", log_header, priv_p->p_flushnum);
		priv_p->p_flushnum = priv_p->p_normnum;
		priv_p->p_idlecnt = 0;
	}
	return priv_p->p_flushnum;
}

#if 0
static int
bfp1_get_flush_num(struct bfp1_interface *bfp1_p)
{
	char *log_header = "bfp1_flush_fuction";
	struct bfp1_private *priv_p = (struct bfp1_private *)bfp1_p->fp_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct wc_interface *wc_p = priv_p->p_wc;
	uint32_t pool_sz, pool_len, pool_nfree;
	uint32_t page_available;

	priv_p->p_rel_page_cnt = wc_p->wc_op->wc_get_release_page_counter(wc_p);

	pool_sz = pp_p->pp_op->pp_get_poolsz(pp_p);
	pool_nfree = pp_p->pp_op->pp_get_nfree(pp_p);
	pool_len = pp_p->pp_op->pp_get_poollen(pp_p);
	page_available = pool_nfree + pool_sz - pool_len + priv_p->p_rel_page_cnt;
	priv_p->p_page_avail = page_available;

	nid_log_debug("%s: page_stat: pool_sz: %u pool_nfree: %u pool_len: %u rel_page_cnt: %u", log_header, pool_sz, pool_nfree, pool_len, priv_p->p_rel_page_cnt);

	priv_p->p_block_noccupied = wc_p->wc_op->wc_get_block_occupied(wc_p);

	if (priv_p->p_block_noccupied > priv_p->p_buffertip ||
		priv_p->p_page_avail < priv_p->p_pagetip) {
		priv_p->p_flushnum = priv_p->p_fastnum;
		nid_log_debug("%s: aggressive flush mode start "
			"(noccupied:%d, pool_len:%d, pool_nfree:%d, rel_page_cnt:%d, tip:%d, avail:%d, pagetip:%d),"
			" %u blocks need to flush",
			log_header, priv_p->p_block_noccupied, pool_len, pool_nfree,
			priv_p->p_rel_page_cnt, priv_p->p_buffertip,
			priv_p->p_page_avail, priv_p->p_pagetip, priv_p->p_flushnum);
		priv_p->p_idlecnt = 0;
		return priv_p->p_flushnum;
	} else {
		if (priv_p->p_idlecnt < 10) {
			priv_p->p_idlecnt++;
			return 0;
		} else {
			priv_p->p_flushnum = priv_p->p_normnum;
			nid_log_debug("%s: normal flush mode start "
				"(noccupied:%d, pool_len:%d, pool_nfree:%d, rel_page_cnt:%d, tip:%d, avail:%d, pagetip:%d),"
				" %u blocks need to flush",
				log_header, priv_p->p_block_noccupied, pool_len, pool_nfree,
				priv_p->p_rel_page_cnt, priv_p->p_buffertip,
				priv_p->p_page_avail, priv_p->p_pagetip, priv_p->p_flushnum);
		priv_p->p_idlecnt = 0;
		return priv_p->p_flushnum;
			priv_p->p_idlecnt = 0;
			return priv_p->p_flushnum;
		}
	}
}
#endif

static int
bfp1_update_water_mark(struct bfp1_interface *bfp1_p, int low_water_mark, int high_water_mark )
{
	char *log_header = "bfp1_update_water_mark";
	struct bfp1_private *priv_p = (struct bfp1_private *)bfp1_p->fp_private;
	nid_log_notice("%s: start ...", log_header);

	if (low_water_mark > high_water_mark ){
		nid_log_warning("%s: high_water_mark must greater than low_water_mark", log_header);
		return -1;
	}

	priv_p->p_high_water_mark = high_water_mark;
	priv_p->p_low_water_mark = low_water_mark;
	priv_p->p_mark_range = priv_p->p_high_water_mark - priv_p->p_low_water_mark;
	priv_p->p_fastnum = (priv_p->p_block_num *  priv_p->p_mark_range) / 100;
	priv_p->p_buffertip = (priv_p->p_block_num * priv_p->p_high_water_mark ) / 100;

	return 0;

}

struct bfp1_operations bfp1_op = {
	.bf_get_flush_num = bfp1_get_flush_num,
	.bf_update_water_mark = bfp1_update_water_mark,
};

int
bfp1_initialization(struct bfp1_interface *bfp1_p, struct bfp1_setup *setup)
{
	char *log_header = "bfp1_initialization";
	struct bfp1_private *priv_p;

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	bfp1_p->fp_op = &bfp1_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bfp1_p->fp_private = priv_p;

	priv_p->p_high_water_mark = setup->high_water_mark;
	priv_p->p_low_water_mark = setup->low_water_mark;
	if(priv_p->p_high_water_mark < priv_p->p_low_water_mark) {
		nid_log_warning("%s: high_water_mark must greater than low_water_mark", log_header);
		assert(0);
	}
	priv_p->p_mark_range = priv_p->p_high_water_mark - priv_p->p_low_water_mark;

	priv_p->p_wc = setup->wc;
	priv_p->p_pp = setup->pp;
	priv_p->p_pagesz = (setup->pagesz >> 20);

	// block size always 4MB
	priv_p->p_blocksz = 4;
	priv_p->p_block_num = (setup->buffersz >> 20)/priv_p->p_blocksz;

	priv_p->p_flushnum = 0;
	priv_p->p_idlecnt = 0;
//	priv_p->p_fastnum = AGGRES_FLUSH_SIZE/priv_p->p_blocksz;
	priv_p->p_fastnum = (priv_p->p_block_num * priv_p->p_mark_range) / 100;
	priv_p->p_normnum = 1;
//	priv_p->p_buffertip = block_num - (THROTTLE_BSIZE/priv_p->p_blocksz);
	priv_p->p_buffertip = (priv_p->p_block_num * priv_p->p_high_water_mark ) / 100;
	priv_p->p_pagetip = THROTTLE_PSIZE/priv_p->p_pagesz;
	priv_p->p_rel_page_cnt = 0;
	nid_log_debug("%s: page size is:%u, block size is:%u ", log_header, priv_p->p_pagesz, priv_p->p_blocksz);
	nid_log_debug("%s: buffertip:%u, pagetip:%u ", log_header, priv_p->p_buffertip, priv_p->p_pagetip);
	return 0;
}
