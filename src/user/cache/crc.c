/*
 * crc.c
 * 	Implementation of CAS (Content-Addressed Storage) Read Cache Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "nid_log.h"
#include "nid_shared.h"
#include "nid.h"
#include "list.h"
#include "allocator_if.h"
#include "cdn_if.h"
#include "srn_if.h"
#include "fpn_if.h"
#include "blksn_if.h"
#include "nse_if.h"
#include "cse_if.h"
#include "sac_if.h"
#include "rw_if.h"
#include "rc_if.h"
#include "dsmgr_if.h"
#include "dsrec_if.h"
#include "crc_if.h"
#include "fpc_if.h"
#include "nse_crc_cbn.h"
#include "cse_crc_cbn.h"
#include "rc_wc_cbn.h"

#define CRC_BLOCK_SHIFT		12
#define CRC_BLOCK_SIZE		(1 << CRC_BLOCK_SHIFT)
#define CRC_BLOCK_MASK		(CRC_BLOCK_SIZE - 1)
#define CRC_SUPER_BLOCK_SIZE	(1UL << 20)		// 1M, must be multiple of 64 blocks
#define CRC_MBLOCK_SIZE		32			// bytes
#define	CRC_MAX_MBLOCK_FLUSH	4096

#define CRC_SUPER_HEADRE_SIZE	64
struct crc_super_header {
	uint64_t	s_blocksz;	// size of block in bytes, generally speaking, 4096
	uint64_t	s_headersz;	// number of blocks for the cache header
	uint64_t	s_bmpsz;	// number of blocks for the bitmap
	uint64_t	s_metasz;	// number of blocks for all meta data
	uint64_t	s_size;		// number of data blocks
};

#define CRC_MBLOCK_SIZE		32
struct crc_meta_block {
	char		FP[32];
};

struct crc_channel {
	char			c_resource[NID_MAX_PATH];
	struct list_head	c_list;
	uint8_t			c_active;
	struct sac_interface	*c_sac;
	struct rw_interface	*c_rw;
	struct nse_interface	c_nse;
	uint8_t			c_do_dropcache;
};

struct crc_private {
	char				p_uuid[NID_MAX_UUID];
	struct allocator_interface	*p_allocator;
	struct cdn_interface		p_cdn;
	struct brn_interface		*p_brn;
	struct rc_interface		*p_rc;
	pthread_mutex_t			p_lck;
	struct list_head		p_chan_head;
	struct list_head		p_inactive_head;
	struct srn_interface		*p_srn;
	struct pp_interface		*p_pp;
	struct cse_interface		p_cse;
	struct fpn_interface		*p_fpn;
	struct fpc_interface	 	p_fpc;
	struct blksn_interface		p_blksn;
	struct dsmgr_interface		p_dsmgr;
	struct dsrec_interface		p_dsrec;
	struct crc_super_header		p_super;
	struct nse_crc_cbn_interface 	p_ncc;
	struct cse_crc_cbn_interface 	p_ccc;
	uint64_t			*p_bitmap;
	struct crc_meta_block		*p_meta_data;
	char				*p_fp_buf;
	char				p_cachedev[NID_MAX_PATH];
	int				p_fd;
	uint32_t			p_blocksz;	// likely be 4K
	uint32_t			p_block_shift;	// LINKELY 12
	uint32_t			p_supersz;	// in blocks, likely 1M/4K = 256
	uint64_t			p_total_size;	// total number of blocks, including super/bitmap/meta/data
	uint64_t			p_bmpsz;	// bitmap size in number of blocks, must be multiple of 64 blocks
	uint64_t			p_metasz;
	uint64_t			p_size;		// number of data blocks, must be multiple of 64 blocks
	uint8_t				p_stop;
};

static void *
crc_create_channel(struct crc_interface *crc_p, void *owner, struct rc_channel_info *rc_info, char *res_p, int *new)
{
	char *log_header = "crc_create_channel";
	struct crc_private *priv_p = (struct crc_private *)crc_p->cr_private;
	struct crc_channel *chan_p = NULL;
	struct nse_interface *nse_p;
	struct nse_setup nse_setup;
	int got_it = 0;

	nid_log_warning("%s: start (%s)", log_header, res_p);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_inactive_head, c_list) {
		if (!strcmp(chan_p->c_resource, res_p)) {
			got_it = 1;
			break;
		}
	}

	if (got_it && owner) {
		nid_log_notice("%s: resource:%s exists in this inactive list, make it active",
			log_header, res_p);
		chan_p->c_sac = (struct sac_interface *)owner;
		assert(!chan_p->c_active);
		list_del(&chan_p->c_list);	// removed from p_inactive_head
		list_add_tail(&chan_p->c_list, &priv_p->p_chan_head);
		chan_p->c_active = 1;
	} else if (got_it) {
		assert(!chan_p->c_active);
		nid_log_notice("%s: resource:%s exists in ths inactive list, cannot active it w/o owner",
			log_header, res_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (got_it) {
		*new = 0;
		goto out;
	}
	*new = 1;

	chan_p = x_calloc(1, sizeof(*chan_p));
	if (!chan_p ) {
		nid_log_error("%s: x_calloc failed.", log_header);
	}
	strncpy(chan_p->c_resource, res_p, NID_MAX_PATH - 1);
	chan_p->c_sac = (struct sac_interface *)owner;
	chan_p->c_rw = (struct rw_interface *)rc_info->rc_rw;

	nse_p = &chan_p->c_nse;
	nse_setup.allocator = priv_p->p_allocator;
	nse_setup.brn = priv_p->p_brn;
	nse_setup.srn = priv_p->p_srn;
	nse_setup.fpn = priv_p->p_fpn;
	nse_setup.block_shift = CRC_BLOCK_SHIFT;
	nse_setup.size = priv_p->p_size;
	nse_initialization(nse_p, &nse_setup);

	pthread_mutex_lock(&priv_p->p_lck);
	if (owner) {
		list_add_tail(&chan_p->c_list, &priv_p->p_chan_head);
		chan_p->c_active = 1;
	} else {
		list_add_tail(&chan_p->c_list, &priv_p->p_inactive_head);
		chan_p->c_active = 0;
	}
	pthread_mutex_unlock(&priv_p->p_lck);

out:
	return chan_p;
}

static void *
crc_prepare_channel(struct crc_interface *crc_p, struct rc_channel_info *rc_info, char *res_p)
{
	char *log_handle = "crc_prepare_channel";
	struct crc_private *priv_p = (struct crc_private *)crc_p->cr_private;
	struct crc_channel *chan_p = NULL;
	int new_chan;

	nid_log_warning("%s: start (%s)", log_handle, res_p);
	assert(priv_p);
	chan_p = crc_create_channel(crc_p, NULL, rc_info, res_p, &new_chan);
	assert(!chan_p || new_chan);
	return chan_p;
}

/*
 * Algorithm:
 * 	Not taking care alignment issue. The caller should guarantee the request (rn_p) must be aligned
 */
static void
crc_search(struct crc_interface *crc_p, void *rc_handle,
	struct sub_request_node *rn_p, struct list_head *not_found_head)
{
	//char *log_header = "crc_search";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p = (struct crc_channel *)rc_handle;
	struct nse_interface *nse_p = &chan_p->c_nse;
	struct cse_interface *cse_p = &priv_p->p_cse;
	struct srn_interface *srn_p = priv_p->p_srn;
	struct fpn_interface *fpn_p = priv_p->p_fpn;
	struct list_head found_head, fp_found_head, fp_not_found_head, not_found_head2;
	struct sub_request_node *new_rn, *rn_p2;
	struct fp_node *fp_np = NULL, *fp_np2;
	int not_found_num;

	INIT_LIST_HEAD(&fp_found_head);
	INIT_LIST_HEAD(&fp_not_found_head);
	INIT_LIST_HEAD(&found_head);
	INIT_LIST_HEAD(&not_found_head2);

	nse_p->ns_op->ns_search(nse_p, rn_p, &fp_found_head, &found_head, not_found_head);
	if (!list_empty(&fp_found_head)) {
		cse_p->cs_op->cs_search_list(cse_p, &fp_found_head, &found_head, &not_found_head2, &fp_not_found_head, &not_found_num);

		if (!list_empty(&not_found_head2))
			list_splice_tail_init(&not_found_head2, not_found_head);

		list_for_each_entry_safe(new_rn, rn_p2, struct sub_request_node, &found_head, sr_list) {
			list_del(&new_rn->sr_list);
			srn_p->sr_op->sr_put_node(srn_p, new_rn);
		}

		list_for_each_entry_safe(fp_np, fp_np2, struct fp_node, &fp_found_head, fp_list) {
			list_del(&fp_np->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fp_np);
		}

		list_for_each_entry_safe(fp_np, fp_np2, struct fp_node, &fp_not_found_head, fp_list) {
			list_del(&fp_np->fp_list);
			fpn_p->fp_op->fp_put_node(fpn_p, fp_np);
		}
	}
}

static void
crc_cse_search_fetch_callback(int errcode, struct cse_crc_cb_node *arg)
{
	struct crc_interface *crc_p = arg->cc_crc_p;
	struct crc_private *priv_p = crc_p->cr_private;
	struct cse_crc_cbn_interface *ccc_p = &priv_p->p_ccc;

	arg->cc_super_callback(errcode, arg->cc_super_arg);
	ccc_p->cc_op->cc_put_node(ccc_p, arg);
}

static void
crc_nse_search_fetch_callback(int errcode, struct nse_crc_cb_node *arg)
{
	struct crc_interface *crc_p = arg->nc_crc_p;
	struct crc_private *priv_p = crc_p->cr_private;
	struct cse_interface *cse_p = &priv_p->p_cse;
	struct nse_crc_cbn_interface *ncc_p = &priv_p->p_ncc;
	struct cse_crc_cbn_interface *ccc_p = &priv_p->p_ccc;
	struct cse_crc_cb_node *cse_arg = NULL;

	if (errcode) {
		/*Error happened, callback with error*/
		arg->nc_super_callback(errcode, arg->nc_super_arg);
	} else {
		if (!list_empty(&arg->nc_fp_found_head)) {
			cse_arg = ccc_p->cc_op->cc_get_node(ccc_p);
			cse_arg->cc_crc_p = crc_p;
			cse_arg->cc_super_callback = arg->nc_super_callback;
			cse_arg->cc_super_arg = arg->nc_super_arg;
			cse_arg->cc_rw = arg->nc_rw;
			cse_arg->cc_rw_handle = arg->nc_rw_handle;
			cse_p->cs_op->cs_search_fetch(cse_p, &arg->nc_fp_found_head, &arg->nc_found_head, crc_cse_search_fetch_callback, cse_arg);
		} else {
			/*All offset not found any FP, callback*/
			assert(list_empty(&arg->nc_found_head));
			arg->nc_super_callback(errcode, arg->nc_super_arg);
		}
	}

	ncc_p->nc_op->nc_put_node(ncc_p, arg);
}

/*
 * Algorithm:
 * 	Not taking care alignment issue. The caller should guarantee the request (rn_p) must be aligned
 */
static void
crc_search_fetch(struct crc_interface *crc_p, void *rc_handle,
	struct sub_request_node *rn_p, crc_callback callback,  struct crc_rc_cb_node *arg)
{
	//char *log_header = "crc_search_fetch";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p = (struct crc_channel *)rc_handle;
	struct rw_interface *rw_p = chan_p->c_rw;
	struct nse_interface *nse_p = &chan_p->c_nse;
	struct nse_crc_cbn_interface *ncc_p = &priv_p->p_ncc;
	struct nse_crc_cb_node *nse_arg;

	nse_arg = ncc_p->nc_op->nc_get_node(ncc_p);
	nse_arg->nc_crc_p = crc_p;
	nse_arg->nc_super_callback = callback;
	nse_arg->nc_super_arg = arg;
	nse_arg->nc_rw = rw_p;
	nse_arg->nc_rw_handle = arg->rm_rw_handle;
	INIT_LIST_HEAD(&nse_arg->nc_found_head);
	INIT_LIST_HEAD(&nse_arg->nc_fp_found_head);

	nse_p->ns_op->ns_search_fetch(nse_p, rn_p, crc_nse_search_fetch_callback, nse_arg);
}

/*
 * Algorithm:
 * 	Not taking care alignment issue. We assume all items in iov and offset
 * 	arre p_blocksz aligned
 */
static void
crc_updatev(struct crc_interface *crc_p, void *rc_handle,
	struct iovec *iov, int iov_counter, off_t offset, struct list_head *ag_fp_head)
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p = (struct crc_channel *)rc_handle;
	struct nse_interface *nse_p = &chan_p->c_nse;
	struct cse_interface *cse_p = &priv_p->p_cse;

	nse_p->ns_op->ns_updatev(nse_p, iov, iov_counter, offset, ag_fp_head);
	cse_p->cs_op->cs_updatev(cse_p, iov, iov_counter, ag_fp_head);
}

/*
 * Algorithm:
 * 	Not taking care alignment issue. We assume length and offset
 * 	are p_blocksz alignment
 */
static void
crc_update(struct crc_interface *crc_p, void *rc_handle, void *data,
	ssize_t len, off_t offset, struct list_head *ag_fp_head)
{
	int iov_counter = len >> CRC_BLOCK_SHIFT;
	struct iovec iov[iov_counter];
	int i;
	void *databuf;

	databuf = data;
	assert((len & CRC_BLOCK_MASK) == 0);
	for (i=0; i < iov_counter; i++) {
		iov[i].iov_base = databuf;
		iov[i].iov_len = CRC_BLOCK_SIZE;
		databuf += CRC_BLOCK_SIZE;
	}
	crc_updatev(crc_p, rc_handle, iov, iov_counter, offset, ag_fp_head);
}

void
__setBit( char ch, int i ,int n)
{
	int j;
	unsigned mask = 0;
	for(j = n-1; j>=i; j--){
		mask = mask | 1 << j;
	}
  	ch =  mask | ch ;  // using bitwise OR
}
void
__UnsetBit( char ch, int i, int n)
{
	int j;
	unsigned mask = 0;
	for(j = n-1; j>=i; j--){
		mask = mask | 1 << j;
	}
	 ch &=  ~mask ;  // using bitwise OR

}
/*
 * Algorithm:
 * 	update bitmap
 *
 * Input:
 * 	space_head: free space to flush to.
 */
static int
crc_setup_cache_bmp(struct crc_interface *crc_p, struct list_head *space_head, int setBit)
{
	struct crc_private *priv_p = crc_p->cr_private;
	int fd = priv_p->p_fd;
	uint64_t block_index, size, left, segment;
	off_t offset, offset_orig;
	struct block_size_node *bk_np;
	char *p, *p_orig;
	int rc = 0, start_bit;

	list_for_each_entry(bk_np, struct block_size_node, space_head, bsn_list) {
		p = priv_p->p_fp_buf;
		p_orig = priv_p->p_fp_buf;
		block_index = bk_np->bsn_index;
		size = bk_np->bsn_size;
		offset_orig = offset = (priv_p->p_supersz << priv_p->p_block_shift) + block_index / 8;
		start_bit = block_index % 8;
		left = size;
		while (left) {
			segment = (uint64_t)(8 - start_bit) > left? left: (uint64_t)(8 - start_bit);
			if(segment < 8){
				rc = pread(fd, p,1, offset);
				if (rc <0)
					goto out;
			}
			if(setBit)
			__setBit (*p, start_bit, start_bit + segment);
			else
			__UnsetBit (*p, start_bit, start_bit + segment);

			p++;
			start_bit = 0;
			offset++;
			left -= segment;
		}
		rc = pwrite(fd, p_orig, p - p_orig, offset_orig);
	}

out:
	return rc;
}

/*
 * Input:
 * 	fp_head: mdata list to flush
 * 	space_head: free space to flush to.
 * 	fp_head and space_head must have same size
 */
static int
crc_setup_cache_mdata(struct crc_interface *crc_p, struct list_head *fp_head, struct list_head *space_head)
{
	struct crc_private *priv_p = crc_p->cr_private;
	char *p;
	int fd = priv_p->p_fd, counter;
	uint64_t block_index, size;
	off_t offset;
	struct block_size_node *bk_np;
	struct content_description_node *cd_np;
	size_t to_write;
	int rc = 0;

	cd_np = list_first_entry(fp_head, struct content_description_node, cn_plist);
	list_for_each_entry(bk_np, struct block_size_node, space_head, bsn_list) {
		block_index = bk_np->bsn_index;
		offset = ((priv_p->p_supersz + priv_p->p_bmpsz) << priv_p->p_block_shift) +
			CRC_MBLOCK_SIZE * block_index;
		size = bk_np->bsn_size;
		counter = 0;
		p = priv_p->p_fp_buf;
		while (size) {
			memcpy(p, cd_np->cn_fp, CRC_FP_SIZE);
			p += CRC_MBLOCK_SIZE;
			cd_np = list_entry(cd_np->cn_plist.next, struct content_description_node, cn_plist);
			counter++;
			size--;
			if (counter == CRC_MAX_MBLOCK_FLUSH) {
				to_write = counter * CRC_MBLOCK_SIZE;
				rc = pwrite(fd, priv_p->p_fp_buf, to_write, offset);
				if (rc < 0)
					goto out;
				offset += to_write;
				counter = 0;
				p = priv_p->p_fp_buf;
			}
		}
		if (counter) {
			to_write = counter * CRC_MBLOCK_SIZE;
			rc = pwrite(fd, priv_p->p_fp_buf, to_write, offset);
			if (rc < 0)
				goto out;
		}
	}
out:
	return rc;

}

/*
 * Input:
 * 	p :pointer to the page.
 * 	space_head: free space to flush to.
 */
static int
crc_setup_cache_data(struct crc_interface *crc_p, char *p, struct list_head *space_head )
{
	struct crc_private *priv_p = crc_p->cr_private;
	int fd = priv_p->p_fd;
	struct block_size_node *bk_np;
	off_t offset;
	int rc = 0;
	uint64_t size, block_index;

        list_for_each_entry(bk_np, struct block_size_node, space_head, bsn_list) {
		block_index = bk_np->bsn_index;
		offset = ((priv_p->p_supersz + priv_p->p_bmpsz + priv_p->p_metasz + block_index) << priv_p->p_block_shift);
		size = bk_np->bsn_size << priv_p->p_block_shift;
		rc = pwrite(fd, p, size, offset);
		if (rc < 0)
			goto out;
		p += size;
	}
out:
	return rc;
}

static uint64_t
crc_freespace(struct crc_interface *crc_p )
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsmgr_interface *dsmgr_p = &priv_p->p_dsmgr;
	uint64_t freespace = 0; // x_calloc( 1, sizeof( uint64_t ) );

	freespace = dsmgr_p->dm_op->dm_get_num_free_blocks(dsmgr_p );
	return freespace;

}

static int*
crc_sp_heads_size(struct crc_interface *crc_p )
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsmgr_interface *dsmgr_p = &priv_p->p_dsmgr;
	int* sp_heads_size; // x_calloc( 1, sizeof( uint64_t ) );

	sp_heads_size = dsmgr_p->dm_op->dm_get_sp_heads_size(dsmgr_p );
	return sp_heads_size;

}

static unsigned char
crc_check_fp(struct crc_interface *crc_p )
{
	struct crc_private *priv_p = crc_p->cr_private;
	int fp_check_rc = 0; // x_calloc( 1, sizeof( uint64_t ) );
	struct cse_interface *cse_p = &priv_p->p_cse;

	fp_check_rc = cse_p->cs_op->cs_check_fp(cse_p );
	if ( fp_check_rc  ) {
	  	return 1;
	} else {
	  	return 0;
	}

}

static struct umessage_crc_information_resp_freespace_dist*
crc_freespace_dist(struct crc_interface *crc_p )
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsmgr_interface *dsmgr_p = &priv_p->p_dsmgr;
	//	uint64_t page_expecting;
	//	struct list_head* space_list_head  = x_calloc( 1, sizeof( struct list_head ) );

	return (struct umessage_crc_information_resp_freespace_dist*)(dsmgr_p->dm_op->dm_check_space(dsmgr_p ));

}

static void
crc_cse_hit(struct crc_interface *crc_p, int *hit_counter, int *unhit_counter )
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct cse_interface *cse_p = &priv_p->p_cse;

//	struct umessage_crc_information_resp_cse_hit res;
	struct cse_info_hit res_cse_info;

	cse_p->cs_op->cs_info_hit(cse_p, &res_cse_info);
	*hit_counter = res_cse_info.cs_hit_counter;
	*unhit_counter = res_cse_info.cs_unhit_counter;

	//return &res;

}

static struct list_head*
crc_dsbmp_rtree_list(struct crc_interface *crc_p )
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsmgr_interface *dsmgr_p = &priv_p->p_dsmgr;

	return dsmgr_p->dm_op->dm_traverse_dsbmp_rtree(dsmgr_p );

}

/*
 * Input:
 * 	flush_head: a list of content_description_node to flush
 * 	flush_size: number of blocks to flush
 */
static void
crc_flush_content(struct crc_interface *crc_p, char *data_p, uint64_t flush_size, struct list_head *space_head)
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsmgr_interface *dsmgr_p = &priv_p->p_dsmgr;
	uint64_t page_expecting;

	/*
	 * get space to flush
	 */
	while ((page_expecting = flush_size)) {
		dsmgr_p->dm_op->dm_get_space(dsmgr_p, &page_expecting, space_head);
		assert(flush_size >= page_expecting);
		flush_size -= page_expecting;
	}

	/*
	 * copy data
	 */
	crc_setup_cache_data(crc_p, data_p, space_head);
}

static void
crc_touch_block(struct crc_interface *crc_p, void *old_block, void *new_fp)
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsrec_interface *dsrec_p = &priv_p->p_dsrec;
	dsrec_p->sr_op->sr_touch_block(dsrec_p, (struct content_description_node *)old_block, (char *)new_fp);
}

static void
crc_insert_block(struct crc_interface *crc_p, void *block_to_insert)
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsrec_interface *dsrec_p = &priv_p->p_dsrec;
	dsrec_p->sr_op->sr_insert_block(dsrec_p, (struct content_description_node *)block_to_insert);
}

/*
 * Algorithm:
 * 	Read a block from cache device
 */
static int
crc_read_block(struct crc_interface *crc_p, char *buf, uint64_t block_index)
{
	struct crc_private *priv_p = crc_p->cr_private;
	int fd = priv_p->p_fd;
	off_t offset;
//	nid_log_debug(" test1 read from  cache device");
	offset = ((priv_p->p_supersz + priv_p->p_bmpsz + priv_p->p_metasz + block_index) << priv_p->p_block_shift);
	return pread(fd, buf, priv_p->p_blocksz, offset);
}

static void
crc_drop_block(struct crc_interface *crc_p, void *block_to_drop)
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct cse_interface *cse_p = &priv_p->p_cse;
	cse_p->cs_op->cs_drop_block(cse_p, (struct content_description_node *)block_to_drop);
}

static void
crc_set_fp_wgt(struct crc_interface *crc_p, uint8_t fp_wgt) {
	struct crc_private *priv_p = crc_p->cr_private;
	struct fpc_interface *fpc_p = &priv_p->p_fpc;
	fpc_p->fpc_op->fpc_set_wgt(fpc_p, fp_wgt);
}

static struct dsrec_stat
crc_get_dsrec_stat(struct crc_interface *crc_p) {
	struct crc_private *priv_p = crc_p->cr_private;
	struct dsrec_interface *dsrec_p = &priv_p->p_dsrec;
	return dsrec_p->sr_op->sr_get_dsrec_stat(dsrec_p);
}

static int
crc_chan_inactive(struct crc_interface *crc_p, void *rc_handle)
{
	char *log_header = "crc_chan_inactive";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p = (struct crc_channel *)rc_handle;

	nid_log_notice("%s: start (%s)...", log_header, chan_p->c_resource);
	pthread_mutex_lock(&priv_p->p_lck);
	if (chan_p->c_active) {
		list_del(&chan_p->c_list);	// removed from p_chan_head
		chan_p->c_active = 0;
		chan_p->c_sac = NULL;
		list_add_tail(&chan_p->c_list, &priv_p->p_inactive_head);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static int
crc_chan_inactive_res(struct crc_interface *crc_p, const char *res)
{
	char *log_header = "crc_chan_inactive_res";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p, *chan_p1;

	nid_log_notice("%s: start (%s)...", log_header, res);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(chan_p, chan_p1, struct crc_channel, &priv_p->p_chan_head, c_list) {
		if (!strcmp(chan_p->c_resource, res)) {
			list_del(&chan_p->c_list);	// removed from p_chan_head
			chan_p->c_active = 0;
			chan_p->c_sac = NULL;
			list_add_tail(&chan_p->c_list, &priv_p->p_inactive_head);
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	return 0;
}

static int
crc_stop(struct crc_interface *crc_p)
{
	char *log_header = "crc_stop";
	struct crc_private *priv_p = crc_p->cr_private;

	nid_log_notice("%s: start...", log_header);
	priv_p->p_stop = 1;
	return 0;
}

static char *
crc_get_uuid(struct crc_interface *crc_p)
{
	struct crc_private *priv_p = crc_p->cr_private;
	if (priv_p->p_uuid[0] != '\0') {
		nid_log_debug( "crc_get_uuid - p_uuid = %s", priv_p->p_uuid  );
	} else  {
		nid_log_debug( "crc_get_uuid - p_uuid is empty!"  );
	}
	return priv_p->p_uuid;
}

static int
crc_dropcache_start(struct crc_interface *crc_p, char *res_uuid, int do_sync)
{
	char *log_header = "crc_dropcache_start";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p;
	int rc = 1;

	nid_log_warning("%s: start (uuid:%s, sync:%d) ...", log_header, res_uuid, do_sync);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_chan_head, c_list) {
		nid_log_warning("%s: comparing %s with %s", log_header, chan_p->c_resource, res_uuid);
		if (!strcmp(chan_p->c_resource, res_uuid)) {
			chan_p->c_do_dropcache = 1;
			rc = 0;
			break;
		}
	}

	if (rc) {
		list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comparing %s with %s", log_header, chan_p->c_resource, res_uuid);
			if (!strcmp(chan_p->c_resource, res_uuid)) {
				chan_p->c_do_dropcache = 1;
				rc = 0;
				break;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (!rc) {
		struct nse_interface *nse_p;
		nse_p = &chan_p->c_nse;
		nse_p->ns_op->ns_dropcache_start(nse_p, do_sync);
	}

	return rc;
}

static int
crc_dropcache_stop(struct crc_interface *crc_p, char *res_uuid)
{
	char *log_header = "crc_dropcache_stop";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p;
	int rc = 1;

	nid_log_warning("%s: start (uuid:%s) ...", log_header, res_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_chan_head, c_list) {
		nid_log_warning("%s: comapring %s with %s", log_header, chan_p->c_resource, res_uuid);
		if (!strcmp(chan_p->c_resource, res_uuid)) {
			chan_p->c_do_dropcache = 0;
			rc = 0;
			break;
		}
	}

	if (rc) {
		list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comapring %s with %s", log_header, chan_p->c_resource, res_uuid);
			if (!strcmp(chan_p->c_resource, res_uuid)) {
				chan_p->c_do_dropcache = 0;
				rc = 0;
				break;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (!rc) {
		struct nse_interface *nse_p;
		nse_p = &chan_p->c_nse;
		nse_p->ns_op->ns_dropcache_stop(nse_p);
	} else {
		nid_log_warning("%s: not matched %s", log_header, res_uuid);
	}

	return rc;
}

static int
crc_get_nse_stat(struct crc_interface *crc_p, char *res_uuid, struct nse_stat *nse_stat)
{
	char *log_header = "crc_information_nse_counter";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p;
	int rc = 0, got_it = 0;

	nid_log_warning("%s: start (uuid:%s) ...", log_header, res_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_chan_head, c_list) {
		nid_log_warning("%s: comparing %s with %s", log_header, chan_p->c_resource, res_uuid);
		if (!strcmp(chan_p->c_resource, res_uuid)) {
			got_it = 1;
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comparing %s with %s", log_header, chan_p->c_resource, res_uuid);
			if (!strcmp(chan_p->c_resource, res_uuid)) {
				got_it = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (got_it) {
		struct nse_interface *nse_p;
		nse_p = &chan_p->c_nse;
		nse_p->ns_op->ns_get_stat(nse_p, nse_stat);
	} else {
		nid_log_warning("%s: not matched %s", log_header, res_uuid);
		rc = -1;
	}

	return rc;
}

static void *
crc_nse_traverse_start(struct crc_interface *crc_p, char *chan_uuid)
{
	char *log_header = "crc_nse_traverse_start";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p;
	int got_it = 0;
	void *chan_handle = NULL;

	nid_log_warning("%s: start (uuid:%s) ...", log_header, chan_uuid);
	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_chan_head, c_list) {
		nid_log_warning("%s: comapring %s with %s", log_header, chan_p->c_resource, chan_uuid);
		if (!strcmp(chan_p->c_resource, chan_uuid)) {
			got_it = 1;
			break;
		}
	}

	if (!got_it) {
		list_for_each_entry(chan_p, struct crc_channel, &priv_p->p_inactive_head, c_list) {
			nid_log_warning("%s: comapring %s with %s", log_header, chan_p->c_resource, chan_uuid);
			if (!strcmp(chan_p->c_resource, chan_uuid)) {
				got_it = 1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);

	if (got_it) {
		struct nse_interface *nse_p;
		nse_p = &chan_p->c_nse;
		if (!nse_p->ns_op->ns_traverse_start(nse_p))
			chan_handle = (void *)chan_p;
	} else {
		nid_log_warning("%s: not matched %s", log_header, chan_uuid);
	}

	return chan_handle;
}

static void
crc_nse_traverse_stop(struct crc_interface *crc_p, void *chan_handle)
{
	char *log_header = "crc_nse_traverse_stop";
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p = (struct crc_channel *)chan_handle;
	struct nse_interface *nse_p;

	assert(priv_p);
	assert(chan_p);
	nid_log_warning("%s: start (uuid:%s) ...", log_header, chan_p->c_resource);
	nse_p = &chan_p->c_nse;
	nse_p->ns_op->ns_traverse_stop(nse_p);
}

static int
crc_nse_traverse_next(struct crc_interface *crc_p, void *chan_handle, char *fp_buf, uint64_t *block_index)
{
	struct crc_private *priv_p = crc_p->cr_private;
	struct crc_channel *chan_p = (struct crc_channel *)chan_handle;
	struct nse_interface *nse_p;
	int rc;

	assert(priv_p);
	nse_p = &chan_p->c_nse;
	rc = nse_p->ns_op->ns_traverse_next(nse_p, fp_buf, block_index);
	return rc;
}

void
__load_super_block(struct crc_private *);

static void
crc_recover(struct crc_interface * crc_p)
{
	char *log_header = "crc_recover";
	struct crc_private *priv_p = crc_p->cr_private;
	struct fpn_interface *fpn_p = priv_p->p_fpn;
	struct cdn_interface *cdn_p;
	struct cdn_setup cdn_setup;
	struct blksn_setup blksn_setup;
	struct dsmgr_setup dsmgr_setup;
	struct dsrec_setup dsrec_setup;
	struct cse_interface *cse_p;
	struct cse_setup cse_setup;
	struct fpc_setup fpc_setup;
	struct nse_crc_cbn_setup ncc_setup;
	struct cse_crc_cbn_setup ccc_setup;
	int flags, mode;

	nid_log_warning("%s: start, crc uuid:%s ...", log_header, priv_p->p_uuid);

	flags = O_RDWR | O_SYNC;
	mode = 0600;
	priv_p->p_fd = open(priv_p->p_cachedev, flags, mode);
	if (priv_p->p_fd < 0) {
		nid_log_error("%s: cannot open %s", log_header, priv_p->p_cachedev);
		assert(0);
	}

	nid_log_warning("%s: step 1, crc uuid:%s ...", log_header, priv_p->p_uuid);

	__load_super_block(priv_p);

	nid_log_warning("%s: step 2, crc uuid:%s ...", log_header, priv_p->p_uuid);

	cdn_setup.allocator = priv_p->p_allocator;
	cdn_setup.set_id = ALLOCATOR_SET_CRC_CDN;
	cdn_setup.seg_size = 1024;
	cdn_setup.fp_size = fpn_p->fp_op->fp_get_fp_size(fpn_p);
	cdn_p = &priv_p->p_cdn;
	cdn_initialization(cdn_p, &cdn_setup);

	nid_log_warning("%s: step 3, crc uuid:%s ...", log_header, priv_p->p_uuid);

	blksn_setup.allocator = priv_p->p_allocator;
	blksn_setup.set_id = ALLOCATOR_SET_CRC_BLKSN;
	blksn_setup.seg_size = 1024;
	blksn_initialization(&priv_p->p_blksn, &blksn_setup);

	nid_log_warning("%s: step 4, crc uuid:%s ...", log_header, priv_p->p_uuid);

	dsmgr_setup.allocator = priv_p->p_allocator;
	dsmgr_setup.brn = priv_p->p_brn;
	dsmgr_setup.blksn = &priv_p->p_blksn;
	dsmgr_setup.block_shift = CRC_BLOCK_SHIFT;
	dsmgr_setup.size = priv_p->p_size;
	dsmgr_setup.bitmap = priv_p->p_bitmap;
	dsmgr_initialization(&priv_p->p_dsmgr, &dsmgr_setup);

	nid_log_warning("%s: step 5, crc uuid:%s ...", log_header, priv_p->p_uuid);

	fpc_setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&priv_p->p_fpc, &fpc_setup);

	nid_log_warning("%s: step 6, crc uuid:%s ...", log_header, priv_p->p_uuid);

	dsrec_setup.dsmgr = &priv_p->p_dsmgr;
	dsrec_setup.blksn = &priv_p->p_blksn;
	dsrec_setup.cdn = &priv_p->p_cdn;
	dsrec_setup.rc = priv_p->p_rc;
	dsrec_setup.rc_size = priv_p->p_size;
	dsrec_setup.fpc = &priv_p->p_fpc;
	dsrec_initialization(&priv_p->p_dsrec, &dsrec_setup);

	nid_log_warning("%s: step 7, crc uuid:%s ...", log_header, priv_p->p_uuid);

	cse_p = &priv_p->p_cse;
	cse_setup.allocator = priv_p->p_allocator;
	cse_setup.rc = priv_p->p_rc;
	cse_setup.srn = priv_p->p_srn;
	cse_setup.fpn = priv_p->p_fpn;
	cse_setup.cdn = &priv_p->p_cdn;
	cse_setup.pp = priv_p->p_pp;
	cse_setup.block_shift = priv_p->p_block_shift;
	cse_setup.fpc = &priv_p->p_fpc;
	cse_setup.blksn = &priv_p->p_blksn;
	cse_setup.d_blocks_num = priv_p->p_size;
	cse_initialization(cse_p, &cse_setup);

	nid_log_warning("%s: step 8, crc uuid:%s ...", log_header, priv_p->p_uuid);

	ncc_setup.allocator = priv_p->p_allocator;
	ncc_setup.seg_size = 128;
	ncc_setup.set_id = ALLOCATOR_SET_NSE_CRC_CB;
	nse_crc_cbn_initialization(&priv_p->p_ncc, &ncc_setup);

	nid_log_warning("%s: step 9, crc uuid:%s ...", log_header, priv_p->p_uuid);

	ccc_setup.allocator = priv_p->p_allocator;
	ccc_setup.seg_size = 128;
	ccc_setup.set_id = ALLOCATOR_SET_CSE_CRC_CB;
	cse_crc_cbn_initialization(&priv_p->p_ccc, &ccc_setup);

	nid_log_warning("%s: end, crc uuid:%s ...", log_header, priv_p->p_uuid);
}

struct crc_operations crc_op = {
	.cr_create_channel = crc_create_channel,
	.cr_prepare_channel = crc_prepare_channel,
	.cr_search = crc_search,
	.cr_search_fetch = crc_search_fetch,
	.cr_updatev = crc_updatev,
	.cr_update = crc_update,
	.cr_setup_cache_data = crc_setup_cache_data,
	.cr_setup_cache_mdata = crc_setup_cache_mdata,
	.cr_setup_cache_bmp = crc_setup_cache_bmp,
	.cr_flush_content = crc_flush_content,
	.cr_freespace = crc_freespace,
	.cr_sp_heads_size = crc_sp_heads_size,
	.cr_freespace_dist = crc_freespace_dist,
	.cr_touch_block = crc_touch_block,
	.cr_insert_block = crc_insert_block,
	.cr_read_block = crc_read_block,
	.cr_drop_block = crc_drop_block,
	.cr_set_fp_wgt = crc_set_fp_wgt,
	.cr_get_dsrec_stat = crc_get_dsrec_stat,
	.cr_chan_inactive = crc_chan_inactive,
	.cr_chan_inactive_res = crc_chan_inactive_res,
	.cr_stop = crc_stop,
	.cr_get_uuid = crc_get_uuid,
	.cr_dsbmp_rtree_list = crc_dsbmp_rtree_list,
	.cr_dropcache_start = crc_dropcache_start,
	.cr_dropcache_stop = crc_dropcache_stop,
	.cr_get_nse_stat = crc_get_nse_stat,
	.cr_nse_traverse_start = crc_nse_traverse_start,
	.cr_nse_traverse_stop = crc_nse_traverse_stop,
	.cr_nse_traverse_next = crc_nse_traverse_next,
	.cr_check_fp = crc_check_fp,
	.cr_cse_hit = crc_cse_hit,
	.cr_recover = crc_recover,
};

/*
 * Algorithm:
 * 	Read the super block. If it's not inited, inited the super block
 * 	and invalid all bitmap
 */
void
__load_super_block(struct crc_private *priv_p)
{
	int fd = priv_p->p_fd;
	struct crc_super_header *super_p = &priv_p->p_super;
	int inited = 1;
	uint64_t bmp_size, meta_size;	// in bytes
	off_t bmp_off, meta_off;

	pread(fd, super_p, sizeof(*super_p), 0);
	assert(!super_p->s_blocksz || super_p->s_blocksz == priv_p->p_blocksz);
	assert(!super_p->s_headersz || super_p->s_headersz == CRC_SUPER_BLOCK_SIZE);
	assert(!super_p->s_bmpsz || super_p->s_bmpsz == priv_p->p_bmpsz);
	assert(!super_p->s_metasz || super_p->s_metasz == priv_p->p_metasz);
	assert(!super_p->s_size || super_p->s_size == priv_p->p_size);

	if (!super_p->s_blocksz) {
		/* this super block not inited yet */
		inited = 0;
		super_p->s_blocksz = priv_p->p_blocksz;
		super_p->s_headersz = CRC_SUPER_BLOCK_SIZE;
		super_p->s_bmpsz = priv_p->p_bmpsz;
		super_p->s_metasz = priv_p->p_metasz;
		super_p->s_size = priv_p->p_size;
	}

	bmp_off = super_p->s_headersz;
	bmp_size = super_p->s_bmpsz * super_p->s_blocksz;
	if (!inited) {
		pwrite(fd, (void *)super_p, CRC_SUPER_HEADRE_SIZE, 0);
		priv_p->p_bitmap = x_calloc(bmp_size, sizeof(char));
		pwrite(fd, priv_p->p_bitmap, bmp_size, bmp_off);	// invalid all bitmap
		inited = 1;
	} else {
		priv_p->p_bitmap = x_malloc(bmp_size * sizeof(char));
		pread(fd, priv_p->p_bitmap, bmp_size, bmp_off);
	}

	meta_off = bmp_off + bmp_size;
	meta_size = super_p->s_metasz * super_p->s_blocksz;
	priv_p->p_meta_data = x_malloc(meta_size * sizeof(char));
	pread(fd, priv_p->p_meta_data, meta_size, meta_off);
}

int
crc_initialization(struct crc_interface *crc_p, struct crc_setup *setup)
{
	char *log_header = "crc_initialization";
	struct crc_private *priv_p;
	uint64_t nr_unit, unit_size, unit_dsize, unit_msize, rcachesz;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	crc_p->cr_op = &crc_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	crc_p->cr_private = priv_p;

	strcpy(priv_p->p_uuid, setup->uuid);
	priv_p->p_allocator = setup->allocator;
	priv_p->p_fpn = setup->fpn;
	priv_p->p_brn = setup->brn;
	priv_p->p_rc = setup->rc;
	priv_p->p_srn = setup->srn;
	priv_p->p_pp = setup->pp;
	INIT_LIST_HEAD(&priv_p->p_chan_head);
	INIT_LIST_HEAD(&priv_p->p_inactive_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);

	strcpy(priv_p->p_cachedev, setup->cachedev);
	rcachesz = ((uint64_t)setup->cachedevsz << 20);
	priv_p->p_blocksz = (uint32_t)setup->blocksz;				// likely 4K
	priv_p->p_total_size = rcachesz / priv_p->p_blocksz;				// number of blocks
	while (((int64_t)setup->blocksz) > 1) {

		priv_p->p_block_shift++;
		setup->blocksz >>= 1;
	}

	x_posix_memalign((void **)&priv_p->p_fp_buf, getpagesize(), CRC_MBLOCK_SIZE * CRC_MAX_MBLOCK_FLUSH);
	assert(CRC_SUPER_BLOCK_SIZE % (64*priv_p->p_blocksz) == 0);	// must be multiple of 64 blocks
	priv_p->p_supersz = CRC_SUPER_BLOCK_SIZE / priv_p->p_blocksz;	// likely 1M/4K = 256

	/*
	 * One unit of bitmap is 64 blocks, which can represent 64*block_size*8 data blocks since
	 * one bit represents one block
	 * One unit of meta data is 64*8*CRC_MBLOCK_SIZE blocks which can hold meta data for
	 * 64*block_size*8 data blocks.
	 */
	unit_msize = 64 + 64 * 8 * CRC_MBLOCK_SIZE;	// number of blocks
	unit_dsize = 64 * priv_p->p_blocksz * 8;
	unit_size = unit_msize + unit_dsize;
	nr_unit = 1;
	assert(priv_p->p_total_size > priv_p->p_supersz + unit_msize);
       	while (priv_p->p_total_size > (priv_p->p_supersz + nr_unit * unit_size + unit_msize))
		nr_unit++;
	priv_p->p_bmpsz = 64 * nr_unit;
       	priv_p->p_metasz = (64 * 8 * CRC_MBLOCK_SIZE) * nr_unit;
	priv_p->p_size = priv_p->p_total_size - (priv_p->p_supersz + priv_p->p_bmpsz + priv_p->p_metasz);

	return 0;
}
