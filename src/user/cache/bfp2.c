/*
 * bfp2.c
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
#include <time.h>

#include "nid_log.h"
#include "wc_if.h"
#include "pp_if.h"
#include "bfp2_if.h"

#define THROTTLE_PSIZE	512
#define THROTTLE_BSIZE 512
#define AGGRES_FLUSH_SIZE 512
#define NORMAL_FLUSH_SIZE 1

#define FLUSH_MODE_NONE		1
#define FLUSH_MODE_AGGRES	2
#define FLUSH_MODE_NORMAL	3

#define FLUSH_HISTORY_MAX	10
#define FLUSH_DELAY_CTL		8
#define FLUSH_IDLE_MAX		10

// idle mode's sub mode, here call both normal mode and idle mode with normal mode
// it means jumped from aggressive flush mode to normal flush mode
#define IDLE_MODE_ATON		1
// it means lies in normal flush mode
#define IDLE_MODE_NTON		2

struct bfp2_private {
	struct wc_interface	*p_wc;
	struct pp_interface	*p_pp;
	uint32_t	p_pagesz;
	uint32_t 	p_blocksz;
	uint32_t	p_page_avail;
	uint16_t	p_block_noccupied;
	uint32_t 	p_pagetip;			//tipping point for memory available page
	uint32_t 	p_buffertip;		//tipping point for buffer device occupied block
	uint32_t	p_rel_page_cnt;
	int 	p_fastnum;			//flushing times for aggressive mode
	int 	p_normnum;			//flushing times for normal mode
	int 	p_flushnum;			//how many times go_flushing need to run
	int 	p_idlecnt;			//counter for normal flush mode
	time_t	p_lastidle_time;	// previous idle time
	time_t	p_lastflush_time;	// previous flush time
	time_t	p_cur_time;			// current time
	int 	p_lastflush_mode;	// previouse flush mode
	double	p_flush_speed[FLUSH_HISTORY_MAX];		// flush speed history
	int		p_lastflush_idx;	// last flush store position
	double	p_high_flush_speed;	// biggest flush speed in history
	int		p_flush_cycle;		// used to prevent trash between aggressive mode and idle mode
	uint32_t	p_last_page_avail;	// available page in last flush action
	int64_t		p_read_counter;		// read bytes from start up
	uint32_t	p_idle_mode;	// detailed mode in cycle flush mode

	double		p_top_flush_speed[FLUSH_HISTORY_MAX];
	double		load_ratio_max;
	double		load_ratio_min;
	double		load_ctrl_level;
	double		flush_delay_ctl;
};

static int
__bfp2_update_highest_flush_speed(struct bfp2_interface *bfp2_p) {
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	int idx;
	
	// update highest flush speed
	for(idx = 0; idx < FLUSH_HISTORY_MAX; idx++) {
		if(priv_p->p_high_flush_speed < priv_p->p_flush_speed[idx]) {
			priv_p->p_high_flush_speed = priv_p->p_flush_speed[idx];
			break;
		}
	}
	
	return 0;
}

static int
__bfp2_min_flush_speed_idx(struct bfp2_interface *bfp2_p) {
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	int idx, min_idx = 0;
	double min_speed = priv_p->p_top_flush_speed[min_idx];

	for(idx = 0; idx < FLUSH_HISTORY_MAX; idx++) {
		if(priv_p->p_top_flush_speed[idx] < min_speed) {
			min_idx = idx;
			min_speed = priv_p->p_top_flush_speed[min_idx];
		}
	}

	return min_idx;
}

static void
__bfp2_update_top_flush_speed(struct bfp2_interface *bfp2_p, double flush_speed) {
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	int min_speed_idx = __bfp2_min_flush_speed_idx(bfp2_p);
	double min_speed = priv_p->p_top_flush_speed[min_speed_idx];

	if(flush_speed > min_speed) {
		priv_p->p_top_flush_speed[min_speed_idx] = flush_speed;
	}

}

static double
__bfp2_update_last_flush_speed(struct bfp2_interface *bfp2_p) {
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	double flush_speed = -1.0;
	
	if(priv_p->p_lastflush_time < 1) {
		time(&priv_p->p_lastflush_time);
		return flush_speed;
	}
	
	// calc last flush speed
	time(&priv_p->p_cur_time);
	flush_speed = priv_p->p_flushnum/(difftime(priv_p->p_cur_time, priv_p->p_lastflush_time));	

	// refresh flush speed history data
	priv_p->p_flush_speed[priv_p->p_lastflush_idx] = flush_speed;
	priv_p->p_lastflush_idx++;
	priv_p->p_lastflush_idx %= FLUSH_HISTORY_MAX;
	
	// update highest flush speed recored
	__bfp2_update_highest_flush_speed(bfp2_p);
	
	// update top flush speed recoreds
	__bfp2_update_top_flush_speed(bfp2_p, flush_speed);

	return flush_speed;
}

static double 
bfp2_get_last_flush_speed(struct bfp2_interface *bfp2_p) {
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	int idx = priv_p->p_lastflush_idx - 1;
	if(idx < 0) {
		idx = FLUSH_HISTORY_MAX - 1;
	}
	
	return priv_p->p_flush_speed[idx];	
}

static double
__bfp2_update_avg_top_flush_speed(struct bfp2_interface *bfp2_p) {
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	double sum_speed = 0.0;
	double avg_speed = 0.0;
	int idx, flush_cnt = 0;

	// less than 0 means it's first time, we can only set last flush time
	// when this function call 2nd time, we can get real flush time
	if(__bfp2_update_last_flush_speed(bfp2_p) < 0) {
		return avg_speed;
	}

	for(idx = 0; idx < FLUSH_HISTORY_MAX; idx++) {
		if(priv_p->p_top_flush_speed[idx] > 0.01) {
			sum_speed += priv_p->p_top_flush_speed[idx];
			flush_cnt++;
		}
	}

	// average speed only make sense when we have enough history
	if(flush_cnt >= 2) {
		avg_speed = sum_speed/flush_cnt;
	}

	return avg_speed;

}

// this function can only called after the 1st flush action in aggressive mode
static double
bfp2_guess_flush_load(struct bfp2_interface *bfp2_p) {
	char *log_header = "bfp2_guess_flush_load";
	double avg_flush_speed = __bfp2_update_avg_top_flush_speed(bfp2_p);
	double last_flush_speed = bfp2_get_last_flush_speed(bfp2_p);
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	double flush_load_ratio;

	// first time call this function, we cannot guess correct disk load
	// so, we return 10 as light load
	// average speed was 0 means
	if(avg_flush_speed < 0.00001) {
		return priv_p->load_ratio_min;
	}

	flush_load_ratio = last_flush_speed/avg_flush_speed;
	if(flush_load_ratio > 1.0) {
		flush_load_ratio = 1.0;
	}
	flush_load_ratio = 1 - flush_load_ratio;

	if(flush_load_ratio < priv_p->load_ratio_min) {
		flush_load_ratio = priv_p->load_ratio_min;
	} else if(flush_load_ratio > priv_p->load_ratio_max) {
		flush_load_ratio = priv_p->load_ratio_max;
	}

	nid_log_debug("%s: ret guess flush load ratio %f", log_header, flush_load_ratio);
	return flush_load_ratio;
}

static int
bfp2_get_flush_num(struct bfp2_interface *bfp2_p)
{
	char *log_header = "bfp2_flush_fuction";
	struct bfp2_private *priv_p = (struct bfp2_private *)bfp2_p->fp_private;
	struct pp_interface *pp_p = priv_p->p_pp;
	struct wc_interface *wc_p = priv_p->p_wc;
	uint32_t pool_sz, pool_len, pool_nfree;
	uint32_t page_available;
	double guess_flush_load;
	uint32_t delay_ctl = 0;
	int64_t cur_read_counter = 0;
	
	priv_p->p_rel_page_cnt = wc_p->wc_op->wc_get_release_page_counter(wc_p);
		
	pool_sz = pp_p->pp_op->pp_get_poolsz(pp_p);
	pool_nfree = pp_p->pp_op->pp_get_nfree(pp_p);
	pool_len = pp_p->pp_op->pp_get_poollen(pp_p);
	page_available = pool_nfree + pool_sz - pool_len + priv_p->p_rel_page_cnt;
	priv_p->p_page_avail = page_available;
	
	priv_p->p_block_noccupied = wc_p->wc_op->wc_get_block_occupied(wc_p);
	nid_log_debug("%s: page_avail is:%u, blocked_opccuied:%u ",
		log_header, priv_p->p_page_avail, priv_p->p_block_noccupied);
	if (priv_p->p_block_noccupied > priv_p->p_buffertip ||
		priv_p->p_page_avail < priv_p->p_pagetip) {
		priv_p->p_flushnum = priv_p->p_fastnum;
		nid_log_debug("%s: aggressive flush mode start "
			"(noccupied:%d, pool_len:%d, pool_nfree:%d, rel_page_cnt:%d, tip:%d, avail:%d, pagetip:%d),"
			" %u blocks need to flush",
			log_header, priv_p->p_block_noccupied, pool_len, pool_nfree,
			priv_p->p_rel_page_cnt, priv_p->p_buffertip,
			priv_p->p_page_avail, priv_p->p_pagetip, priv_p->p_flushnum);
			
		// we only use average flush speed in aggressive mode, when we jumped from other mode
		// we must drop previous flush speed history
		if(priv_p->p_lastflush_mode != FLUSH_MODE_AGGRES) {
			priv_p->p_lastflush_time = -1;
			memset(priv_p->p_flush_speed, 0, sizeof(priv_p->p_flush_speed));
			priv_p->p_lastflush_idx = 0;
		}

		// we can only guess disk work load after we know our 1st flush time cost
		// in 1st flush 
		if(priv_p->p_lastflush_time > 0 && (priv_p->p_lastflush_mode == FLUSH_MODE_AGGRES)) {
			// disk in light load status, can flush without delay
			// flush_load_ratio = last_flush_speed/avg_flush_speed;
//			guess_flush_load = bfp2_guess_flush_load(bfp2_p);
			guess_flush_load = bfp2_guess_flush_load(bfp2_p);

			if(guess_flush_load > priv_p->load_ctrl_level) {
				// disk overload status, must lower down flush speed
				delay_ctl = (int)(priv_p->flush_delay_ctl/(1 - guess_flush_load));
			} 

			// control flush speed by manual delay
			if(delay_ctl > 0) {
				nid_log_debug("%s: aggressive flush mode delay %d",
							log_header, delay_ctl);
				usleep(2000*delay_ctl);
				delay_ctl--;
			}				
		}
		
		priv_p->p_idlecnt = 0;
		priv_p->p_lastflush_mode = FLUSH_MODE_AGGRES;
		time(&priv_p->p_lastflush_time);
		priv_p->p_last_page_avail = priv_p->p_page_avail;
		return priv_p->p_flushnum;
	} else {
		// we just jumped from aggressive mode to idle mode
		// we must increase our idle refresh cycle from 0 to FLUSH_IDLE_MAX step by step
		if(priv_p->p_lastflush_mode == FLUSH_MODE_AGGRES) {
			priv_p->p_flush_cycle = 0;
			priv_p->p_flushnum = priv_p->p_fastnum;
			priv_p->p_idle_mode = IDLE_MODE_ATON;
		} else {
			// we can only use FLUSH_IDLE_MAX flush cycle when we really in normal state
			if(priv_p->p_idle_mode == IDLE_MODE_NTON) {
				priv_p->p_flush_cycle = FLUSH_IDLE_MAX;
			}
		}
		
		priv_p->p_lastflush_mode = FLUSH_MODE_NORMAL;
		
		nid_log_debug("%s: normal flush mode , %u idlecnt and %u flush cycle need to flush", 
		log_header, priv_p->p_idlecnt, priv_p->p_flush_cycle);
		
		if(priv_p->p_idlecnt < priv_p->p_flush_cycle) {
			priv_p->p_idlecnt++;
			if(priv_p->p_idlecnt >= priv_p->p_flush_cycle/2) {
				time(&priv_p->p_cur_time);
				if(difftime(priv_p->p_cur_time, priv_p->p_lastflush_time) >= priv_p->p_flush_cycle) {
					goto lbl_normal;
				} 
			}
			return 0;
		} else {
lbl_normal:			
			// we only need to graceful decrease flush block number when we change flush	
			// mode from aggressive mode to normal mode(IDLE_MODE_ATON), and we exit the 
			// mode when flush number reache to normal number
			if(priv_p->p_idle_mode == IDLE_MODE_ATON) {
				if(priv_p->p_flushnum > priv_p->p_normnum) {
					if(priv_p->p_page_avail > priv_p->p_last_page_avail) {
						priv_p->p_flushnum = priv_p->p_flushnum/2;
						if(priv_p->p_flushnum == priv_p->p_normnum) {
							priv_p->p_idle_mode = IDLE_MODE_NTON;
						}
						
						// flush cycle in normal mode can only be increased when 
						// available page increased in last cycle
						if(priv_p->p_flush_cycle < FLUSH_IDLE_MAX) {
							priv_p->p_flush_cycle++;
						}						
					}
				}					
			} else {
				// check read bytes from disk in bwc module, if read bytes not increase
				// in last cycle, we can flush more blocks in a cycle
				cur_read_counter = wc_p->wc_op->wc_get_read_counter(wc_p);
				// if returen value less than 0, it's a bug
				assert(cur_read_counter >= 0);
			
				// if read counter will reach uint64_t range limit, reset the counter
				// when we need to reset the counter, it means there have read disk ops
				// in last cycle, so we use normal flush disk policy by 1 blocks a time
				if(cur_read_counter >= (INT64_MAX - 1024)) {
					wc_p->wc_op->wc_reset_read_counter(wc_p);
					cur_read_counter = wc_p->wc_op->wc_get_read_counter(wc_p);
				}

				if(cur_read_counter > priv_p->p_read_counter) {
					// here it means there have disk read ops in last cycle
					priv_p->p_flushnum = priv_p->p_normnum;
				} else {
					priv_p->p_flushnum = priv_p->p_fastnum;
				}
				priv_p->p_read_counter = cur_read_counter;

			}
			
			nid_log_debug("%s: normal flush mode start, %u blocks need to flush", log_header, priv_p->p_flushnum);
			priv_p->p_idlecnt = 0;			
			time(&priv_p->p_lastflush_time);
			priv_p->p_last_page_avail = priv_p->p_page_avail;
			return priv_p->p_flushnum;
		}
	}
}


struct bfp2_operations bfp2_op = {
	.bf_get_flush_num = bfp2_get_flush_num,	
}; 

int
bfp2_initialization(struct bfp2_interface *bfp2_p, struct bfp2_setup *setup)
{
	char *log_header = "bfp2_initialization";
	struct bfp2_private *priv_p;

	nid_log_notice("%s: start ...", log_header);
	assert(setup);
	bfp2_p->fp_op = &bfp2_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	bfp2_p->fp_private = priv_p;

	priv_p->p_wc = setup->wc;
	priv_p->p_pp = setup->pp; 
	priv_p->p_pagesz = (setup->pagesz >> 20);

	// block size always 4MB
	priv_p->p_blocksz = 4;
	uint32_t block_num = setup->buffersz/priv_p->p_blocksz;

	priv_p->flush_delay_ctl = setup->flush_delay_ctl;
	priv_p->load_ctrl_level = setup->load_ctrl_level;
	priv_p->load_ratio_max = setup->load_ratio_max;
	priv_p->load_ratio_min = setup->load_ratio_min;
	memset(priv_p->p_top_flush_speed, 0, sizeof(priv_p->p_top_flush_speed));

	priv_p->p_flushnum = 0;
	priv_p->p_idlecnt = 0;
	priv_p->p_fastnum = AGGRES_FLUSH_SIZE/priv_p->p_blocksz;	
	priv_p->p_normnum = 1;						
	priv_p->p_buffertip = block_num - (THROTTLE_BSIZE/priv_p->p_blocksz);
	priv_p->p_pagetip = THROTTLE_PSIZE/priv_p->p_pagesz;		
	priv_p->p_rel_page_cnt = 0;
	priv_p->p_lastflush_mode = FLUSH_MODE_NONE;
	priv_p->p_lastflush_idx = 0;
	priv_p->p_high_flush_speed = 0.0;
	priv_p->p_lastflush_time = 0;
	priv_p->p_lastidle_time = 0;
	priv_p->p_cur_time = 0;
	priv_p->p_flush_cycle = 0;
	priv_p->p_idle_mode = IDLE_MODE_NTON;
	priv_p->p_read_counter = 0;
	memset(priv_p->p_flush_speed, 0, sizeof(priv_p->p_flush_speed));
	nid_log_debug("%s: page size is:%u, block size is:%u ", log_header, priv_p->p_pagesz, priv_p->p_blocksz);
	nid_log_debug("%s: buffertip:%u, pagetip:%u ", log_header, priv_p->p_buffertip, priv_p->p_pagetip);
	return 0;
}



