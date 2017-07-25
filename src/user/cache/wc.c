/*
 * wc.c
 * 	Implementation of Write Cache Module
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "allocator_if.h"
#include "twc_if.h"
#include "bwc_if.h"
#include "wc_if.h"
#include "io_if.h"

struct wc_private {
	char				p_name[NID_MAX_UUID];
	int				p_type;
	void				*p_real_wc;
	struct allocator_interface	*p_allocator;
	volatile int			p_recover_state;
	volatile char			p_post_initialized;
};

static void *
wc_create_channel(struct wc_interface *wc_p, void *owner, struct wc_channel_info *wc_info, char *res_p, int *new)
{
	char *log_header = "wc_create_channel";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	struct twc_interface *twc_p;
	void *wc_handle;

	while (!priv_p->p_post_initialized)
		sleep(1);

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		wc_handle = bwc_p->bw_op->bw_create_channel(bwc_p, owner, wc_info, res_p, new);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		wc_handle = twc_p->tw_op->tw_create_channel(twc_p, owner, wc_info, res_p, new);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
	}

	return wc_handle;
}

static void *
wc_prepare_channel(struct wc_interface *wc_p, struct wc_channel_info *wc_info, char *res_p)
{
	char *log_header = "wc_prepare_channel";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	struct twc_interface *twc_p;
	void *wc_handle;

	while (!priv_p->p_post_initialized)
		sleep(1);

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		wc_handle = bwc_p->bw_op->bw_prepare_channel(bwc_p, wc_info, res_p);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		wc_handle = twc_p->tw_op->tw_prepare_channel(twc_p, wc_info, res_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
	}

	return wc_handle;
}

static void
wc_recover(struct wc_interface *wc_p)
{
	char *log_header = "wc_recover";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;

	while (!priv_p->p_post_initialized)
		sleep(1);

	__sync_lock_test_and_set(&priv_p->p_recover_state, WC_RECOVER_DOING) ;
	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_recover(bwc_p);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_recover(twc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
	__sync_lock_test_and_set(&priv_p->p_recover_state, WC_RECOVER_DONE) ;
}

static void
wc_post_initialization(struct wc_interface *wc_p)
{
	char *log_header = "wc_post_initialization";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_post_initialization(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
	__sync_lock_test_and_set(&priv_p->p_post_initialized, 1) ;
}

static void *
wc_get_cache_obj(struct wc_interface *wc_p)
{
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	return priv_p->p_real_wc;
}

static int
wc_get_type(struct wc_interface *wc_p)
{
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	return priv_p->p_type;
}

static uint32_t
wc_get_poolsz(struct wc_interface *wc_p)
{
	char *log_header = "wc_get_poolsz";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	uint32_t pool_sz = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		pool_sz = bwc_p->bw_op->bw_get_poolsz(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return pool_sz;
}

static uint32_t
wc_get_pagesz(struct wc_interface *wc_p)
{
	char *log_header = "wc_get_pagesz";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	uint32_t page_sz = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		page_sz = bwc_p->bw_op->bw_get_pagesz(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return page_sz;
}

static uint32_t
wc_get_block_occupied(struct wc_interface *wc_p)
{
	char *log_header = "wc_get_block_occupied";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	uint32_t block_occupied = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		block_occupied = bwc_p->bw_op->bw_get_block_occupied(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return block_occupied;
}

static uint32_t
wc_pread(struct wc_interface *wc_p, void *wc_handle, void *buf, size_t count, off_t offset)
{
	char *log_header = "wc_pread";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	uint32_t nread = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		nread = bwc_p->bw_op->bw_pread(bwc_p, wc_handle, buf, count, offset);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		nread = twc_p->tw_op->tw_pread(twc_p, wc_handle, buf, count, offset);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return nread;
}

static void
wc_read_list(struct wc_interface *wc_p, void *wc_handle, struct list_head *read_head, int read_counter)
{
	char *log_header = "wc_read_list";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_read_list(bwc_p, wc_handle, read_head, read_counter);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_read_list(twc_p, wc_handle, read_head);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
}

static void
wc_write_list(struct wc_interface *wc_p, void *wc_handle, struct list_head *write_head, int write_counter)
{
	char *log_header = "wc_write_list";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_write_list(bwc_p, wc_handle, write_head, write_counter);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_write_list(twc_p, wc_handle, write_head, write_counter);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
}

static void
wc_trim_list(struct wc_interface *wc_p, void *wc_handle, struct list_head *rim_head, int trim_counter)
{
	char *log_header = "wc_trim_list";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_trim_list(bwc_p, wc_handle, rim_head, trim_counter);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_trim_list(twc_p, wc_handle, rim_head, trim_counter);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
}

static int
wc_chan_inactive(struct wc_interface *wc_p, void *wc_handle)
{
	char *log_header = "wc_chan_inactive";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_chan_inactive(bwc_p, wc_handle);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		rc = twc_p->tw_op->tw_chan_inactive(twc_p, wc_handle);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_stop(struct wc_interface *wc_p)
{
	char *log_header = "wc_stop";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_stop(bwc_p);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		rc = twc_p->tw_op->tw_stop(twc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_start_page(struct wc_interface *wc_p, void *wc_handle, void *page_p, int do_buffer)
{
	char *log_header = "wc_start_page";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_start_page(bwc_p, wc_handle, page_p, do_buffer);
		break;

	case WC_TYPE_TWC:
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_end_page(struct wc_interface *wc_p, void *wc_handle, void *page_p, int do_buffer)
{
	char *log_header = "wc_end_page";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	struct twc_interface *twc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_end_page(bwc_p, wc_handle, page_p, do_buffer);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_end_page(twc_p, wc_handle, page_p, do_buffer);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_flush_update(struct wc_interface *wc_p, void *wc_handle, uint64_t seq)
{
	char *log_header = "wc_flush_update";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_flush_update(bwc_p, wc_handle, seq);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_get_chan_stat(struct wc_interface *wc_p, void *wc_handle, struct io_chan_stat *sp)
{
	char *log_header = "wc_get_chan_stat";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_get_chan_stat(bwc_p, wc_handle, sp);
		break;

	// in twc, write cache does not exist, return 0 filled value
	case WC_TYPE_TWC:
		memset(sp, 0, sizeof(struct io_chan_stat));
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_get_stat(struct wc_interface *wc_p, struct io_stat *sp)
{
	char *log_header = "wc_get_stat";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_get_stat(bwc_p, sp);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_get_stat(twc_p, sp);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

struct umessage_wc_information_resp_stat;
struct umessage_twc_information_resp_stat;
static int
wc_get_info_stat(struct wc_interface *wc_p, struct umessage_wc_information_resp_stat *sp)
{
	char *log_header = "wc_get_info_stat";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_get_info_stat(bwc_p, (struct umessage_bwc_information_resp_stat *)sp);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_get_info_stat(twc_p, (struct umessage_twc_information_resp_stat *)sp);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_get_vec_stat(struct wc_interface *wc_p, void *wc_handle, struct io_vec_stat *sp)
{
	char *log_header = "wc_get_vec_stat";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_get_vec_stat(bwc_p, wc_handle, sp);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_fast_flush(struct wc_interface *wc_p, char *resource)
{
	char *log_header = "wc_fast_flush";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_fast_flush(bwc_p, resource);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_get_fast_flush(struct wc_interface *wc_p, char *resource)
{
	char *log_header = "wc_get_fast_flush";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_get_fast_flush(bwc_p, resource);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_stop_fast_flush(struct wc_interface *wc_p, char *resource)
{
	char *log_header = "wc_stop_fast_flush";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_stop_fast_flush(bwc_p, resource);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_vec_start(struct wc_interface *wc_p)
{
	char *log_header = "wc_vec_start";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_vec_start(bwc_p, NULL);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_vec_stop(struct wc_interface *wc_p)
{
	char *log_header = "wc_vec_stop";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_vec_stop(bwc_p, NULL);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static uint32_t
wc_get_release_page_counter(struct wc_interface *wc_p)
{
	char *log_header = "wc_get_release_page_counter";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_get_release_page_counter(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static uint32_t
wc_get_flush_page_counter(struct wc_interface *wc_p)
{
	char *log_header = "wc_get_release_page_counter";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_get_flush_page_counter(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static char *
wc_get_uuid(struct wc_interface *wc_p)
{
	char *log_header = "wc_get_uuid";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	char *ret_uuid = NULL;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		ret_uuid = bwc_p->bw_op->bw_get_uuid(bwc_p);
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		ret_uuid = twc_p->tw_op->tw_get_uuid(twc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return ret_uuid;
}

static int
wc_dropcache_start(struct wc_interface *wc_p, char *chan_uuid, int do_sync)
{
	char *log_header = "wc_dropcache_start";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_dropcache_start(bwc_p, chan_uuid, do_sync);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_dropcache_stop(struct wc_interface *wc_p, char *chan_uuid)
{
	char *log_header = "wc_dropcache_stop";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {

	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_dropcache_stop(bwc_p, chan_uuid);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int
wc_get_recover_state(struct wc_interface *wc_p)
{
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	return priv_p->p_recover_state;
}

static	int32_t
wc_reset_read_counter(struct wc_interface *wc_p) {
	char *log_header = "wc_reset_read_counter";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_reset_read_counter(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static int64_t
wc_get_read_counter(struct wc_interface *wc_p) {
	char *log_header = "wc_get_read_counter";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_get_read_counter(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}

static uint64_t
wc_get_coalesce_index(struct wc_interface *wc_p) {
	char *log_header = "wc_get_coalesce_index";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	uint64_t rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_get_coalesce_index(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
	}
	return rc;
}

static void
wc_reset_coalesce_index(struct wc_interface *wc_p) {
	char *log_header = "wc_reset_coalesce_index";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		bwc_p->bw_op->bw_reset_coalesce_index(bwc_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
	}
}

static uint8_t
wc_get_cutoff(struct wc_interface *wc_p, void *wc_handle)
{
	char *log_header = "wc_get_cutoff";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	uint8_t cutoff = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		cutoff = bwc_p->bw_op->bw_get_cutoff(bwc_p, wc_handle);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
	return cutoff;
}

static int
wc_destroy(struct wc_interface *wc_p)
{
	char *log_header = "wc_destroy";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct twc_interface *twc_p;
	struct bwc_interface *bwc_p;
	int rc = 0;

	while (!priv_p->p_post_initialized)
		sleep(1);

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_destroy(bwc_p);
		if (!rc) {
			free((void *)bwc_p);
			free((void *)priv_p);
		}
		break;

	case WC_TYPE_TWC:
		twc_p = (struct twc_interface *)priv_p->p_real_wc;
		twc_p->tw_op->tw_destroy(twc_p);
		free((void *)twc_p);
		free((void *)priv_p);
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return rc;
}

static int
wc_freeze_snapshot_stage1(struct wc_interface *wc_p, char *res_uuid) {
	char *log_header = "wc_freeze_snapshot_stage1";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_freeze_chain_stage1(bwc_p, res_uuid);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong type", log_header);
		break;
        }

	return rc;
}

static int
wc_freeze_snapshot_stage2(struct wc_interface *wc_p, char *res_uuid) {
	char *log_header = "wc_freeze_snapshot_stage2";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_freeze_chain_stage2(bwc_p, res_uuid);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong type", log_header);
		break;
        }

	return rc;
}

static int
wc_unfreeze_snapshot(struct wc_interface *wc_p, char *res_uuid) {
	char *log_header = "wc_unfreeze_snapshot";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_unfreeze_snapshot(bwc_p, res_uuid);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return rc;
}

static char *
wc_get_name(struct wc_interface *wc_p)
{
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	return priv_p->p_name;
}


static void
wc_get_write_list(struct wc_interface *wc_p, struct list_head *write_head)
{
	char *log_header = "wc_get_write_list";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
//	struct bwc_interface *bwc_p;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		(void)priv_p;(void)write_head;
		/* For wcd, not implement yet.*/
		 /* bwc_p->bw_op->bw_get_write_list(bwc_p, write_head); */
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		break;
	}
}

static int
wc_set_ddn_info(struct wc_interface *wc_p, void *chan_p, void *np)
{
	char *log_header = "wc_set_ddn_info";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
//	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		(void)priv_p;(void)chan_p;(void)np;
		/* For wcd, not implement yet.*/
//		rc = bwc_p->bw_op->bw_set_ddn_info(bwc_p, chan_p, np);
		nid_log_error("%s: not support yet", log_header);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return rc;

}


static int
wc_set_rn_info(struct wc_interface *wc_p, void* rn)
{
	char *log_header = "wc_set_rn_info";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		/* For wcd, not implement yet.*/
		(void)priv_p;(void)rn;
		nid_log_error("%s: not support yet", log_header);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return rc;
}



static int
wc_write_response(struct wc_interface *wc_p )
{
	char *log_header = "wc_write_response";
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_write_response(bwc_p);
		break;

	default:
		rc = -1;
		nid_log_error("%s: got wrong type", log_header);
		break;
	}

	return rc;

}

static int
wc_destroy_chain(struct wc_interface *wc_p,  char *res_uuid)
{
	struct wc_private *priv_p = (struct wc_private *)wc_p->wc_private;
	struct bwc_interface *bwc_p;
	int rc = 0;

	switch(priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = (struct bwc_interface *)priv_p->p_real_wc;
		rc = bwc_p->bw_op->bw_destroy_chain(bwc_p, res_uuid);
		break;

	default:
		break;
	}

	return rc;
}

struct wc_operations wc_op = {
	.wc_create_channel = wc_create_channel,
	.wc_prepare_channel = wc_prepare_channel,
	.wc_recover = wc_recover,
	.wc_get_poolsz = wc_get_poolsz,
	.wc_get_pagesz = wc_get_pagesz,
	.wc_get_block_occupied = wc_get_block_occupied,
	.wc_pread = wc_pread,
	.wc_read_list = wc_read_list,
	.wc_write_list = wc_write_list,
	.wc_trim_list = wc_trim_list,
	.wc_chan_inactive = wc_chan_inactive,
	.wc_stop = wc_stop,
	.wc_start_page = wc_start_page,
	.wc_end_page = wc_end_page,
	.wc_flush_update = wc_flush_update,
	.wc_get_cache_obj = wc_get_cache_obj,
	.wc_get_chan_stat = wc_get_chan_stat,
	.wc_get_vec_stat = wc_get_vec_stat,
	.wc_get_stat = wc_get_stat,
	.wc_get_info_stat = wc_get_info_stat,
	.wc_fast_flush = wc_fast_flush,
	.wc_get_fast_flush = wc_get_fast_flush,
	.wc_stop_fast_flush = wc_stop_fast_flush,
	.wc_vec_start = wc_vec_start,
	.wc_vec_stop = wc_vec_stop,
	.wc_get_release_page_counter = wc_get_release_page_counter,
	.wc_get_flush_page_counter = wc_get_flush_page_counter,
	.wc_get_uuid = wc_get_uuid,
	.wc_dropcache_start = wc_dropcache_start,
	.wc_dropcache_stop = wc_dropcache_stop,
	.wc_get_recover_state = wc_get_recover_state,
	.wc_reset_read_counter = wc_reset_read_counter,
	.wc_get_read_counter = wc_get_read_counter,
	.wc_get_coalesce_index = wc_get_coalesce_index,
	.wc_reset_coalesce_index = wc_reset_coalesce_index,
	.wc_get_cutoff = wc_get_cutoff,
	.wc_destroy = wc_destroy,
	.wc_get_name = wc_get_name,
	.wc_freeze_snapshot_stage1 = wc_freeze_snapshot_stage1,
	.wc_freeze_snapshot_stage2 = wc_freeze_snapshot_stage2,
	.wc_unfreeze_snapshot = wc_unfreeze_snapshot,
	.wc_set_ddn_info = wc_set_ddn_info,
	.wc_set_rn_info = wc_set_rn_info,
	.wc_write_response = wc_write_response,
	.wc_get_write_list = wc_get_write_list,
	.wc_get_type = wc_get_type,
	.wc_destroy_chain = wc_destroy_chain,
	.wc_post_initialization = wc_post_initialization,
};

int
wc_initialization(struct wc_interface *wc_p, struct wc_setup *setup)
{
	char *log_header = "wc_initialization";
	struct wc_private *priv_p;
	struct twc_interface *twc_p;
	struct twc_setup twc_setup;
	struct bwc_interface *bwc_p;
	struct bwc_setup bwc_setup;

	int rc = 0;

	nid_log_info("%s: start ...", log_header);
	wc_p->wc_op = &wc_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	wc_p->wc_private = priv_p;

	priv_p->p_type = setup->type;
	priv_p->p_allocator = setup->allocator;
	strcpy(priv_p->p_name, setup->uuid);
	switch (priv_p->p_type) {
	case WC_TYPE_NONE_MEMORY:
		bwc_p = x_calloc(1, sizeof(*bwc_p));
		priv_p->p_real_wc = bwc_p;
		strcpy(bwc_setup.uuid, setup->uuid);
		bwc_setup.srn = setup->srn;
		bwc_setup.wc = wc_p;
		bwc_setup.pp = setup->pp;
		bwc_setup.tp = setup->tp;
		bwc_setup.bfp = NULL;
		strcpy(bwc_setup.bufdevice, setup->bufdevice);
		bwc_setup.bufdevicesz = setup->bufdevicesz;
		bwc_setup.rw_sync = setup->rw_sync;
		bwc_setup.two_step_read = setup->two_step_read;
		bwc_setup.do_fp = setup->do_fp;
		bwc_setup.write_delay_first_level = setup->write_delay_first_level;
		bwc_setup.write_delay_second_level = setup->write_delay_second_level;
		bwc_setup.write_delay_first_level_max_us = setup->write_delay_first_level_max_us;
		bwc_setup.write_delay_second_level_max_us = setup->write_delay_second_level_max_us;
		bwc_setup.max_flush_size = setup->max_flush_size;
		bwc_setup.ssd_mode = setup->ssd_mode;
		bwc_setup.allocator = priv_p->p_allocator;
		bwc_setup.lstn = setup->lstn;
		bwc_setup.bfp_type = setup->bfp_type;
		bwc_setup.coalesce_ratio = setup->coalesce_ratio;
		bwc_setup.load_ratio_max = setup->load_ratio_max;
		bwc_setup.load_ratio_min = setup->load_ratio_min;
		bwc_setup.load_ctrl_level = setup->load_ctrl_level;
		bwc_setup.flush_delay_ctl = setup->flush_delay_ctl;
		bwc_setup.throttle_ratio = setup->throttle_ratio;
		bwc_setup.low_water_mark = setup->low_water_mark;
		bwc_setup.high_water_mark = setup->high_water_mark;
		bwc_setup.mqtt = setup->mqtt;

		bwc_initialization(bwc_p, &bwc_setup);
		break;

	case WC_TYPE_TWC:
		twc_p = x_calloc(1, sizeof(*twc_p));
		priv_p->p_real_wc = twc_p;
		strcpy(twc_setup.uuid, setup->uuid);
		twc_setup.srn = setup->srn;
		twc_setup.tp = setup->tp;
		twc_setup.pp = setup->pp;
		twc_setup.do_fp = setup->do_fp;
		twc_setup.allocator = priv_p->p_allocator;
		twc_setup.lstn = setup->lstn;
		twc_initialization(twc_p, &twc_setup);
		priv_p->p_post_initialized = 1;
		break;

	default:
		nid_log_error("%s: got wrong type", log_header);
		rc = -1;
		break;
	}

	return rc;
}
