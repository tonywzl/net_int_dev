/*
 * sac_if.h
 * 	Interface of Service Agetn Channel Module
 */

#ifndef _NID_SAC_IF_H
#define _NID_SAC_IF_H

#include <stdint.h>
#include <sys/time.h>
#include "nid.h"
#include "tp_if.h"
#include "tpg_if.h"
#include "io_if.h"
#include "nid_shared.h"
#include "umpk_sac_if.h"

#define SAC_WHERE_FREE 0
#define SAC_WHERE_BUSY 1

struct request_node {
	struct tp_jobheader	r_header;	// this item must the the first one !!!
	int			r_ds_index;
	void			*r_io_handle;
	struct sac_interface	*r_sac;
	struct ds_interface	*r_ds;
	void			*r_io_obj;
	struct list_head	r_head;
	struct list_head	r_list;
	struct list_head	r_read_list;
	struct list_head	r_write_list;
	struct list_head	r_trim_list;
	uint32_t		r_len;
	uint32_t		r_resp_len_1;	// for r_resp_buf
	uint64_t		r_offset;
	uint64_t		r_seq;
	struct nid_request	r_ir;
	char			*r_resp_buf_1;	// for read
	char			*r_resp_buf_2;	// for read
	struct timeval		r_recv_time;
	struct timeval		r_ready_time;
	struct timeval		r_resp_time;
	struct timeval		r_io_start_time;
	struct timeval		r_io_end_time;
	uint16_t		r_resp_delay;	// delay on slowing down the write IO by usleep() on the response before cutting it off, unit: 100 milliseconds
};

struct sac_req_stat {
	uint8_t			sa_is_running;
	uint64_t		sa_req_num;
	uint64_t		sa_req_len;
};

struct sac_stat {
	int			sa_io_type;
	char			*sa_uuid;
	uint8_t			sa_stat;
	char			sa_alevel;
	uint64_t		sa_read_counter;
	uint64_t		sa_write_counter;
	uint64_t		sa_trim_counter;
	uint64_t		sa_keepalive_counter;
	uint64_t		sa_write_ready_counter;
	uint64_t		sa_read_resp_counter;
	uint64_t		sa_write_resp_counter;
	uint64_t		sa_trim_resp_counter;
	uint64_t		sa_keepalive_resp_counter;
	uint64_t		sa_data_not_ready_counter;
	uint64_t		sa_recv_sequence;
	uint64_t		sa_wait_sequence;
	uint64_t		sa_read_last_resp_counter;
	uint64_t		sa_write_last_resp_counter;
	uint32_t		sa_read_stuck_counter;
	uint32_t		sa_write_stuck_counter;
	struct timeval		sa_start_tv;
	char 			*sa_ipaddr;
	struct io_chan_stat	sa_io_stat;
	uint64_t		sa_read_ready_counter;
};

struct sac_operations {
	int			(*sa_accept_new_channel)(struct sac_interface *, int, char *);
	void			(*sa_do_channel)(struct sac_interface *);
	struct request_node*	(*sa_pickup_request)(struct sac_interface *);
	void			(*sa_do_request)(struct tp_jobheader *);
	void			(*sa_add_response_node)(struct sac_interface *, struct request_node *);
	void			(*sa_add_response_list)(struct sac_interface *, struct list_head *);
	void			(*sa_get_stat)(struct sac_interface *, struct sac_stat *);
	void			(*sa_reset_stat)(struct sac_interface *);
	void			(*sa_drop_page)(struct sac_interface *, void *);
	void			(*sa_cleanup)(struct sac_interface *);
	void			(*sa_stop)(struct sac_interface *);
	void			(*sa_prepare_stop)(struct sac_interface *);
	int			(*sa_get_io_type)(struct sac_interface *);
	char*			(*sa_get_uuid)(struct sac_interface *);
	char			(*sa_get_alevel)(struct sac_interface *);
	int			(*sa_upgrade_alevel)(struct sac_interface *);
	void			(*sa_set_max_workers)(struct sac_interface *, uint16_t);
	char			(*sa_get_stop_state)(struct sac_interface *);
	char*			(*sa_get_aip)(struct sac_interface *);
	void			(*sa_get_vec_stat)(struct sac_interface *,struct io_vec_stat *);
	struct wc_interface*	(*sa_get_wc)(struct sac_interface *);
	void			(*sa_remove_wc)(struct sac_interface *);
	int			(*sa_switch_wc)(struct sac_interface *, char *, int);
	void			(*sa_disable_int_keep_alive)(struct sac_interface *);
	void			(*sa_enable_int_keep_alive)(struct sac_interface *, uint32_t);
	struct tp_interface*	(*sa_get_tp)(struct sac_interface *);
	void			(*sa_start_req_counter)(struct sac_interface *);
	void			(*sa_check_req_counter)(struct sac_interface *, struct sac_req_stat *);
	void			(*sa_stop_req_counter)(struct sac_interface *);
	void			(*sa_get_list_stat)(struct sac_interface *, struct umessage_sac_list_stat_resp *);
};

struct sac_interface {
	void			*sa_private;
	struct sac_operations	*sa_op;
};

/* sac configuration information */
struct sac_info {
	char			uuid[NID_MAX_UUID];
	char			sync;
	char			direct_io;
	char			enable_kill_myself;
	int			alignment;
	char			ds_name[NID_MAX_DSNAME];
	char			dev_name[NID_MAX_DEVNAME];
	char			wc_uuid[NID_MAX_UUID];
	char			rc_uuid[NID_MAX_UUID];
	char			tp_name[NID_MAX_TPNAME];
	int			io_type;
	int			ds_type;
	int			wc_type;
	int			rc_type;
	char			export_name[NID_MAX_PATH];
	uint32_t		dev_size;
	struct list_head 	info_list;
	uint8_t			ready;
};

struct sac_setup {
	int			rsfd;
	struct sacg_interface	*sacg;
	struct sdsg_interface	*sdsg;
	struct cdsg_interface	*cdsg;
	struct wcg_interface	*wcg;
	struct crcg_interface	*crcg;
	struct rwg_interface	*rwg;
	struct tpg_interface	*tpg;
	struct io_interface	**all_io;
	struct mqtt_interface	*mqtt;
};

extern int sac_initialization(struct sac_interface *, struct sac_setup *);
#endif
