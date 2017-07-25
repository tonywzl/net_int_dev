/*
 * rc.c
 * 	Implementation of  rc Module
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "list.h"
#include "crc_if.h"
#include "rc_if.h"
#include "dsrec_if.h"

struct allocator_interface;
struct fpn_interface;
struct brn_interface;
struct rc_private {
	char				p_name[NID_MAX_UUID];
	int				p_type;
	void				*p_real_rc;
	struct allocator_interface	*p_allocator;
	struct fpn_interface		*p_fpn;
	struct brn_interface		*p_brn;
	volatile int			p_recover_state;
};

static void *
rc_create_channel(struct rc_interface *rc_p, void *owner, struct rc_channel_info *rc_info, char *res_p, int *new)
{
	char *log_header = "rc_create_channel";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	void *rc_handle;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc_handle = crc_p->cr_op->cr_create_channel(crc_p, owner, rc_info, res_p, new);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return rc_handle;
}

static void *
rc_prepare_channel(struct rc_interface *rc_p, struct rc_channel_info *rc_info, char *res_p)
{
	char *log_header = "rc_prepare_channel";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	void *rc_handle;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc_handle = crc_p->cr_op->cr_prepare_channel(crc_p, rc_info, res_p);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return rc_handle;
}

/*
 * Output:
 * 	not_found_head: list head for not-found sub_request_node list
 */
static void
rc_search(struct rc_interface *rc_p, void *rc_handle, struct sub_request_node *rn_p, struct list_head *not_found_head)
{
	char *log_header = "rc_search";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_search(crc_p, rc_handle, rn_p, not_found_head);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_search_fetch(struct rc_interface *rc_p, void *rc_handle, struct sub_request_node *rn_p, rc_callback callback, struct rc_wc_cb_node *arg)
{
	char *log_header = "rc_search";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_search_fetch(crc_p, rc_handle, rn_p, (crc_callback)callback, (struct crc_rc_cb_node*)arg);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_search_fetch_twc(struct rc_interface *rc_p, void *rc_handle, struct sub_request_node *rn_p, rc_callback_twc callback, struct rc_twc_cb_node *arg)
{
	char *log_header = "rc_search";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_search_fetch(crc_p, rc_handle, rn_p, (crc_callback)callback, (struct crc_rc_cb_node*)arg);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_updatev(struct rc_interface *rc_p, void *rc_handle, struct iovec *iov, int iov_counter, off_t offset, struct list_head *ag_fp_head)
{
	char *log_header = "crc_updatev";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	/*
	 * May be called after wc recovered and started to flush to target.
	 */
	while (priv_p->p_recover_state != RC_RECOVER_DONE)
		usleep(500000);

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_updatev(crc_p, rc_handle, iov, iov_counter, offset, ag_fp_head);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_update (struct rc_interface *rc_p, void *rc_handle, void *data, ssize_t size, off_t offset, struct list_head *ag_fp_head) {
	char *log_header = "crc_update";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	/*
	 * May be called after wc recovered and started to flush to target.
	 */
	while (priv_p->p_recover_state != RC_RECOVER_DONE)
		usleep(500000);

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_update(crc_p, rc_handle, data, size, offset, ag_fp_head);
		break;
	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static ssize_t
rc_setup_cache_bmp(struct rc_interface *rc_p, struct list_head *space_head)
{
	char *log_header = "rc_setup_cache_bmp";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	ssize_t rc = -1;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_setup_cache_bmp(crc_p,space_head, 1);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return rc;
}
static ssize_t
rc_setup_cache_mdata(struct rc_interface *rc_p, struct list_head *fp_head, struct list_head *space_head)
{
	char *log_header = "rc_setup_cache_mdata";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	ssize_t rc = -1;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_setup_cache_mdata(crc_p, fp_head, space_head);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return rc;
}

static ssize_t
rc_setup_cache_data(struct rc_interface *rc_p, char *p, struct list_head *space_head)
{
	char *log_header = "rc_setup_cache_data";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	ssize_t rc = -1;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_setup_cache_data(crc_p, p, space_head);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return rc;
}

static void
rc_flush_content(struct rc_interface *rc_p, char *data_p, uint64_t flush_size, struct list_head *space_head)
{
	char *log_header = "rc_flush_content";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_flush_content(crc_p, data_p, flush_size, space_head);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_touch_block(struct rc_interface *rc_p, void *old_block, void *new_fp)
{
	char *log_header = "rc_touch_block";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_touch_block(crc_p, old_block, new_fp);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_insert_block(struct rc_interface *rc_p, void *block_to_insert)
{
	char *log_header = "rc_insert_block";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_insert_block(crc_p, block_to_insert);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

/*
 * Algorithm:
 * 	Read a block from cache device
 */
static int
rc_read_block(struct rc_interface *rc_p, char *buf, uint64_t block_index)
{
	char *log_header = "rc_read_block";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	int rc;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_read_block(crc_p, buf, block_index);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
		rc = -1;
	}

	return rc;
}

static void
rc_drop_block(struct rc_interface *rc_p, void *block_to_drop)
{
	char *log_header = "rc_drop_block";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_drop_block(crc_p, block_to_drop);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
}

static void
rc_set_fp_wgt(struct rc_interface *rc_p, uint8_t fp_wgt) {
	char *log_header = "rc_set_fp_wgt";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_set_fp_wgt(crc_p, fp_wgt);
		break;
	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

}

static struct dsrec_stat
rc_get_dsrec_stat(struct rc_interface *rc_p) {
	char *log_header = "rc_get_dsrec_stat";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	struct dsrec_stat dsrec_stat;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		dsrec_stat = crc_p->cr_op->cr_get_dsrec_stat(crc_p);
		break;
	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return dsrec_stat;

}

static int
rc_chan_inactive_res(struct rc_interface *rc_p, const char* res) {
	char *log_header = "rc_chan_inactive_res";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	int ret = 0;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		ret = crc_p->cr_op->cr_chan_inactive_res(crc_p, res);
		break;
	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return ret;

}

static uint64_t 
rc_freespace(struct rc_interface *rc_p )
{
	char *log_header = "rc_freespace";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	uint64_t ret_uuid = 0;
	//	uint64_t out_len;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		ret_uuid = crc_p->cr_op->cr_freespace(crc_p );
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}
	//	umpk_p->um_op->um_encode(umpk_p, buf, &len, ctype, (void *)&nid_msg);
	// __encode_rc_freespace_result(ret_uuid, &out_len, struct umessage_crc_hdr *msghdr_p);
	return ret_uuid;
}

static struct umessage_crc_information_resp_freespace_dist* 
rc_freespace_dist(struct rc_interface *rc_p )
{
	char *log_header = "rc_freespace_dist";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	// struct list_head* space_list_head = NULL;
	struct umessage_crc_information_resp_freespace_dist* ssm = NULL;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		ssm = crc_p->cr_op->cr_freespace_dist(crc_p );
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return ssm;
}

static struct list_head* 
rc_dsbmp_rtree_list(struct rc_interface *rc_p )
{
	char *log_header = "rc_dsbmp_rtree_list";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	// struct list_head* space_list_head = NULL;
	struct list_head* p_dsbmp_rtree_list = NULL;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		p_dsbmp_rtree_list = crc_p->cr_op->cr_dsbmp_rtree_list(crc_p );
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return p_dsbmp_rtree_list;
}

static unsigned char
rc_check_fp(struct rc_interface *rc_p)
{
	char *log_header = "rc_check_fp";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	unsigned char rc = 0;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_check_fp(crc_p );
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = 1;
		break;
	}

	return rc;
}

static int 
rc_dropcache_start(struct rc_interface *rc_p, char *chan_uuid, int do_sync)
{
	char *log_header = "rc_dropcache_start";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_dropcache_start(crc_p, chan_uuid, do_sync);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int 
rc_dropcache_stop(struct rc_interface *rc_p, char *chan_uuid)
{
	char *log_header = "rc_dropcache_stop";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_dropcache_stop(crc_p, chan_uuid);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int 
rc_get_nse_stat(struct rc_interface *rc_p, char *chan_uuid, struct nse_stat *stat)
{
	char *log_header = "rc_information_nse_counter";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_get_nse_stat(crc_p, chan_uuid, stat);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static void * 
rc_nse_traverse_start(struct rc_interface *rc_p, char *chan_uuid)
{
	char *log_header = "rc_information_nse_counter";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	void *ret_handle = NULL;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		ret_handle = crc_p->cr_op->cr_nse_traverse_start(crc_p, chan_uuid);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return ret_handle;
}

static void 
rc_nse_traverse_stop(struct rc_interface *rc_p, void *chan_handle)
{
	char *log_header = "rc_nse_traverse_stop";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_nse_traverse_stop(crc_p, chan_handle);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
}

static int
rc_nse_traverse_next(struct rc_interface *rc_p, void *chan_handle, char *fp_buf, uint64_t *block_index)
{
	char *log_header = "rc_information_nse_counter";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;
	int rc;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		rc = crc_p->cr_op->cr_nse_traverse_next(crc_p, chan_handle, fp_buf, block_index);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

char*  rc_get_uuid(struct rc_interface * p_rc)
{
  // assert( p_rc );
	struct rc_private *priv_p = (struct rc_private *)p_rc->rc_private;
	struct crc_interface *crc_p = (struct crc_interface *)priv_p->p_real_rc;
	return crc_p->cr_op->cr_get_uuid( crc_p );
  	// return NULL;
}

static struct crc_interface*
rc_get_crc( struct rc_interface* p_rc )
{
	struct rc_private *priv_p = (struct rc_private *)p_rc->rc_private;
	struct crc_interface *crc_p = (struct crc_interface *)priv_p->p_real_rc;
	return crc_p;
}

static char *
rc_get_name( struct rc_interface* p_rc)
{
	struct rc_private *priv_p = (struct rc_private *)p_rc->rc_private;
	return priv_p->p_name;
}

static int
rc_get_recover_state(struct rc_interface *rc_p)
{
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	return priv_p->p_recover_state;
}

static void
rc_set_recover_state(struct rc_interface *rc_p, int recover_state)
{
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	__sync_lock_test_and_set(&priv_p->p_recover_state, recover_state);
}

static void
rc_recover(struct rc_interface *rc_p)
{
	char *log_header = "rc_recover";
	struct rc_private *priv_p = (struct rc_private *)rc_p->rc_private;
	struct crc_interface *crc_p;

	switch(priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = (struct crc_interface *)priv_p->p_real_rc;
		crc_p->cr_op->cr_recover(crc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
}

struct rc_operations rc_op = {
	.rc_create_channel = rc_create_channel,
	.rc_prepare_channel = rc_prepare_channel,
	.rc_search = rc_search,
	.rc_search_fetch = rc_search_fetch,
	.rc_search_fetch_twc = rc_search_fetch_twc,
	.rc_updatev = rc_updatev,
	.rc_update = rc_update,
	.rc_setup_cache_data = rc_setup_cache_data,
	.rc_setup_cache_mdata = rc_setup_cache_mdata,
	.rc_setup_cache_bmp = rc_setup_cache_bmp,
	.rc_flush_content = rc_flush_content,
	.rc_touch_block = rc_touch_block,
	.rc_insert_block = rc_insert_block,
	.rc_read_block = rc_read_block,
	.rc_drop_block = rc_drop_block,
	.rc_set_fp_wgt = rc_set_fp_wgt,
	.rc_get_dsrec_stat = rc_get_dsrec_stat,
	.rc_chan_inactive_res = rc_chan_inactive_res,
	.rc_get_uuid = rc_get_uuid,
	.rc_freespace = rc_freespace,
	.rc_freespace_dist = rc_freespace_dist,
	.rc_dsbmp_rtree_list = rc_dsbmp_rtree_list,
	.rc_dropcache_start = rc_dropcache_start,
	.rc_dropcache_stop = rc_dropcache_stop,
	.rc_get_nse_stat = rc_get_nse_stat,
	.rc_nse_traverse_start = rc_nse_traverse_start,
	.rc_nse_traverse_stop = rc_nse_traverse_stop,
	.rc_nse_traverse_next = rc_nse_traverse_next,
	.rc_check_fp = rc_check_fp,
	.rc_get_crc = rc_get_crc,
	.rc_get_name = rc_get_name,
	.rc_set_recover_state = rc_set_recover_state,
	.rc_get_recover_state = rc_get_recover_state,
	.rc_recover = rc_recover,
};

int
rc_initialization(struct rc_interface *rc_p, struct rc_setup *setup)
{
	char *log_header = "rc_initialization";
	struct rc_private *priv_p;
	struct crc_interface *crc_p;
	struct crc_setup crc_setup;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	rc_p->rc_op = &rc_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	rc_p->rc_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_fpn = setup->fpn;
	priv_p->p_brn = setup->brn;
	priv_p->p_type = setup->type;
	strcpy(priv_p->p_name, setup->uuid);
	switch (priv_p->p_type) {
	case RC_TYPE_CAS:
		crc_p = x_calloc(1, sizeof(*crc_p));
		priv_p->p_real_rc = crc_p;
		crc_setup.rc = rc_p;
		crc_setup.fpn = priv_p->p_fpn;
		crc_setup.srn = setup->srn;
		crc_setup.pp = setup->pp;
		strcpy(crc_setup.cachedev, setup->cachedev);
		crc_setup.cachedevsz = setup->cachedevsz;
		strcpy(crc_setup.uuid, setup->uuid);
		crc_setup.allocator = priv_p->p_allocator;
		crc_setup.blocksz = 4096;
		crc_setup.brn = priv_p->p_brn;
		nid_log_debug("%s: crc_setup.blocksz = %lu",log_header, crc_setup.blocksz);
		crc_initialization(crc_p, &crc_setup);
		break;

	default:
		nid_log_error("%s: got wrong type (%d)", log_header, priv_p->p_type);
	}

	return 0;
}
