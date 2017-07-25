/*
 * smc.c
 * 	Implemantation	of Server Manager Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "util_nw.h"
#include "list.h"
#include "nid_log.h"
#include "nid_shared.h"
#include "mpk_if.h"
#include "sac_if.h"
#include "tp_if.h"
#include "bio_if.h"
#include "smcg_if.h"
#include "smc_if.h"
#include "nid_shared.h"
#include "ini_if.h"
#include "mqtt_if.h"

struct smc_private {
	struct smcg_interface	*p_smcg;
	int			p_rsfd;
	struct mpk_interface	*p_mpk;
	struct ini_interface	*p_ini;
	struct mqtt_interface	*p_mqtt;
};

typedef char* (*stat_process_func_t)(struct mpk_interface *, struct smc_stat_record *, int *, struct list_head *);
stat_process_func_t stat_func[NID_REQ_MAX];

static char*
__stat_process_sac(struct mpk_interface *mpk_p, struct smc_stat_record *sr_p, int *out_len, struct list_head *more_head)
{
	struct sac_stat *stat_p = (struct sac_stat *)sr_p->r_data;
	struct nid_message nid_msg;
	char *msg_buf;
	int len = 0;
	struct timeval now;

	assert(more_head);
	msg_buf = x_malloc(4096);
	len = 4096;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_STAT_SAC;
	nid_msg.m_uuid = stat_p->sa_uuid;
	nid_msg.m_uuidlen = strlen(stat_p->sa_uuid);
	nid_msg.m_has_uuid = 1;

	nid_msg.m_state = stat_p->sa_stat;
	nid_msg.m_has_state = 1;

	if (stat_p->sa_stat == NID_STAT_INACTIVE)
		goto do_encode;

	nid_msg.m_alevel = stat_p->sa_alevel;
	nid_msg.m_has_alevel = 1;

	nid_msg.m_rcounter = stat_p->sa_read_counter;
	nid_msg.m_has_rcounter = 1;
	nid_msg.m_r_rcounter = stat_p->sa_read_resp_counter;
	nid_msg.m_has_r_rcounter = 1;
	nid_msg.m_rreadycounter = stat_p->sa_read_ready_counter;
	nid_msg.m_has_rreadycounter = 1;

	nid_msg.m_wcounter = stat_p->sa_write_counter;
	nid_msg.m_has_wcounter = 1;
	nid_msg.m_wreadycounter = stat_p->sa_write_ready_counter;
	nid_msg.m_has_wreadycounter = 1;
	nid_msg.m_r_wcounter = stat_p->sa_write_resp_counter;
	nid_msg.m_has_r_wcounter = 1;

	nid_msg.m_kcounter = stat_p->sa_keepalive_counter;
	nid_msg.m_has_kcounter = 1;
	nid_msg.m_r_kcounter = stat_p->sa_keepalive_resp_counter;
	nid_msg.m_has_r_kcounter = 1;

	nid_msg.m_recv_sequence = stat_p->sa_recv_sequence;
	nid_msg.m_has_recv_sequence = 1;
	nid_msg.m_wait_sequence = stat_p->sa_wait_sequence;
	nid_msg.m_has_wait_sequence = 1;

	gettimeofday(&now, NULL);
	//nid_msg.m_live_time = (1000000 * now.tv_sec + now.tv_usec) -
	//	(1000000 * stat_p->sa_start_tv.tv_sec + stat_p->sa_start_tv.tv_usec);
	nid_msg.m_live_time = now.tv_sec - stat_p->sa_start_tv.tv_sec;
	nid_msg.m_has_live_time = 1;

	if (stat_p->sa_ipaddr) {
		nid_msg.m_ip = stat_p->sa_ipaddr;
		nid_msg.m_iplen = strlen(stat_p->sa_ipaddr);
		nid_msg.m_has_ip = 1;
	}

	if (stat_p->sa_io_type == IO_TYPE_BUFFER) {
		nid_msg.m_rtree_segsz = stat_p->sa_io_stat.s_rtree_segsz;
		nid_msg.m_has_rtree_segsz = 1;
		nid_msg.m_rtree_nseg = stat_p->sa_io_stat.s_rtree_nseg;
		nid_msg.m_has_rtree_nseg = 1;
		nid_msg.m_rtree_nfree = stat_p->sa_io_stat.s_rtree_nfree;
		nid_msg.m_has_rtree_nfree = 1;
		nid_msg.m_rtree_nused = stat_p->sa_io_stat.s_rtree_nused;
		nid_msg.m_has_rtree_nused = 1;

		nid_msg.m_btn_segsz = stat_p->sa_io_stat.s_btn_segsz;
		nid_msg.m_has_btn_segsz = 1;
		nid_msg.m_btn_nseg = stat_p->sa_io_stat.s_btn_nseg;
		nid_msg.m_has_btn_nseg = 1;
		nid_msg.m_btn_nfree = stat_p->sa_io_stat.s_btn_nfree;
		nid_msg.m_has_btn_nfree = 1;
		nid_msg.m_btn_nused = stat_p->sa_io_stat.s_btn_nused;
		nid_msg.m_has_btn_nused = 1;
	}

do_encode:
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	*out_len = len;
	return msg_buf;
}

static char*
__stat_process_tp(struct mpk_interface *mpk_p, struct smc_stat_record *sr_p, int *out_len, struct list_head *more_head)
{
	struct tp_stat *stat_p = (struct tp_stat *)sr_p->r_data;
	struct nid_message nid_msg;
	char *msg_buf;
	int len = 0;

	assert(more_head);
	msg_buf = x_malloc(4096);
	len = 4096;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_STAT_WTP;
	nid_msg.m_nused = stat_p->s_nused;
	nid_msg.m_has_nused = 1;
	nid_msg.m_nfree = stat_p->s_nfree;
	nid_msg.m_has_nfree = 1;
	nid_msg.m_nnofree = stat_p->s_no_free;
	nid_msg.m_has_nnofree = 1;
	nid_msg.m_nworker = stat_p->s_workers;
	nid_msg.m_has_nworker = 1;
	nid_msg.m_nmaxworker = stat_p->s_max_workers;
	nid_msg.m_has_nmaxworker = 1;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	*out_len = len;
	return msg_buf;
}

static char*
__stat_process_io(struct mpk_interface *mpk_p, struct smc_stat_record *sr_p, int *out_len, struct list_head *more_head)
{
	struct io_stat *stat_p = (struct io_stat *)sr_p->r_data;
	struct nid_message nid_msg;
	char *msg_buf;
	int do_it = 0, len = 0;

	msg_buf = x_malloc(4096);
	len = 4096;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_STAT_IO;
	if (stat_p->s_io_type_bio) {
#if 0
		struct smc_stat_record *bio_sr_p, *bio_sr_p2;
		int int_out_len, nwrite;
		char *p;
#endif
		nid_msg.m_state = stat_p->s_stat;
		nid_msg.m_has_state = 1;
		nid_msg.m_io_type_bio = stat_p->s_io_type_bio;
		nid_msg.m_has_io_type_bio = 1;
		nid_msg.m_block_occupied = stat_p->s_block_occupied;
		nid_msg.m_has_block_occupied = 1;
		nid_msg.m_flush_nblocks = stat_p->s_flush_nblocks;
		nid_msg.m_has_flush_nblocks = 1;
		nid_msg.m_cur_write_block = stat_p->s_cur_write_block;
		nid_msg.m_has_cur_write_block = 1;
		nid_msg.m_cur_flush_block = stat_p->s_cur_flush_block;
		nid_msg.m_has_cur_flush_block = 1;
		nid_msg.m_seq_flushed = stat_p->s_seq_flushed;
		nid_msg.m_has_seq_flushed = 1;
		nid_msg.m_rec_seq_flushed = stat_p->s_rec_seq_flushed;
		nid_msg.m_has_rec_seq_flushed = 1;
		nid_msg.m_seq_assigned = stat_p->s_seq_assigned;
		nid_msg.m_has_seq_assigned = 1;
		nid_msg.m_buf_avail = stat_p->s_buf_avail;
		nid_msg.m_has_buf_avail = 1;
		do_it = 1;

		list_splice_tail_init(&stat_p->s_inactive_head, more_head);
#if 0
		list_for_each_entry_safe(bio_sr_p,  bio_sr_p2, struct smc_stat_record, &stat_p->s_inactive_head, r_list) {
			p = stat_func[bio_sr_p->r_type](mpk_p, bio_sr_p, &int_out_len);
			if (!int_out_len)
				continue;
			nwrite= write(sfd, p, int_out_len);
			if (nwrite != int_out_len) {
				nid_log_error("__stat_process_io: cannot write to the nid manager");
			}
			free(p);
			list_del(&bio_sr_p->r_list);
			free(bio_sr_p->r_data);
			free(bio_sr_p);
		}
#endif
	}

	if (do_it) {
		mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		*out_len = len;
	} else {
		*out_len = 0;
	}

	return msg_buf;
}

static char*
_stat_process_bio_vec(struct mpk_interface *mpk_p, struct smc_stat_record *sr_p, int *out_len, struct list_head *more_head){
	struct io_vec_stat *stat_p = (struct io_vec_stat *)sr_p->r_data;
		struct nid_message_bio_vec nid_msg;
		char *msg_buf;
		int do_it = 0, len = 0;

		assert(more_head);
		msg_buf = x_malloc(4096);
		len = 4096;
		memset(&nid_msg, 0, sizeof(nid_msg));
		nid_msg.m_req = NID_REQ_BIO;
		nid_msg.m_req_code = NID_REQ_BIO_VEC_STOP;

		if (stat_p->s_io_type_bio) {
			nid_msg.m_vec_flushnum = stat_p->s_flush_num;
			nid_msg.m_vec_vecnum = stat_p ->s_vec_num;
			nid_msg.m_vec_writesz = stat_p->s_write_size;
			do_it = 1;

		}
		if (do_it) {
			mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
			*out_len = len;
		} else {
			*out_len = 0;
		}

		return msg_buf;
}



static int
smc_accept_new_channel(struct smc_interface *smc_p, int sfd)
{
	struct smc_private *priv_p = smc_p->s_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
smc_do_channel(struct smc_interface *smc_p)
{
	char *log_header = "smc_do_channel";
	struct smc_private *priv_p = (struct smc_private *)smc_p->s_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	int sfd = priv_p->p_rsfd;
	struct smcg_interface *smcg_p = priv_p->p_smcg;
	char cmd, cmd_code, *p;
	struct list_head stat_head, ext_head;
	struct smc_stat_record *sr_p, *sr_p2;
	int out_len, nwrite;
	uint32_t cmd_len;
	struct mqtt_interface *mqtt_p = priv_p->p_mqtt;
	struct mqtt_message msg;

	read(sfd, &cmd, 1);
	if (cmd == NID_REQ_STOP) {
		nid_log_debug("smc_do_channel: got stop command");
		smcg_p->smg_op->smg_stop(smcg_p);
		close(sfd);

		msg.type = "nidserver";
		msg.module = "NIDServer";
		msg.message = "NID server stopped";
		msg.status = "OK";
		strcpy (msg.uuid, "NULL");
		strcpy (msg.ip, "NULL");
		mqtt_p->mt_op->mt_publish(mqtt_p, &msg);

		mqtt_p->mt_op->mt_cleanup(mqtt_p);


		exit(0);
	} else if (cmd == NID_REQ_STAT_RESET) {
		nid_log_debug("smc_do_channel: got reset stat command");
		smcg_p->smg_op->smg_reset_stat(smcg_p);
		close(sfd);
	} else if (cmd == NID_REQ_STAT_GET) {
		nid_log_debug("smc_do_channel: got get stat command");
		INIT_LIST_HEAD(&stat_head);
		smcg_p->smg_op->smg_get_stat(smcg_p, &stat_head);
		INIT_LIST_HEAD(&ext_head);
do_more:
		list_for_each_entry_safe(sr_p, sr_p2, struct smc_stat_record, &stat_head, r_list) {
			p = stat_func[sr_p->r_type](mpk_p, sr_p, &out_len, &ext_head);
			nid_log_debug("smc_do_channel: sending a package(type:%d) of size %d",
				sr_p->r_type, out_len);
			if (!out_len)
				continue;
			nwrite= write(sfd, p, out_len);
			if (nwrite != out_len) {
				nid_log_error("smc_do_channel: cannot write to the nid manager");
			}
			free(p);
			list_del(&sr_p->r_list);
			free(sr_p->r_data);
			free(sr_p);
		}
		if (!list_empty(&ext_head)) {
			list_splice_tail_init(&ext_head, &stat_head);
			goto do_more;
		}
		close(sfd);
	} else if (cmd == NID_REQ_UPDATE) {
		nid_log_debug("smc_do_channel: got update command");
		smcg_p->smg_op->smg_update(smcg_p);
		close(sfd);
	} else if (cmd == NID_REQ_CHECK_CONN) {
		char len, ack, uuid[NID_MAX_UUID] = {0};
		char cc_exists = 0;
		nid_log_debug("smc_do_channel: got check_conn command");
		read(sfd, &len, 1);
	       	read(sfd, uuid, len);
		nid_log_debug("smc_do_channel: got check_conn command for %s", uuid);
		cc_exists = smcg_p->smg_op->smg_check_conn(smcg_p, uuid);
		write(sfd, &cc_exists, 1);
		read(sfd, &ack, 1);
		close(sfd);
	} else if (cmd == NID_REQ_SET_LOG_LEVEL) {
		int level;
		read(sfd, &level, sizeof(level));
		if (level >= LOG_EMERG && level <= LOG_DEBUG) {
			nid_log_debug("smc_do_channel: NID_REQ_SET_LOG_LEVEL: %d", level);
			nid_log_set_level(level);
		}
		close(sfd);
	} else if (cmd == NID_REQ_READ) {
		struct ini_key_content *kc_p;
		struct ini_interface *ini_p = priv_p->p_ini;
		char src_uuid[NID_MAX_UUID], *exportname, *dev_name;
		int uuid_len, read_len;
		uint8_t read_rc = 0;
		off_t read_off;
		int fd;
		char read_buf[1024*1024];

		nid_log_debug("start check partition");
		read(sfd, &uuid_len, sizeof(uuid_len));
		read(sfd, src_uuid, uuid_len);
		read(sfd, &read_off, sizeof(read_off));
		read(sfd, &read_len, sizeof(read_len));

		kc_p = ini_p->i_op->i_search_key_from_section(ini_p, src_uuid, "dev_name");
		dev_name = (char *)(kc_p->k_value);

		kc_p = ini_p->i_op->i_search_key_from_section(ini_p, dev_name, "exportname");
		exportname = (char *)(kc_p->k_value);
		nid_log_debug("smc_do_channel: read %s", exportname);

		fd = open(exportname, O_RDWR, 0600);
		if (fd < 0) {
			nid_log_debug("cant open  %s", exportname);
			read_rc = errno;
			write(sfd, &read_rc, sizeof(read_rc));
		} else {
			nid_log_debug("can open  %s", exportname);
			write(sfd, &read_rc, sizeof(read_rc));
			read_len = (read_len <= 1024*1024) ? read_len : (1024*1024);
			pread(fd, read_buf, read_len, read_off);
			nid_log_debug("len:%d,  buffer: %s", read_len, read_buf);
			write(sfd, &read_len, sizeof(read_len));
			write(sfd, read_buf, read_len);
		}
	} else if (cmd == NID_REQ_GET_VERSION) {
		char ver_buf[16];

		memset(ver_buf, 0, 16);
		sprintf(ver_buf, "%s", __DATE__);
		write(sfd, ver_buf, 16);

		memset(ver_buf, 0, 16);
		sprintf(ver_buf, "%s", __TIME__);
		write(sfd, ver_buf, 16);

		memset(ver_buf, 0, 16);
		sprintf(ver_buf, "%d", nid_log_level);
		write(sfd, ver_buf, 16);
		close(sfd);
	} else if (cmd == NID_REQ_CHECK_SERVER) {
		nid_log_debug("smc_do_channel: got check server command");
		char msg_buf[32];
		int len;
		struct nid_message nid_msg;

		memset(&nid_msg, 0, sizeof(nid_msg));
		nid_msg.m_req = NID_REQ_CHECK_SERVER;
		len = 32;
		mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
		write(sfd, msg_buf, len);
		close(sfd);
	} else if (cmd == 50) {
		nid_log_debug("%s: start ...", log_header);
		smcg_p->smg_op->smg_mem_size(smcg_p);
		close(sfd);
	} else if (cmd == NID_REQ_BIO_FAST_FLUSH) {
		nid_log_debug("%s: start ...", log_header);
		//TODO: analyze the input and see if there is resource specified
		smcg_p->smg_op->smg_bio_fast_flush(smcg_p, NULL);
		close(sfd);
	} else if (cmd == NID_REQ_BIO_STOP_FAST_FLUSH) {
		nid_log_debug("%s: start ...", log_header);
		//TODO: analyze the input and see if there is resource specified
		smcg_p->smg_op->smg_bio_stop_fast_flush(smcg_p, NULL);
		close(sfd);
	} else if (cmd == NID_REQ_BIO) {
		char msg_buf[4096];
		p = msg_buf;
		*p++ = NID_REQ_BIO;
		read(sfd, p, 1);
		cmd_code = *p++;
		util_nw_read_n(sfd, p, 4);
		cmd_len = be32toh(*(uint32_t *)(p));
		assert(cmd_len <= 4096);
		p += 4;
		util_nw_read_n(sfd, p, cmd_len - 6);

		if (cmd_code == NID_REQ_BIO_RELEASE_START) {
			struct nid_message_bio_release nid_msg;
			nid_log_debug("%s: got NID_REQ_BIO_RELEASE_START",  log_header);
			//nid_msg.m_req = NID_REQ_BIO;
			//nid_msg.m_req_code = NID_REQ_BIO_RELEASE_START;
			mpk_p->m_op->m_decode(mpk_p, msg_buf, (int*)&cmd_len, (struct nid_message_hdr *)&nid_msg);
			assert(!cmd_len);
			nid_log_debug("%s: got NID_REQ_BIO_RELEASE_START, uuid:%s",  log_header, nid_msg.m_chan_uuid);
			close(sfd);

		} else if (cmd_code == NID_REQ_BIO_RELEASE_STOP) {
			struct nid_message_bio_release nid_msg;
			nid_log_debug("%s: got NID_REQ_BIO_RELEASE_STOP",  log_header);
			//nid_msg.m_req = NID_REQ_BIO;
			//nid_msg.m_req_code = NID_REQ_BIO_RELEASE_STOP;
			mpk_p->m_op->m_decode(mpk_p, msg_buf, (int*)&cmd_len, (struct nid_message_hdr *)&nid_msg);
			assert(!cmd_len);
			nid_log_debug("%s: got NID_REQ_BIO_RELEASE_STOP, uuid:%s",  log_header, nid_msg.m_chan_uuid);
			close(sfd);

		} else if (cmd_code == NID_REQ_BIO_VEC_START){

			nid_log_debug("%s: start ...", log_header);
			smcg_p->smg_op->smg_bio_vec_start(smcg_p);
			close(sfd);
		} else if (cmd_code == NID_REQ_BIO_VEC_STOP){
			nid_log_debug("%s: start ...", log_header);
			smcg_p->smg_op->smg_bio_vec_stop(smcg_p);
			close(sfd);

		} else if (cmd_code == NID_REQ_BIO_VEC_STAT){
			nid_log_debug("%s: start ...", log_header);
			INIT_LIST_HEAD(&stat_head);
			smcg_p->smg_op->smg_bio_vec_stat(smcg_p, &stat_head);
			INIT_LIST_HEAD(&ext_head);
do_vec_more:
			list_for_each_entry_safe(sr_p, sr_p2, struct smc_stat_record, &stat_head, r_list) {
				p = _stat_process_bio_vec(mpk_p, sr_p, &out_len, &ext_head);
				nid_log_debug("smc_do_channel: sending a package(type:%d) of size %d",
						sr_p->r_type, out_len);
				if (!out_len)
					continue;
				nwrite= write(sfd, p, out_len);
				if (nwrite != out_len) {
					nid_log_error("smc_do_channel: cannot write to the nid manager");
				}
				free(p);
				list_del(&sr_p->r_list);
				free(sr_p->r_data);
				free(sr_p);
				}
			if (!list_empty(&ext_head)) {
				list_splice_tail_init(&ext_head, &stat_head);
				goto do_vec_more;
			}
				close(sfd);
		}
	} else {
		nid_log_debug("smc_do_channel: got unknown command");
		close(sfd);
	}

}

static void
smc_cleanup(struct smc_interface *smc_p)
{
	nid_log_debug("smc_cleanup start, smc_p:%p", smc_p);
	if (smc_p->s_private != NULL) {
		free(smc_p->s_private);
		smc_p->s_private = NULL;
	}
}

struct smc_operations smc_op = {
	.s_accept_new_channel = smc_accept_new_channel,
	.s_do_channel = smc_do_channel,
	.s_cleanup = smc_cleanup,
};

int
smc_initialization(struct smc_interface *smc_p, struct smc_setup *setup)
{
	char *log_header = "smc_initialization";
	struct smc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	smc_p->s_private = priv_p;
	priv_p->p_rsfd = -1;
	priv_p->p_mpk = setup->mpk;
	priv_p->p_smcg = setup->smcg;
	priv_p->p_ini = setup->ini;
	priv_p->p_mqtt = setup->mqtt;
	smc_p->s_op = &smc_op;
	stat_func[NID_REQ_STAT_SAC] = __stat_process_sac;
	stat_func[NID_REQ_STAT_WTP] = __stat_process_tp;
	stat_func[NID_REQ_STAT_IO] = __stat_process_io;

	return 0;
}
