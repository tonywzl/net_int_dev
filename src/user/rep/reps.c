/*
 * reps.c
 * 	Implementation of Replication Src Module
 */

#include <stdlib.h>
#include <pthread.h>
#include "nid_log.h"
#include "nid.h"
#include "util_nw.h"
#include "allocator_if.h"
#include "pp2_if.h"
#include "reps_if.h"
#include "nid_shared.h"
#include "tx_shared.h"
#include "lstn_if.h"
#include "tp_if.h"
#include "umpk_if.h"
#include "umpk_rept_if.h"

#ifdef METASERVER
#include "ms_intf.h"

#ifdef REP_PESUDO_MDS
MsRet
MS_Read_Fp_Async_UT(const char *voluuid,
                       const char *snapuuid,
                       off_t voff,
                       size_t len,
                       Callback func,
                       void *arg,
                       void *fp,		// non-existing fp will cleared to 0
                       bool *fpFound, 	// output parm indicating fp found or not
                       IoFlag flag)
{
	char *log_header = "reps MS_Read_Fp_Async UT";
	nid_log_warning("%s: start ...", log_header);
	assert(voluuid || voluuid==NULL);
	assert(snapuuid || snapuuid==NULL);
	assert(voff>=0);
	assert(len);
	assert(flag>=0);
	fpFound[0] = 1;
	fpFound[2] = 1;

	memcpy(fp, "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456", FP_SIZE*3);
	(*func)(0, arg);
	return retOk;
}

MsRet
MS_Read_Fp_Snapdiff_Async_UT(const char *voluuid,
					const char *snapuuidPre,
					const char *snapuuidCur,
					off_t voff,
					size_t len,
					Callback func,
					void *arg,
					void *fp,
					bool *fpDiff)

{
	char *log_header = "reps MS_Read_Fp_Snapdiff_Async UT";
	nid_log_warning("%s: start ...", log_header);
	assert(voluuid || voluuid==NULL);
	assert(snapuuidPre || snapuuidPre==NULL);
	assert(snapuuidCur || snapuuidCur==NULL);
	assert(voff>=0);
	assert(len);
	fpDiff[0] = 1;
	fpDiff[2] = 1;

	memcpy(fp, "987654321098765432109876543210987654321098765432109876543210987654321098765432109876543210987654", FP_SIZE*3);
	(*func)(0, arg);
	return retOk;
}

MsRet
MS_Read_Data_Async_UT(struct iovec* vec,
                         size_t count,
                         void *fp,
                         // bool *fpToRead, enable until required by nid
                         Callback func,
                         void *arg,
                         IoFlag flag)
{
	char *log_header = "reps MS_Read_Data_Async UT";
	nid_log_warning("%s: start ...", log_header);

	assert(vec);
	assert(count);
	assert(fp);
	assert(flag>=0);
	memcpy(vec->iov_base, "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij", 100);
	nid_log_warning("%s: run callback", log_header);
	(*func)(0, arg);
	nid_log_warning("%s: finish callback", log_header);
	return retOk;
}
#endif

#define ERROR_VOL_LEN_INVALID -10
#define ERROR_REPS_RUNNING -11
#define ERROR_DXA_FAILED -12
#define ERROR_REPT_FAILED -13
#define ERROR_REPS_PAUSE_FAILED -14
#define ERROR_REPS_CONTINUE_FAILED -15
#define ERROR_SNAPUUID_EMPTY -16
#define ERROR_SNAPUUID2_EMPTY -17

#define REPS_NW_TIMEOUT 10
#define WRITE_INTERVAL 10
#define FILE_NAME_LEN 1024
#define OFFSET_LEN 1024
#define OFFSET_FILE_PRIFIX "/etc/ilio/reps_offset_"

#define REPS_SEND_RETRY_NUM 10
#define REPS_DELAY 1
#define REPS_RESEND_DELAY 20


struct reps_private {
	char			p_rs_name[NID_MAX_UUID];
	char			p_rt_name[NID_MAX_UUID];
	char			p_dxa_name[NID_MAX_UUID];
	char			p_voluuid[NID_MAX_UUID];
	char			p_snapuuid[NID_MAX_UUID];
	char			p_snapuuid2[NID_MAX_UUID];
	struct dxag_interface	*p_dxag;
	struct dxa_interface	*p_dxa;
	struct umpk_interface	*p_umpk;
	int			p_read_fp_snapdiff;
	size_t			p_ns_page_size;
	size_t			p_fp_size;
	size_t			p_bitmap_len;
	size_t			p_buf_len;
	size_t			p_vol_len;
	off_t			p_read_fp_offset;
	int			p_request_limit;
	int			p_fp_request_waiting;
	int			p_data_request_waiting;
	int			p_reps_done;
	int			p_reps_pause;
	int			p_reps_running;
	int			p_need_resend;
	pthread_mutex_t		p_lck;
	struct list_head	p_wait_fp_missed_head;
	struct list_head	p_data_done_head;
	off_t			p_expect_offset;
	off_t			p_prev_expect_offset;
	struct pp2_interface 	*p_pp2;
	struct pp2_interface 	*p_pp2_data;
	struct tp_interface	*p_tp;
};

struct reps_start_job {
	struct tp_jobheader	j_header;
	struct reps_interface	*j_reps;
	int			j_delay;
};

/*
void
__write_buffer(void *to_write, void *content, int to_wirte_len, int content_len)
{
	while (to_wirte_len >= content_len) {
		memcpy(to_write, content, content_len);
		to_wirte_len -= content_len;
		to_write += content_len;
	}
	if (to_wirte_len > 0) {
		memcpy(to_write, content, to_wirte_len);
	}
	return;
}*/

static char *
reps_get_name(struct reps_interface *reps_p)
{
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	return priv_p->p_rs_name;
}

static float
reps_get_progress(struct reps_interface *reps_p)
{
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if ( (size_t)priv_p->p_expect_offset >= priv_p->p_vol_len || priv_p->p_reps_done == 1)
		return 1.0;

	return ((float)priv_p->p_read_fp_offset + priv_p->p_expect_offset)/priv_p->p_vol_len/2;
}

static int
reps_set_pause(struct reps_interface *reps_p)
{
	char *log_header = "reps_set_pause";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if (priv_p->p_reps_running != 0 && priv_p->p_reps_pause == 0) {
		priv_p->p_reps_pause = 1;
		return 0;
	}
	nid_log_error("%s: can not pause a reps running = %d, pause = %d", log_header, priv_p->p_reps_running, priv_p->p_reps_pause);
	return ERROR_REPS_PAUSE_FAILED;
}

static int
reps_set_continue(struct reps_interface *reps_p)
{
	char *log_header = "reps_set_continue";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if (priv_p->p_reps_running != 0 && priv_p->p_reps_pause != 0) {
		priv_p->p_reps_pause = 0;
		return 0;
	}
	nid_log_error("%s: can not continue a reps running = %d, pause = %d", log_header, priv_p->p_reps_running, priv_p->p_reps_pause);
	return ERROR_REPS_CONTINUE_FAILED;
}

static void
__reps_write_offset_local(char * file_name, off_t offset)
{
	char *log_header = "__reps_write_offset_local";
	int fd = open(file_name, O_RDWR|O_TRUNC|O_CREAT);
	if (fd < 0) {
		nid_log_error("%s: open offset file failed, file_name = %s, errno = %d", log_header, file_name, errno);
		return;
	}
	int nwrite;
	char str_offset[OFFSET_LEN];
	memset(str_offset, 0, OFFSET_LEN);
	snprintf(str_offset, OFFSET_LEN, "%ld", offset);
	nwrite = write(fd, str_offset, strlen(str_offset));
	close(fd);
	if (nwrite <= 0) {
		nid_log_error("%s: write offset file failed, file_name = %s, errno = %d, nwrite = %d", log_header, file_name, errno, nwrite);
		assert(0);
	}
	nid_log_warning("%s: nwrite = %d, offset = %ldMB", log_header, nwrite, offset/1024/1024);
}

static off_t
__reps_read_offset_local(char * file_name)
{
	char *log_header = "__reps_read_offset_local";
	int fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		nid_log_error("%s: open offset file failed, file_name = %s, errno = %d", log_header, file_name, errno);
		return 0;
	}

	off_t offset;
	int nread;
	char str_offset[OFFSET_LEN];
	memset(str_offset, 0, OFFSET_LEN);
	nread = read(fd, str_offset, OFFSET_LEN);
	close(fd);
	if (nread <= 0) {
		nid_log_error("%s: read offset file failed, file_name = %s, errno = %d, nread = %d", log_header, file_name, errno, nread);
		return 0;
	}
	offset = strtoll(str_offset, NULL, 10);
	nid_log_warning("%s: nread = %d, offset = %ldMB", log_header, nread, offset/1024/1024);

	return offset;
}

static void
__reps_write_expect_offset(struct reps_interface *reps_p)
{
	char *log_header = "__reps_write_expect_offset";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	assert(priv_p->p_expect_offset >= 0);
	nid_log_warning("%s: write_expect_offset = %ldMB", log_header, priv_p->p_expect_offset/1024/1024);
	char file_name[FILE_NAME_LEN];
	memset(file_name, 0, OFFSET_LEN);
	strncpy(file_name, OFFSET_FILE_PRIFIX, OFFSET_LEN);
	strncat(file_name, priv_p->p_rs_name, OFFSET_LEN-strlen(OFFSET_FILE_PRIFIX));
	__reps_write_offset_local(file_name, priv_p->p_expect_offset);
	return;
}

static off_t
__reps_read_expect_offset(struct reps_interface *reps_p)
{
	char *log_header = "__reps_read_expect_offset";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	off_t expect_offset = 0;
	char file_name[FILE_NAME_LEN];
	memset(file_name, 0, OFFSET_LEN);
	strncpy(file_name, OFFSET_FILE_PRIFIX, OFFSET_LEN);
	strncat(file_name, priv_p->p_rs_name, OFFSET_LEN-strlen(OFFSET_FILE_PRIFIX));

	expect_offset = __reps_read_offset_local(file_name);
	nid_log_warning("%s: read_expect_offset = %ldMB", log_header, expect_offset/1024/1024);
	assert(expect_offset >= 0);
	assert((off_t)((expect_offset/priv_p->p_ns_page_size/priv_p->p_bitmap_len)*priv_p->p_ns_page_size*priv_p->p_bitmap_len) == expect_offset);
	return expect_offset;
}

void
__reps_decrease_request_waiting(struct reps_interface *reps_p, int req_type)
{
	assert(reps_p);
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if (req_type == UMSG_REPS_REQ_FP) {
		pthread_mutex_lock(&priv_p->p_lck);
		priv_p->p_fp_request_waiting--;
		pthread_mutex_unlock(&priv_p->p_lck);
	} else if (req_type == UMSG_REPS_REQ_DATA) {
		pthread_mutex_lock(&priv_p->p_lck);
		priv_p->p_data_request_waiting--;
		pthread_mutex_unlock(&priv_p->p_lck);
	}

}

static void
__reps_send_pkg(struct reps_interface *reps_p, struct rep_fp_data_pkg *fp_pkg, int req_type)
{
	char *log_header = "__reps_send_pkg";
	assert(reps_p);
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct dxa_interface *dxa_p = priv_p->p_dxa;
	struct dxt_interface *dxt_p = dxa_p->dx_op->dx_get_dxt(dxa_p);
	void *send_buf = NULL;
	void *send_dbuf = NULL;
	struct umessage_tx_hdr *um_reps_hdr_p;
	int retry_num = REPS_SEND_RETRY_NUM;
	int send_rc = 0;

	if (fp_pkg != NULL && fp_pkg->data_type != DATA_TYPE_NONE && fp_pkg->data_type != DATA_TYPE_YES && fp_pkg->data_type != DATA_TYPE_YES_OFFSET) {
		nid_log_error("%s: send wrong data type = %d", log_header, fp_pkg->data_type);
		assert(0);
	}

	while (send_buf == NULL && retry_num > 0) {
		send_buf = dxt_p->dx_op->dx_get_buffer(dxt_p, UMSG_TX_HEADER_LEN);
		if (send_buf == NULL) {
			retry_num--;
			sleep(REPS_DELAY);
		}
	}
	if (send_buf == NULL) {
		priv_p->p_need_resend = 1;
		__reps_decrease_request_waiting(reps_p, req_type);
		nid_log_error("%s: get send buf from dxt failed", log_header);
		return;
	}

	um_reps_hdr_p = (struct umessage_tx_hdr *)send_buf;
	um_reps_hdr_p->um_req = req_type;
	um_reps_hdr_p->um_req_code = UMSG_REPS_REQ_CODE_NO;
	um_reps_hdr_p->um_len = UMSG_TX_HEADER_LEN;
	nid_log_warning("%s: req_type = %d", log_header, req_type);
	if (fp_pkg != NULL) {
		um_reps_hdr_p->um_dlen = fp_pkg->fp_pkg_size;
		retry_num = REPS_SEND_RETRY_NUM;
		while (send_dbuf == NULL && retry_num > 0) {
			send_dbuf = dxt_p->dx_op->dx_get_buffer(dxt_p, fp_pkg->fp_pkg_size);
			if (send_dbuf == NULL) {
				retry_num--;
				sleep(REPS_DELAY);
			}
		}
		if (send_dbuf == NULL) {
			priv_p->p_need_resend = 1;
			__reps_decrease_request_waiting(reps_p, req_type);
			//todo dxt_p->dx_op->dx_put_buffer(dxt_p, send_buf) when it's available.
			nid_log_error("%s: get send dbuf from dxt failed", log_header);
			return;
		}

		memcpy(send_dbuf , fp_pkg, fp_pkg->fp_pkg_size);
		nid_log_warning("%s: send data type = %d, pkg_size %ld", log_header, fp_pkg->data_type, fp_pkg->fp_pkg_size);
		send_rc = dxt_p->dx_op->dx_send(dxt_p, send_buf, UMSG_TX_HEADER_LEN, send_dbuf, fp_pkg->fp_pkg_size);
	} else {
		um_reps_hdr_p->um_dlen = 0;
		send_rc = dxt_p->dx_op->dx_send(dxt_p, send_buf, UMSG_TX_HEADER_LEN, NULL, 0);
	}

	if (send_rc != 0) {
		priv_p->p_need_resend = 1;
		__reps_decrease_request_waiting(reps_p, req_type);
		nid_log_error("%s: dxt send failed", log_header);
	}
}

static void
__reps_send_ctrl_pkg(struct reps_interface *reps_p, int req_type)
{
	__reps_send_pkg(reps_p, NULL, req_type);
}

static void
__reps_send_fp(struct rep_data_node *data_node)
{
	char *log_header = "__reps_send_fp";
	nid_log_warning("%s: start ...", log_header);
	struct reps_interface *reps_p = (struct reps_interface *)data_node->rep_p;
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	size_t i = 0;
	int fp_len = 0;
	int fp_pkg_size = 0;
	struct rep_fp_data_pkg *fp_pkg;
	bool *bitmap_p;
	void *fp_p;

	//count not empty fp in data_node
	for (i = 0; i < data_node->bitmap_len; i++) {
		if (data_node->bitmap[i]) {
			fp_len++;
		}
	}
	nid_log_warning("%s: p_read_fp_offset=%ld, data_len=%ld, fp_len=%d",
		log_header, priv_p->p_read_fp_offset, data_node->len, fp_len);
	fp_pkg_size = sizeof(*fp_pkg) + sizeof(bool) * data_node->bitmap_len + fp_len * priv_p->p_fp_size;
	fp_pkg = (struct rep_fp_data_pkg*)pp2_p->pp2_op->pp2_get(pp2_p, fp_pkg_size);
	strcpy(fp_pkg->voluuid, priv_p->p_voluuid);
	fp_pkg->offset = data_node->offset;
	fp_pkg->len = data_node->len;
	fp_pkg->fp_pkg_size = fp_pkg_size;
	fp_pkg->fp_type = FP_TYPE_NONE_ZERO;
	fp_pkg->bitmap_len = data_node->bitmap_len;
	fp_pkg->fp_len = fp_len;
	fp_pkg->data_type = DATA_TYPE_NONE;
	bitmap_p = (bool*)fp_pkg->data;
	//continuously store bitmap+fp
	memcpy(bitmap_p, data_node->bitmap, sizeof(bool) * fp_pkg->bitmap_len);
	//eliminate empty fp in data_node
	if (fp_len != 0) {
		fp_p = fp_pkg->data + fp_pkg->bitmap_len;
		for (i = 0; i < fp_pkg->bitmap_len; i++) {
			if (data_node->bitmap[i]) {
				memcpy(fp_p, data_node->fp + priv_p->p_fp_size * i, priv_p->p_fp_size);
				fp_p += priv_p->p_fp_size;
			}
		}

	}

	__reps_send_pkg(reps_p, fp_pkg, UMSG_REPS_REQ_FP);
	pp2_p->pp2_op->pp2_put(pp2_p, fp_pkg);
}

static void
__reps_read_fp_async_callback(int err, void* arg)
{
	struct rep_data_node *data_node = arg;
	if (err != 0) {
		nid_log_error("reps_read_fp_async_callback error = %d", err);
		return;
	}
	char *log_header = "__reps_read_fp_async_callback";
	nid_log_warning("%s: start ...", log_header);

	nid_log_warning("%s: fp offset %ldMB", log_header, data_node->offset/1024/1024);

	__reps_send_fp(data_node);
}

static void
__reps_read_fp_async(struct reps_interface *reps_p)
{
	char *log_header = "reps_read_fp_async";
	nid_log_warning("%s: start ...", log_header);

	assert(reps_p);
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	void *fp_p = NULL;
	bool *bitmap_found = NULL;
	struct rep_data_node *data_node;
	char *voluuid = NULL;
	char *snapuuid = NULL;
	char *snapuuid2 = NULL;

	if (priv_p->p_read_fp_offset >= (off_t)priv_p->p_vol_len)
		return;

	data_node = (struct rep_data_node*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(*data_node));
	data_node->offset = priv_p->p_read_fp_offset;
	if (priv_p->p_vol_len - priv_p->p_read_fp_offset >= priv_p->p_buf_len) {
		data_node->len = priv_p->p_buf_len;
		data_node->bitmap_len = priv_p->p_bitmap_len;
	} else {
		data_node->len = priv_p->p_vol_len - priv_p->p_read_fp_offset;
		data_node->bitmap_len = (data_node->len + priv_p->p_ns_page_size -1) / priv_p->p_ns_page_size;
	}
	data_node->fp_type = FP_TYPE_ZERO_PLACEHOLDER;
	bitmap_found = (bool*)pp2_p->pp2_op->pp2_get(pp2_p, sizeof(bool) * data_node->bitmap_len);
	fp_p = (void*)pp2_p->pp2_op->pp2_get(pp2_p, priv_p->p_fp_size * data_node->bitmap_len);
	data_node->bitmap = bitmap_found;
	data_node->fp = fp_p;
	data_node->data = NULL;
	data_node->rep_p = (void*)reps_p;

	priv_p->p_read_fp_offset += data_node->len;
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_fp_request_waiting++;
	pthread_mutex_unlock(&priv_p->p_lck);
	//add the node(without eliminated empty fp) to priv_p->p_wait_fp_missed_head
	list_add(&data_node->rep_data_list, &priv_p->p_wait_fp_missed_head);

	if (strlen(priv_p->p_voluuid) > 0)
		voluuid = priv_p->p_voluuid;
	if (strlen(priv_p->p_snapuuid) > 0)
		snapuuid = priv_p->p_snapuuid;
	if (strlen(priv_p->p_snapuuid2) > 0)
		snapuuid2 = priv_p->p_snapuuid2;

	if (priv_p->p_read_fp_snapdiff == 0)
#ifdef REP_PESUDO_MDS
		MS_Read_Fp_Async_UT(voluuid, snapuuid, data_node->offset, data_node->len,
			(Callback)__reps_read_fp_async_callback, data_node, data_node->fp, data_node->bitmap, 0);
#else
		MS_Read_Fp_Async(voluuid, snapuuid, data_node->offset, data_node->len,
			(Callback)__reps_read_fp_async_callback, data_node, data_node->fp, data_node->bitmap, 0);
#endif
	else
#ifdef REP_PESUDO_MDS
		MS_Read_Fp_Snapdiff_Async_UT(voluuid, snapuuid, snapuuid2, data_node->offset, data_node->len,
			(Callback)__reps_read_fp_async_callback, data_node, data_node->fp, data_node->bitmap);
#else
		MS_Read_Fp_Snapdiff_Async(voluuid, snapuuid, snapuuid2, data_node->offset, data_node->len,
			(Callback)__reps_read_fp_async_callback, data_node, data_node->fp, data_node->bitmap);
#endif
}

static void
__reps_send_data(struct rep_data_node *data_node)
{
	struct reps_interface *reps_p = (struct reps_interface *)data_node->rep_p;
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_data_p = priv_p->p_pp2_data;
	int fp_pkg_size = 0;
	struct rep_fp_data_pkg *fp_pkg;
	bool *bitmap_p;
	void *fp_p;
	void *data_p;

	fp_pkg_size = sizeof(*fp_pkg) + sizeof(bool) * data_node->bitmap_len
		+ data_node->fp_len * priv_p->p_fp_size + data_node->fp_len * priv_p->p_ns_page_size;
	fp_pkg = (struct rep_fp_data_pkg*)pp2_data_p->pp2_op->pp2_get(pp2_data_p, fp_pkg_size);
	strcpy(fp_pkg->voluuid, priv_p->p_voluuid);
	fp_pkg->offset = data_node->offset;
	fp_pkg->len = data_node->len;
	fp_pkg->fp_pkg_size = fp_pkg_size;
	fp_pkg->fp_type = FP_TYPE_NONE_ZERO;
	fp_pkg->bitmap_len = data_node->bitmap_len;
	fp_pkg->fp_len = data_node->fp_len;
	fp_pkg->data_type = DATA_TYPE_YES;
	if (fp_pkg->offset == priv_p->p_prev_expect_offset) {
		//notice target it is a resume start
		fp_pkg->data_type = DATA_TYPE_YES_OFFSET;
	}
	bitmap_p = (bool*)fp_pkg->data;
	//continuously store bitmap+fp+data
	memcpy(bitmap_p, data_node->bitmap, sizeof(bool) * fp_pkg->bitmap_len);
	if (data_node->fp_len != 0) {
		fp_p = fp_pkg->data + sizeof(bool) * fp_pkg->bitmap_len;
		memcpy(fp_p, data_node->fp, data_node->fp_len * priv_p->p_fp_size);
		data_p = fp_p + priv_p->p_fp_size * data_node->fp_len;
		memcpy(data_p, data_node->data, data_node->fp_len * priv_p->p_ns_page_size);
	}

	__reps_send_pkg(reps_p, fp_pkg, UMSG_REPS_REQ_DATA);
	pp2_data_p->pp2_op->pp2_put(pp2_data_p, fp_pkg);
}

static void
__reps_read_data_async_callback(int err, void* arg)
{
	char *log_header = "__reps_read_data_async_callback";
	nid_log_warning("%s: start ...", log_header);

	struct rep_data_node *data_node = arg;
	if (err != 0) {
		nid_log_error("reps_read_data_async_callback error = %d", err);
		return;
	}
	struct reps_interface *reps_p = (struct reps_interface *)data_node->rep_p;
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_data_p = priv_p->p_pp2_data;
	nid_log_warning("%s: data offset %ldMB", log_header, data_node->offset/1024/1024);

	//send data node to target, target will do ms_write_data_async
	nid_log_warning("%s: data_node->fp_len=%ld, fp=%s, data=%s", log_header, data_node->fp_len, (char *)data_node->fp, (char *)data_node->data);
	__reps_send_data(data_node);

	pp2_data_p->pp2_op->pp2_put(pp2_data_p, data_node->bitmap);
	pp2_data_p->pp2_op->pp2_put(pp2_data_p, data_node->fp);
	pp2_data_p->pp2_op->pp2_put(pp2_data_p, data_node->data);
	pp2_data_p->pp2_op->pp2_put(pp2_data_p, data_node);
}

static void
__reps_read_data_async(struct reps_interface *reps_p, struct rep_data_node *data_node)
{
	char *log_header = "__reps_read_data_async";
	nid_log_warning("%s: start ...", log_header);

	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;

	data_node->iov_data.iov_base = data_node->data;
	data_node->iov_data.iov_len = priv_p->p_ns_page_size * data_node->fp_len;
	if (data_node->fp_len == 0) {
		//there is no missed fp, call callback directly, then return
		nid_log_warning("%s: there is no missed fp, call callback directly", log_header);
		__reps_read_data_async_callback(0, data_node);
		return;
	}

#ifdef REP_PESUDO_MDS
	MS_Read_Data_Async_UT(&data_node->iov_data, 1, data_node->fp,
		(Callback)__reps_read_data_async_callback, data_node, 0);
#else
	MS_Read_Data_Async(&data_node->iov_data, 1, data_node->fp,
		(Callback)__reps_read_data_async_callback, data_node, 0);
#endif
}

static void
__reps_recv_fp_missed(struct reps_interface *reps_p, void *pkg)
{
	char *log_header = "reps_recv_fp_missed";
	nid_log_warning("%s: start ...", log_header);

	assert(reps_p);
	assert(pkg);
	struct rep_fp_data_pkg *data_pkg = (struct rep_fp_data_pkg *)pkg;
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct pp2_interface *pp2_data_p = priv_p->p_pp2_data;
	struct rep_data_node *data_node;
	struct rep_data_node *new_data_node;
	bool *pkg_bitmap = NULL;
	size_t i = 0;
	size_t fp_len = 0;
	void *fp = NULL;
	bool *bitmap = NULL;
	void *data = NULL;

	assert(!list_empty(&priv_p->p_wait_fp_missed_head));
	//find the node in p_wait_fp_missed_head with same offest
	list_for_each_entry(data_node, struct rep_data_node, &priv_p->p_wait_fp_missed_head, rep_data_list) {
		if (data_node->offset == data_pkg->offset) {
			break;
		}
	}
	if (&data_node->rep_data_list == &priv_p->p_wait_fp_missed_head) {
		nid_log_error("%s: data node not found for the offset %lu", log_header, data_pkg->offset);
		assert(0);
	}
	if (data_pkg->bitmap_len > priv_p->p_bitmap_len
		|| strcmp(data_pkg->voluuid, priv_p->p_voluuid)
		|| data_pkg->offset+data_pkg->len > priv_p->p_vol_len
		|| data_pkg->len > priv_p->p_buf_len
		|| data_pkg->len != data_pkg->bitmap_len * priv_p->p_ns_page_size
		|| data_pkg->fp_pkg_size != sizeof(*data_pkg) + sizeof(bool) * data_pkg->bitmap_len
		|| data_pkg->fp_type != FP_TYPE_NONE_FP || data_pkg->data_type != DATA_TYPE_NONE) {
		nid_log_error("incompatible package received for recv missed fp bitmap.");
		assert(0);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_fp_request_waiting--;
	priv_p->p_data_request_waiting++;
	pthread_mutex_unlock(&priv_p->p_lck);
	list_del(&data_node->rep_data_list);

	pkg_bitmap = (bool*)data_pkg->data;
	for (i = 0; i < data_pkg->bitmap_len; i++) {
		if (pkg_bitmap[i])
			fp_len++;
	}
	assert( data_pkg->fp_len == fp_len);

	//pp2 get a new node with space to read data
	new_data_node = (struct rep_data_node*)pp2_data_p->pp2_op->pp2_get(pp2_data_p, sizeof(*new_data_node));
	bitmap = (bool*)pp2_data_p->pp2_op->pp2_get(pp2_data_p, sizeof(bool) * data_node->bitmap_len);
	//no need to check if (fp_len != 0) for pp2_get
	fp = (void*)pp2_data_p->pp2_op->pp2_get(pp2_data_p, priv_p->p_fp_size * fp_len);
	data = (void*)pp2_data_p->pp2_op->pp2_get(pp2_data_p, priv_p->p_ns_page_size * fp_len);
	new_data_node->bitmap = bitmap;
	new_data_node->fp = fp;
	new_data_node->data = data;
	new_data_node->offset = data_node->offset;
	new_data_node->len = data_node->len;
	new_data_node->fp_type = FP_TYPE_NONE_ZERO;
	new_data_node->fp_len = fp_len;
	new_data_node->bitmap_len = data_node->bitmap_len;
	new_data_node->rep_p = (void *)reps_p;
	//find fp from data_node, based on missed bitmap,
	//copy the missed fp to new_data_node->fp
	memcpy(new_data_node->bitmap, pkg_bitmap, sizeof(bool) * new_data_node->bitmap_len);
	if (fp_len != 0) {
		for (i = 0; i < new_data_node->bitmap_len; i++) {
			if (!data_node->bitmap[i] && new_data_node->bitmap[i]) {
				nid_log_error("%s: data_node bitmap is 0 and missed bitmap is 1", log_header);
				assert(0);
			}
			if (new_data_node->bitmap[i]) {
				memcpy(fp, data_node->fp + priv_p->p_fp_size * i, priv_p->p_fp_size);
				fp += priv_p->p_fp_size;
			}
		}
	}

	//data_node related items are from pp2_get() in __reps_read_fp_async()
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->bitmap);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node->fp);
	pp2_p->pp2_op->pp2_put(pp2_p, data_node);
	pp2_p->pp2_op->pp2_put(pp2_p, pkg);

	__reps_read_data_async(reps_p, new_data_node);
}

//insert received data done pkg to list, by offset ascending order.
static void
__reps_pkg_add_ascending(struct rep_fp_data_pkg *data_pkg, struct list_head *head)
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

//delete continuous pkg from list after p_expect_offset.
//then write p_expect_offset.
static void
__reps_update_expect_offset(struct reps_interface *reps_p)
{
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct list_head *p;
	struct list_head *head;
	static int write_offset_count = 0;

	head = &(priv_p->p_data_done_head);
	p = head;
	while (p->next != head && priv_p->p_expect_offset >= ((struct rep_fp_data_pkg *)p->next)->offset) {
		if (priv_p->p_expect_offset == ((struct rep_fp_data_pkg *)p->next)->offset)
			priv_p->p_expect_offset += ((struct rep_fp_data_pkg *)p->next)->len;

		p = p->next;
		p->next->prev = p->prev;
		p->prev->next = p->next;
		pp2_p->pp2_op->pp2_put(pp2_p, (struct rep_fp_data_pkg *)p);
		p = head;

		write_offset_count++;
		if (write_offset_count >= WRITE_INTERVAL) {
			write_offset_count = 0;
			__reps_write_expect_offset(reps_p);
		}
	}

	if ((size_t)priv_p->p_expect_offset >= priv_p->p_vol_len)
		priv_p->p_reps_done = 1;
}

static void
__reps_recv_data_done(struct reps_interface *reps_p, void *pkg)
{
	char *log_header = "reps_recv_data_done";
	nid_log_warning("%s: start ...", log_header);

	assert(reps_p);
	assert(pkg);
	struct rep_fp_data_pkg *data_pkg = (struct rep_fp_data_pkg *)pkg;
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if (data_pkg->bitmap_len > priv_p->p_bitmap_len
		|| strcmp(data_pkg->voluuid, priv_p->p_voluuid)
		|| data_pkg->offset+data_pkg->len > priv_p->p_vol_len
		|| data_pkg->len > priv_p->p_buf_len
		|| data_pkg->len != data_pkg->bitmap_len * priv_p->p_ns_page_size
		|| data_pkg->fp_pkg_size != sizeof(*data_pkg)
		|| data_pkg->fp_type != FP_TYPE_NONE_FP || data_pkg->data_type != DATA_TYPE_DONE) {
		nid_log_error("incompatible package received for recv data done.");
		assert(0);
	}
	pthread_mutex_lock(&priv_p->p_lck);
	priv_p->p_data_request_waiting--;
	pthread_mutex_unlock(&priv_p->p_lck);

	__reps_pkg_add_ascending(data_pkg, &priv_p->p_data_done_head);
	__reps_update_expect_offset(reps_p);
}

static void
__reps_try_recv_pkg(struct reps_interface *reps_p, void **pkg_pp)
{
	char *log_header = "__reps_try_recv_pkg";
	nid_log_warning("%s: start ...", log_header);
	assert(reps_p);
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;
	struct dxa_interface *dxa_p = priv_p->p_dxa;
	struct dxt_interface *dxt_p = dxa_p->dx_op->dx_get_dxt(dxa_p);
	struct umessage_tx_hdr *um_reps_hdr_p;
	char *dbuf = NULL;

	struct list_node *req_p = dxt_p->dx_op->dx_get_request(dxt_p);
	if (req_p == NULL) {
		nid_log_warning("%s: no pkg head reveived", log_header);
		return;
	}
	um_reps_hdr_p = (struct umessage_tx_hdr *)req_p->ln_data;
	if ((um_reps_hdr_p->um_req != UMSG_REPT_REQ_FP_MISSED && um_reps_hdr_p->um_req != UMSG_REPT_REQ_DATA_WRITE_DONE)
		|| um_reps_hdr_p->um_req_code != UMSG_REPT_REQ_CODE_NO) {
		nid_log_error("%s: recv um_req = %d", log_header, um_reps_hdr_p->um_req);
		dxt_p->dx_op->dx_put_request(dxt_p, req_p);
		return;
	}

	while (dbuf == NULL) {
		dbuf = dxt_p->dx_op->dx_get_dbuf(dxt_p, req_p);
		if (dbuf == NULL) {
			nid_log_warning("%s: no pkg body received", log_header);
			sleep(REPS_DELAY);
		}
	}

	*pkg_pp = pp2_p->pp2_op->pp2_get(pp2_p, um_reps_hdr_p->um_dlen);
	memcpy(*pkg_pp, dbuf, um_reps_hdr_p->um_dlen);

	dxt_p->dx_op->dx_put_dbuf(dbuf);
	dxt_p->dx_op->dx_put_request(dxt_p, req_p);
	nid_log_warning("%s: END ...", log_header);
}

static void
__reps_start(struct tp_jobheader *jheader)
{
	char *log_header = "__reps_start";
	nid_log_warning("%s: start ...", log_header);

	struct reps_start_job *job = (struct reps_start_job *)jheader;
	struct reps_interface *reps_p = job->j_reps;
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	int request_count = 0;
	int recv_count = 0;
	void *pkg = NULL;
	struct timeval last_pkg_time;
	struct timeval now;
	gettimeofday(&last_pkg_time, NULL);
	struct rep_fp_data_pkg *rep_pkg;

	priv_p->p_reps_running = 1;
	priv_p->p_reps_done = 0;
	priv_p->p_expect_offset = __reps_read_expect_offset(reps_p);
	priv_p->p_prev_expect_offset = priv_p->p_expect_offset;
	priv_p->p_read_fp_offset = priv_p->p_expect_offset;

	while (priv_p->p_reps_done == 0) {
		gettimeofday(&now, NULL);
		if ( now.tv_sec - last_pkg_time.tv_sec > REP_TIMEOUT) {
			nid_log_error("%s: have not received pkg for %d seconds in running state, stop reps.", log_header, REP_TIMEOUT);
			return;
		}
		for (request_count = 0; request_count < PP_SIZE; request_count++) {
			if (priv_p->p_read_fp_offset < (off_t)priv_p->p_vol_len
				&& priv_p->p_fp_request_waiting < priv_p->p_request_limit
				&& priv_p->p_data_request_waiting < priv_p->p_request_limit
				&& priv_p->p_reps_pause == 0)
				__reps_read_fp_async(reps_p);
			else
				break;
		}
		for (recv_count = 0; recv_count < PP_SIZE; recv_count++) {
			pkg = NULL;
			__reps_try_recv_pkg(reps_p, &pkg);
			if (pkg != NULL) {
				rep_pkg = (struct rep_fp_data_pkg *)pkg;
				if (rep_pkg->data_type == DATA_TYPE_NONE)
					__reps_recv_fp_missed(reps_p, pkg);
				else if (rep_pkg->data_type == DATA_TYPE_DONE)
					__reps_recv_data_done(reps_p, pkg);
				else {
					nid_log_error("%s: wrong data_type = %d", log_header, rep_pkg->data_type);
					break;
				}
			}
			else
				break;
		}
		if (priv_p->p_reps_pause != 0) {
			__reps_send_ctrl_pkg(reps_p, UMSG_REPS_REQ_HEARTBEAT);
			gettimeofday(&last_pkg_time, NULL);
			sleep(REPS_DELAY);
			continue;
		}
		if (recv_count != 0) {
			gettimeofday(&last_pkg_time, NULL);
		}
		if (priv_p->p_need_resend != 0) {
			priv_p->p_read_fp_offset = priv_p->p_expect_offset;
			priv_p->p_need_resend = 0;
			sleep(REPS_RESEND_DELAY);
		}
		nid_log_warning("%s: request_count = %d , recv_count = %d, fp_request_waiting = %d, data_request_waiting = %d",
			log_header, request_count, recv_count, priv_p->p_fp_request_waiting, priv_p->p_data_request_waiting);
		if (request_count != PP_SIZE && recv_count != PP_SIZE) {
			sleep(REPS_DELAY);
		}
	}
	priv_p->p_expect_offset = 0;
	__reps_write_expect_offset(reps_p);
}

static void
__reps_start_free(struct tp_jobheader *jheader)
{
	struct reps_start_job *job = (struct reps_start_job *)jheader;
	struct reps_interface *reps_p = job->j_reps;

	char *log_header = "__reps_start_free";
	nid_log_warning("%s: start ...", log_header);
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct dxa_interface *dxa_p = priv_p->p_dxa;
	assert(dxa_p);
	int rc;
	rc = 0;
	//todo dxa cleanup after the feature is ready.
	//rc = dxa_p->dx_op->dx_cleanup(dxa_p);
	if (rc != 0) {
		nid_log_error("%s: failed to cleanup dxa", log_header);
		return;
	}

	priv_p->p_read_fp_offset = 0;
	priv_p->p_reps_running = 0;

	free(job);
	nid_log_warning("%s: END ...", log_header);
}

static int
__reps_send_rept_start(struct reps_interface *reps_p)
{
	char *log_header = "__reps_send_rept_start";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct dxa_interface *dxa_p = priv_p->p_dxa;
	uint8_t chan_type = NID_CTYPE_REPT;	//talk to rept.
	uint32_t nwrite;
	uint32_t nread;
	struct umessage_rept_start nid_msg;
	struct umessage_rept_hdr *msghdr;
	struct umessage_rept_start_resp nid_msg_resp;
	char *p;
	char buf[4096];
	uint32_t len;
	int rc = 0;

	char *ip = dxa_p->dx_op->dx_get_server_ip(dxa_p);
	int csfd = util_nw_make_connection(ip, NID_SERVER_PORT);
	if (csfd < 0) {
		nid_log_error("%s: failed to connect (%s)", log_header, ip);
		rc = -1;
		goto out;
	}

	nwrite = util_nw_write_timeout(csfd, (char *)&chan_type, 1, REPS_NW_TIMEOUT);
	if (nwrite != 1) {
		nid_log_error("%s: failed to send out channel type, errno:%d", log_header, errno);
		rc = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_rept_hdr *)&nid_msg;
	msghdr->um_req = UMSG_REPT_CMD_START;
	msghdr->um_req_code = UMSG_REPT_CODE_START;

	nid_msg.um_rt_name_len = strlen(priv_p->p_rt_name);
	strcpy(nid_msg.um_rt_name, priv_p->p_rt_name);
	umpk_p->um_op->um_encode(umpk_p, buf, &len, (int)chan_type, (void *)&nid_msg);
	nwrite = util_nw_write_timeout(csfd, (char *)buf, len, REPS_NW_TIMEOUT);

	if (nwrite != len) {
		nid_log_error("%s: failed to send rept start, errno:%d", log_header, errno);
		rc = -1;
		goto out;
	}

	msghdr = (struct umessage_rept_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(csfd, buf, UMSG_REPT_HEADER_LEN);
	if (nread != UMSG_REPT_HEADER_LEN) {
		nid_log_error("%s: failed to read rept start response, errno:%d", log_header, errno);
		rc = -1;
		goto out;
	}
	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_info("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);
	assert(msghdr->um_req == UMSG_REPT_CMD_START);
	assert(msghdr->um_req_code == UMSG_REPT_CODE_RESP_START);
	assert(msghdr->um_len >= UMSG_REPT_HEADER_LEN);
	nread = util_nw_read_n(csfd, p, msghdr->um_len - UMSG_REPT_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_REPT_HEADER_LEN)) {
		rc = -1;
		nid_log_error("%s: failed to read rept start response content, errno:%d", log_header, errno);
		goto out;
	}
	umpk_p->um_op->um_decode(umpk_p, buf, msghdr->um_len, NID_CTYPE_REPT, msghdr);
	if (nid_msg_resp.um_resp_code == 0) {
		nid_log_info("%s: start rept %s successfully.\n", log_header, priv_p->p_rt_name);
		rc = 0;
	} else {
		nid_log_error("%s: start rept %s failed.\n", log_header, priv_p->p_rt_name);
		rc = -1;
	}

out:
	if (csfd >= 0) {
		close(csfd);
		csfd = -1;
	}
	return rc;

}

static int
reps_start(struct reps_interface *reps_p)
{
	char *log_header = "reps_start";
	nid_log_warning("%s: start ...", log_header);
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	struct tp_interface *tp_p;
	struct reps_start_job *job;

	if (priv_p->p_vol_len/priv_p->p_ns_page_size*priv_p->p_ns_page_size != priv_p->p_vol_len) {
		nid_log_error("%s: volume length %ld is not multiple of page size %ld. Failed to start reps.", log_header, priv_p->p_vol_len, priv_p->p_ns_page_size);
		return ERROR_VOL_LEN_INVALID;
	}
	if (priv_p->p_reps_running != 0) {
		nid_log_error("%s: the reps %s is running. Can not start it again.", log_header, priv_p->p_rs_name);
		return ERROR_REPS_RUNNING;
	}

	struct dxag_interface *dxag_p = priv_p->p_dxag;
	if (priv_p->p_dxa == NULL) {
		priv_p->p_dxa = dxag_p->dxg_op->dxg_create_channel(dxag_p, priv_p->p_dxa_name);
		struct dxa_interface *dxa_p = priv_p->p_dxa;
		int rc;
		rc = dxa_p->dx_op->dx_start(dxa_p);
		if (rc != 0) {
			nid_log_error("%s: failed to start dxa", log_header);
			priv_p->p_dxa = NULL;
			return ERROR_DXA_FAILED;
		}
	}

	//notice target to start rept.
	if (__reps_send_rept_start(reps_p) != 0) {
		nid_log_error("%s: failed to start rept, quit reps (%s)", log_header, priv_p->p_rs_name);
		//do not return for ut
		#ifndef REP_PESUDO_MDS
		return ERROR_REPT_FAILED;
		#endif
	}

	tp_p = priv_p->p_tp;
	job = calloc(1, sizeof(*job));
	job->j_reps = reps_p;
	job->j_delay = 0;
	job->j_header.jh_do = __reps_start;
	job->j_header.jh_free = __reps_start_free;

	tp_p->t_op->t_job_insert(tp_p, (struct tp_jobheader *)job);
	return 0;
}

static int
reps_start_snap(struct reps_interface *reps_p, const char *snapuuid)
{
	char *log_header = "reps_start_snap";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if (snapuuid != NULL && strlen(snapuuid) != 0) {
		strcpy(priv_p->p_snapuuid, snapuuid);
		priv_p->p_read_fp_snapdiff = 0;
	} else {
		nid_log_error("%s: snapuuid is empty. Failed to start reps.", log_header);
		return ERROR_SNAPUUID_EMPTY;
	}

	return reps_start(reps_p);
}

static int
reps_start_snapdiff(struct reps_interface *reps_p, const char *snapuuid, const char *snapuuid2)
{
	char *log_header = "reps_start_snapdiff";
	struct reps_private *priv_p = (struct reps_private *)reps_p->rs_private;
	if (snapuuid != NULL && strlen(snapuuid) != 0) {
		strcpy(priv_p->p_snapuuid, snapuuid);
	} else {
		nid_log_error("%s: snapuuid is empty. Failed to start reps.", log_header);
		return ERROR_SNAPUUID_EMPTY;
	}
	if (snapuuid2 != NULL && strlen(snapuuid2) != 0) {
		strcpy(priv_p->p_snapuuid2, snapuuid2);
		priv_p->p_read_fp_snapdiff = 1;
	} else {
		nid_log_error("%s: snapuuid2 is empty. Failed to start reps.", log_header);
		return ERROR_SNAPUUID2_EMPTY;
	}

	return reps_start(reps_p);
}



struct reps_operations reps_op = {
	.rs_get_name = reps_get_name,
	.rs_start = reps_start,
	.rs_start_snap = reps_start_snap,
	.rs_start_snapdiff = reps_start_snapdiff,
	.rs_get_progress = reps_get_progress,
	.rs_set_pause = reps_set_pause,
	.rs_set_continue = reps_set_continue,
};

int
reps_initialization(struct reps_interface *reps_p, struct reps_setup *setup)
{
	char *log_header = "reps_initialization";
	struct reps_private *priv_p;
	struct pp2_setup pp2_setup;
	struct pp2_interface *pp2_p;
	int node_len;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	reps_p->rs_op = &reps_op;
	priv_p = calloc(1, sizeof(*priv_p));
	strcpy(priv_p->p_rs_name, setup->rs_name);
	strcpy(priv_p->p_rt_name, setup->rt_name);
	strcpy(priv_p->p_dxa_name, setup->dxa_name);
	strcpy(priv_p->p_voluuid, setup->voluuid);
	priv_p->p_dxag = setup->dxag_p;
	priv_p->p_dxa = NULL;
	//it will set snapuuid and snapuuid2 again from nidmanager command
	priv_p->p_read_fp_snapdiff = 0;
	if (setup->snapuuid != NULL && strlen(setup->snapuuid) != 0) {
		strcpy(priv_p->p_snapuuid, setup->snapuuid);
		if (setup->snapuuid2 != NULL && strlen(setup->snapuuid2) != 0) {
			strcpy(priv_p->p_snapuuid2, setup->snapuuid2);
			priv_p->p_read_fp_snapdiff = 1;
		}
	}

	if (setup->bitmap_len > 0)
		priv_p->p_bitmap_len = setup->bitmap_len;
	else
		priv_p->p_bitmap_len = BITMAP_LEN;
	priv_p->p_ns_page_size = NS_PAGE_SIZE;
	priv_p->p_buf_len = priv_p->p_ns_page_size * priv_p->p_bitmap_len;
	priv_p->p_fp_size = FP_SIZE;
	priv_p->p_vol_len = setup->vol_len;
	priv_p->p_read_fp_offset = 0;

	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_reps_fp";
	pp2_setup.allocator = setup->allocator;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_REP;
	/* The priv_p->p_pp2 is for bitmap+fp in reading fp. */
	/* Mega byte in page size. Determine page size based on buf+node len. */
	node_len = (sizeof(bool) + priv_p->p_fp_size) * priv_p->p_bitmap_len + sizeof(struct rep_data_node);
	//Add 1MB in page_size for pp2 alignment and round-off.
	pp2_setup.page_size = PP_SIZE * node_len/1024/1024 + 1;
	if (pp2_setup.page_size < PAGE_SIZE_MIN)
		pp2_setup.page_size = PAGE_SIZE_MIN;
	pp2_setup.pool_size = POOL_SIZE;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 0;
	pp2_initialization(pp2_p, &pp2_setup);
	priv_p->p_pp2 = pp2_p;

	pp2_p = calloc(1, sizeof(*pp2_p));
	pp2_setup.name = "server_reps_data";
	pp2_setup.allocator = setup->allocator;
	pp2_setup.set_id = ALLOCATOR_SET_SERVER_REP;
	/* The priv_p->p_pp2_data is for bitmap+fp+data in reading data. */
	/* There are two pp2 to avoid page exhuasted in reading fp, then it can't allocate any page in reading data.*/
	node_len = (sizeof(bool) + priv_p->p_fp_size) * priv_p->p_bitmap_len + priv_p->p_buf_len + sizeof(struct rep_data_node);
	pp2_setup.page_size = PP_SIZE * node_len/1024/1024 + 1;
	if (pp2_setup.page_size < PAGE_SIZE_MIN)
		pp2_setup.page_size = PAGE_SIZE_MIN;
	pp2_setup.pool_size = POOL_SIZE;
	pp2_setup.get_zero = 1;
	pp2_setup.put_zero = 0;
	pp2_initialization(pp2_p, &pp2_setup);
	priv_p->p_pp2_data = pp2_p;

	priv_p->p_request_limit = PP_SIZE * POOL_SIZE / PP_MARGIN;
	priv_p->p_fp_request_waiting = 0;
	priv_p->p_data_request_waiting = 0;
	priv_p->p_tp = setup->tp;
	priv_p->p_umpk = setup->umpk;

	priv_p->p_reps_done = 0;
	priv_p->p_reps_pause = 0;
	priv_p->p_reps_running = 0;
	priv_p->p_need_resend = 0;
	INIT_LIST_HEAD(&priv_p->p_wait_fp_missed_head);
	INIT_LIST_HEAD(&priv_p->p_data_done_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	reps_p->rs_private = priv_p;
	nid_log_warning("%s: END", log_header);

	return 0;
}

#else
int
reps_initialization(struct reps_interface *reps_p, struct reps_setup *setup)
{
	assert(setup);
	assert(reps_p);
	return 0;
}
#endif

