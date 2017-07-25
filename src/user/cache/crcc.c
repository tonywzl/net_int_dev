/*
 * rcc.c
 * 	Implemantation	of Read Cache Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_crc_if.h"
#include "crcg_if.h"
#include "crcc_if.h"
#include "crc_if.h"
#include "list.h"
#include "blksn_if.h"

struct crcc_private {
	struct crcg_interface	*p_crcg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
crcc_accept_new_channel(struct crcc_interface *crcc_p, int sfd)
{
	struct crcc_private *priv_p = crcc_p->r_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__crcc_dropcache(struct crcc_private *priv_p, char *msg_buf, struct umessage_crc_dropcache *dc_msg)
{
	char *log_header = "__crcc_dropcache";
	struct umpk_interface *umpk_p = priv_p->p_umpk; 
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)dc_msg;
	int ctype = NID_CTYPE_CRC;

	assert(msghdr->um_req == UMSG_CRC_CMD_DROPCACHE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_CRC_HEADER_LEN);
	if (cmd_len > UMSG_CRC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_CRC_HEADER_LEN, cmd_len - UMSG_CRC_HEADER_LEN);

	umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)dc_msg);

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code); 
	switch (msghdr->um_req_code) {
	case UMSG_CRC_CODE_START:
		nid_log_debug("%s: got UMSG_CRC_CODE_START, uuid:%s",  log_header, dc_msg->um_chan_uuid);
		crcg_p->rg_op->rg_dropcache_start(crcg_p,  dc_msg->um_rc_uuid, dc_msg->um_chan_uuid, 0);
		close(sfd);
		break;

	case UMSG_CRC_CODE_START_SYNC:
		nid_log_debug("%s: got UMSG_CRC_CODE_START, uuid:%s",  log_header, dc_msg->um_chan_uuid);
		crcg_p->rg_op->rg_dropcache_start(crcg_p, dc_msg->um_rc_uuid, dc_msg->um_chan_uuid, 1);
		close(sfd);
		break;

	case UMSG_CRC_CODE_STOP:
		nid_log_debug("%s: got UMSG_CRC_CODE_STOP, uuid:%s",  log_header, dc_msg->um_chan_uuid);
		crcg_p->rg_op->rg_dropcache_stop(crcg_p, dc_msg->um_rc_uuid, dc_msg->um_chan_uuid);
		close(sfd);
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
}

static void
__crcc_information(struct crcc_private *priv_p, char *msg_buf, struct umessage_crc_information *info_msg)
{
	char *log_header = "__crcc_information";
	struct umpk_interface *umpk_p = priv_p->p_umpk; 
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)info_msg;
	int ctype = NID_CTYPE_CRC;
	uint32_t len;
	int i;

	assert(msghdr->um_req == UMSG_CRC_CMD_INFORMATION);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_CRC_HEADER_LEN);
	if (cmd_len > UMSG_CRC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_CRC_HEADER_LEN, cmd_len - UMSG_CRC_HEADER_LEN);

	umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)info_msg);

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code); 
	switch (msghdr->um_req_code) {
	case UMSG_CRC_CODE_FLUSHING:
	  	nid_log_debug("%s: got UMSG_CRC_CODE_FLUSHING, uuid:%s",  log_header, info_msg->um_rc_uuid);
		close(sfd);
		break;


	case UMSG_CRC_CODE_FREE_SPACE:
		nid_log_debug("%s: got UMSG_CRC_CODE_FREE_SPACE, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {
			uint64_t free_space = crcg_p->rg_op->rg_freespace(crcg_p, info_msg->um_rc_uuid);
			char umpk_buf[ 1024 ];

			struct umessage_crc_information_resp_freespace p_rc_free_space_rslt;
			p_rc_free_space_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			p_rc_free_space_rslt.um_num_blocks_free = free_space;
			p_rc_free_space_rslt.um_header.um_req_code = UMSG_CRC_CODE_FREE_SPACE_RSLT;
			umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&p_rc_free_space_rslt) );

			int ret_val = write( sfd, umpk_buf, len );
			nid_log_debug("%s: got UMSG_CRC_CODE_FREE_SPACE, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);
		}
		close(sfd);
		break;


	case UMSG_CRC_CODE_SP_HEADS_SIZE:
		nid_log_debug("%s: got UMSG_CRC_CODE_SP_HEADS_SIZE, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {
			int* p_sp_heads_size = crcg_p->rg_op->rg_sp_heads_size(crcg_p, info_msg->um_rc_uuid);
			char umpk_buf[ 20480 ];

			struct umessage_crc_information_resp_sp_heads_size p_rc_sp_heads_size_rslt;
			p_rc_sp_heads_size_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			for (i=0; i<DSMGR_ALL_SP_HEADS; i++) {
				p_rc_sp_heads_size_rslt.um_sp_heads_size[i] = p_sp_heads_size[i];
			}
			p_rc_sp_heads_size_rslt.um_header.um_req_code = UMSG_CRC_CODE_SP_HEADS_SIZE_RSLT;
			umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&p_rc_sp_heads_size_rslt) );

			int ret_val = write( sfd, umpk_buf, len );
			nid_log_debug("%s: got UMSG_CRC_CODE_SP_HEADS_SIZE, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);
		}
		close(sfd);
		break;

	case UMSG_CRC_CODE_CSE_HIT_CHECK:
		nid_log_debug("%s: got UMSG_CRC_CODE_CSE_HIT_CHECK, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {
			int hit_counter = 0,unhit_counter = 0;
			struct umessage_crc_information_resp_cse_hit p_rc_cse_hit_rslt;
			crcg_p->rg_op->rg_cse_hit(crcg_p, info_msg->um_rc_uuid, &hit_counter, &unhit_counter);
			char umpk_buf[ 1024 ];


			p_rc_cse_hit_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			p_rc_cse_hit_rslt.um_header.um_req_code = UMSG_CRC_CODE_CSE_HIT_CHECK_RSLT;
			p_rc_cse_hit_rslt.um_rc_hit_counter = hit_counter;
			p_rc_cse_hit_rslt.um_rc_unhit_counter = unhit_counter;
			umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&p_rc_cse_hit_rslt) );

			int ret_val = write( sfd, umpk_buf, len );
			nid_log_debug("%s: got UMSG_CRC_CODE_CSE_HIT_CHECK_RSLT, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);
		}
		close(sfd);
		break;

	case UMSG_CRC_CODE_CHECK_FP:
		nid_log_debug("%s: got UMSG_CRC_CODE_CHECK_FP, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {
			unsigned char check_fp_rc = crcg_p->rg_op->rg_check_fp( crcg_p, info_msg->um_rc_uuid);
			char umpk_buf[ 1024 ];

			struct umessage_crc_information_resp_check_fp p_rc_check_fp_rslt;
			p_rc_check_fp_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			p_rc_check_fp_rslt.um_check_fp_rc = check_fp_rc;
			p_rc_check_fp_rslt.um_header.um_req_code = UMSG_CRC_CODE_CHECK_FP_RSLT;
			umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&p_rc_check_fp_rslt) );

			int ret_val = write( sfd, umpk_buf, len );
			nid_log_debug("%s: sent UMSG_CRC_CODE_CHECK_FP_RSLT, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);
		}
		close(sfd);
		break;

	case UMSG_CRC_CODE_FREE_SPACE_DIST:
		nid_log_debug("%s: got UMSG_CRC_CODE_FREE_SPACE DIST, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {

		  struct umessage_crc_information_resp_freespace_dist* p_rc_free_space_dist_rslt = crcg_p->rg_op->rg_freespace_dist(crcg_p, info_msg->um_rc_uuid);

			char* umpk_buf_ss = x_calloc( (p_rc_free_space_dist_rslt->um_num_sizes +1 ) * 20, sizeof( char ) );
			p_rc_free_space_dist_rslt->um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			p_rc_free_space_dist_rslt->um_header.um_req_code = UMSG_CRC_CODE_FREE_SPACE_DIST_RSLT;
			umpk_p->um_op->um_encode( umpk_p, umpk_buf_ss, &len, NID_CTYPE_CRC, (void*)p_rc_free_space_dist_rslt );

			write( sfd, umpk_buf_ss, len );
			free( umpk_buf_ss );
		}
		close(sfd);
		break;

	case UMSG_CRC_CODE_NSE_STAT:
		nid_log_debug("%s: got UMSG_CRC_CODE_NSE_COUNTER DIST, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if (crcg_p) {
			struct umessage_crc_information_resp_nse_stat resp_msg;
			struct umessage_crc_hdr *resp_msghdr = (struct umessage_crc_hdr *)&resp_msg;
			char resp_buf[4096];
			struct nse_stat stat;
			int rc;

			rc = crcg_p->rg_op->rg_get_nse_stat(crcg_p, info_msg->um_rc_uuid, info_msg->um_chan_uuid, &stat);
			if (!rc) {
				resp_msghdr = (struct umessage_crc_hdr *)&resp_msg;
				resp_msghdr->um_req = UMSG_CRC_CMD_INFORMATION; 
				resp_msghdr->um_req_code = UMSG_CRC_CODE_RESP_NSE_STAT;
				resp_msg.um_nse_stat.fp_max_num = stat.fp_max_num;
				resp_msg.um_nse_stat.fp_min_num = stat.fp_min_num;
				resp_msg.um_nse_stat.fp_num = stat.fp_num;
				resp_msg.um_nse_stat.fp_rec_num = stat.fp_rec_num;
				resp_msg.um_nse_stat.hit_num = stat.hit_num;
				resp_msg.um_nse_stat.unhit_num = stat.unhit_num;
				resp_msg.um_rc_uuid_len = info_msg->um_rc_uuid_len;
				strcpy(resp_msg.um_rc_uuid, info_msg->um_rc_uuid);
				resp_msg.um_chan_uuid_len = info_msg->um_chan_uuid_len;
				strcpy(resp_msg.um_chan_uuid, info_msg->um_chan_uuid);

				umpk_p->um_op->um_encode(umpk_p, resp_buf, &len, NID_CTYPE_CRC, (void*)resp_msghdr);

				write(sfd, resp_buf, len);
			}
		}
		close(sfd);
		break;

	case UMSG_CRC_CODE_NSE_DETAIL: {
		struct umessage_crc_information_resp_nse_detail resp_msg;
		struct umessage_crc_hdr *resp_msghdr = (struct umessage_crc_hdr *)&resp_msg;
		char resp_buf[4096], nothing;
		void *chan_handle;
		int nwrite, rc = 0, nw_error = 0;
		struct nse_stat stat;

		nid_log_debug("%s: got UMSG_CRC_CODE_NSE_DETAIL uuid:%s",  log_header, info_msg->um_rc_uuid);
		chan_handle = crcg_p->rg_op->rg_nse_traverse_start(crcg_p, info_msg->um_rc_uuid, info_msg->um_chan_uuid);
		resp_msghdr = (struct umessage_crc_hdr *)&resp_msg;
		resp_msghdr->um_req = UMSG_CRC_CMD_INFORMATION;
		resp_msghdr->um_req_code = UMSG_CRC_CODE_RESP_NSE_DETAIL;
		resp_msg.um_flags = 0;
		if (!chan_handle) {
			nid_log_warning("%s: trvverse service for channel (%s) is busy.",
					log_header, info_msg->um_chan_uuid);
			close(sfd);
			break;
		}

		rc = crcg_p->rg_op->rg_get_nse_stat(crcg_p, info_msg->um_rc_uuid, info_msg->um_chan_uuid, &stat);
		if (!rc) {
			resp_msghdr = (struct umessage_crc_hdr *)&resp_msg;
			resp_msg.um_nse_stat.fp_max_num = stat.fp_max_num;
			resp_msg.um_nse_stat.fp_min_num = stat.fp_min_num;
			resp_msg.um_nse_stat.fp_num = stat.fp_num;
			resp_msg.um_nse_stat.fp_rec_num = stat.fp_rec_num;
			resp_msg.um_nse_stat.hit_num = stat.hit_num;
			resp_msg.um_nse_stat.unhit_num = stat.unhit_num;
			resp_msg.um_rc_uuid_len = info_msg->um_rc_uuid_len;
			strcpy(resp_msg.um_rc_uuid, info_msg->um_rc_uuid);
			resp_msg.um_chan_uuid_len = info_msg->um_chan_uuid_len;
			strcpy(resp_msg.um_chan_uuid, info_msg->um_chan_uuid);
			resp_msg.um_flags = UMSG_CRC_FLAG_START;

			umpk_p->um_op->um_encode(umpk_p, resp_buf, &len, NID_CTYPE_CRC, (void*)resp_msghdr);

			write(sfd, resp_buf, len);
		}

		resp_msg.um_flags = 0;
		while (1) {
			rc = crcg_p->rg_op->rg_nse_traverse_next(crcg_p, info_msg->um_rc_uuid,
				chan_handle, resp_msg.um_fp, &resp_msg.um_block_index);
			if (rc)
				break;

			nid_log_warning("%s: block_index:%lu", log_header, resp_msg.um_block_index);
			umpk_p->um_op->um_encode(umpk_p, resp_buf, &len, NID_CTYPE_CRC, (void*)&resp_msg);
			nwrite = write(sfd, resp_buf, len);
			if (nwrite != (int)len) {
				nw_error = 1;
				break;
			}
		}
		crcg_p->rg_op->rg_nse_traverse_stop(crcg_p, info_msg->um_rc_uuid, chan_handle);

		if (!nw_error) {
			resp_msg.um_flags = UMSG_CRC_FLAG_END;
			umpk_p->um_op->um_encode(umpk_p, resp_buf, &len, NID_CTYPE_CRC, (void*)&resp_msg);
			write(sfd, resp_buf, len);
		}

		read(sfd, &nothing, 1);
		close(sfd);
		break;
	}

	case UMSG_CRC_CODE_DSBMP_RTREE:
		nid_log_debug("%s: got UMSG_CRC_CODE_DSBMP_RTREE, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {
		  	// write the header first
		  	char umpk_buf[ 100 ];
			memset( umpk_buf, 0, 100 * sizeof( char ) );
			struct umessage_crc_information_resp_dsbmp_rtree_node rc_dsbmp_rtree_node_rslt;
			rc_dsbmp_rtree_node_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
		       	rc_dsbmp_rtree_node_rslt.um_header.um_req_code = UMSG_CRC_CODE_DSBMP_RTREE_RSLT;

			umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&rc_dsbmp_rtree_node_rslt) );
			int ret_val = write( sfd, umpk_buf, len );
			nid_log_debug("%s: send UMSG_CRC_CODE_DSBMP_RTREE_RSLT, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);

			// get and send the nodes
			memset( umpk_buf, 0, 100 * sizeof( char ) );
			struct umessage_crc_information_resp_dsbmp_bsn rc_bsn_rslt;
			rc_bsn_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			rc_bsn_rslt.um_header.um_req_code = UMSG_CRC_CODE_BSN_RSLT;

			struct list_head* dsbmp_rtree_list = crcg_p->rg_op->rg_dsbmp_rtree_list( crcg_p, info_msg->um_rc_uuid);
			struct block_size_node* bsnp, *bsnp_temp;
			list_for_each_entry_safe( bsnp, bsnp_temp, struct block_size_node, dsbmp_rtree_list, bsn_olist ) {
			  	rc_bsn_rslt.um_offset = bsnp->bsn_index;
			  	rc_bsn_rslt.um_size = bsnp->bsn_size;
				umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&rc_bsn_rslt) );
				ret_val = write(sfd, umpk_buf, len );
				nid_log_debug("%s: send UMSG_CRC_CODE_BSN_RSLT, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);

			}

			// send trailer packet to signify the end of the series
			memset( umpk_buf, 0, 100 * sizeof( char ) );
			struct umessage_crc_information_resp_dsbmp_rtree_node_finish rc_dsbmp_rtree_node_end_rslt;
			rc_dsbmp_rtree_node_end_rslt.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
		       	rc_dsbmp_rtree_node_end_rslt.um_header.um_req_code = UMSG_CRC_CODE_DSBMP_RTREE_END_RSLT;

			umpk_p->um_op->um_encode( umpk_p, umpk_buf, &len, NID_CTYPE_CRC, (void*)(&rc_dsbmp_rtree_node_end_rslt) );
			ret_val = write( sfd, umpk_buf, len );
			nid_log_debug("%s: sent UMSG_CRC_CODE_DSBMP_RTREE_END_RSLT, uuid:%s, wrote %d bytes",  log_header, info_msg->um_rc_uuid, ret_val);

		}
		close(sfd);
		break;

	case UMSG_CRC_CODE_DSREC_STAT:
		nid_log_debug("%s: got UMSG_CRC_CODE_DSREC_STAT, uuid:%s",  log_header, info_msg->um_rc_uuid);
		if ( NULL != crcg_p ) {
			struct umessage_crc_information_resp_dsrec_stat dsrec_stat;
			dsrec_stat.um_header.um_req = UMSG_CRC_CMD_INFORMATION;
			dsrec_stat.um_header.um_req_code = UMSG_CRC_CODE_DSREC_STAT;
			dsrec_stat.um_dsrec_stat = crcg_p->rg_op->rg_get_dsrec_stat(crcg_p, info_msg->um_rc_uuid);
			msghdr = (struct umessage_crc_hdr*)&dsrec_stat;
			umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			assert(cmd_len < NID_MAX_CMD_RESP);
			write(sfd, msg_buf, cmd_len);
		}
		close(sfd);
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
}

static void
__crcc_setwgt(struct crcc_private *priv_p, char *msg_buf, struct umessage_crc_setwgt *sp_msg)
{
	char *log_header = "__crcc_setwgt";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)sp_msg;
	int ctype = NID_CTYPE_CRC;

	assert(msghdr->um_req == UMSG_CRC_CMD_SETWGT);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_CRC_HEADER_LEN);
	if (cmd_len > UMSG_CRC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_CRC_HEADER_LEN, cmd_len - UMSG_CRC_HEADER_LEN);

	umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)sp_msg);

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);
	switch (msghdr->um_req_code) {
	case UMSG_CRC_CODE_WGT:
		nid_log_debug("%s: got UMSG_CRC_CODE_WGT, uuid:%s",  log_header, sp_msg->um_rc_uuid);
		crcg_p->rg_op->rg_set_fp_wgt(crcg_p, sp_msg->um_rc_uuid, sp_msg->um_wgt);
		close(sfd);
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}

}

static void
__crcc_add(struct crcc_private *priv_p, char *msg_buf, struct umessage_crc_add *add_msg)
{
	char *log_header = "__crcc_add";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)add_msg;
	int ctype = NID_CTYPE_CRC;
	struct umessage_crc_add_resp add_resp;
	char *crc_uuid, *pp_name, *cache_device;
	int cache_size, rc;

	assert(msghdr->um_req == UMSG_CRC_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_CRC_HEADER_LEN);
	if (cmd_len > UMSG_CRC_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_CRC_HEADER_LEN, cmd_len - UMSG_CRC_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, (void *)add_msg);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);
	crc_uuid = add_msg->um_rc_uuid;
	pp_name = add_msg->um_pp_name;
	cache_device = add_msg->um_cache_device;
	cache_size = (int)add_msg->um_cache_size;
	msghdr = (struct umessage_crc_hdr *)&add_resp;
	msghdr->um_req = UMSG_CRC_CMD_ADD;
	msghdr->um_req_code = UMSG_CRC_CODE_RESP_ADD;
	memcpy(add_resp.um_rc_uuid, add_msg->um_rc_uuid, add_msg->um_rc_uuid_len);
	add_resp.um_rc_uuid_len  = add_msg->um_rc_uuid_len;
	add_resp.um_resp_code = crcg_p->rg_op->rg_add_crc(crcg_p, crc_uuid, pp_name, cache_device, cache_size);
	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);

out:
	close(sfd);
}

static void
__crcc_display(struct crcc_private *priv_p, char *msg_buf, struct umessage_crc_display *dis_msg)
{
	char *log_header = "__crcc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct crcg_interface *crcg_p = priv_p->p_crcg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_crc_hdr *msghdr;
	int ctype = NID_CTYPE_CRC, rc;
	char nothing_back;
	struct umessage_crc_display_resp dis_resp;
	struct rc_interface *crc_p;
	struct crc_setup *crc_setup_p;
	int *working_crc = NULL, cur_index = 0;
	int num_crc = 0, num_working_crc = 0, get_it = 0;
	int i;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_crc_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_CRC_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_CRC_HEADER_LEN);

	if (cmd_len > UMSG_CRC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_CRC_HEADER_LEN, cmd_len - UMSG_CRC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, crc_uuid: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_rc_uuid);

	switch (msghdr->um_req_code){
	case UMSG_CRC_CODE_S_DISP:
		if (dis_msg->um_rc_uuid == NULL)
			goto out;
		crc_setup_p = crcg_p->rg_op->rg_get_all_crc_setup(crcg_p, &num_crc);
		if (crc_setup_p == NULL)
			goto out;
		for (i = 0; i < num_crc; i++, crc_setup_p++) {
			if (!strcmp(crc_setup_p->uuid ,dis_msg->um_rc_uuid)) {
				get_it = 1;
				break;
			}
		}
		if (get_it) {
			dis_resp.um_rc_uuid_len = strlen(crc_setup_p->uuid);
			memcpy(dis_resp.um_rc_uuid, crc_setup_p->uuid, dis_resp.um_rc_uuid_len);
			dis_resp.um_pp_name_len = strlen(crc_setup_p->pp_name);
			memcpy(dis_resp.um_pp_name, crc_setup_p->pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_cache_device_len = strlen(crc_setup_p->cachedev);
			memcpy(dis_resp.um_cache_device, crc_setup_p->cachedev, dis_resp.um_cache_device_len);
			dis_resp.um_cache_size = crc_setup_p->cachedevsz;
			dis_resp.um_block_size = crc_setup_p->blocksz;

			msghdr = (struct umessage_crc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_CRC_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_CRC_CODE_S_DISP_ALL:
		crc_setup_p = crcg_p->rg_op->rg_get_all_crc_setup(crcg_p, &num_crc);
		for (i = 0; i < num_crc; i++, crc_setup_p++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			dis_resp.um_rc_uuid_len = strlen(crc_setup_p->uuid);
			memcpy(dis_resp.um_rc_uuid, crc_setup_p->uuid, dis_resp.um_rc_uuid_len);
			dis_resp.um_pp_name_len = strlen(crc_setup_p->pp_name);
			memcpy(dis_resp.um_pp_name, crc_setup_p->pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_cache_device_len = strlen(crc_setup_p->cachedev);
			memcpy(dis_resp.um_cache_device, crc_setup_p->cachedev, dis_resp.um_cache_device_len);
			dis_resp.um_cache_size = crc_setup_p->cachedevsz;
			dis_resp.um_block_size = crc_setup_p->blocksz;
			msghdr = (struct umessage_crc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_CRC_CODE_S_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_crc_hdr *)&dis_resp;
		msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_CRC_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_CRC_CODE_W_DISP:
		if (dis_msg->um_rc_uuid == NULL)
			goto out;
		crc_p = crcg_p->rg_op->rg_search_crc(crcg_p, dis_msg->um_rc_uuid);

		if (crc_p == NULL)
			goto out;

		crc_setup_p = crcg_p->rg_op->rg_get_all_crc_setup(crcg_p, &num_crc);
		for (i = 0; i <num_crc; i++, crc_setup_p++) {
			if (!strcmp(crc_setup_p->uuid, dis_msg->um_rc_uuid)) {
				dis_resp.um_rc_uuid_len = strlen(crc_setup_p->uuid);
				memcpy(dis_resp.um_rc_uuid, crc_setup_p->uuid, dis_resp.um_rc_uuid_len);
				dis_resp.um_pp_name_len = strlen(crc_setup_p->pp_name);
				memcpy(dis_resp.um_pp_name, crc_setup_p->pp_name, dis_resp.um_pp_name_len);
				dis_resp.um_cache_device_len = strlen(crc_setup_p->cachedev);
				memcpy(dis_resp.um_cache_device, crc_setup_p->cachedev, dis_resp.um_cache_device_len);
				dis_resp.um_cache_size = crc_setup_p->cachedevsz;
				dis_resp.um_block_size = crc_setup_p->blocksz;

				msghdr = (struct umessage_crc_hdr *)&dis_resp;
				msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_CRC_CODE_W_RESP_DISP;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				read(sfd, &nothing_back, 1);
				break;
			}
		}
		break;

	case UMSG_CRC_CODE_W_DISP_ALL:
		crc_setup_p = crcg_p->rg_op->rg_get_all_crc_setup(crcg_p, &num_crc);
		working_crc = crcg_p->rg_op->rg_get_working_crc_index(crcg_p, &num_working_crc);

		for (i = 0; i < num_working_crc; i++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			cur_index = working_crc[i];
			dis_resp.um_rc_uuid_len = strlen(crc_setup_p[cur_index].uuid);
			memcpy(dis_resp.um_rc_uuid, crc_setup_p[cur_index].uuid, dis_resp.um_rc_uuid_len);
			dis_resp.um_pp_name_len = strlen(crc_setup_p[cur_index].pp_name);
			memcpy(dis_resp.um_pp_name, crc_setup_p[cur_index].pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_cache_device_len = strlen(crc_setup_p[cur_index].cachedev);
			memcpy(dis_resp.um_cache_device, crc_setup_p[cur_index].cachedev, dis_resp.um_cache_device_len);
			dis_resp.um_cache_size = crc_setup_p[cur_index].cachedevsz;
			dis_resp.um_block_size = crc_setup_p[cur_index].blocksz;

			msghdr = (struct umessage_crc_hdr *)&dis_resp;
			msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_CRC_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_crc_hdr *)&dis_resp;
		msghdr->um_req = UMSG_CRC_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_CRC_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
out:
	free(working_crc);
	close(sfd);
}

static void
__crcc_hello(struct crcc_private *priv_p, char *msg_buf, struct umessage_crc_hello *hello_msg)
{
	char *log_header = "__crcc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_crc_hdr *msghdr;
	int ctype = NID_CTYPE_CRC, rc;
	char nothing_back;
	struct umessage_crc_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_crc_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_CRC_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_CRC_HEADER_LEN);

	if (cmd_len > UMSG_CRC_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_CRC_HEADER_LEN, cmd_len - UMSG_CRC_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_CRC_CODE_HELLO:
		msghdr = (struct umessage_crc_hdr *)&hello_resp;
		msghdr->um_req = UMSG_CRC_CMD_HELLO;
		msghdr->um_req_code = UMSG_CRC_CODE_RESP_HELLO;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}

		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
out:
	close(sfd);

}

static void
crcc_do_channel(struct crcc_interface *crcc_p, struct scg_interface *scg_p)
{
	char *log_header = "crcc_do_channel";
	struct crcc_private *priv_p = (struct crcc_private *)crcc_p->r_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_crc wc_msg; 
	struct umessage_crc_hdr *msghdr = (struct umessage_crc_hdr *)&wc_msg;

	util_nw_read_n(sfd, msg_buf, UMSG_CRC_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;

	if (!priv_p->p_crcg) {
		nid_log_warning("%s support_crc should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_CRC_CMD_DROPCACHE:
		__crcc_dropcache(priv_p, msg_buf, (struct umessage_crc_dropcache *)msghdr);
		break;

	case UMSG_CRC_CMD_INFORMATION:
		__crcc_information(priv_p, msg_buf, (struct umessage_crc_information *)msghdr);
		break;

	case UMSG_CRC_CMD_SETWGT:
		__crcc_setwgt(priv_p, msg_buf, (struct umessage_crc_setwgt *)msghdr);
		break;

	case UMSG_CRC_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__crcc_add(priv_p, msg_buf, (struct umessage_crc_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_CRC_CMD_DISPLAY:
		__crcc_display(priv_p, msg_buf, (struct umessage_crc_display *)msghdr);
		break;

	case UMSG_CRC_CMD_HELLO:
		__crcc_hello(priv_p, msg_buf, (struct umessage_crc_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
		close(sfd);
	}
}

static void
crcc_cleanup(struct crcc_interface *crcc_p)
{
	nid_log_debug("crcc_cleanup start, crcc_p:%p", crcc_p);
	if (crcc_p->r_private != NULL) {
		free(crcc_p->r_private);
		crcc_p->r_private = NULL;
	}
}

struct crcc_operations crcc_op = {
	.crcc_accept_new_channel = crcc_accept_new_channel,
	.crcc_do_channel = crcc_do_channel,
	.crcc_cleanup = crcc_cleanup,
};

int
crcc_initialization(struct crcc_interface *crcc_p, struct crcc_setup *setup)
{
	char *log_header = "crcc_initialization";
	struct crcc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	crcc_p->r_private = priv_p;
	crcc_p->r_op = &crcc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_crcg = setup->crcg;
	return 0;
}
