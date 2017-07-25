/*
 * mpk.c
 * 	Implementation of Message Packaging Module
 */

#ifdef __KERNEL__
//#include <asm/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#else
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "pp2_if.h"
#endif

#include "nid_log.h"
#include "nid_shared.h"
#include "mpk_if.h"

struct mpk_private {
	int p_type;
#ifndef __KERNEL__
	struct pp2_interface *p_pp2;
	char p_do_pp2;
#endif
};

static int
_mpk_encode_type_0(char *mpk_buf, int *size, struct nid_message_hdr *msghdr_p)
{
	char *log_header = "_mpk_encode_type_0";
	struct nid_message *msg_p = (struct nid_message *)msghdr_p;
	char *p = mpk_buf;
	int len = 0, rc = 0;
	uint16_t sub_len;

	*p++ = msg_p->m_req;
	len++;
	*p++ = msg_p->m_req_code;
	len++;
	p += 4;
	len += 4;
	switch (msg_p->m_req) {
	case NID_REQ_BIO:
		rc = encode_type_bio(p, &len, msghdr_p);
		break;

	case NID_REQ_DOA:
		rc = encode_type_doa(p, &len, msghdr_p);
		break;

	case NID_REQ_DELETE_DEVICE:
	case NID_REQ_KEEPALIVE:
	case NID_REQ_RECOVER:
	case NID_REQ_INIT_DEVICE:
	case NID_REQ_STAT:
	case NID_REQ_STAT_SAC:
	case NID_REQ_STAT_WTP:
	case NID_REQ_STAT_IO:
	case NID_REQ_AGENT_STOP:
	case NID_REQ_DELETE:
	case NID_REQ_IOERROR_DEVICE:
	case NID_REQ_CHECK_AGENT:
	case NID_REQ_UPGRADE:
	case NID_REQ_CHECK_SERVER:
	case NID_REQ_STAT_BOC:
	case NID_REQ_CHANNEL:
	case NID_REQ_EXECUTOR:
	case NID_REQ_SET_HOOK:
	case NID_REQ_CHECK_DRIVER:

		if (msg_p->m_has_cmd) {
			*p++ = NID_ITEM_HOOK_CMD;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_cmd);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_alevel) {
			*p++ = NID_ITEM_ALEVEL;
			len++;
			*p++ = msg_p->m_alevel;
			len++;
		}

		if (msg_p->m_has_rcounter) {
			*p++ = NID_ITEM_RCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_rcounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_r_rcounter) {
			*p++ = NID_ITEM_RRCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_r_rcounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_wcounter) {
			*p++ = NID_ITEM_WCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_wcounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_wreadycounter) {
			*p++ = NID_ITEM_WREADYCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_wreadycounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_r_wcounter) {
			*p++ = NID_ITEM_RWCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_r_wcounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_kcounter) {
			*p++ = NID_ITEM_KCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_kcounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_r_kcounter) {
			*p++ = NID_ITEM_RKCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_r_kcounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_recv_sequence) {
			*p++ = NID_ITEM_RSEQUENCE;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_recv_sequence);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_wait_sequence) {
			*p++ = NID_ITEM_WSEQUENCE;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_wait_sequence);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_srv_status) {
			*p++ = NID_ITEM_SRVSTATUS;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_srv_status);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_state) {
			*p++ = NID_ITEM_CPOS;
			len++;
			*p++ = msg_p->m_state;
			len++;
		}

		if (msg_p->m_has_devminor) {
			*p++ = NID_ITEM_DEV_MINOR;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_devminor);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_devname) {
			*p++ = NID_ITEM_DEVNAME;
			len++;
			sub_len = msg_p->m_devnamelen;
			*(uint16_t *)p = x_htobe16(sub_len);
			p += 2;
			len += 2;
			memcpy(p, msg_p->m_devname, sub_len);
			p += sub_len;
			len += sub_len;

		}

		if (msg_p->m_has_dsfd) {
			*p++ = NID_ITEM_DSOCK;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_dsfd);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_rsfd) {
			*p++ = NID_ITEM_RSOCK;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_rsfd);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_blksize) {
			*p++ = NID_ITEM_SIZE_BLOCK;
			len++;
			*(uint64_t *)p = x_htobe32(msg_p->m_blksize);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_size) {
			*p++ = NID_ITEM_SIZE;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_size);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_uuid) {
			*p++ = NID_ITEM_UUID;
			len++;
			sub_len = msg_p->m_uuidlen;
			*(uint16_t *)p = x_htobe16(sub_len);
			p += 2;
			len += 2;
			memcpy(p, msg_p->m_uuid, sub_len);
			p += sub_len;
			len += sub_len;
		}

		if (msg_p->m_has_ip) {
			*p++ = NID_ITEM_IP;
			len++;
			sub_len = msg_p->m_iplen;
			*(uint8_t *)p = sub_len;
			p += 1;
			len += 1;
			memcpy(p, msg_p->m_ip, sub_len);
			p += sub_len;
			len += sub_len;
			nid_log_debug("encode ipname= %s", msg_p->m_ip );
		}

		if (msg_p->m_has_nused) {
			*p++ = NID_ITEM_NUSED;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_nused);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_nfree) {
			*p++ = NID_ITEM_NFREE;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_nfree);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_nnofree) {
			*p++ = NID_ITEM_NNOFREE;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_nnofree);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_nworker) {
			*p++ = NID_ITEM_NWORKER;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_nworker);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_nmaxworker) {
			*p++ = NID_ITEM_NMAXWORKER;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_nmaxworker);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_live_time) {
			*p++ = NID_ITEM_TIME;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_live_time);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_code) {
			*p++ = NID_ITEM_CODE;
			len++;
			*p++ = msg_p->m_code;
			len++;
		}
		if (msg_p->m_has_upgrade_force) {
			*p++ = NID_ITEM_UPGRADE_FORCE;
			len++;
			*p++ = msg_p->m_upgrade_force;
			len++;
		}

		if (msg_p->m_has_rtree_segsz) {
			*p++ = NID_ITEM_RTREE_SEGSZ;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_rtree_segsz);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_rtree_nseg) {
			*p++ = NID_ITEM_RTREE_NSEG;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_rtree_nseg);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_rtree_nfree) {
			*p++ = NID_ITEM_RTREE_NFREE;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_rtree_nfree);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_rtree_nused) {
			*p++ = NID_ITEM_RTREE_NUSED;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_rtree_nused);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_btn_segsz) {
			*p++ = NID_ITEM_BTN_SEGSZ;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_btn_segsz);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_btn_nseg) {
			*p++ = NID_ITEM_BTN_NSEG;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_btn_nseg);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_btn_nfree) {
			*p++ = NID_ITEM_BTN_NFREE;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_btn_nfree);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_btn_nused) {
			*p++ = NID_ITEM_BTN_NUSED;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_btn_nused);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_io_type_bio) {
			*p++ = NID_ITEM_IO_BIO;
			len++;
			*(uint8_t *)p = msg_p->m_io_type_bio;
			p += 1;
			len += 1;
		}

		if (msg_p->m_has_block_occupied) {
			*p++ = NID_ITEM_IO_OCCUPIED;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_block_occupied);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_flush_nblocks) {
			*p++ = NID_ITEM_IO_NBLOCKS;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_flush_nblocks);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_cur_write_block) {
			*p++ = NID_ITEM_IO_WBLOCK;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_cur_write_block);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_cur_flush_block) {
			*p++ = NID_ITEM_IO_FBLOCK;
			len++;
			*(uint16_t *)p = x_htobe16(msg_p->m_cur_flush_block);
			len += 2;
			p += 2;
		}

		if (msg_p->m_has_seq_flushed) {
			*p++ = NID_ITEM_IO_SFLUSHED;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_seq_flushed);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_rec_seq_flushed) {
			*p++ = NID_ITEM_IO_RECSFLUSHED;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_rec_seq_flushed);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_seq_assigned) {
			*p++ = NID_ITEM_IO_SASSIGNED;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_seq_assigned);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_buf_avail) {
			*p++ = NID_ITEM_BUF_AVAIL;
			len++;
			*(uint32_t *)p = x_htobe32(msg_p->m_buf_avail);
			len += 4;
			p += 4;
		}

		if (msg_p->m_has_io_type_rio) {
			*p++ = NID_ITEM_IO_RIO;
			len++;
			*(uint8_t *)p = msg_p->m_io_type_rio;
			p += 1;
			len += 1;
		}

		if (msg_p->m_has_rreadycounter) {
			*p++ = NID_ITEM_RREADYCOUNTER;
			len++;
			*(uint64_t *)p = x_htobe64(msg_p->m_rreadycounter);
			len += 8;
			p += 8;
		}

		if (msg_p->m_has_sfd) {
			*p++ = NID_ITEM_VERSION_SFD;
			len++;
			*(uint32_t *)p = msg_p->m_sfd;
			p += 4;
			len += 4;
		}

		if (msg_p->m_has_version) {
			*p++ = NID_ITEM_VERSION;
			len++;
			sub_len = msg_p->m_versionlen;
			*(uint8_t *)p = sub_len;
			p += 1;
			len += 1;
			memcpy(p, msg_p->m_version, sub_len);
			p += sub_len;
			len += sub_len;
		}

		break;

	default:
		nid_log_error("%s: got unknown req (req:%d, code:%d)",
			log_header, msg_p->m_req, msg_p->m_req_code);
		len = 0;
		rc = -1;
		break;
	}

	if (!rc) {
		p = mpk_buf + 2;
		*(uint32_t *)p = x_htobe32(len);
		*size = len;
		nid_log_debug("len=%d",len);
	}
	return rc;
}

static int
mpk_encode(struct mpk_interface *mpk_p, char *mpk_buf, int *size, struct nid_message_hdr *msghdr_p)
{
	struct mpk_private *priv_p = (struct mpk_private *)mpk_p->m_private;
	if (priv_p->p_type == 0)
		return _mpk_encode_type_0(mpk_buf, size, msghdr_p);
	else
		return -1;
}

static int
_mpk_decode_type_0(char *mpk_buf, int *len, struct nid_message_hdr *reshdr_p)
{
	char *log_header = "_mpk_decode_type_0";
	struct nid_message *res_p = (struct nid_message *)reshdr_p;
	char *p = mpk_buf;
	int left, fall_through, to_consume = 0, rc = 0;
	int8_t item;

	res_p->m_req = *p++;
	res_p->m_req_code = *p++;
	switch (res_p->m_req) {
	case NID_REQ_BIO:
		to_consume = x_be32toh(*(uint32_t *)p);
		left = to_consume - 2;
		p += 4;
		left -= 4;
		rc = decode_type_bio(p, &left, reshdr_p);
		break;

	case NID_REQ_DOA:
		to_consume = x_be32toh(*(uint32_t *)p);
		left = to_consume - 2;
		p += 4;
		left -= 4;
		rc = decode_type_doa (p, &left, reshdr_p);
		break;

	case NID_REQ_DELETE_DEVICE:
	case NID_REQ_INIT_DEVICE:
	case NID_REQ_IOERROR_DEVICE:
	case NID_REQ_KEEPALIVE:
	case NID_REQ_RECOVER:
	case NID_REQ_STAT:
	case NID_REQ_STAT_SAC:
	case NID_REQ_STAT_WTP:
	case NID_REQ_STAT_IO:
	case NID_REQ_AGENT_STOP:
	case NID_REQ_DELETE:
	case NID_REQ_CHECK_AGENT:
	case NID_REQ_UPGRADE:
	case NID_REQ_CHECK_SERVER:
	case NID_REQ_STAT_BOC:
	case NID_REQ_CHANNEL:
	case NID_REQ_EXECUTOR:
	case NID_REQ_SET_HOOK:
	case NID_REQ_CHECK_DRIVER:
		to_consume = x_be32toh(*(uint32_t *)p);
		left = to_consume - 2;
		p += 4;
		left -= 4;

		while (left && !rc) {
			left--;
			fall_through = 0;
			item = *p;
			switch (*p++) {
			case NID_ITEM_HOOK_CMD:
				res_p->m_cmd= x_be32toh(*(uint32_t *)p);
				res_p->m_has_cmd = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_ALEVEL:
				res_p->m_alevel = *(uint8_t *)p;
				res_p->m_has_alevel = 1;
				p += 1;
				left -= 1;
				break;

			case NID_ITEM_CPOS:
				res_p->m_state = *(uint8_t *)p;
				res_p->m_has_state = 1;
				p += 1;
				left -= 1;
				break;

			case NID_ITEM_DEV_MINOR:
				res_p->m_devminor = x_be32toh(*(uint32_t *)p);
				res_p->m_has_devminor = 1;
				fall_through = 1;
			case NID_ITEM_SIZE_BLOCK:
				if (!fall_through) {
					res_p->m_blksize = x_be32toh(*(int32_t *)p);
					res_p->m_has_blksize = 1;
					fall_through = 1;
				}
			case NID_ITEM_DSOCK:
				if (!fall_through) {
					res_p->m_dsfd = x_be32toh(*(int32_t *)p);
					res_p->m_has_dsfd = 1;
					fall_through = 1;
				}
			case NID_ITEM_RSOCK:
				if (!fall_through) {
					res_p->m_rsfd = x_be32toh(*(int32_t *)p);
					res_p->m_has_rsfd = 1;
				}
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_DEVNAME:
				res_p->m_devnamelen = x_be16toh(*(int16_t *)p);
				p += 2;
				left -= 2;
				res_p->m_devname = p;
				res_p->m_has_devname = 1;
				p += res_p->m_devnamelen;
				left -= res_p->m_devnamelen;

				break;

			case NID_ITEM_NFREE:
				res_p->m_nfree = x_be16toh(*(int16_t *)p);
				res_p->m_has_nfree = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_NNOFREE:
				res_p->m_nnofree = x_be16toh(*(int16_t *)p);
				res_p->m_has_nnofree = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_NUSED:
				res_p->m_nused = x_be16toh(*(int16_t *)p);
				res_p->m_has_nused = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_NWORKER:
				res_p->m_nworker = x_be16toh(*(int16_t *)p);
				res_p->m_has_nworker = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_NMAXWORKER:
				res_p->m_nmaxworker = x_be16toh(*(int16_t *)p);
				res_p->m_has_nmaxworker = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_KCOUNTER:
				res_p->m_kcounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_kcounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_RKCOUNTER:
				res_p->m_r_kcounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_r_kcounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_RCOUNTER:
				res_p->m_rcounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_rcounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_RRCOUNTER:
				res_p->m_r_rcounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_r_rcounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_WCOUNTER:
				res_p->m_wcounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_wcounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_WREADYCOUNTER:
				res_p->m_wreadycounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_wreadycounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_RWCOUNTER:
				res_p->m_r_wcounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_r_wcounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_RSEQUENCE:
				res_p->m_recv_sequence = x_be64toh(*(int64_t *)p);
				res_p->m_has_recv_sequence = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_SRVSTATUS:
				res_p->m_srv_status = x_be32toh(*(int32_t *)p);
				res_p->m_has_srv_status = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_WSEQUENCE:
				res_p->m_wait_sequence = x_be64toh(*(int64_t *)p);
				res_p->m_has_wait_sequence = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_UUID:
				res_p->m_uuidlen = x_be16toh(*(int16_t *)p);
				p += 2;
				left -= 2;
				res_p->m_uuid = p;
				res_p->m_has_uuid = 1;
				p += res_p->m_uuidlen;
				left -= res_p->m_uuidlen;
				break;

			case NID_ITEM_IP:
				res_p->m_iplen = *p;
				p += 1;
				left -= 1;
				res_p->m_ip = p;
				res_p->m_has_ip = 1;
				p += res_p->m_iplen;
				left -= res_p->m_iplen;
				break;

			case NID_ITEM_SIZE:
				res_p->m_size = x_be64toh(*(int64_t *)p);
				res_p->m_has_size = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_TIME:
				res_p->m_live_time = x_be32toh(*(int32_t *)p);
				res_p->m_has_live_time = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_CODE:
				res_p->m_code= *p;
				res_p->m_has_code = 1;
				p++;
				left--;
				break;
			case NID_ITEM_UPGRADE_FORCE:
				res_p->m_has_upgrade_force = 1;
				res_p->m_upgrade_force = *p;
				p++;
				left--;
				break;

			case NID_ITEM_RTREE_SEGSZ:
				res_p->m_rtree_segsz = x_be32toh(*(uint32_t *)p);
				res_p->m_has_rtree_segsz = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_RTREE_NSEG:
				res_p->m_rtree_nseg = x_be32toh(*(uint32_t *)p);
				res_p->m_has_rtree_nseg = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_RTREE_NFREE:
				res_p->m_rtree_nfree = x_be32toh(*(uint32_t *)p);
				res_p->m_has_rtree_nfree = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_RTREE_NUSED:
				res_p->m_rtree_nused = x_be32toh(*(uint32_t *)p);
				res_p->m_has_rtree_nused = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_BTN_SEGSZ:
				res_p->m_btn_segsz = x_be32toh(*(uint32_t *)p);
				res_p->m_has_btn_segsz = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_BTN_NSEG:
				res_p->m_btn_nseg = x_be32toh(*(uint32_t *)p);
				res_p->m_has_btn_nseg = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_BTN_NFREE:
				res_p->m_btn_nfree = x_be32toh(*(uint32_t *)p);
				res_p->m_has_btn_nfree = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_BTN_NUSED:
				res_p->m_btn_nused = x_be32toh(*(uint32_t *)p);
				res_p->m_has_btn_nused = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_IO_BIO:
				res_p->m_io_type_bio = *(uint8_t *)p;
				res_p->m_has_io_type_bio = 1;
				p += 1;
				left -= 1;
				break;

			case NID_ITEM_IO_OCCUPIED:
				res_p->m_block_occupied = x_be16toh(*(int16_t *)p);
				res_p->m_has_block_occupied = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_IO_NBLOCKS:
				res_p->m_flush_nblocks = x_be16toh(*(int16_t *)p);
				res_p->m_has_flush_nblocks = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_IO_WBLOCK:
				res_p->m_cur_write_block= x_be16toh(*(int16_t *)p);
				res_p->m_has_cur_write_block = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_IO_FBLOCK:
				res_p->m_cur_flush_block = x_be16toh(*(int16_t *)p);
				res_p->m_has_cur_flush_block = 1;
				p += 2;
				left -= 2;
				break;

			case NID_ITEM_IO_SFLUSHED:
				res_p->m_seq_flushed = x_be64toh(*(int64_t *)p);
				res_p->m_has_seq_flushed = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_IO_RECSFLUSHED:
				res_p->m_rec_seq_flushed = x_be64toh(*(int64_t *)p);
				res_p->m_has_rec_seq_flushed = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_IO_SASSIGNED:
				res_p->m_seq_assigned = x_be64toh(*(int64_t *)p);
				res_p->m_has_seq_assigned = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_BUF_AVAIL:
				res_p->m_buf_avail = x_be32toh(*(int32_t *)p);
				res_p->m_has_buf_avail = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_IO_RIO:
				res_p->m_io_type_rio = *(uint8_t *)p;
				res_p->m_has_io_type_rio = 1;
				p += 1;
				left -= 1;
				break;

			case NID_ITEM_RREADYCOUNTER:
				res_p->m_rreadycounter= x_be64toh(*(int64_t *)p);
				res_p->m_has_rreadycounter = 1;
				p += 8;
				left -= 8;
				break;

			case NID_ITEM_VERSION_SFD:
				res_p->m_sfd = *(int32_t *)p;
				res_p->m_has_sfd = 1;
				p += 4;
				left -= 4;
				break;

			case NID_ITEM_VERSION:
				res_p->m_versionlen = *(int8_t *)p;
				p += 1;
				left -= 1;
				res_p->m_version = p;
				res_p->m_has_version = 1;
				p += res_p->m_versionlen;
				left -= res_p->m_versionlen;
				break;

			default:
				rc = -1;
				nid_log_error("%s: unknown item %d", log_header, item);
				break;
			}
		}
		break;

	default:
		rc = -2;
		p = mpk_buf;
		nid_log_error("%s: got unknown req (req:%d, code:%d)",
			 log_header, res_p->m_req, res_p->m_req_code);
	}

	if (!rc) {
		*len -= to_consume;
	} else {
		nid_log_error("%s: error", log_header);
	}

	return rc;
}

static int
mpk_decode(struct mpk_interface *mpk_p, char *mpk_buf, int *len, struct nid_message_hdr *reshdr_p)
{
	struct mpk_private *priv_p = (struct mpk_private *)mpk_p->m_private;
	if (priv_p->p_type == 0)
		return _mpk_decode_type_0(mpk_buf, len, reshdr_p);
	else
		return -1;
}

static void
mpk_cleanup(struct mpk_interface *mpk_p)
{
	struct mpk_private *priv_p = (struct mpk_private *)mpk_p->m_private;

#ifdef __KERNEL__
	kfree(priv_p);
#else
	char do_pp2 = priv_p->p_do_pp2;
	if (do_pp2 == 1) {
		struct pp2_interface *pp2_p = priv_p->p_pp2;
		pp2_p->pp2_op->pp2_put(pp2_p, priv_p);
	}
	else {
		free(priv_p);
	}
#endif
	mpk_p->m_private = NULL;
}

static struct mpk_operations mpk_op = {
	.m_decode = mpk_decode,
	.m_encode = mpk_encode,
	.m_cleanup = mpk_cleanup,
};

int
mpk_initialization(struct mpk_interface *mpk_p, struct mpk_setup *setup)
{
	struct mpk_private *priv_p;

	nid_log_debug("mpk_initialization: start with type %d ...", setup->type);
#ifdef __KERNEL__
	priv_p = kcalloc(1, sizeof(*priv_p), GFP_KERNEL);
#else
	struct pp2_interface *pp2_p;
	char do_pp2 = setup->do_pp2;

	if (do_pp2 == 1) {
		pp2_p = setup->pp2;
		priv_p = (struct mpk_private *)pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
		priv_p->p_pp2 = pp2_p;
	}
	else {
		priv_p = (struct mpk_private *)calloc(1, sizeof(*priv_p));
	}
	priv_p->p_do_pp2 = do_pp2;
#endif
	mpk_p->m_private = priv_p;
	mpk_p->m_op = &mpk_op;
	priv_p->p_type = setup->type;

	return 0;
}
