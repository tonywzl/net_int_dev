/*
 * rept.c
 * 	Implementation of Replication Target Module
 */

#include <stdlib.h>
#include <pthread.h>
#include "nid_log.h"
#include "allocator_if.h"
#include "pp2_if.h"
#include "rept_if.h"
#include "nid_shared.h"
#include "tx_shared.h"
#include "lstn_if.h"
#include "tp_if.h"
#ifdef METASERVER
#include "ms_intf.h"

#ifdef REP_PESUDO_MDS
MsRet
MS_Write_Fp_Async_UT(const char *voluuid,
                        off_t voff,
                        size_t len,
                        Callback func,
                        void *arg,
                        void *fp,
                        bool *fpToWrite,
                        bool *fpMiss)
{
	char *log_header = "rept MS_Write_Fp_Async_UT";
	nid_log_warning("%s: start ...", log_header);
	assert(voluuid || voluuid==NULL);
	assert(voff>=0);
	assert(len);
	assert(fp);
	assert(fpToWrite);
	fpMiss[2] = 1;
	(*func)(0, arg);
	return retOk;
}

MsRet
MS_Write_Data_Async_UT(const char *voluuid,
                          struct iovec *iovec,
                          size_t count,
                          off_t voff,
                          size_t len,
                          Callback func,
                          void *arg,
                          void *fp,
                          bool *fpToWrite)
{
	assert(voluuid || voluuid==NULL);
	assert(iovec);
	assert(count);
	assert(voff>=0);
	assert(len);
	assert(fp);
	assert(fpToWrite);
	(*func)(0, arg);
	return retOk;
}
#endif

#define REPT_SEND_RETRY_NUM 10

struct rept_private {
	char			p_rt_name[NID_MAX_UUID];
	char			p_dxp_name[NID_MAX_UUID];
	char			p_voluuid[NID_MAX_UUID];
	struct dxpg_interface 	*p_dxpg;
	struct dxp_interface 	*p_dxp;
	size_t			p_ns_page_size;
	size_t			p_fp_size;
	size_t			p_bitmap_len;
	size_t			p_buf_len;
	size_t			p_vol_len;
	int			p_rept_done;
	int			p_rept_stop;
	off_t			p_write_data_offset;
	struct list_head	p_data_done_head;
	struct timeval		p_last_pkg_time;
	pthread_mutex_t		p_lck;
	struct pp2_interface 	*p_pp2;
	struct tp_interface	*p_tp;
};

struct rept_start_job {
	struct tp_jobheader	j_header;
	struct rept_interface	*j_rept;
	int			j_delay;
};

static char *
rept_get_name(struct rept_interface *rept_p)
{
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	return priv_p->p_rt_name;
}

static float
rept_get_progress(struct rept_interface *rept_p)
{
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	if ( priv_p->p_vol_len == (size_t)priv_p->p_write_data_offset || priv_p->p_rept_done != 0)
		return 1.0;

	return ((float)priv_p->p_write_data_offset)/priv_p->p_vol_len;
}

static void
__rept_send_pkg(struct rept_interface *rept_p, struct rep_fp_data_pkg *fp_pkg, int req_type)
{
	char *log_header = "__rept_send_pkg";
	assert(rept_p);
	assert(fp_pkg);
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct dxp_interface *dxp_p = priv_p->p_dxp;
	struct dxt_interface *dxt_p = dxp_p->dx_op->dx_get_dxt(dxp_p);
	void *send_buf = NULL;
	void *send_dbuf = NULL;
	struct umessage_tx_hdr *um_rept_hdr_p;
	int retry_num = REPT_SEND_RETRY_NUM;
	int send_rc = 0;

	while (send_buf == NULL && retry_num > 0) {
		send_buf = dxt_p->dx_op->dx_get_buffer(dxt_p, UMSG_TX_HEADER_LEN);
		if (send_buf == NULL) {
			retry_num--;
			sleep(1);
		}
	}
	if (send_buf == NULL) {
		nid_log_error("%s: get send buf from dxt failed", log_header);
		priv_p->p_rept_stop = 1;
		//todo: how to deal with pkgs in dx buffer, after stopped?
		//how about do not stop rept and wait for reps overtime, then rept will quit itself?
		return;
	}

	um_rept_hdr_p = (struct umessage_tx_hdr *)send_buf;
	um_rept_hdr_p->um_req = req_type;
	um_rept_hdr_p->um_req_code = UMSG_REPT_REQ_CODE_NO;
	um_rept_hdr_p->um_len = UMSG_TX_HEADER_LEN;
	um_rept_hdr_p->um_dlen = fp_pkg->fp_pkg_size;

	retry_num = REPT_SEND_RETRY_NUM;
	while (send_dbuf == NULL && retry_num > 0) {
		send_dbuf = dxt_p->dx_op->dx_get_buffer(dxt_p, fp_pkg->fp_pkg_size);
		if (send_dbuf == NULL) {
			retry_num--;
			sleep(1);
		}
	}
	if (send_dbuf == NULL) {
		//todo dxt_p->dx_op->dx_put_buffer(dxt_p, send_buf) when it's available.
		nid_log_error("%s: get send dbuf from dxt failed", log_header);
		priv_p->p_rept_stop = 1;
		return;
	}

	memcpy(send_dbuf , fp_pkg, fp_pkg->fp_pkg_size);

	send_rc = dxt_p->dx_op->dx_send(dxt_p, send_buf, UMSG_TX_HEADER_LEN, send_dbuf, fp_pkg->fp_pkg_size);
	if (send_rc != 0) {
		priv_p->p_rept_stop = 1;
		nid_log_error("%s: dxt send failed", log_header);
	}

}


static void
__rept_send_bitmap(struct rep_data_node *data_node)
{
	char *log_header = "__rept_send_bitmap";

	struct rept_interface *rept_p = (struct rept_interface *)data_node->rep_p;
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	int fp_pkg_size = 0;
	struct rep_fp_data_pkg *fp_pkg;

	fp_pkg_size = sizeof(*fp_pkg) + sizeof(bool) * data_node->bitmap_len;
	fp_pkg = (struct rep_fp_data_pkg*)pp2_p->pp2_op->pp2_get(pp2_p, fp_pkg_size);
	strcpy(fp_pkg->voluuid, priv_p->p_voluuid);
	fp_pkg->offset = data_node->offset;
	fp_pkg->len = data_node->len;
	fp_pkg->fp_pkg_size = fp_pkg_size;
	fp_pkg->fp_type = FP_TYPE_NONE_FP;
	fp_pkg->bitmap_len = data_node->bitmap_len;
	fp_pkg->data_type = DATA_TYPE_NONE;
	memcpy((bool*)fp_pkg->data, data_node->bitmap_miss, sizeof(bool) * fp_pkg->bitmap_len);

	size_t i = 0;
	int fp_len = 0;
	for (i = 0; i < data_node->bitmap_len; i++) {
		if (data_node->bitmap_miss[i]) {
			fp_len++;
		}
	}
	nid_log_warning("%s: fp offset %ldMB, missed fp_len=%d", log_header, fp_pkg->offset/1024/1024,fp_len);
	fp_pkg->fp_len = fp_len;
	__rept_send_pkg(rept_p, fp_pkg, UMSG_REPT_REQ_FP_MISSED);

	//data_node related items are from pp2_get() in __rept_recv_fp()
	pp2_p->pp2_op->pp2_put(pp2_p, fp_pkg);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->bitmap_miss);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->bitmap);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->fp);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node);
}

static void
__rept_write_fp_async_callback(int err, void* arg)
{
	char *log_header = "__rept_write_fp_async_callback";
	nid_log_warning("%s: start ...", log_header);
	struct rep_data_node *data_node = arg;
	if (err != 0) {
		nid_log_error("%s: error = %d", log_header, err);
		return;
	}

	__rept_send_bitmap(data_node);
}

static void
__rept_recv_fp(struct rept_interface *rept_p, void *pkg)
{
	assert(rept_p);
	assert(pkg);
	char *log_header = "rept_recv_fp";
	nid_log_warning("%s: start ...", log_header);
	struct rep_fp_data_pkg *data_pkg = (struct rep_fp_data_pkg *)pkg;
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct rep_data_node *data_node;
	bool *bitmap_p = NULL;
	void *fp_p = NULL;
	char *voluuid = NULL;

	if (data_pkg->bitmap_len > priv_p->p_bitmap_len
		|| strcmp(data_pkg->voluuid, priv_p->p_voluuid)
		|| data_pkg->offset+data_pkg->len > priv_p->p_vol_len
		|| data_pkg->len > priv_p->p_buf_len
		|| data_pkg->fp_pkg_size != sizeof(*data_pkg) + sizeof(bool) * data_pkg->bitmap_len + data_pkg->fp_len * priv_p->p_fp_size
		|| data_pkg->fp_type != FP_TYPE_NONE_ZERO || data_pkg->data_type != DATA_TYPE_NONE) {
		nid_log_error("incompatible package received for recv fp.");
		assert(0);
		return;
	}

	bitmap_p = (bool*)data_pkg->data;
	fp_p = (void *)data_pkg->data + sizeof(bool)*data_pkg->bitmap_len;
	data_node = (struct rep_data_node*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*data_node));
	data_node->offset = data_pkg->offset;
	data_node->len = data_pkg->len;
	data_node->fp_type = FP_TYPE_NONE_ZERO;
	data_node->bitmap_len = data_pkg->bitmap_len;
	data_node->fp_len = data_pkg->fp_len;
	data_node->bitmap = (bool*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(bool) * data_node->bitmap_len);
	data_node->bitmap_miss = (bool*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(bool) * data_node->bitmap_len);
	data_node->fp = pp2_p->pp2_op->pp2_get(pp2_p, priv_p->p_fp_size * data_node->fp_len);
	memcpy(data_node->bitmap, bitmap_p, sizeof(bool) * data_node->bitmap_len);
	memcpy(data_node->fp, fp_p, priv_p->p_fp_size * data_node->fp_len);
	data_node->rep_p = (void *)rept_p;

	pp2_p->pp2_op->pp2_put(pp2_p, pkg);
	if (strlen(priv_p->p_voluuid) > 0)
		voluuid = priv_p->p_voluuid;

#ifdef REP_PESUDO_MDS
	MS_Write_Fp_Async_UT(voluuid, data_node->offset, data_node->len,
		(Callback)__rept_write_fp_async_callback, data_node, data_node->fp, data_node->bitmap, data_node->bitmap_miss);
#else
	MS_Write_Fp_Async(voluuid, data_node->offset, data_node->len,
		(Callback)__rept_write_fp_async_callback, data_node, data_node->fp, data_node->bitmap, data_node->bitmap_miss);
#endif

}

//insert sent data done pkg to list, by offset ascending order.
static void
__rept_pkg_add_ascending(struct rep_fp_data_pkg *data_pkg, struct list_head *head)
{
	struct list_head *p = head;
	while (p->next != head && data_pkg->offset > ((struct rep_fp_data_pkg *)p->next)->offset) {
		p = p->next;
	}

	data_pkg->rep_pkg_list.next = p->next;
	data_pkg->rep_pkg_list.prev = p;
	p->next->prev = &(data_pkg->rep_pkg_list);
	p->next = &(data_pkg->rep_pkg_list);
	return;
}

//delete continuous pkg from list after p_write_data_offset.
//also update p_write_data_offset.
static void
__rept_update_write_data_offset(struct rept_interface *rept_p)
{
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct list_head *p;
	struct list_head *head;

	head = &(priv_p->p_data_done_head);
	p = head;
	while (p->next != head && priv_p->p_write_data_offset >= ((struct rep_fp_data_pkg *)p->next)->offset) {
		if (priv_p->p_write_data_offset == ((struct rep_fp_data_pkg *)p->next)->offset)
			priv_p->p_write_data_offset += ((struct rep_fp_data_pkg *)p->next)->len;

		p = p->next;
		p->next->prev = p->prev;
		p->prev->next = p->next;
		pp2_p->pp2_op->pp2_put(pp2_p, (struct rep_fp_data_pkg *)p);
		p = head;
	}
}

static void
__rept_send_data_done(struct rep_data_node *data_node)
{
	char *log_header = "__rept_send_data_done";
	nid_log_warning("%s: start ...", log_header);

	struct rept_interface *rept_p = (struct rept_interface *)data_node->rep_p;
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	int fp_pkg_size = 0;
	struct rep_fp_data_pkg *fp_pkg;

	fp_pkg_size = sizeof(*fp_pkg);
	fp_pkg = (struct rep_fp_data_pkg*)pp2_p->pp2_op->pp2_get(pp2_p, fp_pkg_size);
	strcpy(fp_pkg->voluuid, priv_p->p_voluuid);
	fp_pkg->offset = data_node->offset;
	fp_pkg->len = data_node->len;
	fp_pkg->fp_pkg_size = fp_pkg_size;
	fp_pkg->fp_type = FP_TYPE_NONE_FP;
	fp_pkg->bitmap_len = data_node->bitmap_len;
	fp_pkg->data_type = DATA_TYPE_DONE;
	fp_pkg->fp_len = 0;
	__rept_send_pkg(rept_p, fp_pkg, UMSG_REPT_REQ_DATA_WRITE_DONE);

	pthread_mutex_lock(&priv_p->p_lck);
	__rept_pkg_add_ascending(fp_pkg, &priv_p->p_data_done_head);
	__rept_update_write_data_offset(rept_p);
	if ((size_t)priv_p->p_write_data_offset >= priv_p->p_vol_len)
		priv_p->p_rept_done = 1;
	pthread_mutex_unlock(&priv_p->p_lck);
	//data_node related items are from pp2_get() in __rept_recv_data()
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->data);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->bitmap);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->fp);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node);

}

static void
__rept_write_data_async_callback(int err, void* arg)
{
	char *log_header = "__rept_write_data_async_callback";
	nid_log_warning("%s: start ...", log_header);
	struct rep_data_node *data_node = arg;
	if (err != 0) {
		nid_log_error("%s: error = %d", log_header, err);
		return;
	}

	__rept_send_data_done(data_node);
}

static void
__rept_recv_data(struct rept_interface *rept_p, void *pkg)
{
	char *log_header = "__rept_recv_data";
	assert(rept_p);
	assert(pkg);
	struct rep_fp_data_pkg *data_pkg = (struct rep_fp_data_pkg *)pkg;
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct rep_data_node *data_node;
	bool *bitmap_p = NULL;
	void *fp_p = NULL;
	void *data_p = NULL;
	char *voluuid = NULL;

	if (data_pkg->bitmap_len > priv_p->p_bitmap_len
		|| strcmp(data_pkg->voluuid, priv_p->p_voluuid)
		|| data_pkg->offset+data_pkg->len > priv_p->p_vol_len
		|| data_pkg->len > priv_p->p_buf_len
		|| data_pkg->fp_pkg_size != sizeof(*data_pkg) + sizeof(bool) * data_pkg->bitmap_len
			+ data_pkg->fp_len * priv_p->p_fp_size + data_pkg->fp_len * priv_p->p_ns_page_size
		|| data_pkg->fp_type != FP_TYPE_NONE_ZERO
		|| (data_pkg->data_type != DATA_TYPE_YES && data_pkg->data_type != DATA_TYPE_YES_OFFSET)) {
		nid_log_error("incompatible package received for recv data.");
		assert(0);
		return;
	}

	bitmap_p = (bool*)data_pkg->data;
	fp_p = (void *)data_pkg->data + sizeof(bool)*data_pkg->bitmap_len;
	data_p = fp_p + priv_p->p_fp_size * data_pkg->fp_len;

	data_node = (struct rep_data_node*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*data_node));
	data_node->offset = data_pkg->offset;
	data_node->len = data_pkg->len;
	data_node->fp_type = FP_TYPE_NONE_ZERO;
	data_node->bitmap_len = data_pkg->bitmap_len;
	data_node->fp_len = data_pkg->fp_len;
	data_node->bitmap = (bool*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(bool) * data_node->bitmap_len);
	data_node->fp = pp2_p->pp2_op->pp2_get(pp2_p, priv_p->p_fp_size * data_node->fp_len);
	data_node->data = pp2_p->pp2_op->pp2_get(pp2_p, priv_p->p_ns_page_size * data_node->fp_len);
	memcpy(data_node->bitmap, bitmap_p, sizeof(bool) * data_node->bitmap_len);
	memcpy(data_node->fp, fp_p, priv_p->p_fp_size * data_node->fp_len);
	memcpy(data_node->data, data_p, priv_p->p_ns_page_size * data_node->fp_len);
	data_node->rep_p = (void *)rept_p;
	data_node->iov_data.iov_base = data_node->data;
	data_node->iov_data.iov_len = priv_p->p_ns_page_size * data_node->fp_len;

	if (data_pkg->data_type == DATA_TYPE_YES_OFFSET) {
		pthread_mutex_lock(&priv_p->p_lck);
		priv_p->p_write_data_offset = data_pkg->offset;
		pthread_mutex_unlock(&priv_p->p_lck);
	}
	pp2_p->pp2_op->pp2_put(pp2_p, pkg);

	if (data_pkg->fp_len == 0) {
		//there is no missed fp nor data, call callback directly, then return
		nid_log_warning("%s: there is no missed fp nor data, call callback directly", log_header);
		__rept_write_data_async_callback(0, data_node);
		return;
	}
	nid_log_warning("%s: recv data offset %ldMB with %ld missed fp", log_header, data_pkg->offset/1024/1024, data_pkg->fp_len);
	if (strlen(priv_p->p_voluuid) > 0)
		voluuid = priv_p->p_voluuid;

#ifdef REP_PESUDO_MDS
	MS_Write_Data_Async_UT(voluuid, &data_node->iov_data, 1, data_node->offset, data_node->len,
		(Callback)__rept_write_data_async_callback, data_node, data_node->fp, data_node->bitmap);
#else
	MS_Write_Data_Async(voluuid, &data_node->iov_data, 1, data_node->offset, data_node->len,
		(Callback)__rept_write_data_async_callback, data_node, data_node->fp, data_node->bitmap);
#endif
}

static void
__rept_try_recv_pkg(struct rept_interface *rept_p, void **pkg_pp)
{
	char *log_header = "__rept_try_recv_pkg";
	nid_log_warning("%s: start ...", log_header);
	assert(rept_p);
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct dxp_interface *dxp_p = priv_p->p_dxp;
	struct dxt_interface *dxt_p = dxp_p->dx_op->dx_get_dxt(dxp_p);
	struct umessage_tx_hdr *um_rept_hdr_p;
	struct rep_fp_data_pkg *rep_pkg;
	char * dbuf = NULL;

	struct list_node *req_p = dxt_p->dx_op->dx_get_request(dxt_p);
	if (req_p == NULL) {
		nid_log_warning("%s: no pkg head reveived", log_header);
		return;
	}
	um_rept_hdr_p = (struct umessage_tx_hdr *)req_p->ln_data;
	if ((um_rept_hdr_p->um_req != UMSG_REPS_REQ_FP && um_rept_hdr_p->um_req != UMSG_REPS_REQ_DATA && um_rept_hdr_p->um_req != UMSG_REPS_REQ_HEARTBEAT)
		|| um_rept_hdr_p->um_req_code != UMSG_REPS_REQ_CODE_NO) {
		nid_log_error("%s: recv um_req = %d", log_header, um_rept_hdr_p->um_req);;
		dxt_p->dx_op->dx_put_request(dxt_p, req_p);
		return;
	}
	gettimeofday(&priv_p->p_last_pkg_time, NULL);

//	if (um_rept_hdr_p->um_req == UMSG_REPS_REQ_HEARTBEAT) {
	if (um_rept_hdr_p->um_dlen == 0) {
		return;
	}
	while (dbuf == NULL) {
		dbuf = dxt_p->dx_op->dx_get_dbuf(dxt_p, req_p);
		if (dbuf == NULL) {
			nid_log_warning("%s: no pkg body received", log_header);
			sleep(1);
		}
	}

	rep_pkg = (struct rep_fp_data_pkg *)dbuf;
//	nid_log_warning("%s: offset %ldMB, data_type %d", log_header, rep_pkg->offset/1024/1024, rep_pkg->data_type);
	if (rep_pkg->data_type != DATA_TYPE_NONE && rep_pkg->data_type != DATA_TYPE_YES && rep_pkg->data_type != DATA_TYPE_YES_OFFSET) {
		nid_log_error("recv wrong data type = %d", rep_pkg->data_type);
		nid_log_error("%s: offset %ld, data_type %d", "recv wrong", rep_pkg->offset, rep_pkg->data_type);
		nid_log_error("%s: offset %ld, data_type %ld", "recv wrong", rep_pkg->len, rep_pkg->fp_pkg_size);
		nid_log_error("%s: offset %d, data_type %ld", "recv wrong", rep_pkg->fp_type, rep_pkg->bitmap_len);

		dxt_p->dx_op->dx_put_dbuf(dbuf);
		dxt_p->dx_op->dx_put_request(dxt_p, req_p);
		return;
	}
	*pkg_pp = pp2_p->pp2_op->pp2_get(pp2_p, um_rept_hdr_p->um_dlen);
	memcpy(*pkg_pp, dbuf, um_rept_hdr_p->um_dlen);

	dxt_p->dx_op->dx_put_dbuf(dbuf);
	dxt_p->dx_op->dx_put_request(dxt_p, req_p);
	nid_log_warning("%s: END ...", log_header);
}


static void
__rept_start(struct tp_jobheader *jheader)
{
	char *log_header = "__rept_start";
	nid_log_warning("%s: start ...", log_header);

	struct rept_start_job *job = (struct rept_start_job *)jheader;
	struct rept_interface *rept_p = job->j_rept;
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	int write_count = 0;
	void *pkg = NULL;
	struct timeval now;
	gettimeofday(&priv_p->p_last_pkg_time, NULL);
	struct rep_fp_data_pkg *rep_pkg;
	while (priv_p->p_rept_done == 0 && priv_p->p_rept_stop == 0) {
		gettimeofday(&now, NULL);
		if ( now.tv_sec - priv_p->p_last_pkg_time.tv_sec > REP_TIMEOUT) {
			nid_log_error("%s: have not received pkg for %d seconds, stop rept.", log_header, REP_TIMEOUT);
			return;
		}
		for (write_count = 0; write_count < PP_SIZE; write_count++) {
			pkg = NULL;
			__rept_try_recv_pkg(rept_p, &pkg);
			if (pkg != NULL) {
				rep_pkg = (struct rep_fp_data_pkg *)pkg;
				if (rep_pkg->data_type == DATA_TYPE_NONE)
					__rept_recv_fp(rept_p, pkg);
				else if (rep_pkg->data_type == DATA_TYPE_YES || rep_pkg->data_type == DATA_TYPE_YES_OFFSET)
					__rept_recv_data(rept_p, pkg);
				else {
					nid_log_error("%s: wrong data_type = %d", log_header, rep_pkg->data_type);
					break;
				}
			}
			else
				break;
		}

		nid_log_warning("%s: write_count = %d", log_header, write_count);
		if (write_count != PP_SIZE) {
			sleep(1);
		}
	}
	nid_log_warning("%s: END ...", log_header);
}

static void
__rept_start_free(struct tp_jobheader *jheader)
{
	char *log_header = "__rept_start_free";
	nid_log_warning("%s: start ...", log_header);
	struct rept_start_job *job = (struct rept_start_job *)jheader;
	struct rept_interface *rept_p = job->j_rept;
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	free(job);

	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_write_data_offset = 0;
	pthread_mutex_unlock(&priv_p->p_lck);

	nid_log_warning("%s: END ...", log_header);
}

static void
rept_start(struct rept_interface *rept_p)
{
	char *log_header = "rept_start";
	nid_log_warning("%s: start ...", log_header);
	struct rept_private *priv_p = (struct rept_private *)rept_p->rt_private;
	struct tp_interface *tp_p;
	struct rept_start_job *job;

	priv_p->p_rept_done = 0;
	priv_p->p_rept_stop = 0;
	struct dxpg_interface *dxpg_p = priv_p->p_dxpg;
	if (priv_p->p_dxp == NULL)
		priv_p->p_dxp = dxpg_p->dxg_op->dxg_get_dxp_by_uuid(dxpg_p, priv_p->p_dxp_name);

	tp_p = priv_p->p_tp;
	job = calloc(1, sizeof(*job));
	job->j_rept = rept_p;
	job->j_delay = 0;
	job->j_header.jh_do = __rept_start;
	job->j_header.jh_free = __rept_start_free;

	tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job);
}

struct rept_operations rept_op = {
	.rt_get_name = rept_get_name,
	.rt_start = rept_start,
	.rt_get_progress = rept_get_progress,
};

int
rept_initialization(struct rept_interface *rept_p, struct rept_setup *setup)
{
	char *log_header = "rept_initialization";
	struct rept_private *priv_p;
	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p;
	int node_len;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	rept_p->rt_op = &rept_op;
	priv_p = calloc(1, sizeof(*priv_p));
	strcpy(priv_p->p_rt_name, setup->rt_name);
	strcpy(priv_p->p_dxp_name, setup->dxp_name);
	strcpy(priv_p->p_voluuid, setup->voluuid);
	priv_p->p_dxpg = setup->dxpg_p;
	priv_p->p_dxp = NULL;

	if (setup->bitmap_len != 0)
		priv_p->p_bitmap_len = setup->bitmap_len;
	else
		priv_p->p_bitmap_len = BITMAP_LEN;
	priv_p->p_ns_page_size = NS_PAGE_SIZE;
	priv_p->p_buf_len = priv_p->p_ns_page_size * priv_p->p_bitmap_len;
	priv_p->p_fp_size = FP_SIZE;
	priv_p->p_vol_len = setup->vol_len;

	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_rept";
	pp2_setup.allocator = setup->allocator;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_REP;
	/* Mega byte in page size. Determine page size based on buf len. */
	/* No need to have two pp2, as it does not hold fp/data for subsequent operation in rept. */
	node_len = (sizeof(bool) + priv_p->p_fp_size) * priv_p->p_bitmap_len + priv_p->p_buf_len + sizeof(struct rep_data_node);
	//Add 1MB in page_size for pp2 alignment and round-off.
	pp2_setup.page_size = PP_SIZE * node_len/1024/1024 + 1;
	if (pp2_setup.page_size < PAGE_SIZE_MIN)
		pp2_setup.page_size = PAGE_SIZE_MIN;
	pp2_setup.pool_size = POOL_SIZE;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 0;
	pp2_initialization(pp2_p, &pp2_setup);
	priv_p->p_pp2 = pp2_p;

	priv_p->p_tp = setup->tp;
	priv_p->p_rept_done = 0;
	priv_p->p_rept_stop = 0;
	priv_p->p_write_data_offset = 0;
	gettimeofday(&priv_p->p_last_pkg_time, NULL);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	INIT_LIST_HEAD(&priv_p->p_data_done_head);

	rept_p->rt_private = priv_p;
	return 0;
}
#else
int
rept_initialization(struct rept_interface *rept_p, struct rept_setup *setup)
{
	assert(setup);
	assert(rept_p);
	return 0;
}
#endif

