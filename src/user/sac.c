/*
 * sac.c
 * 	Implementation of Server Agent Channel Module
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <errno.h>
#include <endian.h>
#include <assert.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_shared.h"
#include "list.h"
#include "nid_log.h"
#include "tp_if.h"
#include "node_if.h"
#include "lstn_if.h"
#include "ini_if.h"
#include "sah_if.h"
#include "ds_if.h"
#include "io_if.h"
#include "wc_if.h"
#include "rc_if.h"
#include "rw_if.h"
#include "sacg_if.h"
#include "sac_if.h"
#include "sdsg_if.h"
#include "cdsg_if.h"
#include "crcg_if.h"
#include "rwg_if.h"
#include "wcg_if.h"
#include "mqtt_if.h"

#define PAGE_STAT_INUSE		0x01
#define TIME_CONSUME_ALERT	1000000
#define TIME_WAIT_CONSUME_ALERT	1800
#define SOCKET_READ_TIMER	20

#define time_difference(t1, t2) \
	((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec))

#define time_difference_sec(t1, t2) \
	(t2.tv_sec - t1.tv_sec)

struct request_stream {
	char		*r_buf;	// buffer for request input
	uint32_t	r_start;
	uint32_t	r_curr_pos;
	uint32_t	r_size;
};

struct sac_private {
	struct sacg_interface	*p_sacg;
	struct sdsg_interface	*p_sdsg;
	struct cdsg_interface	*p_cdsg;
	struct wcg_interface	*p_wcg;
	struct crcg_interface	*p_crcg;
	struct rwg_interface	*p_rwg;
	struct tpg_interface	*p_tpg;
	struct tp_interface	*p_tp;
	struct ds_interface	*p_ds;
	int			p_ds_type;
	char			p_ds_name[NID_MAX_UUID];
	struct io_interface	**p_all_io;
	struct io_interface	*p_io;
	int			p_io_type;
	struct wc_interface	*p_wc;
	int			p_wc_type;
	struct rc_interface	*p_rc;
	int			p_rc_type;
	struct rw_interface	*p_rw;
	uint32_t		p_ds_pagesz;
	int			p_in_ds;
	int			p_out_ds;
	void			*p_io_handle;
	void			*p_wc_handle;
	void			*p_rc_handle;
	void			*p_rw_handle;
	struct sac_stat		p_stat;
	struct list_head	p_free_head1;		// list head of all free request_node
	struct list_head	p_free_head2;		// list head of all free request_node
	struct list_head	p_resp_head1;		// list head of all response node
	struct list_head	p_resp_head2;		// list head of all response node
	struct list_head	p_dresp_head1;		// list head of all data response node
	struct list_head	p_dresp_head2;		// list head of all data response node
	struct list_head	p_req_head;		// requests
	struct list_head	p_to_req_head;		// requests
	struct list_head	p_read_head;		// read request list
	struct list_head	p_to_read_head;		//
	struct list_head	p_write_head;		// write request list
	struct list_head	p_to_write_head;	//
	struct list_head	p_trim_head;		// trim request list
	struct list_head	p_to_trim_head;	//
	struct list_head	*p_free_ptr;		// pointer to the free head
	struct list_head	*p_to_free_ptr;		// pointer to the the head ready to be free, protected by p_free_lck
	struct list_head	*p_resp_ptr;		// pointer to the response head
	struct list_head	*p_to_resp_ptr;		// pointer to the the head ready to response, protected by p_resp_lck
	struct list_head	*p_dresp_ptr;		// pointer to the response data head
	struct list_head	*p_to_dresp_ptr;	// pointer to the the head of data ready to response, protected by p_resp_lck
	pthread_mutex_t		p_free_lck;		// protect p_to_free_ptr
	pthread_mutex_t		p_req_lck;		// protect p_to_resp_ptr
	pthread_mutex_t		p_resp_lck;		// protect p_to_resp_ptr
	pthread_mutex_t		p_dresp_lck;		// protect p_to_dresp_ptr
	pthread_mutex_t		p_input_lck;		// protect p_data_head/p_read_head/p_write_head
	pthread_cond_t		p_resp_cond;		//
	pthread_cond_t		p_dresp_cond;		//
	struct sah_interface	*p_sah;
	uint64_t		p_last_sequence;
	uint32_t		p_last_len;
	int			p_rsfd;			// sockfd for request connection
	int			p_dsfd;			// sockfd for data connection
	char			p_ipaddr[NID_MAX_IP];
	u_short			p_dport;		// port for data connection
	struct request_stream	p_req_stream;
	struct request_node	p_req_nodes[NID_MAX_REQNODES];
	uint64_t		p_do_req_counter;	// number of worker threads which are doing requests
	uint32_t		p_max_read_stuck;
	uint32_t		p_max_write_stuck;
	char			*p_uuid;
	uint32_t		p_dev_size;
	char			*p_exportname;
	char			p_bufdevice[NID_MAX_DEVNAME];
	char			p_alevel;		// RDONLY | RDWR
	char			p_sync;
	char			p_direct_io;
	char			p_chan_established;
	char			p_resp_busy;
	char			p_dresp_busy;
	char			p_data_done;
	char			p_pickup_done;
	char			p_resp_done;
	char			p_dresp_done;
	char			p_req_done;
	uint8_t			p_to_stop;
	char			p_int_keep_alive;
	uint32_t		p_int_keep_alive_interval;
	uint32_t		p_int_keep_alive_cur_num;
	uint32_t		p_int_keep_alive_break_num;
	int			p_alignment;	// alignment for block size
	int			p_stop_keep_live;
	int			p_where;
	uint64_t		p_req_num_counter;
	uint64_t		p_req_len_counter;
	uint8_t			p_req_counter_flag;
	char			p_enable_kill_myself;
	int			p_slowio_read_counter;
	int			p_slowio_write_counter;
	int			p_slowio_trim_counter;
	struct mqtt_interface	*p_mqtt;
};

struct sac_data_job {
	struct tp_jobheader	j_header;
	struct sac_interface 	*s_sac;
};

struct sac_response_job {
	struct tp_jobheader	j_header;
	struct sac_interface 	*s_sac;
};

static inline void
__add_response(struct sac_interface *sac_p, struct request_node *rn_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	pthread_mutex_lock(&priv_p->p_resp_lck);
	list_add(&rn_p->r_list, priv_p->p_to_resp_ptr);
	if (!priv_p->p_resp_busy) {
		nid_log_debug("__add_response: to signal p_resp_cond, uuid:%s ...",
			priv_p->p_uuid);
		pthread_cond_signal(&priv_p->p_resp_cond);
	}
	pthread_mutex_unlock(&priv_p->p_resp_lck);
}

static inline void
__add_response_tail(struct sac_interface *sac_p, struct request_node *rn_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	pthread_mutex_lock(&priv_p->p_resp_lck);
	list_add_tail(&rn_p->r_list, priv_p->p_to_resp_ptr);
	if (!priv_p->p_resp_busy) {
		nid_log_debug("__add_response_tail: to signal p_resp_cond, uuid:%s ...",
			priv_p->p_uuid);
		pthread_cond_signal(&priv_p->p_resp_cond);
	}
	pthread_mutex_unlock(&priv_p->p_resp_lck);
}

static int
__do_channel_read(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	int out_ds_index = priv_p->p_out_ds;
	struct io_interface *io_p = priv_p->p_io;
	struct sac_stat *stat_p = &priv_p->p_stat;
	uint32_t got_len, left_len = rn_p->r_len;

	stat_p->sa_read_counter++;
	gettimeofday(&rn_p->r_recv_time, NULL);
	memcpy(&rn_p->r_ir, ir_p, NID_SIZE_REQHDR);
	rn_p->r_len = ntohl(ir_p->len);
	rn_p->r_offset = be64toh(ir_p->offset);

	priv_p->p_where = SAC_WHERE_BUSY;
	rn_p->r_resp_buf_1 = ds_p->d_op->d_get_buffer(ds_p, out_ds_index, &got_len, io_p);
	priv_p->p_where = SAC_WHERE_FREE;
	if (!rn_p->r_resp_buf_1 || priv_p->p_to_stop)
		return -1;

	rn_p->r_seq = ds_p->d_op->d_sequence(ds_p, out_ds_index);
	if (left_len <= got_len) {
		/* the buffer we got is big enough for this read */
		rn_p->r_resp_len_1 = left_len;
		ds_p->d_op->d_confirm(ds_p, out_ds_index, rn_p->r_resp_len_1);
	} else {
		char *second_buf;
		uint64_t second_seq;

		rn_p->r_resp_len_1 = got_len;
		ds_p->d_op->d_confirm(ds_p, out_ds_index, rn_p->r_resp_len_1);
		left_len -= got_len;
		priv_p->p_where = SAC_WHERE_BUSY;
		second_buf = ds_p->d_op->d_get_buffer(ds_p, out_ds_index, &got_len, io_p);
		priv_p->p_where = SAC_WHERE_FREE;
		if (!second_buf || priv_p->p_to_stop)
			return -1;
		assert(left_len <= got_len);
		second_seq = ds_p->d_op->d_sequence(ds_p, out_ds_index);
		ds_p->d_op->d_confirm(ds_p, out_ds_index, left_len);
		if (ds_p->d_op->d_sequence_in_row(ds_p, out_ds_index, rn_p->r_seq, second_seq)) {
			rn_p->r_resp_len_1 = rn_p->r_len;
		} else {
			rn_p->r_resp_buf_2 = second_buf;
		}
	}
	gettimeofday(&rn_p->r_ready_time, NULL);
	list_add_tail(&rn_p->r_list, &priv_p->p_to_req_head);
	list_add_tail(&rn_p->r_read_list, &priv_p->p_to_read_head);
	stat_p->sa_read_ready_counter++;
	return 0;
}

static int
__do_channel_write(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	char *log_header = "__do_channel_write";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;

	gettimeofday(&rn_p->r_recv_time, NULL);
	memcpy(&rn_p->r_ir, ir_p, NID_SIZE_REQHDR);
	rn_p->r_len = ntohl(ir_p->len);
	rn_p->r_offset = be64toh(ir_p->offset);
	rn_p->r_seq = be64toh(ir_p->dseq);

	if (rn_p->r_seq == priv_p->p_last_sequence + priv_p->p_last_len) {
		list_add_tail(&rn_p->r_list, &priv_p->p_to_req_head);
		list_add_tail(&rn_p->r_write_list, &priv_p->p_to_write_head);
		stat_p->sa_write_counter++;
		priv_p->p_last_sequence = rn_p->r_seq;
		priv_p->p_last_len = rn_p->r_len;
		stat_p->sa_wait_sequence = priv_p->p_last_sequence + priv_p->p_last_len;
		if (priv_p->p_req_counter_flag) {
			priv_p->p_req_num_counter++;
			priv_p->p_req_len_counter += rn_p->r_len;
		}
	} else {
		nid_log_error("%s: wrong sequence! stop!"
			"last_seq:%lu, last_len:%u, seq:%lu, len:%u",
			log_header, priv_p->p_last_sequence, priv_p->p_last_len,
			rn_p->r_seq, rn_p->r_len);
		sac_p->sa_op->sa_stop(sac_p);
		return -1;
	}

	return 0;
}

static int
__do_channel_trim(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;

	assert(ir_p);
	ir_p->len = ntohl(ir_p->len);
	ir_p->offset = be64toh(ir_p->offset);

	list_add_tail(&rn_p->r_list, &priv_p->p_to_req_head);
	list_add_tail(&rn_p->r_trim_list, &priv_p->p_to_trim_head);
	stat_p->sa_trim_counter++;
	if (priv_p->p_req_counter_flag) {
		priv_p->p_req_num_counter++;
	}

	return 0;
}

static int
__do_channel_keepalive(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	char *log_header = "__do_channel_keepalive";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;

	nid_log_debug("%s: got keepalive, uuid:%s", log_header, priv_p->p_uuid);
	assert(ir_p);
	pthread_mutex_lock(&priv_p->p_req_lck);
	priv_p->p_do_req_counter++;
	pthread_mutex_unlock(&priv_p->p_req_lck);

	if (!priv_p->p_to_stop) {
		/* now we are sure that the response thread will process this request */
		stat_p->sa_keepalive_counter++;
		nid_log_debug("%s: __add_response %luth keepalive, uuid:%s",
			log_header, stat_p->sa_keepalive_counter, priv_p->p_uuid);
		__add_response(sac_p, rn_p);
	} else {
		pthread_mutex_lock(&priv_p->p_req_lck);
		priv_p->p_do_req_counter--;
		pthread_mutex_unlock(&priv_p->p_req_lck);
	}

	return 0;
}

static int
__do_channel_upgrade(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	char *log_header = "__do_channel_upgrade";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct mqtt_interface *mqtt_p = priv_p->p_mqtt;
	struct mqtt_message msg;
	int rc;

	nid_log_info("%s: start ...", log_header);
	assert(ir_p);
	pthread_mutex_lock(&priv_p->p_req_lck);
	priv_p->p_do_req_counter++;
	pthread_mutex_unlock(&priv_p->p_req_lck);
	if (!priv_p->p_to_stop) {
		/* now we are sure that the response thread will process this request */
		rc = sacg_p->sag_op->sag_upgrade_alevel(sacg_p, sac_p);
		nid_log_notice("%s: %s to upgrade: %s, %s:%d, rsfd:%d, dsfd:%d",
			log_header,
			rc ? "Failed" : "Successful",
			priv_p->p_uuid,
			priv_p->p_ipaddr, priv_p->p_dport,
			priv_p->p_rsfd, priv_p->p_dsfd);
		rn_p->r_ir.code = rc;
		__add_response(sac_p, rn_p);
		if (!rc){
			msg.type = "nidserver";
			msg.module = "NIDConnection";
			msg.message = "An NID connection upgraded";
			msg.status = "OK";
			strcpy (msg.uuid, priv_p->p_uuid);
			strcpy (msg.ip, priv_p->p_ipaddr);
			mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
		}
	} else {
		pthread_mutex_lock(&priv_p->p_req_lck);
		priv_p->p_do_req_counter--;
		pthread_mutex_unlock(&priv_p->p_req_lck);
	}

	return 0;
}

static int
__do_channel_fupgrade(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	char *log_header = "__do_channel_fupgrade";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct mqtt_interface *mqtt_p = priv_p->p_mqtt;
	struct mqtt_message msg;
	int rc;

	nid_log_info("%s: start ...", log_header);
	assert(ir_p);
	pthread_mutex_lock(&priv_p->p_req_lck);
	priv_p->p_do_req_counter++;
	pthread_mutex_unlock(&priv_p->p_req_lck);
	if (!priv_p->p_to_stop) {
		/* now we are sure that the response thread will process this request */
		rc = sacg_p->sag_op->sag_fupgrade_alevel(sacg_p, sac_p);
		nid_log_notice("%s: %s forcely upgrade: %s, %s:%d, rsfd:%d, dsfd:%d",
			log_header,
			rc ? "Failed" : "Successful",
			priv_p->p_uuid,
			priv_p->p_ipaddr, priv_p->p_dport,
			priv_p->p_rsfd, priv_p->p_dsfd);
		rn_p->r_ir.code = (rc) ? 1 : 0;
		__add_response(sac_p, rn_p);
		if (!rc){
			msg.type = "nidserver";
			msg.module = "NIDConnection";
			msg.message = "An NID connection upgraded";
			msg.status = "OK";
			strcpy (msg.uuid, priv_p->p_uuid);
			strcpy (msg.ip, priv_p->p_ipaddr);
			mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
		}
	} else {
		pthread_mutex_lock(&priv_p->p_req_lck);
		priv_p->p_do_req_counter--;
		pthread_mutex_unlock(&priv_p->p_req_lck);
	}

	return 0;
}

static int
__do_channel_delete_device(struct sac_interface *sac_p, struct request_node *rn_p, struct nid_request *ir_p)
{
	char *log_header = "__do_channel_delete_device";
	nid_log_info("%s: start ...", log_header);
	assert(rn_p && ir_p);
	sac_p->sa_op->sa_stop(sac_p);
	return 0;
}

static void
sac_add_response_node(struct sac_interface *sac_p, struct request_node *rn_p)
{
	__add_response_tail(sac_p, rn_p);
}

static void
sac_add_response_list(struct sac_interface *sac_p, struct list_head *resp_head)
{
	char *log_header = "sac_add_response_list";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	pthread_mutex_lock(&priv_p->p_resp_lck);
	list_splice_tail_init(resp_head, priv_p->p_to_resp_ptr);
	if (!priv_p->p_resp_busy) {
		nid_log_debug("%s: to signal p_resp_cond, uuid:%s ...",
			log_header, priv_p->p_uuid);
		pthread_cond_signal(&priv_p->p_resp_cond);
	}
	pthread_mutex_unlock(&priv_p->p_resp_lck);
}

static int
sac_accept_new_channel(struct sac_interface *sac_p, int sfd, char *cip)
{
	char *log_header = "sac_accept_new_channel";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct cdsg_interface *cdsg_p = priv_p->p_cdsg;
	struct wcg_interface *wcg_p = priv_p->p_wcg;
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	struct rwg_interface *rwg_p = priv_p->p_rwg;
	struct tpg_interface *tpg_p = priv_p->p_tpg;
	struct sac_stat *stat_p = &priv_p->p_stat;
	struct sah_interface *sah_p = priv_p->p_sah;
	struct sac_info *sac_info_p;
	struct ds_interface *ds_p;
	struct io_interface *io_p;
	struct wc_interface *wc_p;
	struct rc_interface *rc_p;
	struct rw_interface *rw_p;
	struct io_channel_info io_info;
	struct wc_channel_info wc_info;
	struct rc_channel_info rc_info;
	struct request_node *rn_p;
	int do_buffer, i, optval, rc = 0, new_io = 0, new_rc = 0, new_wc = 0;
	char *ds_name, *wc_uuid, *rc_uuid, *rw_name, *tp_name;

	nid_log_notice("%s: start (sfd:%d, ip:%s) ...", log_header, sfd, cip);
	gettimeofday(&stat_p->sa_start_tv, NULL);
	rc = sah_p->h_op->h_channel_establish(sah_p);
	if (rc) {
		nid_log_error("%s: handshake failed ", log_header);
		goto out;
	}

	/*
	 * setup request/data input connections
	 */
	priv_p->p_rsfd = sfd;
	priv_p->p_dsfd = sah_p->h_op->h_get_dsfd(sah_p);
	strcpy(priv_p->p_ipaddr, cip);
	optval = 1;
	if (setsockopt(priv_p->p_rsfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) < 0) {
		nid_log_error("%s: cannot set TCP_NODELAY of rsfd", log_header);
	} else {
		nid_log_debug("%s: set TCP_NODELAY of rsfd successfully", log_header);
	}

	priv_p->p_uuid = sah_p->h_op->h_get_uuid(sah_p);
	priv_p->p_exportname = sah_p->h_op->h_get_exportname(sah_p);
	sac_info_p = sah_p->h_op->h_get_sac_info(sah_p);
	priv_p->p_ds_type = sac_info_p->ds_type;
	strcpy(priv_p->p_ds_name, sac_info_p->ds_name);
	ds_name = sac_info_p->ds_name;
	wc_uuid = sac_info_p->wc_uuid;
	rc_uuid = sac_info_p->rc_uuid;
	rw_name = sac_info_p->dev_name;
	tp_name = sac_info_p->tp_name;
	priv_p->p_wc_type = sac_info_p->wc_type;
	priv_p->p_rc_type = sac_info_p->rc_type;
	priv_p->p_sync = sac_info_p->sync;
	priv_p->p_direct_io = sac_info_p->direct_io;
	priv_p->p_enable_kill_myself = sac_info_p->enable_kill_myself;
	priv_p->p_alignment = sac_info_p->alignment;
	priv_p->p_dev_size = sac_info_p->dev_size;
	priv_p->p_tp = tpg_p->tg_op->tpg_search_and_create_tp(tpg_p, tp_name);

	/*
	 * set up ds
	 */
	if (sdsg_p)
		priv_p->p_ds = sdsg_p->dg_op->dg_search_sds(sdsg_p, ds_name);
	if (!priv_p->p_ds && cdsg_p)
		priv_p->p_ds = cdsg_p->dg_op->dg_search_cds(cdsg_p, ds_name);

	/*
	 * setup wc
	 *	Now we have async bwc recovery. The p_recovered of scg is no long used
	 *	to check if all the bwcs are recovered and ready to accept channels.
	 *	So we can be here without bwc recovered, or even initialized and added.
	 *	If the bwc is not added and recovered, we need to stop creating this channel.
	 */
	if (wc_uuid[0] != '\0') {
		if (wcg_p)
			priv_p->p_wc = wcg_p->wg_op->wg_search_and_create_wc(wcg_p, wc_uuid);
		if (!priv_p->p_wc ||
				priv_p->p_wc->wc_op->wc_get_recover_state(priv_p->p_wc) != WC_RECOVER_DONE) {
			nid_log_warning("%s: wc:%s not ready, going to stop... %p", log_header, wc_uuid, priv_p->p_wc);
			rc = -1;
			goto out;
		}
	}

	/*
	 * setup rc
	 */
	if (priv_p->p_rc_type == RC_TYPE_CAS) {
		if (crcg_p)
			priv_p->p_rc = crcg_p->rg_op->rg_search_crc(crcg_p, rc_uuid);

		if (!priv_p->p_rc ||
				priv_p->p_rc->rc_op->rc_get_recover_state(priv_p->p_rc) != RC_RECOVER_DONE) {
			nid_log_warning("%s: rc:%s not ready, going to stop...", log_header, rc_uuid);
			rc = -1;
			goto out;
		}
	}

	/*
	 * setup free node list
	 */
	priv_p->p_free_ptr = &priv_p->p_free_head1;
	priv_p->p_to_free_ptr = &priv_p->p_free_head2;
	INIT_LIST_HEAD(priv_p->p_free_ptr);
	INIT_LIST_HEAD(priv_p->p_to_free_ptr);
	rn_p = priv_p->p_req_nodes;
	for (i = 0; i < NID_MAX_REQNODES; i++, rn_p++) {
		list_add_tail(&rn_p->r_list, priv_p->p_free_ptr);
	}

	/*
	 * setup request list
	 */
	INIT_LIST_HEAD(&priv_p->p_req_head);
	INIT_LIST_HEAD(&priv_p->p_to_req_head);

	/*
	 * setup nodata request list
	 */
	INIT_LIST_HEAD(&priv_p->p_read_head);
	INIT_LIST_HEAD(&priv_p->p_to_read_head);
	INIT_LIST_HEAD(&priv_p->p_write_head);
	INIT_LIST_HEAD(&priv_p->p_to_write_head);
	INIT_LIST_HEAD(&priv_p->p_trim_head);
	INIT_LIST_HEAD(&priv_p->p_to_trim_head);

	/*
	 * setup rw
	 */
	if (rwg_p)
		priv_p->p_rw = rwg_p->rwg_op->rwg_search(rwg_p, rw_name);
	assert(priv_p->p_rw);

	rw_p = priv_p->p_rw;

	new_rc = 0;
	if (priv_p->p_rc) {
		rc_p = priv_p->p_rc;
		rc_info.rc_rw = priv_p->p_rw;
		priv_p->p_rc_handle =  rc_p->rc_op->rc_create_channel(rc_p, sac_p,
			&rc_info, priv_p->p_uuid, &new_rc);
	}

	new_wc = 0;
	if (priv_p->p_wc) {
		wc_p = priv_p->p_wc;
		wc_info.wc_rc = priv_p->p_rc;
		wc_info.wc_rc_handle = priv_p->p_rc_handle;
		wc_info.wc_rw = priv_p->p_rw;
		wc_info.wc_rw_exportname = priv_p->p_exportname;
		wc_info.wc_rw_sync = priv_p->p_sync;
		wc_info.wc_rw_direct_io = priv_p->p_direct_io;
		wc_info.wc_rw_alignment = priv_p->p_alignment;
		wc_info.wc_rw_dev_size = priv_p->p_dev_size;
		priv_p->p_wc_handle = wc_p->wc_op->wc_create_channel(wc_p, sac_p,
			&wc_info, priv_p->p_uuid, &new_wc);
		priv_p->p_io_type = IO_TYPE_BUFFER;
	} else {
		priv_p->p_io_type = IO_TYPE_RESOURCE;
	}

	if (priv_p->p_io_type == IO_TYPE_RESOURCE) {
		/* Only RIO need create rw_handle */
		priv_p->p_rw_handle = rw_p->rw_op->rw_create_worker(rw_p, priv_p->p_exportname,
			priv_p->p_sync, priv_p->p_direct_io, priv_p->p_alignment, priv_p->p_dev_size);
		if (!priv_p->p_rw_handle) {
			nid_log_error("%s: cannot create rw worker", log_header);
			rc = -1;
			goto out;
		}
	}

	priv_p->p_io = priv_p->p_all_io[priv_p->p_io_type];
	assert(priv_p->p_io);
	priv_p->p_stat.sa_io_type = priv_p->p_io_type;

	io_p = priv_p->p_io;
	io_info.io_rw = priv_p->p_rw;
	io_info.io_rw_handle = priv_p->p_rw_handle;
	io_info.io_wc = priv_p->p_wc;
	io_info.io_wc_handle = priv_p->p_wc_handle;
	io_info.io_rc = priv_p->p_rc;
	io_info.io_rc_handle = priv_p->p_rc_handle;
	io_info.io_sdsg = sdsg_p;
	io_info.io_cdsg = cdsg_p;
	io_info.io_ds_name = ds_name;
	priv_p->p_io_handle = io_p->io_op->io_create_worker(io_p, sac_p,
		&io_info, priv_p->p_exportname, &new_io);
	if (!priv_p->p_io_handle) {
		nid_log_error("%s: cannot create io worker", log_header);
		return -1;
	}

	switch (priv_p->p_io_type) {
	case IO_TYPE_RESOURCE:
		do_buffer = 0;
		break;
	case IO_TYPE_BUFFER:
		do_buffer = 1;
		break;
	default:
		nid_log_error("%s: wrong io type", log_header);
		assert(0);
	}

	ds_p = priv_p->p_ds;
	nid_log_warning("%s: sac:%p create ds worker ds_p is: %p", log_header, sac_p, ds_p);
	priv_p->p_in_ds = ds_p->d_op->d_create_worker(ds_p, priv_p->p_io_handle, do_buffer, io_p);
	if (priv_p->p_in_ds < 0) {
		nid_log_error("%s: cannot create ds in worker", log_header);
		rc = -1;
		goto out;
	}
	nid_log_warning("%s: sac:%p create ds worker ds_p is: %p", log_header, sac_p, ds_p);
	priv_p->p_out_ds = ds_p->d_op->d_create_worker(ds_p, priv_p->p_io_handle, 0, io_p);
	if (priv_p->p_out_ds < 0) {
		nid_log_error("%s: cannot create ds out worker", log_header);
		rc = -1;
		goto out;
	}
	priv_p->p_ds_pagesz = ds_p->d_op->d_get_pagesz(ds_p, priv_p->p_out_ds);

	/*
	 * setup response node list
	 */
	priv_p->p_resp_ptr = &priv_p->p_resp_head1;
	priv_p->p_to_resp_ptr = &priv_p->p_resp_head2;
	INIT_LIST_HEAD(priv_p->p_resp_ptr);
	INIT_LIST_HEAD(priv_p->p_to_resp_ptr);
	priv_p->p_dresp_ptr = &priv_p->p_dresp_head1;
	priv_p->p_to_dresp_ptr = &priv_p->p_dresp_head2;
	INIT_LIST_HEAD(priv_p->p_dresp_ptr);
	INIT_LIST_HEAD(priv_p->p_to_dresp_ptr);

	/*
	 * init locks
	 */
	pthread_mutex_init(&priv_p->p_free_lck, NULL);
	pthread_mutex_init(&priv_p->p_input_lck, NULL);
	pthread_mutex_init(&priv_p->p_req_lck, NULL);
	pthread_mutex_init(&priv_p->p_resp_lck, NULL);
	pthread_mutex_init(&priv_p->p_dresp_lck, NULL);
	pthread_cond_init(&priv_p->p_resp_cond, NULL);
	pthread_cond_init(&priv_p->p_dresp_cond, NULL);

	/*
	 * init stat
	 */
	priv_p->p_stat.sa_uuid = priv_p->p_uuid;
	priv_p->p_stat.sa_alevel = priv_p->p_alevel;
	priv_p->p_stat.sa_ipaddr = priv_p->p_ipaddr;

	/*
	 * Now , the channel has been established
	 */
	priv_p->p_chan_established = 1;
	nid_log_warning("%s: %s, %s, rsfd:%d, dsfd:%d",
		log_header, priv_p->p_uuid, priv_p->p_ipaddr, priv_p->p_rsfd, priv_p->p_dsfd);


out:
	nid_log_warning("%s: uuid:%s, rc:%d", log_header, priv_p->p_uuid, rc);
	if (!priv_p->p_chan_established)
		priv_p->p_to_stop = 1;
	return rc;
}


static int
__response_doing_well_checking(struct sac_private *priv_p)
{
	char *log_header = "__response_doing_well_checking";
	struct sac_stat *stat_p = &priv_p->p_stat;
	int rc = 0;

	if (stat_p->sa_read_last_resp_counter < stat_p->sa_read_resp_counter) {
		/* there are some read responses from last time */
		stat_p->sa_read_last_resp_counter = stat_p->sa_read_resp_counter;
		stat_p->sa_read_stuck_counter = 0;
	} else {
		if (stat_p->sa_read_resp_counter == stat_p->sa_read_counter) {
			/* all read request got responsed */
			stat_p->sa_read_stuck_counter = 0;
		} else {
			/* got stuck */
			stat_p->sa_read_stuck_counter++;
			if (stat_p->sa_read_stuck_counter > priv_p->p_max_read_stuck)
				rc = -1;
		}
	}

	if (stat_p->sa_write_last_resp_counter < stat_p->sa_write_resp_counter) {
		/* there are some write responses from last time */
		stat_p->sa_write_last_resp_counter = stat_p->sa_write_resp_counter;
		stat_p->sa_write_stuck_counter = 0;
	} else {
		if (stat_p->sa_write_resp_counter == stat_p->sa_write_counter) {
			/* all write request got responsed */
			stat_p->sa_write_stuck_counter = 0;
		} else {
			/* got stuck */
			stat_p->sa_write_stuck_counter++;
			if (stat_p->sa_write_stuck_counter > priv_p->p_max_write_stuck)
				rc = -1;
		}
	}

	if (rc) {
		nid_log_error("%s: rcounter:%lu, rreadycounter:%lu, rrcounter:%lu, rscounter:%u,"
			"wcounter:%lu, rwcounter:%lu, wscounter:%u",
			log_header,
			stat_p->sa_read_counter,
			stat_p->sa_read_ready_counter,
			stat_p->sa_read_resp_counter,
			stat_p->sa_read_stuck_counter,
			stat_p->sa_write_counter,
			stat_p->sa_write_resp_counter,
			stat_p->sa_write_stuck_counter);
	}

	return rc;
}



static void
__kill_myself()
{
	char *log_header = "__kill_myself";
	int fd,rc;
	char *op = "b";
//	nid_log_error("%s: start ...", log_header);
	//system("echo 'b' > /proc/sysrq-trigger");
	fd = open("/proc/sys/vm/drop_caches", O_RDWR);
	if (fd < 0)
		nid_log_error("%s: open file failed", log_header);
	else
		rc = write(fd, "3", 1);
	if (rc < 0) {
		nid_log_error("%s: fail to write, errno: %d",log_header, errno);
	}

	fd = open("/proc/sysrq-trigger",O_RDWR);
	if (fd < 0)
		nid_log_error("%s: open file failed", log_header);
	rc = write(fd, op, 1);
	if (rc < 0)
		nid_log_error("%s: fail to write, errno: %d",log_header, errno);
}

/*
 * Algorithm:
 * 	Response both header and data through request and data
 * 	connections respectively
 */
static void *
_do_response_job_thread(void *arg)
{
	char *log_header = "_do_response_job_thread";
	struct sac_interface *sac_p = (struct sac_interface *)arg;
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct io_interface *io_p = priv_p->p_io;
	struct sac_stat *stat_p = &priv_p->p_stat;
	struct ds_interface *ds_p = priv_p->p_ds;
	int in_ds_index = priv_p->p_in_ds;
	struct request_node *rn_p, *rn_p1;
	struct nid_request *ir_p;
	int nwrite, error_quit = 0;
	uint32_t resp_counter = 0;
	struct timeval now;
	struct timespec ts;
	suseconds_t time_consume, time_consume2, time_consume3, time_consume4, time_consume5;
	struct mqtt_interface *mqtt_p = priv_p->p_mqtt;
	struct mqtt_message msg;

	nid_log_notice("%s: start ...", log_header);
next_node:
	resp_counter = 0;
	if (priv_p->p_to_stop && !priv_p->p_do_req_counter) {
		/*
		 * after p_req_done, p_do_req_counter cannot be increased
		 * so we are safe to leave now
		 */
		goto out;
	} else if (priv_p->p_to_stop) {
		nid_log_notice("_do_response_job_thread: to stop, p_do_req_counter:%lu, uuid:%s",
			priv_p->p_do_req_counter, priv_p->p_uuid);
	}

	if (list_empty(priv_p->p_resp_ptr)) {
		struct list_head *tmp_list;
		pthread_mutex_lock(&priv_p->p_resp_lck);
		while (list_empty(priv_p->p_to_resp_ptr)) {
			priv_p->p_resp_busy = 0;
			//nid_log_debug("_do_response_job_thread: before pthread_cond_wait,"
			//	"p_do_req_counter:%lu, uuid:%s",
			//	priv_p->p_do_req_counter, priv_p->p_uuid);

			gettimeofday(&now, NULL);
			ts.tv_sec = now.tv_sec + 1;
			ts.tv_nsec = now.tv_usec * 1000;
			pthread_cond_timedwait(&priv_p->p_resp_cond, &priv_p->p_resp_lck, &ts);

			//nid_log_debug("_do_response_job_thread: after pthread_cond_wait,"
			//	"p_do_req_counter:%lu, uuid:%s",
			//	priv_p->p_do_req_counter, priv_p->p_uuid);
			priv_p->p_resp_busy = 1;
			if (priv_p->p_to_stop) {
				pthread_mutex_unlock(&priv_p->p_resp_lck);
				goto next_node;
			}
		}
		tmp_list = priv_p->p_resp_ptr;
		priv_p->p_resp_ptr = priv_p->p_to_resp_ptr;
		priv_p->p_to_resp_ptr = tmp_list;
		pthread_mutex_unlock(&priv_p->p_resp_lck);
		goto next_node;
	}

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, priv_p->p_resp_ptr, r_list) {
		if (error_quit) {
			switch (rn_p->r_ir.cmd) {
			case NID_REQ_WRITE:
				ds_p->d_op->d_put_buffer(ds_p, in_ds_index, rn_p->r_seq, rn_p->r_len, io_p);
				break;

			default:
				break;
			}

			resp_counter++;
			continue;
		}

		gettimeofday(&rn_p->r_resp_time, NULL);
		time_consume = time_difference(rn_p->r_recv_time, rn_p->r_resp_time);

		switch (rn_p->r_ir.cmd) {
		case NID_REQ_READ:
			/*
			 * delay the response in some special scenrio,
			 * e.x. when the channel is preparing for bwc merging
			 */
			if (rn_p->r_resp_delay)
				usleep(((useconds_t)(rn_p->r_resp_delay)) * 100000);

			if (time_consume > TIME_CONSUME_ALERT) {
				time_consume2 = time_difference(rn_p->r_ready_time, rn_p->r_resp_time);
				time_consume3 = time_difference(rn_p->r_ready_time, rn_p->r_io_start_time);
				time_consume4 = time_difference(rn_p->r_io_start_time, rn_p->r_io_end_time);
				time_consume5 = time_difference(rn_p->r_io_end_time, rn_p->r_resp_time);
				nid_log_info("%s: one read consumed %lu(us), internal %lu(us), pre_io %lu, the io %lu(us), post_io:%lu",
					log_header, time_consume, time_consume2, time_consume3, time_consume4, time_consume5);
				if (!(priv_p->p_slowio_read_counter % 100)){
					msg.type = "nidserver";
					msg.module = "NIDIOSlowStorage";
					msg.message = "Extreme slow IO request from the underlying disk";
					msg.status = "WARN";
					strcpy (msg.uuid, priv_p->p_uuid);
					strcpy (msg.ip, priv_p->p_ipaddr);
					mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
				}
				priv_p->p_slowio_read_counter++;
			} else {
				priv_p->p_slowio_read_counter = 0;
			}

			ir_p = &rn_p->r_ir;
			nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
			if (nwrite != NID_SIZE_REQHDR) {
				resp_counter++;
				nid_log_error("_do_response_job_thread: write error, nwrite:%d, errno:%d, uuid:%s",
					nwrite, errno, priv_p->p_uuid);
				error_quit = 1;
				continue;
			}

			list_del(&rn_p->r_list);
			pthread_mutex_lock(&priv_p->p_dresp_lck);
			list_add_tail(&rn_p->r_list, priv_p->p_to_dresp_ptr);
			if (!priv_p->p_dresp_busy)
				pthread_cond_signal(&priv_p->p_dresp_cond);
			pthread_mutex_unlock(&priv_p->p_dresp_lck);
			break;

		case NID_REQ_WRITE:
			/*
			 * delay the response in some special scenrio,
			 * e.x. when the channel is preparing for bwc merging
			 */
			if (rn_p->r_resp_delay)
				usleep(((useconds_t)(rn_p->r_resp_delay)) * 100000);

			if (time_consume > TIME_CONSUME_ALERT) {
				time_consume2 = time_difference(rn_p->r_ready_time, rn_p->r_resp_time);
				time_consume3 = time_difference(rn_p->r_ready_time, rn_p->r_io_start_time);
				time_consume4 = time_difference(rn_p->r_io_start_time, rn_p->r_io_end_time);
				time_consume5 = time_difference(rn_p->r_io_end_time, rn_p->r_resp_time);
				nid_log_info("%s: one write consumed %lu(us), internal %lu(us), pre_io %lu, the io %lu(us), post_io:%lu",
					log_header, time_consume, time_consume2, time_consume3, time_consume4, time_consume5);
				if (!(priv_p->p_slowio_write_counter % 100)){
					msg.type = "nidserver";
					msg.module = "NIDIOSlowStorage";
					msg.message = "Extreme slow IO request to the underlying disk";
					msg.status = "WARN";
					strcpy (msg.uuid, priv_p->p_uuid);
					strcpy (msg.ip, priv_p->p_ipaddr);
					mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
				}
				priv_p->p_slowio_write_counter++;
			} else {
				priv_p->p_slowio_write_counter = 0;
			}

			resp_counter++;
			ir_p = &rn_p->r_ir;
			nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
			ds_p->d_op->d_put_buffer(ds_p, in_ds_index, rn_p->r_seq, rn_p->r_len, io_p);
			stat_p->sa_write_resp_counter++;
			if (nwrite != NID_SIZE_REQHDR) {
				nid_log_error("_do_response_job_thread: write error, nwrite:%d, errno:%d, uuid:%s",
					nwrite, errno, priv_p->p_uuid);
				error_quit = 1;
				continue;
			}
			break;

		case NID_REQ_TRIM:
			resp_counter++;
			ir_p = &rn_p->r_ir;
			nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
			stat_p->sa_trim_resp_counter++;
			if (nwrite != NID_SIZE_REQHDR) {
				nid_log_error("_do_response_job_thread: write error, nwrite:%d, errno:%d, uuid:%s",
					nwrite, errno, priv_p->p_uuid);
				error_quit = 1;
				continue;
			}
			break;

		case NID_REQ_KEEPALIVE:
			nid_log_debug("_do_response_job_thread: send keepalive, uuid:%s", priv_p->p_uuid);
			if (priv_p->p_enable_kill_myself && __response_doing_well_checking(priv_p)) {
				/* something wrong, we cannot handle it */
				__kill_myself();
			}

			resp_counter++;
			ir_p = &rn_p->r_ir;
			ir_p->dseq = htobe64(stat_p->sa_recv_sequence);
			ir_p->offset = htobe64(stat_p->sa_wait_sequence);
			ir_p->len = htobe32( priv_p->p_where);
			if (priv_p->p_int_keep_alive) {
				if (priv_p->p_stop_keep_live || priv_p->p_int_keep_alive_cur_num++ == priv_p->p_int_keep_alive_break_num) {
					nwrite = 0;
					priv_p->p_stop_keep_live = 1;
					priv_p->p_int_keep_alive_break_num += priv_p->p_int_keep_alive_interval;
				} else {
					nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
				}
			} else {
				nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
			}
			stat_p->sa_keepalive_resp_counter++;
			if (nwrite != NID_SIZE_REQHDR) {
				nid_log_error("_do_response_job_thread: write error, nwrite:%d, errno:%d, uuid:%s",
					nwrite, errno, priv_p->p_uuid);
				error_quit = 1;
				continue;
			}
			break;

		case NID_REQ_UPGRADE:
			resp_counter++;
			ir_p = &rn_p->r_ir;
			nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
			if (nwrite != NID_SIZE_REQHDR) {
				nid_log_error("_do_response_job_thread: write error, nwrite:%d, errno:%d, uuid:%s",
					nwrite, errno, priv_p->p_uuid);
				error_quit = 1;
				continue;
			}
			break;

		case NID_REQ_FUPGRADE:
			resp_counter++;
			ir_p = &rn_p->r_ir;
			nwrite = write(priv_p->p_rsfd, ir_p, NID_SIZE_REQHDR);
			if (nwrite != NID_SIZE_REQHDR) {
				nid_log_error("_do_response_job_thread: write error, nwrite:%d, errno:%d, uuid:%s",
					nwrite, errno, priv_p->p_uuid);
				error_quit = 1;
				continue;
			}
			break;

		default:
			nid_log_error("_do_response_job_thread: got unknown request, uuid:%s", priv_p->p_uuid);
			break;
		}
	}

	pthread_mutex_lock(&priv_p->p_free_lck);
	if (!list_empty(priv_p->p_resp_ptr))
		list_splice_tail_init(priv_p->p_resp_ptr, priv_p->p_to_free_ptr);
	pthread_mutex_unlock(&priv_p->p_free_lck);

	pthread_mutex_lock(&priv_p->p_req_lck);
	assert(priv_p->p_do_req_counter >= resp_counter);
	priv_p->p_do_req_counter -= resp_counter;
	pthread_mutex_unlock(&priv_p->p_req_lck);

	if (error_quit)
		priv_p->p_to_stop = 1;
	goto next_node;

out:
	nid_log_warning("_do_response_job_thread: end, uuid:%s (rsfd:%d, dsfd:%d)",
		priv_p->p_uuid, priv_p->p_rsfd, priv_p->p_dsfd);
	close(priv_p->p_rsfd);
	priv_p->p_rsfd = -1;
	pthread_mutex_lock(&priv_p->p_dresp_lck);
	if (!priv_p->p_dresp_busy)
		pthread_cond_signal(&priv_p->p_dresp_cond);
	pthread_mutex_unlock(&priv_p->p_dresp_lck);
	priv_p->p_resp_done = 1;
	nid_log_warning("_do_response_job_thread: done, uuid:%s", priv_p->p_uuid);
	return NULL;
}

/*
 * Algorithm:
 * 	Response both header and data through request and data
 * 	connections respectively
 */
static void *
_do_dresponse_job_thread(void *arg)
{
	char *log_header = "_do_dresponse_job_thread";
	struct sac_interface *sac_p = (struct sac_interface *)arg;
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct io_interface *io_p = priv_p->p_io;
	struct sac_stat *stat_p = &priv_p->p_stat;
	struct ds_interface *ds_p = priv_p->p_ds;
	int out_ds_index = priv_p->p_out_ds;
	struct request_node *rn_p, *rn_p1;
	int error_quit = 0;
	uint32_t dresp_counter = 0;
	struct timeval now;
	struct timespec ts;

	nid_log_debug("_do_dresponse_job_thread: start");
next_node:
	dresp_counter = 0;
	if (priv_p->p_to_stop && !priv_p->p_do_req_counter) {
		/*
		 * after p_req_done, p_do_req_counter cannot be increased
		 * so we are safe to leave now
		 */
		goto out;
	} else if (priv_p->p_to_stop) {
		nid_log_notice("_do_dresponse_job_thread: to stop, p_do_req_counter:%lu, uuid:%s",
			priv_p->p_do_req_counter, priv_p->p_uuid);
	}

	if (list_empty(priv_p->p_dresp_ptr)) {
		struct list_head *tmp_list;
		pthread_mutex_lock(&priv_p->p_dresp_lck);
		while (list_empty(priv_p->p_to_dresp_ptr)) {
			priv_p->p_dresp_busy = 0;
			gettimeofday(&now, NULL);
			ts.tv_sec = now.tv_sec + 1;
			ts.tv_nsec = now.tv_usec * 1000;
			pthread_cond_timedwait(&priv_p->p_dresp_cond, &priv_p->p_dresp_lck, &ts);
			priv_p->p_dresp_busy = 1;
			if (priv_p->p_to_stop) {
				pthread_mutex_unlock(&priv_p->p_dresp_lck);
				goto next_node;
			}
		}
		tmp_list = priv_p->p_dresp_ptr;
		priv_p->p_dresp_ptr = priv_p->p_to_dresp_ptr;
		priv_p->p_to_dresp_ptr = tmp_list;
		pthread_mutex_unlock(&priv_p->p_dresp_lck);
		goto next_node;
	}

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, priv_p->p_dresp_ptr, r_list) {
		switch (rn_p->r_ir.cmd) {
		case NID_REQ_READ:
			dresp_counter++;
			if (!error_quit && util_nw_write_timeout_stop(priv_p->p_dsfd, rn_p->r_resp_buf_1, rn_p->r_resp_len_1, 10, &priv_p->p_to_stop)) {
				nid_log_error("%s: util_nw_write_timeout_stop error, uuid:%s", log_header, priv_p->p_uuid);
				error_quit = 1;
			}

			if ((rn_p->r_len > rn_p->r_resp_len_1) && !error_quit) {
				if (util_nw_write_timeout_stop(priv_p->p_dsfd, rn_p->r_resp_buf_2, rn_p->r_len - rn_p->r_resp_len_1, 10, &priv_p->p_to_stop)) {
					nid_log_error("%s: util_nw_write_timeout_stop error 2, uuid:%s", log_header, priv_p->p_uuid);
					error_quit = 1;
				}
			}

			ds_p->d_op->d_put_buffer(ds_p, out_ds_index, rn_p->r_seq, rn_p->r_len, io_p);
			stat_p->sa_read_resp_counter++;
			break;

		default:
			nid_log_error("_do_dresponse_job_thread: got unknown request, uuid:%s", priv_p->p_uuid);
			assert(0);
			break;
		}
	}

	pthread_mutex_lock(&priv_p->p_free_lck);
	list_splice_tail_init(priv_p->p_dresp_ptr, priv_p->p_to_free_ptr);
	pthread_mutex_unlock(&priv_p->p_free_lck);

	pthread_mutex_lock(&priv_p->p_req_lck);
	assert(priv_p->p_do_req_counter >= dresp_counter);
	priv_p->p_do_req_counter -= dresp_counter;
	pthread_mutex_unlock(&priv_p->p_req_lck);

	if (error_quit)
		priv_p->p_to_stop = 1;
	goto next_node;

out:
	nid_log_warning("_do_dresponse_job_thread: end, uuid:%s (rsfd:%d, dsfd:%d)",
		priv_p->p_uuid, priv_p->p_rsfd, priv_p->p_dsfd);
	close(priv_p->p_dsfd);
	priv_p->p_dsfd = -1;
	pthread_mutex_lock(&priv_p->p_resp_lck);
	if (!priv_p->p_resp_busy)
		pthread_cond_signal(&priv_p->p_resp_cond);
	pthread_mutex_unlock(&priv_p->p_resp_lck);
	priv_p->p_dresp_done = 1;
	nid_log_warning("_do_dresponse_job_thread: done, uuid:%s", priv_p->p_uuid);
	return NULL;
}

/*
 * the thread receiving data from the client
 */
static void *
_do_data_connection_thread(void *arg)
{
	struct sac_interface *sac_p = (struct sac_interface *)arg;
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct io_interface *io_p = priv_p->p_io;
	struct ds_interface *ds_p = priv_p->p_ds;
	int in_ds_index = priv_p->p_in_ds;
	char *p;
	int nread;
	uint32_t to_read;

	nid_log_debug("_do_data_connection_thread start ...");
next_read:
	if (priv_p->p_to_stop) {
		goto out;
	}

	priv_p->p_where = SAC_WHERE_BUSY;
	p = ds_p->d_op->d_get_buffer(ds_p, in_ds_index, &to_read, io_p);
	priv_p->p_where = SAC_WHERE_FREE;
	if (!p || priv_p->p_to_stop)
		goto next_read;
	nread = util_nw_read_stop(priv_p->p_dsfd, p, to_read, 0, &priv_p->p_to_stop);
	if (nread <= 0) {
		nid_log_error("_do_data_connection_thread: util_nw_read_stop error,"
			"(uuid:%s): p:%p, to_read:%d, read ret:%d, errno:%d",
			priv_p->p_uuid, p, to_read, nread, errno);
		sac_p->sa_op->sa_stop(sac_p);
		priv_p->p_to_stop = 1;
		goto out;
	}
	assert((uint32_t)nread <= to_read);
	ds_p->d_op->d_confirm(ds_p, in_ds_index, nread);
	stat_p->sa_recv_sequence += nread;
	sacg_p->sag_op->sag_request_coming(sacg_p);
	goto next_read;

out:
	nid_log_debug("_do_data_connection_thread: end, uuid:%s", priv_p->p_uuid);
	priv_p->p_data_done = 1;

	return NULL;
}

/*
 * Algorithm:
 * 	start data stream thread, response thread and read request stream
 */
static void
sac_do_channel(struct sac_interface *sac_p)
{
	char *log_header = "sac_do_channel";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct io_interface *io_p = priv_p->p_io;
	struct rc_interface *rc_p = priv_p->p_rc;
	struct sac_stat *stat_p = &priv_p->p_stat;
	pthread_attr_t attr;
	pthread_t thread_id;
	struct request_stream *rs_p = &priv_p->p_req_stream;
	struct ds_interface *ds_p = priv_p->p_ds;
	struct request_node *rn_p;
	struct nid_request *ir_p;
	struct mqtt_interface *mqtt_p = priv_p->p_mqtt;
	struct mqtt_message msg;
	char *p;
	int counter, nread, to_read, rc;
	uint64_t read_len;
	struct timeval wait_exit_start, wait_exit_now;
	time_t wait_exit_consume;

	nid_log_notice("%s: start ...", log_header);
	priv_p->p_alevel = NID_ALEVEL_RDONLY;
	stat_p->sa_alevel = priv_p->p_alevel;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _do_data_connection_thread, (void *)sac_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _do_response_job_thread, (void *)sac_p);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, _do_dresponse_job_thread, (void *)sac_p);

	/*
	 * read request stream
	 */
read_next:
	if (priv_p->p_to_stop) {
		nid_log_notice("%s: got to_stop", log_header);
		goto out;
	}

	p = rs_p->r_buf + rs_p->r_curr_pos;
	/*
	 * if unprocessed part is less than a request header, read more
	 */
	if (rs_p->r_start + NID_SIZE_REQHDR > rs_p->r_curr_pos) {
		if (rs_p->r_curr_pos >= rs_p->r_size) {
			rs_p->r_start = 0;
			rs_p->r_curr_pos = 0;
			p = rs_p->r_buf + rs_p->r_curr_pos;
		}
		to_read = rs_p->r_size - rs_p->r_curr_pos;
		nread = util_nw_read_stop(priv_p->p_rsfd, p, to_read, SOCKET_READ_TIMER, &priv_p->p_to_stop);
		if (nread <= 0) {
			nid_log_error("%s: util_nw_read_stop error,"
				"(uuid:%s): p:%p, to_read:%d, read ret:%d, errno:%d",
				log_header, priv_p->p_uuid, p, to_read, nread, errno);
			sac_p->sa_op->sa_stop(sac_p);
			goto out;
		}
		rs_p->r_curr_pos += nread;
		p += nread;
	}

	counter = 0;
	read_len = 0;
	while (rs_p->r_start + NID_SIZE_REQHDR <= rs_p->r_curr_pos) {
		/*
		 * first of all, we need a request node to handle this request
		 */
		while (list_empty(priv_p->p_free_ptr)) {
			struct list_head *tmp_list;
			pthread_mutex_lock(&priv_p->p_free_lck);
			tmp_list = priv_p->p_free_ptr;
			priv_p->p_free_ptr = priv_p->p_to_free_ptr;
		     	priv_p->p_to_free_ptr = tmp_list;
			pthread_mutex_unlock(&priv_p->p_free_lck);
			if (list_empty(priv_p->p_free_ptr)) {
				/*
				 * NID_MAX_REQNODES is defined big enough that the nid driver should never
				 * send out requests more than it
				 */
				nid_log_warning("%s: out of free node, this shouldn't happen at all, to stop ...", log_header);
				sac_p->sa_op->sa_stop(sac_p);
				goto read_next;
			}
		}

		/*
		 * Now we are sure there is at least one free request node
		 */
		rn_p = list_first_entry(priv_p->p_free_ptr, struct request_node, r_list);
		list_del(&rn_p->r_list);
		rn_p->r_sac = sac_p;
		rn_p->r_ds = ds_p;
		ir_p = (struct nid_request *)(rs_p->r_buf + rs_p->r_start);
		memcpy(&rn_p->r_ir, ir_p, NID_SIZE_REQHDR);
		rn_p->r_len = ntohl(ir_p->len);
		rn_p->r_offset = be64toh(ir_p->offset);
		gettimeofday(&rn_p->r_recv_time, NULL);

		switch (rn_p->r_ir.cmd) {
		case NID_REQ_READ:
			rc = __do_channel_read(sac_p, rn_p, ir_p);
			if (rc < 0)
				goto read_next;
			read_len += rn_p->r_len;
			break;

		case NID_REQ_WRITE:
			rc = __do_channel_write(sac_p, rn_p, ir_p);
			if (rc < 0)
				goto read_next;
			break;

		case NID_REQ_TRIM:
			rc = __do_channel_trim(sac_p, rn_p, ir_p);
			if (rc < 0)
				goto read_next;
			break;

		case NID_REQ_KEEPALIVE:
			__do_channel_keepalive(sac_p, rn_p, ir_p);
			break;

		case NID_REQ_UPGRADE:
			__do_channel_upgrade(sac_p, rn_p, ir_p);
			break;

		case NID_REQ_FUPGRADE:
			__do_channel_fupgrade(sac_p, rn_p, ir_p);
			break;

		case NID_REQ_DELETE_DEVICE:
			__do_channel_delete_device(sac_p, rn_p, ir_p);
			break;

		default:
			nid_log_warning("%s: got unknown command (%d), to stop ...", log_header, rn_p->r_ir.cmd);
			sac_p->sa_op->sa_stop(sac_p);
			goto out;
		}
		rs_p->r_start += NID_SIZE_REQHDR;	// next request
		++counter;

		if (read_len >= priv_p->p_ds_pagesz)
			break;
	}

	if (counter) {
		pthread_mutex_lock(&priv_p->p_input_lck);
		list_splice_tail_init(&priv_p->p_to_req_head, &priv_p->p_req_head);
		list_splice_tail_init(&priv_p->p_to_read_head, &priv_p->p_read_head);
		list_splice_tail_init(&priv_p->p_to_write_head, &priv_p->p_write_head);
		list_splice_tail_init(&priv_p->p_to_trim_head, &priv_p->p_trim_head);
		sacg_p->sag_op->sag_request_coming(sacg_p);
		pthread_mutex_unlock(&priv_p->p_input_lck);
	}
	goto read_next;

out:
	/* I want to leave */
	priv_p->p_req_done = 1;
	pthread_mutex_lock(&priv_p->p_resp_lck);
	if (!priv_p->p_resp_busy) {
		pthread_cond_signal(&priv_p->p_resp_cond);
	}
	pthread_mutex_unlock(&priv_p->p_resp_lck);

	gettimeofday(&wait_exit_start, NULL);

	/* wait the data thread done and response thread done */
	while (!priv_p->p_data_done || !priv_p->p_resp_done || !priv_p->p_dresp_done) {
		nid_log_warning("%s: waiting to stop,"
			"uuid:%s, p_data_done:%d,"
			"p_resp_done:%d, p_dresp_done:%d, p_do_req_counter:%lu",
			log_header,
			priv_p->p_uuid,	priv_p->p_data_done?1:0,
			priv_p->p_resp_done?1:0, priv_p->p_dresp_done?1:0,
			priv_p->p_do_req_counter);
		gettimeofday(&wait_exit_now, NULL);
		wait_exit_consume = time_difference_sec(wait_exit_start, wait_exit_now);
		if (wait_exit_consume < TIME_WAIT_CONSUME_ALERT){
			nid_log_warning("%s: Hang more than half hour for channel exit, kill nidserver and gerate dump",log_header);
			msg.type = "nidserver";
			msg.module = "NIDConnection";
			msg.message = "An NID connection hang";
			msg.status = "ERROR";
			strcpy (msg.uuid, priv_p->p_uuid);
			strcpy (msg.ip, priv_p->p_ipaddr);
			mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
		}
		sleep(1);
	}

	if (priv_p->p_alevel >= NID_ALEVEL_RDWR) {
		msg.type = "nidserver";
		msg.module = "NIDConnection";
		msg.message = "An NID connection disconnected";
		msg.status = "ERROR";
		strcpy (msg.uuid, priv_p->p_uuid);
		strcpy (msg.ip, priv_p->p_ipaddr);
		mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
	}

	if (priv_p->p_in_ds >= 0) {
		ds_p->d_op->d_release_worker(ds_p, priv_p->p_in_ds, io_p);
		priv_p->p_in_ds = -1;
	}
	if (priv_p->p_out_ds >= 0) {
		ds_p->d_op->d_release_worker(ds_p, priv_p->p_out_ds, io_p);
		priv_p->p_out_ds = -1;
	}
	if (rc_p) {
		rc_p->rc_op->rc_chan_inactive_res(rc_p, priv_p->p_uuid);
	}
	if (priv_p->p_io_handle) {
		io_p->io_op->io_close(io_p, priv_p->p_io_handle);
		priv_p->p_io_handle = NULL;
	}

	nid_log_debug("%s: end, uuid:%s", log_header, priv_p->p_uuid);

	return;
}

static struct request_node*
sac_pickup_request(struct sac_interface *sac_p)
{
	char *log_header = "sac_pickup_request";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;
	struct io_interface *io_p = priv_p->p_io;
	struct ds_interface *ds_p = priv_p->p_ds;
	int in_ds_index = priv_p->p_in_ds;
	int out_ds_index = priv_p->p_out_ds;
	struct request_node *rn_p = NULL;
	struct list_head read_head, write_head, trim_head;
	int write_counter = 0, read_counter = 0, trim_counter = 0;
	uint8_t cutoff;

	/* quick checking */
	if (priv_p->p_to_stop || list_empty(&priv_p->p_req_head))
		return NULL;

	pthread_mutex_lock(&priv_p->p_req_lck);
	priv_p->p_do_req_counter++;
	pthread_mutex_unlock(&priv_p->p_req_lck);

	pthread_mutex_lock(&priv_p->p_input_lck);
	/*
	 * Need to check p_to_stop again!!!
	 * _do_response_job_thread thread may got exist before we increase p_do_req_counter
	 */
	if (priv_p->p_to_stop || list_empty(&priv_p->p_req_head))
		goto out;

	/*
	 * the reponse thread (_do_response_job_thread) should exist before process this request
	 */
	rn_p = list_first_entry(&priv_p->p_req_head, struct request_node, r_list);
retry:
	switch (rn_p->r_ir.cmd) {
	case NID_REQ_READ:
		switch (priv_p->p_io_type) {
		case IO_TYPE_RESOURCE:
			list_del(&rn_p->r_list);
			list_del(&rn_p->r_read_list);
			break;

		case IO_TYPE_BUFFER:
			/*
			 * cut off read IO for bwc merging.
			 */
			if(priv_p->p_wc_type == WC_TYPE_NONE_MEMORY) {
				cutoff = io_p->io_op->io_get_cutoff(io_p, priv_p->p_io_handle);
				if (cutoff)
					break;
			}

			INIT_LIST_HEAD(&read_head);
			while (rn_p) {
				read_counter++;
				list_del(&rn_p->r_list);
				list_del(&rn_p->r_read_list);
				list_add_tail(&rn_p->r_read_list, &read_head);
				rn_p->r_ds_index = out_ds_index;
				rn_p->r_io_handle = priv_p->p_wc_handle;
				/* Next read, if there is */
				if (!list_empty(&priv_p->p_read_head))
					rn_p = list_first_entry(&priv_p->p_read_head, struct request_node, r_read_list);
				else
					rn_p = NULL;
			}
			if (read_counter > 0) {
				pthread_mutex_lock(&priv_p->p_req_lck);
				priv_p->p_do_req_counter += read_counter;
				pthread_mutex_unlock(&priv_p->p_req_lck);
			}
			if (read_counter > 0)
				io_p->io_op->io_read_list(io_p, priv_p->p_io_handle, &read_head, read_counter);
			break;

		default:
			nid_log_error("%s: worng io_type:%d", log_header, priv_p->p_io_type);
			assert(0);
		}
		break;

	case NID_REQ_WRITE:
		switch (priv_p->p_io_type) {
		case IO_TYPE_RESOURCE:
			/*
			 * if data is ready
			 */
			if (ds_p->d_op->d_ready(ds_p, in_ds_index, rn_p->r_seq + rn_p->r_len)) {
				/* Now, we know data is ready for this request */
				gettimeofday(&rn_p->r_ready_time, NULL);
				list_del(&rn_p->r_list);
				list_del(&rn_p->r_write_list);
				stat_p->sa_write_ready_counter++;
				break;
			}
			stat_p->sa_data_not_ready_counter++;
			rn_p = NULL;
			break;

		case IO_TYPE_BUFFER:
			/*
			 * cut off write IO for bwc merging.
			 * notice that we only cutoff picking up the request here, thus no further request process and no ack as a result.
			 * let _do_data_connection_thread() continue receiving data.
			 * - if the bwc is merging to a local one, we assume we are going to join the same sac into new bwc soon.
			 * - if the bwc is merging to a remote one, don't bother the sac is still receiving more requests,
			 *   which will be removed by a configuration update soon.
			 */
			if(priv_p->p_wc_type == WC_TYPE_NONE_MEMORY) {
				cutoff = io_p->io_op->io_get_cutoff(io_p, priv_p->p_io_handle);
				if (cutoff)
					break;
			}

			INIT_LIST_HEAD(&write_head);
			write_counter = 0;
			while (rn_p) {
				if (!ds_p->d_op->d_ready(ds_p, in_ds_index, rn_p->r_seq + rn_p->r_len)) {
					stat_p->sa_data_not_ready_counter++;
					rn_p = NULL;
					break;
				}

				/* Now, we know data is ready for this request */
				list_del(&rn_p->r_list);
				list_del(&rn_p->r_write_list);
				list_add_tail(&rn_p->r_write_list, &write_head);
				gettimeofday(&rn_p->r_ready_time, NULL);
				rn_p->r_ds_index = in_ds_index;
				rn_p->r_io_handle = priv_p->p_wc_handle;
				stat_p->sa_write_ready_counter++;
				write_counter++;

				/* Next write, if there is */
				if (!list_empty(&priv_p->p_write_head))
					rn_p = list_first_entry(&priv_p->p_write_head, struct request_node, r_write_list);
				else
					rn_p = NULL;
			}

			if (write_counter > 0) {
				pthread_mutex_lock(&priv_p->p_req_lck);
				priv_p->p_do_req_counter += write_counter;
				pthread_mutex_unlock(&priv_p->p_req_lck);

				nid_log_notice("%s: calling io_write_list", log_header);
				io_p->io_op->io_write_list(io_p, priv_p->p_io_handle, &write_head, write_counter);
			}

			/*
			 * rn_p is always NULL at this step. That's fine, since bio write operation
			 * doesn't consuem the tp at all. we want try a read here
			 */
			break;

		default:
			nid_log_error("%s: worng io_type:%d", log_header, priv_p->p_io_type);
			assert(0);
		}
		if (rn_p)
			break;

		/*
		 * try next read request
		 */
		if (!list_empty(&priv_p->p_read_head)) {
			rn_p = list_first_entry(&priv_p->p_read_head, struct request_node, r_read_list);
			goto retry;
		}

		/*
		 * data is not ready and there is no nodata request, pickup nothing
		 */
		break;

	case NID_REQ_TRIM:
		switch (priv_p->p_io_type) {
		case IO_TYPE_RESOURCE:
			// only need to prepare trim list in buffer io configure
			list_del(&rn_p->r_list);
			list_del(&rn_p->r_trim_list);
			break;

		case IO_TYPE_BUFFER:
			INIT_LIST_HEAD(&trim_head);
			trim_counter = 0;
			while (rn_p) {
				list_del(&rn_p->r_list);
				list_del(&rn_p->r_trim_list);
				list_add_tail(&rn_p->r_trim_list, &trim_head);
				rn_p->r_io_handle = priv_p->p_wc_handle;
				trim_counter++;

				/* Next trim, if there is */
				if (!list_empty(&priv_p->p_trim_head))
					rn_p = list_first_entry(&priv_p->p_trim_head, struct request_node, r_trim_list);
				else
					rn_p = NULL;
			}

			if (trim_counter > 0) {
				pthread_mutex_lock(&priv_p->p_req_lck);
				priv_p->p_do_req_counter += trim_counter;
				pthread_mutex_unlock(&priv_p->p_req_lck);

				io_p->io_op->io_trim_list(io_p, priv_p->p_io_handle, &trim_head, trim_counter);
			}

			/*
			* rn_p is always NULL at this step. That's fine, since bio trim operation
			* doesn't consuem the tp at all. we want try a read here
			 */
			break;

		default:
			nid_log_error("%s: worng io_type:%d", log_header, priv_p->p_io_type);
			assert(0);
		}
		if (rn_p)
			break;

		/*
		 * try next trim request
		 */
		if (!list_empty(&priv_p->p_trim_head)) {
			rn_p = list_first_entry(&priv_p->p_trim_head, struct request_node, r_trim_list);
			goto retry;
		}

		break;

	default:
		nid_log_debug("sac_pickup_request: got unknown request, %d",
			rn_p->r_ir.cmd);
		list_del(&rn_p->r_list);
		pthread_mutex_lock(&priv_p->p_free_lck);
		list_add_tail(&rn_p->r_list, priv_p->p_to_free_ptr);
		pthread_mutex_unlock(&priv_p->p_free_lck);
		rn_p = NULL;
		break;
	}

out:
	pthread_mutex_unlock(&priv_p->p_input_lck);
	if (!rn_p) {
		pthread_mutex_lock(&priv_p->p_req_lck);
		priv_p->p_do_req_counter--;
		pthread_mutex_unlock(&priv_p->p_req_lck);
	}
	return rn_p;
}

static void
sac_do_request(struct tp_jobheader *job)
{
	char *log_header = "sac_do_request";
	struct request_node *rn_p = (struct request_node *)job;
	struct sac_interface *sac_p = rn_p->r_sac;
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	int in_ds_index = priv_p->p_in_ds;
	struct io_interface *io_p = priv_p->p_io;
	void *io_handle = priv_p->p_io_handle;
	struct nid_request *ir_p = &rn_p->r_ir;
	int do_resp = 0;
	uint32_t len;
	char *p;
	int rc;

	gettimeofday(&rn_p->r_io_start_time, NULL);
	switch (ir_p->cmd) {
	case NID_REQ_READ:
		if (priv_p->p_alevel >= NID_ALEVEL_RDONLY ||
		    rn_p->r_offset+len-1 <= NID_OFF_COMMON_HIGH) {
			rc = io_p->io_op->io_pread(io_p, io_handle, rn_p->r_resp_buf_1, rn_p->r_resp_len_1, rn_p->r_offset);
		} else {
			nid_log_error("%s: read EPERM, alevel:%d", log_header, priv_p->p_alevel);
			rc = -1;
			errno = EPERM;
		}
		if (!(rc < 0) && rn_p->r_resp_len_1 < rn_p->r_len) {
			rc = io_p->io_op->io_pread(io_p, io_handle, rn_p->r_resp_buf_2,
				rn_p->r_len - rn_p->r_resp_len_1, rn_p->r_offset + rn_p->r_resp_len_1);
		}
		ir_p->code = (rc < 0) ? errno : 0;
		if (rc < 0) {
			nid_log_error("%s: read error, rc:%d, errno:%d", log_header, rc, ir_p->code);
		}
		do_resp = 1;
		break;

	case NID_REQ_WRITE:
		len = rn_p->r_len;
		p = ds_p->d_op->d_position_length(ds_p, in_ds_index, rn_p->r_seq, &len);
		if (priv_p->p_alevel >= NID_ALEVEL_RDWR ||
		    rn_p->r_offset+len-1 <= NID_OFF_COMMON_HIGH) {
			rc = io_p->io_op->io_pwrite(io_p, io_handle, p, len, rn_p->r_offset);
		} else {
			nid_log_error("sac_do_request: write EPERM, alevel:%d", priv_p->p_alevel);
			rc = -1;
			errno = EPERM;
		}
		if (!(rc < 0) && len < rn_p->r_len) {
			p = ds_p->d_op->d_position(ds_p, in_ds_index, rn_p->r_seq + len);
			rc = io_p->io_op->io_pwrite(io_p, io_handle, p, rn_p->r_len - len, rn_p->r_offset + len);
		}
		ir_p->code = (rc < 0) ? errno : 0;
		if (rc < 0) {
			nid_log_error("%s: write error, rc:%d, errno:%d", log_header, rc, ir_p->code);
		}
		ir_p->len = ntohl(0);
		do_resp = 1;
		break;

	case NID_REQ_TRIM:
		if (priv_p->p_alevel >= NID_ALEVEL_RDWR) {
			rc = io_p->io_op->io_trim(io_p, io_handle, rn_p->r_offset, rn_p->r_len);
		} else {
			nid_log_error("sac_do_request: trim EPERM, alevel:%d", priv_p->p_alevel);
			rc = -1;
			errno = EPERM;
		}

		// set return code
		ir_p->code = (rc < 0) ? errno : 0;
		if (rc < 0) {
			nid_log_error("%s: trim error, rc:%d, errno:%d", log_header, rc, ir_p->code);
		}
		ir_p->len = ntohl(0);
		do_resp = 1;
		break;

	default:
		nid_log_debug("%s: got unknown request", log_header);
		break;
	}
	gettimeofday(&rn_p->r_io_end_time, NULL);

	if (do_resp)
		__add_response_tail(sac_p, rn_p);
}

static void
sac_start_req_counter(struct sac_interface *sac_p)
{
	char *log_header = "sac_start_req_counter";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;

	nid_log_debug("%s: start ...", log_header);
	priv_p->p_req_counter_flag  = 1;
	priv_p->p_req_num_counter = 0;
	priv_p->p_req_len_counter = 0;
}

static void
sac_check_req_counter(struct sac_interface *sac_p, struct sac_req_stat *stat_p)
{
	char *log_header = "sac_get_req_stat";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;

	nid_log_debug("%s: start ...", log_header);
	stat_p->sa_is_running = priv_p->p_req_counter_flag;
	stat_p->sa_req_num = priv_p->p_req_num_counter;
	stat_p->sa_req_len = priv_p->p_req_len_counter;
}

static void
sac_stop_req_counter(struct sac_interface *sac_p)
{
	char *log_header = "sac_stop_req_counter";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;

	nid_log_debug("%s: start ...", log_header);
	priv_p->p_req_counter_flag  = 0;
}


static void
sac_get_stat(struct sac_interface *sac_p, struct sac_stat *stat_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct io_interface *io_p = priv_p->p_io;
	memcpy(stat_p, &priv_p->p_stat, sizeof(*stat_p));
	if(!priv_p->p_to_stop) {
		io_p->io_op->io_get_chan_stat(io_p, priv_p->p_io_handle, &stat_p->sa_io_stat);
	}

	if (priv_p->p_chan_established)
		stat_p->sa_stat = NID_STAT_ACTIVE;
	else
		stat_p->sa_stat = NID_STAT_INACTIVE;
}

static void
sac_get_vec_stat(struct sac_interface *sac_p, struct io_vec_stat *stat_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct io_interface *io_p = priv_p->p_io;
	if(!priv_p->p_to_stop) {
		io_p->io_op->io_get_vec_stat(io_p, priv_p->p_io_handle, stat_p);
	}
}

static void
sac_reset_stat(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;
	nid_log_debug("sac_reset_stat: start...");

	memset(stat_p, 0, sizeof(*stat_p));
	priv_p->p_stat.sa_uuid = priv_p->p_uuid;
	stat_p->sa_ipaddr = priv_p->p_ipaddr;
}

static void
sac_drop_page(struct sac_interface *sac_p, void *page)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct ds_interface *ds_p = priv_p->p_ds;
	int in_ds_index = priv_p->p_in_ds;
	ds_p->d_op->d_drop_page(ds_p, in_ds_index, page);
}

static void
sac_get_list_stat(struct sac_interface *sac_p, struct umessage_sac_list_stat_resp *list_stat)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct request_node *rn_p, *rn_p1;

	nid_log_warning("sac_get_list_stat: start...");
	list_for_each_entry(rn_p, struct request_node, &priv_p->p_req_head, r_list)
		list_stat->um_req_counter++;

	list_for_each_entry(rn_p, struct request_node, &priv_p->p_to_req_head, r_list)
		list_stat->um_to_req_counter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, &priv_p->p_read_head, r_read_list)
		list_stat->um_rcounter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, &priv_p->p_to_read_head, r_read_list)
		list_stat->um_to_rcounter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, &priv_p->p_write_head, r_write_list)
		list_stat->um_wcounter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, &priv_p->p_to_write_head, r_write_list)
		list_stat->um_to_wcounter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, priv_p->p_resp_ptr, r_list)
		list_stat->um_resp_counter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, priv_p->p_to_resp_ptr, r_list)
		list_stat->um_to_resp_counter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, priv_p->p_dresp_ptr, r_list)
		list_stat->um_dresp_counter++;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, priv_p->p_to_dresp_ptr, r_list)
		list_stat->um_to_dresp_counter++;

}

static void
sac_cleanup(struct sac_interface *sac_p)
{
	char *log_header = "sac_cleanup";
	struct sac_private *priv_p;
	struct sah_interface *sah_p;
	struct ds_interface *ds_p;
	int in_ds_index;
	int out_ds_index;
	struct io_interface *io_p;

	nid_log_warning("%s: start", log_header);
	if (sac_p == NULL) {
		nid_log_error("%s: sac_p is NULL", log_header);
		return;
	}

	priv_p = (struct sac_private *)sac_p->sa_private;
 	if (priv_p == NULL) {
		nid_log_notice("%s: sac_p:%p priv_p is NULL", log_header, sac_p);
		return;
	}
	nid_log_warning("%s: %s:%d, rsfd:%d, dsfd:%d",
		log_header, priv_p->p_ipaddr, priv_p->p_dport, priv_p->p_rsfd, priv_p->p_dsfd);

	sah_p = priv_p->p_sah;
	ds_p = priv_p->p_ds;
	in_ds_index = priv_p->p_in_ds;
	out_ds_index = priv_p->p_out_ds;
	io_p = priv_p->p_io;

	if (priv_p->p_rsfd >= 0) {
		close(priv_p->p_rsfd);
		priv_p->p_rsfd = -1;
	}
	if (priv_p->p_dsfd >= 0) {
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = -1;
	}
	if (in_ds_index >= 0) {
		ds_p->d_op->d_release_worker(ds_p, in_ds_index, io_p);
		priv_p->p_in_ds = -1;
	}
	if (out_ds_index >= 0) {
		ds_p->d_op->d_release_worker(ds_p, out_ds_index, io_p);
		priv_p->p_out_ds = -1;
	}
	if (priv_p->p_io_handle) {
		io_p->io_op->io_close(io_p, priv_p->p_io_handle);
		priv_p->p_io_handle = NULL;
	}
	sah_p->h_op->h_cleanup(sah_p);
	free(sah_p);

	if (priv_p->p_chan_established) {
		assert(!priv_p->p_do_req_counter);
	}

	if (priv_p->p_req_stream.r_buf)
		free(priv_p->p_req_stream.r_buf);

	free(priv_p);
	sac_p->sa_private = NULL;
}

static void
sac_stop(struct sac_interface *sac_p)
{
	char *log_header = "sac_stop";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;;
	struct ds_interface *ds_p;
	int in_ds_index;
	int out_ds_index;

	nid_log_notice("%s: start (%s) ...",
		log_header, priv_p->p_uuid);
	ds_p = priv_p->p_ds;
	in_ds_index = priv_p->p_in_ds;
	out_ds_index = priv_p->p_out_ds;
	if (!priv_p->p_to_stop) {
		priv_p->p_to_stop = 1;
		if(in_ds_index >= 0) {
			ds_p->d_op->d_stop(ds_p, in_ds_index);
		}
		if(out_ds_index >= 0) {
			ds_p->d_op->d_stop(ds_p, out_ds_index);
		}
	}
}

static void
sac_prepare_stop(struct sac_interface *sac_p)
{
	char *log_header = "sac_prepare_stop";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;;

	nid_log_notice("%s: start (%s) ...",
		log_header, priv_p->p_uuid);
	if (!priv_p->p_to_stop) {
		priv_p->p_to_stop = 1;
	}
}

static int
sac_get_io_type(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	return priv_p->p_io_type;
}

static char*
sac_get_uuid(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	return priv_p->p_uuid;
}

static char
sac_get_alevel(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	return priv_p->p_alevel;
}

static char
sac_get_stop_state(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;

	nid_log_notice("sac_get_stop_state: uuid:%s, ip:%s stop state: %d",
		priv_p->p_uuid, priv_p->p_ipaddr, priv_p->p_to_stop);
	return priv_p->p_to_stop;
}

static char*
sac_get_aip(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	return priv_p->p_ipaddr;
}

static int
sac_upgrade_alevel(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct sac_stat *stat_p = &priv_p->p_stat;
	if (priv_p->p_alevel < NID_ALEVEL_MAX) {
		priv_p->p_alevel++;
		stat_p->sa_alevel = priv_p->p_alevel;
	}

	nid_log_notice("sac_upgrade_alevel: %s, %s, rsfd:%d, dsfd:%d, alevel:%d",
		priv_p->p_uuid, priv_p->p_ipaddr, priv_p->p_rsfd, priv_p->p_dsfd, priv_p->p_alevel);

	return 0;
}

/*
 * Caller should hold p_rs_mlck of sacg
 */
static struct wc_interface *
sac_get_wc(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct wc_interface *wc_p;

	wc_p = priv_p->p_wc;
	return wc_p;
}

static void
sac_remove_wc(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;

	priv_p->p_wc = NULL;
}

/*
 * Caller should hold p_rs_mlck of sacg
 */
static int
sac_switch_wc(struct sac_interface *sac_p, char *wc_uuid, int wc_type)
{
	char *log_header = "sac_switch_wc";
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	struct wcg_interface *wcg_p = priv_p->p_wcg;
	struct sdsg_interface *sdsg_p = priv_p->p_sdsg;
	struct ds_interface *ds_p = priv_p->p_ds;
	int rc = 1;
#if 0
	struct wc_channel_info wc_info;
	int new_wc = 0, wc_found = 0;
#endif

	nid_log_notice("%s: start...", log_header);


	if(wcg_p) {
		priv_p->p_wc = wcg_p->wg_op->wg_search_and_create_wc(wcg_p, wc_uuid);
		if (!priv_p->p_wc)
			return rc;
		if (wc_type == WC_TYPE_NONE_MEMORY)
			rc = sdsg_p->dg_op->dg_switch_sds_wc(sdsg_p, ds_p->d_op->d_get_wc_name(ds_p), wc_uuid);
		else if (wc_type == WC_TYPE_TWC)
			rc = 0;
	}
	/* Don't create bwc channel here, which should be done when accepting new channel. */
#if 0
	if (!wc_found ||
			!priv_p->p_wc->wc_op->wc_get_recover_state(priv_p->p_wc)) {
		nid_log_notice("%s: wc:%s not ready, going to stop...", log_header, wc_uuid);
	} else {
		wc_info.wc_rw = priv_p->p_rw;
		wc_info.wc_rw_handle = priv_p->p_rw_handle;
		wc_info.wc_rc = priv_p->p_rc;
		wc_info.wc_rc_handle = priv_p->p_rc_handle;
		priv_p->p_wc_handle = wc_p->wc_op->wc_create_channel(wc_p, sac_p,
			&wc_info, priv_p->p_uuid, &new_wc);

		/*
		 * only set sac with new bwc after succeeded considering idempotency
		 */
		if (priv_p->p_wc_handle) {
			priv_p->p_wc_type = wc_type;
			priv_p->p_wc = wc_p;
			rc = 0;
		} else {
			nid_log_warning("%s: p_wc_handle for:%s failed to be created!", log_header, wc_uuid);
		}
	}
#endif
	return rc;
}

static void
sac_enable_int_keep_alive(struct sac_interface *sac_p, uint32_t break_interval)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	if (!priv_p->p_int_keep_alive) {
		priv_p->p_int_keep_alive_interval = break_interval;
		priv_p->p_int_keep_alive_cur_num = 0;
		priv_p->p_int_keep_alive_break_num = break_interval;
		priv_p->p_int_keep_alive = 1;
	}
}

static void
sac_disable_int_keep_alive(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	if (priv_p->p_int_keep_alive) {
		priv_p->p_int_keep_alive = 0;
	}
}

static struct tp_interface *
sac_get_tp(struct sac_interface *sac_p)
{
	struct sac_private *priv_p = (struct sac_private *)sac_p->sa_private;
	return priv_p->p_tp;
}

struct sac_operations sac_op = {
	.sa_accept_new_channel = sac_accept_new_channel,
	.sa_do_channel = sac_do_channel,
	.sa_pickup_request = sac_pickup_request,
	.sa_do_request = sac_do_request,
	.sa_add_response_node = sac_add_response_node,
	.sa_add_response_list = sac_add_response_list,
	.sa_get_stat = sac_get_stat,
	.sa_reset_stat = sac_reset_stat,
	.sa_drop_page = sac_drop_page,
	.sa_cleanup = sac_cleanup,
	.sa_stop = sac_stop,
	.sa_prepare_stop = sac_prepare_stop,
	.sa_get_io_type = sac_get_io_type,
	.sa_get_uuid = sac_get_uuid,
	.sa_get_alevel = sac_get_alevel,
	.sa_upgrade_alevel = sac_upgrade_alevel,
	.sa_get_stop_state = sac_get_stop_state,
	.sa_get_aip = sac_get_aip,
	.sa_get_vec_stat = sac_get_vec_stat,
	.sa_get_wc = sac_get_wc,
	.sa_remove_wc = sac_remove_wc,
	.sa_switch_wc = sac_switch_wc,
	.sa_enable_int_keep_alive = sac_enable_int_keep_alive,
	.sa_disable_int_keep_alive = sac_disable_int_keep_alive,
	.sa_get_tp = sac_get_tp,
	.sa_start_req_counter = sac_start_req_counter,
	.sa_check_req_counter = sac_check_req_counter,
	.sa_stop_req_counter = sac_stop_req_counter,
	.sa_get_list_stat = sac_get_list_stat,
};

int
sac_initialization(struct sac_interface *sac_p, struct sac_setup *setup)
{
	char *log_header = "sac_initialization";
	struct sac_private *priv_p;
	struct request_stream *rs_p;
	struct sah_setup sah_setup;
	struct sah_interface *sah_p;

	nid_log_notice("%s: start ...", log_header);
	sac_p->sa_op = &sac_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	sac_p->sa_private = priv_p;
	priv_p->p_tpg = setup->tpg;
	priv_p->p_all_io = setup->all_io;
	priv_p->p_sacg = setup->sacg;
	priv_p->p_sdsg = setup->sdsg;
	priv_p->p_cdsg = setup->cdsg;
	priv_p->p_wcg = setup->wcg;
	priv_p->p_crcg = setup->crcg;
	priv_p->p_rwg = setup->rwg;
	priv_p->p_rsfd = setup->rsfd;
	priv_p->p_dsfd = -1;
	priv_p->p_alevel = NID_ALEVEL_INVALID;
	priv_p->p_in_ds = -1;
	priv_p->p_out_ds = -1;
	priv_p->p_mqtt = setup->mqtt;

	sah_setup.sacg = priv_p->p_sacg;
	sah_setup.rwg = priv_p->p_rwg;
	sah_setup.rsfd = priv_p->p_rsfd;
	sah_p = x_calloc(1, sizeof(*sah_p));
	sah_initialization(sah_p, &sah_setup);
	priv_p->p_sah = sah_p;

	/*
	 * setup request input stream
	 */
	rs_p = &priv_p->p_req_stream;
	rs_p->r_buf = x_malloc(NID_SIZE_REQBUF);
	rs_p->r_size = NID_SIZE_REQBUF;
	rs_p->r_start = 0;
	rs_p->r_curr_pos = 0;

	priv_p->p_max_read_stuck = 100;
	priv_p->p_max_write_stuck = 100;

	return 0;
}
