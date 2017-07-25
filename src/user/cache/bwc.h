#ifndef _BWC_H
#define _BWC_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU
#include <fcntl.h>
#undef __USE_GNU
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <openssl/md5.h>
#include <string.h>
#include <stdio.h>

#include "list.h"
#include "nid_log.h"
#include "nid_shared.h"
#include "lck_if.h"
#include "allocator_if.h"
#include "umpk_bwc_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "ini_if.h"
#include "ds_if.h"
#include "rw_if.h"
#include "rc_if.h"
#include "wc_if.h"
#include "bwc_if.h"
#include "bfp_if.h"
#include "bse_if.h"
#include "bfe_if.h"
#include "bre_if.h"
#include "d2an_if.h"
#include "d2bn_if.h"
#include "ddn_if.h"
#include "pp_if.h"
#include "srn_if.h"
#include "smc_if.h"
#include "fpn_if.h"
#include "lstn_if.h"
#include "rc_wc_cbn.h"
#include "rc_wcn.h"
#include "bwc_rbn_if.h"
#include "mqtt_if.h"
#include "util_bdev.h"

/*
 * Notice:
 * 	- Do remember to update version name for a new release.
 * 	- Do remember add a new version item to bwc_ver_cmp_table[] accordingly.
 */
#define BWC_VERSZ		8
#define	BWC_DEV_HEADER_VER	"35100000"
#define	BWC_DEV_TRAILER_VER	BWC_DEV_HEADER_VER
#define	BWC_DEV_MDZ_VER		BWC_DEV_HEADER_VER

#define BWC_LOGICAL_BLOCK_SIZE	512
#define BWC_MAX_RESOURCE	128

#define BWC_MAGICSZ		8
#if 0
#define BWC_SEG_HEADER_MAGIC	"01234567"
#define BWC_SEG_TRAILER_MAGIC	"12345678"
#endif
#define	BWC_DEV_HEADER_MAGIC	"23456789"
#define	BWC_DEV_TRAILER_MAGIC	"23456789"

#define BWC_MAX_IOV		1024
#define BWC_BLOCK_SIZE		(1<<22)
#define BWC_MIN_FLUSH_BLOCKSZ	(1 << 20)
#define BWC_FLUSH_THROTTLE	512
#define BWC_FLUSH_AGGRESSIVE	768
#define BWC_MAX_SEQSZ		(1<<24)

#define	BWC_PRIV_INDEX		0
#define	BWC_REQNODE_INDEX	1

#define	BWC_RDNODE_INDEX	0
#define	BWC_SREQNODE_INDEX	1

/*
 * per resource fast flushing state for bwc merging preparation:
 * 	0: not started
 * 	1: flushing
 * 	2: flushed
 */
#define BWC_C_FFLUSH_NONE	0
#define BWC_C_FFLUSH_DOING	1
#define BWC_C_FFLUSH_DONE	2

/*
 * steps of slowing down the IO of the channel
 * unit: 100 milliseconds
 */
#define BWC_C_WR_SLOWDOWN_STEP	1
#define BWC_C_RD_SLOWDOWN_STEP	1

/*
 *  thresholds of number of IO requests received during last period
 */
#define BWC_C_WR_CNT_THRESHD	100
#define BWC_C_RD_CNT_THRESHD	100

/* Limit write delay first level max time to 0.2 second */
#define BWC_WRITE_DELAY_FIRST_LEVEL_MAX_TIME 200000
/* Limit write delay second level max time to 0.5 second */
#define BWC_WRITE_DELAY_SECOND_LEVEL_MAX_TIME 500000


#define __round_up(len, alignment)	\
	(((len) & ~((alignment) - 1)) + (((len) & ((alignment) - 1)) ? (alignment) : 0))

#define __round_down(len, alignment)	((len) & ~((alignment) - 1))

#define BWC_RECOVER_MAXREAD	4096

#define TIME_CONSUME_ALERT      1000000

#define time_difference(t1, t2) \
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

/*
 * metadata zone related
 */
struct bwc_mdz_header {
	unsigned char	md_checksum[16];
	char		md_magic_ver[128];
	uint32_t	md_size;		// number of mdz segments
	uint32_t	md_start_seg;		// the mdz segment to start mdz recovery from
	uint32_t	md_end_seg;		// the last mdz segment
	uint64_t	md_flushed_seq;		// the sequence number of flushed request
	uint64_t	md_start_seq;
	off_t		md_last_hseg_off;	// offset of hdz segment which was just flushed to target
	uint64_t	md_last_hseg_seq;	// seq of hdz segment which was just flushed to target
	uint64_t	md_last_flushed_seq;	// current flushed seq of the last request in mdz zone
	uint64_t	md_last_seq;		// last seq in mdz
	char		md_start;		// set when first time rebuild
	char		md_first_seg;		// set after write the first segment
	char 		md_empty;		// is the mdz empty
};

#define BWC_MDZ_SEGSZ		(1 << 13)	// number of requests in one mdz seqment

#define BWC_MDZ_SCAN_N_SEG	512		// small enough for BWC_RBN_BUFSZ(32K)

#define BWC_REQ_HEADERSZ	32		// >= sizeof(struct bwc_req_header) and factor of 512
struct bwc_req_header {
	uint16_t	resource_id;
	char		is_mdz:1;
	uint32_t	data_len;
	off_t		offset;
	off_t		wc_offset;
	uint64_t	my_seq;
};

#define BWC_SEG_HEADERSZ	(priv_p->p_dio_alignment)
struct bwc_seg_header {
	char		header_magic[BWC_MAGICSZ];
	uint32_t	req_nr;		// number of requests contained in this seqment
	uint32_t	header_len;	// aligned length of all request headers
	uint32_t	data_len;	// lenght of this segment excluding seg header/trailer and header_len
	uint64_t	my_seq;
	uint64_t	flushed_seq;
	uint64_t	wc_offset;
};

#define BWC_SEG_TRAILERSZ	(priv_p->p_dio_alignment)
struct bwc_seg_trailer {
	char		trailer_magic[BWC_MAGICSZ];
	uint32_t	req_nr;
	uint32_t	header_len;
	uint32_t	data_len;
	uint64_t	my_seq;
	uint64_t	flushed_seq;
	uint64_t	wc_offset;
};

#define BWC_DEV_HEADERSZ	(1 << 20)
struct bwc_dev_header {
	char		header_magic[BWC_MAGICSZ];
	char		header_ver[BWC_VERSZ];
	uint16_t	dh_counter;
	int		dh_data_size;
	char		dh_seg_header_magic[BWC_MAGICSZ];
	char		dh_seg_trailer_magic[BWC_MAGICSZ];
};

#define BWC_DEV_OBJ_HEADERSZ	8
struct bwc_dev_obj_header {
	uint16_t	obj_type;
	uint16_t	unused;
	uint16_t	obj_id;
	uint16_t	obj_len;
};

#define BWC_DEV_TRAILERSZ	(1 << 20)
struct bwc_dev_trailer {
	char		trailer_magic[BWC_MAGICSZ];
	char		trailer_ver[BWC_VERSZ];
	off_t		dt_offset;
	uint64_t	dt_my_seq;
	uint64_t	dt_flushed_seq;
};

struct bwc_flushing_st {
	uint64_t	f_seq;
	off_t		f_end_offset;
	uint8_t		f_occupied;
	uint32_t	data_len;
};

struct bwc_chain {
	struct list_head	c_list;
	struct list_head	c_chan_head;
	struct list_head	c_search_head;
	char			c_resource[NID_MAX_UUID];
	uint64_t		c_seq_flushed;
	uint64_t		c_seq_assigned;
	uint8_t			c_freeze;
	pthread_mutex_t		c_lck;
	char			*c_pos;		// position in c_page

	struct pp_page_node	*c_page;
	struct ddn_interface	c_ddn;
	struct bse_interface	c_bse;
	struct bfe_interface	c_bfe;
	struct bre_interface	c_bre;
	uint8_t			c_active;
	uint8_t			c_busy;
	uint16_t		c_id;

	struct bwc_interface	*c_bwc;
	struct rc_interface	*c_rc;
	void			*c_rc_handle;
	struct rw_interface	*c_rw;
	void			*c_rw_handle;
	int			c_target_sync;
	pthread_mutex_t		c_rlck;
	pthread_mutex_t		c_wlck;
	pthread_mutex_t		c_trlck;

	struct list_head	c_fflush_list;	// list node of fast fast flushing
	volatile uint8_t	c_fflush_state;		// fast flushing state for bwc merging preparation: 0: not started; 1: flushing; 2: flushed
	uint8_t			c_do_dropcache;

	volatile uint8_t	c_cutoff;		// stop IO before merging to new bwc
	uint16_t		c_wr_slowdown_step;	// step of slowing down the write IO before cutting it off, unit: 100 milliseconds
	uint16_t		c_rd_slowdown_step;	// step of slowing down the read IO before cutting it off, unit: 100 milliseconds
};

struct bwc_channel {
	struct list_head	c_list;
	struct bwc_chain	*c_chain;
	struct sac_interface	*c_sac;
	struct list_head	c_read_head;
	struct list_head	c_write_head;
	struct list_head	c_trim_head;
	struct list_head	c_resp_head;
	uint64_t		c_wr_recv_counter;	// accumulated number of all write requests received
	uint64_t		c_rd_recv_counter;	// accumulated number of all read requests received
	uint64_t		c_tr_recv_counter;	// accumulated number of all trim requests received
	volatile uint64_t	c_rd_done_counter;	// accumulated number of all processed read
};

struct bwc_private {
	char				p_uuid[NID_MAX_UUID];
	struct allocator_interface	*p_allocator;
	struct d2an_interface		p_d2an;
	struct d2bn_interface		p_d2bn;
	struct wc_interface		*p_wc;
	struct bfp_interface		*p_bfp;
	int				p_bfp_type;
	double				p_coalesce_ratio;
	double  			p_load_ratio_max;
	double  			p_load_ratio_min;
	double  			p_load_ctrl_level;
	double  			p_flush_delay_ctl;
	double				p_throttle_ratio;
	int					p_low_water_mark;
	int					p_high_water_mark;
	struct list_head		p_chain_head;
	struct list_head		p_frozen_chain_head;	// list of frozen chain
	struct list_head		p_freezing_chain_head;	// list of freezing chain
	struct list_head		p_inactive_head;
	struct list_head		p_cfflush_head; 	// list of fast flushing channels
	pthread_mutex_t			p_lck;
	struct lck_interface		p_lck_if;
	struct lck_node			p_lnode;		// For list p_chain_head, p_inactive_head, p_cfflush_head
	pthread_mutex_t			p_snapshot_lck;
	pthread_cond_t			p_snapshot_cond;
	pthread_mutex_t			p_new_trim_lck;
	pthread_cond_t			p_new_trim_cond;
	pthread_mutex_t			p_new_write_lck;
	pthread_cond_t			p_new_write_cond;
	pthread_mutex_t			p_new_read_lck;
	pthread_cond_t			p_new_read_cond;
	pthread_mutex_t			p_seq_lck;
	pthread_cond_t			p_seq_cond;
	pthread_mutex_t			p_occ_lck;				// occupied lock
	pthread_cond_t			p_occ_cond;
	char				p_bufdevice[NID_MAX_PATH];
	char				*p_resources[BWC_MAX_RESOURCE];
	struct bwc_flushing_st		*p_flushing_st;				// flushing status
	char				*p_rh_vec;
	struct bwc_dev_header		*p_dhp;
	struct bwc_dev_trailer		*p_dtp;
	struct bwc_seg_header		*p_shp;
	struct bwc_seg_trailer		*p_stp;
	char				*p_seg_content;
	int				p_fhandle;				// for bufdevice
	int				p_read_max_retry_time;
	int				p_max_flush_size;
	int				p_write_delay_first_level;		// Block left number
	int				p_write_delay_second_level;		// Block left number
	int				p_write_delay_first_level_increase_interval;
	int				p_write_delay_second_level_start_ms;
	int				p_write_delay_second_level_increase_interval;
	ssize_t				p_dio_alignment;
	ssize_t				p_bufdevicesz;
	uint64_t			p_mdz_size;
	uint32_t			p_mdz_next_req;
	struct bwc_mdz_header		*p_mdz_hp;
	struct bwc_req_header		*p_mdz_rhp;
	uint32_t			p_mdz_headersz;
	uint32_t			p_mdz_reqheadersz;
	uint32_t			p_mdz_segsz;
	uint32_t			p_mdz_seg_num;
	uint64_t			p_mdz_segsz_mask;
	uint64_t			p_mdz_last_seq;
	off_t				p_first_seg_off;			// offset of the first segment
	uint64_t			p_data_flushed;				// bytes
	uint64_t			p_data_pkg_flushed;			// bytes
	uint64_t			p_nondata_pkg_flushed;			// bytes
	uint64_t			p_seq_assigned;
	uint64_t			p_seq_flushable;
	uint64_t			p_seq_flushed;				// current block seq flushed
	uint64_t			p_rec_seq_flushed;			// current block seq flushed which already recorded in bufdevice
	uint64_t			p_seq_flush_aviable;
	struct pp_interface		*p_pp;
	struct tp_interface		*p_tp;
	struct srn_interface		*p_srn;
	struct lstn_interface		*p_lstn;
	struct rc_wc_cbn_interface      p_rc_wc_cbn;
	struct rc_wcn_interface         p_rc_wcn;
	struct bwc_rbn_interface       p_rbn;
	struct list_head		p_write_head;
	struct list_head		p_trim_head;
	pthread_cond_t			p_read_post_cond;
	struct list_head 		p_read_finished_head;
	pthread_mutex_t			p_read_finish_lck;
	uint64_t			p_resp_counter;
	volatile int64_t		p_read_counter;				// bytes of read from start
	off_t				p_cur_offset;				// current offset of bufdevice
	uint64_t			p_flush_blocksz;
	uint32_t			p_pagesz;
	uint32_t			p_segsz;				// max size in one wirte to bufdevice
	uint32_t			p_page_avail;
	uint16_t			p_cur_block;
	uint16_t			p_cur_flush_block;
	uint16_t			p_noccupied;				// number of occupied on the bufdevice
	uint16_t			p_flush_nblocks;
	uint8_t				p_rw_sync;
	uint8_t				p_two_step_read;
	uint8_t				p_do_fp;
	uint8_t				p_new_trim_task;
	uint8_t				p_new_write_task;
	uint8_t				p_new_read_task;
	uint8_t				p_fast_flush;
	uint8_t				p_stop;
	uint8_t				p_pause;
	uint8_t				p_channel_changing;
	uint8_t				p_release_stop;
	uint8_t				p_trim_stop;
	uint8_t				p_write_stop;
	uint8_t				p_read_stop;
	uint8_t				p_snapshot_to_pause;
	uint8_t				p_snapshot_pause;
	uint8_t				p_post_read_stop;
	uint8_t				p_ssd_mode;
	volatile uint8_t		p_busy;
	volatile uint8_t		p_sync_rec_seq;
	uint8_t				p_mdz_recover;
	struct bwc_chain		*p_freeze_chain;
	int				p_mdz_last_is_trail;
	uint8_t				p_start_count;
	uint64_t			p_req_num;
	uint64_t			p_req_len;
	struct mqtt_interface		*p_mqtt;
	__useconds_t			p_record_delay;
	int				p_read_busy;
	int				p_write_busy;
	uint64_t			p_write_vec_counter;
	uint32_t 		sec_size;	// cache device sector size
	uint64_t 		dev_size;	// cache device size in bytes
	uint8_t			is_dev;		// cache device is real bock device or only a file
};

static inline ssize_t
__read_n(int fd, void *buf, size_t n)
{
	char *p = buf;
	ssize_t nread;
	size_t left = n;
	int nretry = 12;

	while (left) {
retry:
		nread = read(fd, p, left);
		if (nread == -1) {
			if (nretry--) {
				sleep(5);
				goto retry;
			} else {
				return -errno;
			}
		}
		if (nread == 0)
			return (n - left);
		left -= nread;
		p += nread;
	}
	return n;
}

static inline ssize_t
__write_n(int fd, void *buf, size_t n)
{
	char *p = buf;
	ssize_t nwritten;
	size_t left = n;
	int nretry = 12;

	while (left) {
retry:
		nwritten = write(fd, p, left);
		if (nwritten == -1) {
			if (nretry--) {
				sleep(5);
				goto retry;
			} else {
				return -errno;
			}
		}
		if (nwritten == 0)
			return (n - left);
		left -= nwritten;
		p += nwritten;
	}
	return n;
}

static inline ssize_t
__pread_n(int fd, void *buf, size_t n, off_t off)
{
	char *p = buf;
	ssize_t nread;
	size_t left = n;
	int nretry = 12;

	while (left) {
retry:
		nread = pread(fd, p, left, off);
		if (nread == -1) {
			if (nretry--) {
				sleep(5);
				goto retry;
			} else {
				return -errno;
			}
		}
		if (nread == 0)
			return (n - left);
		left -= nread;
		off += nread;
		p += nread;
	}
	return n;
}

static inline ssize_t
__pwrite_n(int fd, void *buf, size_t n, off_t off)
{
	char *p = buf;
	ssize_t nwritten;
	size_t left = n;
	int nretry = 12;

	while (left) {
retry:
		nwritten = pwrite(fd, p, left, off);
		if (nwritten == -1) {
			if (nretry--) {
				sleep(5);
				goto retry;
			} else {
				return -errno;
			}
		}
		if (nwritten == 0)
			return (n - left);
		left -= nwritten;
		off += nwritten;
		p += nwritten;
	}
	return n;
}

static inline uint64_t
__my_first_seq(struct bwc_seg_header *shp)
{
	if (shp->req_nr)
		return shp->my_seq - shp->req_nr + 1;
	else
		return shp->my_seq;
}

extern void __bwc_start_page(struct bwc_chain *, void *, int);
extern int __bwc_dev_trailer_checking(struct bwc_private *, struct bwc_dev_trailer *);
extern struct bwc_chain * __get_inactive_chain_by_id(struct bwc_private *, uint16_t);
extern void __bufdevice_update_header(struct bwc_private *);
extern void __bwc_hdz_update_offset(struct bwc_private *, uint32_t, uint32_t, off_t);
extern void __bwc_mdz_update_offset(struct bwc_private *, uint32_t, off_t);
extern void __bwc_mdz_add_request(struct bwc_private *priv_p, struct bwc_req_header *);
extern void __bwc_mdz_flush_header(struct bwc_private *);
extern void __bwc_mdz_try_invalid(struct bwc_private *, struct bwc_seg_header *);
extern void __bwc_mdz_try_flush(struct bwc_private *, struct bwc_seg_header *);
extern int __bufdevice_rebuild_mdz_header(struct bwc_private *);
extern int __bufdevice_rebuild_dev_header(struct bwc_private *);
extern int __bwc_seg_checking(struct bwc_private *, struct bwc_seg_header *, struct bwc_seg_trailer *,
	struct bwc_rb_node **, off_t *, struct list_head *, uint64_t);
extern void __bwc_mdz_scan(struct bwc_private *, uint64_t *, off_t *, uint64_t *, struct list_head *, int *);
extern int __bwc_hdz_scan(struct bwc_private *, uint64_t, uint64_t *, uint64_t *, struct list_head *, int);
extern int __bwc_scan_bufdevice(struct bwc_private *, off_t *, uint64_t *, uint64_t *, uint64_t *,
	struct bwc_dev_trailer *, struct list_head *);
extern int __bwc_bufdevice_restore(struct bwc_interface *, off_t, uint64_t, uint64_t, uint64_t,
	struct bwc_dev_trailer *, struct list_head *);
extern int __bwc_bufdevice_restore2(struct bwc_interface *, uint64_t, uint64_t, uint64_t,
	struct bwc_dev_trailer *, struct list_head *, uint64_t);
extern void __bwc_write_delay(struct bwc_private *);
extern __useconds_t _bwc_calculate_sleep_time(struct bwc_private *, uint16_t);
extern int bwc_unfreeze_snapshot(struct bwc_interface *, char *);
extern int bwc_freeze_chain_stage1(struct bwc_interface *, char *);
extern int bwc_freeze_chain_stage2(struct bwc_interface *, char *);
extern int bwc_destroy_chain(struct bwc_interface *, char *);
extern void __bwc_destroy_chain(struct bwc_chain *chain_p);

extern int bwc_fast_flush(struct bwc_interface *, char *);
extern int bwc_get_fast_flush(struct bwc_interface *, char *);
extern int bwc_stop_fast_flush(struct bwc_interface *, char *);

extern int __bwc_ver_cmp_check(char *, char *);
#endif
