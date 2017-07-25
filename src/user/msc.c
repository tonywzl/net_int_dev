/*
 * msc.c
 * 	Implementation of Manage Service Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "msc_if.h"
#include "nid_shared.h"
#include "util_nw.h"
#include "version.h"

struct msc_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct mpk_interface	*p_mpk;

};

typedef void (*stat_unpack_func_t)(struct nid_message *);
stat_unpack_func_t stat_unpack[NID_REQ_MAX];

static int
read_n(int fd, void *buf, int n)
{
	char *p = buf;
	int nread , left = n;

	while (left > 0) {
		nread = read(fd, p, left);
		if (nread <= 0) {
			nid_log_error("read_n: fd:%d, nread:%d, errno:%d", fd, nread, errno);
			return -1;
		}
		left -= nread;
		p += nread;
	}
	return n;
}

static int
make_connection(char *ipstr, u_short port)
{
	struct sockaddr_in saddr;
	int sfd;
	//char chan_type = NID_CTYPE_ADM;
	uint16_t chan_type = NID_CTYPE_ADM;
	nid_log_debug("make_connection start (ip:%s port:%d)", ipstr, port);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d",
			ipstr, port, errno);
		close(sfd);
		sfd = -1;
	} else {
		nid_log_info("make_connection successful!!!");
		if (util_nw_write_two_byte(sfd, chan_type) == NID_SIZE_CTYPE_MSG) {
			nid_log_info("sending chan_type(SAC_CTYPE_ADM) successful!!!");
		} else {
			nid_log_info("make_connection: failed to send chan_type, errno:%d", errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

static int
msc_stop(struct msc_interface *msc_p)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char msc_cmd = NID_REQ_STOP;
	char nothing;

	nid_log_debug("msc_stop %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	read(sfd, &nothing, 1);
	printf("nids stop successfully\n");

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static void
_get_tp_stat(struct nid_message *nid_msg)
{
	char *dent = "   ";

	printf("Worker thread pool:\n");
	if (nid_msg->m_has_nused) {
		printf("%snused:%d\n", dent, nid_msg->m_nused);
	}

	if (nid_msg->m_has_nfree) {
		printf("%snfree:%d\n", dent, nid_msg->m_nfree);
	}

	if (nid_msg->m_has_nworker) {
		printf("%snworker:%d\n", dent, nid_msg->m_nworker);
	}

	if (nid_msg->m_has_nmaxworker) {
		printf("%snmaxworker:%d\n", dent, nid_msg->m_nmaxworker);
	}

	if (nid_msg->m_has_nnofree) {
		printf("%snofree:%d\n", dent, nid_msg->m_nnofree);
	}
}

static void
_get_io_stat(struct nid_message *nid_msg)
{
	char *dent = "   ";

	printf("IO Infomation: \n");
	if (nid_msg->m_has_io_type_bio) {
		printf("%sBIO:\n", dent);

		if (nid_msg->m_has_state) {
			char *m_state;
			if (nid_msg->m_state == NID_STAT_ACTIVE)
				m_state = "active";
			else if (nid_msg->m_state == NID_STAT_INACTIVE)
				m_state = "inactive";
			else
				m_state = "unknown";
			printf("%sstate:%s(%d)\n", dent, m_state, nid_msg->m_state);
		}

		if (nid_msg->m_has_block_occupied) {
			printf("%sblock_occupied:%d", dent, nid_msg->m_block_occupied);
		}

		if (nid_msg->m_has_flush_nblocks) {
			printf("/%d\n", nid_msg->m_flush_nblocks);
		}
		else {
			printf("\n");
		}

		if (nid_msg->m_has_cur_write_block) {
			printf("%scur_write_block:%d\n", dent, nid_msg->m_cur_write_block);
		}

		if (nid_msg->m_has_cur_flush_block) {
			printf("%scur_flush_block:%d\n", dent, nid_msg->m_cur_flush_block);
		}

		if (nid_msg->m_has_seq_flushed) {
			printf("%sseq_flushed:%lu\n", dent, nid_msg->m_seq_flushed);
		}

		if (nid_msg->m_has_rec_seq_flushed) {
			printf("%srec_seq_flushed:%lu\n", dent, nid_msg->m_rec_seq_flushed);
		}

		if (nid_msg->m_has_seq_assigned) {
			printf("%sseq_assigned:%lu\n", dent, nid_msg->m_seq_assigned);
		}

		if (nid_msg->m_has_buf_avail) {
			printf("%sbuf_avail:%u\n", dent, nid_msg->m_buf_avail);
		}
	}

	if (nid_msg->m_has_io_type_rio) {
		printf("%sRIO:\n", dent);
	}
}

static void
_get_sac_stat(struct nid_message *nid_msg)
{
	char uuid[NID_MAX_UUID], ip[NID_MAX_IP];
	char *dent = "   ";

	printf("Service Agent Channel:\n");
	if (nid_msg->m_has_uuid) {
		memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
		uuid[nid_msg->m_uuidlen] = 0;
		printf("%suuid:%s\n", dent, uuid);
	}

	if (nid_msg->m_has_state) {
		char *m_state;
		if (nid_msg->m_state == NID_STAT_ACTIVE)
			m_state = "active";
		else if (nid_msg->m_state == NID_STAT_INACTIVE)
			m_state = "inactive";
		else
			m_state = "unknown";
		printf("%sstate:%s(%d)\n", dent, m_state, nid_msg->m_state);
	}

	if (nid_msg->m_has_ip) {
		memcpy(ip, nid_msg->m_ip, nid_msg->m_iplen);
		ip[nid_msg->m_iplen] = 0;
		printf("%sip:%s\n", dent, ip);
	}

	if (nid_msg->m_has_alevel) {
		printf("%salevel:%s\n", dent, nid_alevel_to_str(nid_msg->m_alevel));
	}

	if (nid_msg->m_has_rcounter) {
		printf("%srcounter:%lu\n", dent, nid_msg->m_rcounter);
	}

	if (nid_msg->m_has_rreadycounter) {
		printf("%srreadycounter:%lu\n", dent, nid_msg->m_rreadycounter);
	}

	if (nid_msg->m_has_r_rcounter) {
		printf("%srrcounter:%lu\n", dent, nid_msg->m_r_rcounter);
	}

	if (nid_msg->m_has_wcounter) {
		printf("%swcounter:%lu\n", dent, nid_msg->m_wcounter);
	}

	if (nid_msg->m_has_wreadycounter) {
		printf("%swreadycounter:%lu\n", dent, nid_msg->m_wreadycounter);
	}

	if (nid_msg->m_has_r_wcounter) {
		printf("%srwcounter:%lu\n", dent, nid_msg->m_r_wcounter);
	}

	if (nid_msg->m_has_kcounter) {
		printf("%skcounter:%lu\n", dent, nid_msg->m_kcounter);
	}

	if (nid_msg->m_has_r_kcounter) {
		printf("%srkcounter:%lu\n", dent, nid_msg->m_r_kcounter);
	}

	if (nid_msg->m_has_recv_sequence) {
		printf("%srecv_sequence:%lu\n", dent, nid_msg->m_recv_sequence);
	}

	if (nid_msg->m_has_wait_sequence) {
		printf("%swait_sequence:%lu\n", dent, nid_msg->m_wait_sequence);
	}

	if (nid_msg->m_has_rtree_segsz) {
		printf("%srtree_segsz:%u\n", dent, nid_msg->m_rtree_segsz);
	}

	if (nid_msg->m_has_rtree_nseg) {
		printf("%srtree_nseg:%u\n", dent, nid_msg->m_rtree_nseg);
	}

	if (nid_msg->m_has_rtree_nfree) {
		printf("%srtree_nfree:%u\n", dent, nid_msg->m_rtree_nfree);
	}

	if (nid_msg->m_has_rtree_nused) {
		printf("%srtree_nused:%u\n", dent, nid_msg->m_rtree_nused);
	}

	if (nid_msg->m_has_btn_segsz) {
		printf("%sbtn_segsz:%u\n", dent, nid_msg->m_btn_segsz);
	}

	if (nid_msg->m_has_btn_nseg) {
		printf("%sbtn_nseg:%u\n", dent, nid_msg->m_btn_nseg);
	}

	if (nid_msg->m_has_btn_nfree) {
		printf("%sbtn_nfree:%u\n", dent, nid_msg->m_btn_nfree);
	}

	if (nid_msg->m_has_btn_nused) {
		printf("%sbtn_nused:%u\n", dent, nid_msg->m_btn_nused);
	}

	if (nid_msg->m_has_live_time) {
		//printf("%slive_time:%dsec + %dusec\n", dent,
		//	nid_msg->m_live_time/1000000, nid_msg->m_live_time%1000000);
		printf("%slive_time:%dsec\n", dent, nid_msg->m_live_time);
	}
}

static void
_get_io_vec_stat(struct nid_message_bio_vec *nid_msg){
	nid_log_debug("vec_update");
	char *dent = "   ";
	printf("BIO VEC INFORMATION:\n");
	printf("%sflush_num:%u\n", dent, nid_msg->m_vec_flushnum);
	printf("%svec_num:%lu\n", dent, nid_msg->m_vec_vecnum);
	printf("%swrite_size:%lu\n", dent, nid_msg->m_vec_writesz);

}


static int
msc_stat_get(struct msc_interface *msc_p, struct msc_stat *stat_p)
{
	struct msc_private *priv_p = msc_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message res;
	int sfd = -1, rc = 0;
	char msc_cmd = NID_REQ_STAT_GET;
	char *msg_buf, msg_hdr[5];
	int len, idx;

	assert(stat_p);
	nid_log_debug("msc_stat_get %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	while (read_n(sfd, &msg_hdr, 6) == 6) {
		len = be32toh(*(uint32_t *)(msg_hdr + 2));
		if (len > 6) {
			msg_buf = x_malloc(len);
			if (read_n(sfd, msg_buf + 6, len - 6) < 0) {
				rc = -1;
				goto out;
			}
			memcpy(msg_buf, msg_hdr, 6);
		}
		memset(&res, 0, sizeof(res));
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&res);
		idx = msg_buf[0];
		stat_unpack[idx](&res);
	}
	printf("nids get stat successfully\n");
out:
	if (sfd >= 0) {
		close(sfd);
	}
	printf("nids get stat rc:%d\n", rc);
	return rc;
}

static int
msc_stat_reset(struct msc_interface *msc_p)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char msc_cmd = NID_REQ_STAT_RESET;
	char nothing;

	assert(priv_p);
	nid_log_debug("msc_stat_reset %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	read(sfd, &nothing, 1);
	printf("nids reset stat successfully\n");
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
msc_update(struct msc_interface *msc_p)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char msc_cmd = NID_REQ_UPDATE;
	char nothing;

	assert(priv_p);
	nid_log_debug("msc_update %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		nid_log_error("msc_update: make_connection failed");
		rc = 1;
		goto out;
	}

	if (write(sfd, &msc_cmd, 1) != 1) {
		nid_log_error("msc_update: failed to write command (NID_REQ_UPDATE)");
		rc = 1;
		goto out;
	}

	read(sfd, &nothing, 1);
out:
	if (sfd >= 0) {
		close(sfd);
	}

	if(!rc) {
		printf("nids update successfully\n");
	} else {
	       printf("nids update failed\n");
	}
	return rc;
}

static int
msc_bio_fast_flush(struct msc_interface *msc_p)
{
	char *log_header = "msc_bio_fast_flush";
	struct msc_private *priv_p = msc_p->m_private;
	int rc = 0, sfd = -1;
	char nothing, msc_cmd = NID_REQ_BIO_FAST_FLUSH;

	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
msc_mem_size(struct msc_interface *msc_p)
{
	char *log_header = "msc_mem_size";
	struct msc_private *priv_p = msc_p->m_private;
	int rc = 0, sfd = -1;
	char nothing, msc_cmd = 50;

	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}
static int
msc_bio_stop_fast_flush(struct msc_interface *msc_p)
{
	char *log_header = "msc_bio_stop_fast_flush";
	struct msc_private *priv_p = msc_p->m_private;
	int rc = 0, sfd = -1;
	char nothing, msc_cmd = NID_REQ_BIO_STOP_FAST_FLUSH;

	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
msc_bio_vec_start(struct msc_interface *msc_p)
{
	char *log_header = "msc_bio_vec_start";
	struct msc_private *priv_p = msc_p->m_private;
	int rc = 0, sfd = -1;
	char nothing, msc_cmd = NID_REQ_BIO, msc_cmd_code = NID_REQ_BIO_VEC_START;
	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
			rc = -1;
			goto out;
	}

	write(sfd, &msc_cmd, 1);
	write(sfd, &msc_cmd_code, 1);
	read(sfd, &nothing, 1);
	nid_log_debug("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

static int
msc_bio_vec_stop(struct msc_interface *msc_p)
{
	char *log_header = "msc_bio_vec_stop";
	struct msc_private *priv_p = msc_p->m_private;
	int rc = 0, sfd = -1;
	char nothing, msc_cmd = NID_REQ_BIO, msc_cmd_code = NID_REQ_BIO_VEC_STOP;
	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
			rc = -1;
			goto out;
	}

	write(sfd, &msc_cmd, 1);
	write(sfd, &msc_cmd_code, 1);
	read(sfd, &nothing, 1);
	nid_log_debug("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}

	return rc;

}

static int
msc_bio_vec_stat(struct msc_interface *msc_p)
{
	char *log_header = "msc_bio_vec_stat";
	struct msc_private *priv_p = msc_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message_bio_vec res;
	int rc = 0, sfd = -1;
	char *msg_buf, msg_hdr[5];
	char msc_cmd = NID_REQ_BIO, msc_cmd_code = NID_REQ_BIO_VEC_STAT;;
	int len;
	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
			rc = -1;
			goto out;
	}

	write(sfd, &msc_cmd, 1);
	write(sfd, &msc_cmd_code, 1);
	while (read_n(sfd, &msg_hdr, 6) == 6){
		len = be32toh(*(uint32_t*)(msg_hdr +2));
		if (len > 6){
			msg_buf = x_malloc(len);
			if(read_n(sfd, msg_buf +6, len - 6) < 0){
				rc = -1;
				goto out;

			}
			memcpy(msg_buf, msg_hdr, 6);
		}
		memset(&res, 0 ,sizeof(res));
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&res);
		_get_io_vec_stat(&res);
	}

out:
	if (sfd >= 0) {
		close(sfd);
	}
	nid_log_debug("nids get io_vec stat %d\n",rc);
	return rc;

}

static int
msc_check_conn(struct msc_interface *msc_p, char *uuid)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char msc_cmd = NID_REQ_CHECK_CONN;
	char nothing, got_it = 1, sac_exists, len = strlen(uuid);

	nid_log_debug("msc_conn_check: %s:%u, uuid:%s", priv_p->p_ipstr, priv_p->p_port, uuid);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &msc_cmd, 1);
	write(sfd, &len, 1);
	write(sfd, uuid, len);
	read(sfd, &sac_exists, 1);
	printf("nids check conn successfully (sac_exists:%d)\n", sac_exists);
	if (!sac_exists)
		rc = 1;
	write(sfd, &got_it, 1);
	read(sfd, &nothing, 1);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
msc_do_read(struct msc_interface *msc_p, char* uuid, off_t offset, int len, char* fname)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char msc_cmd = NID_REQ_READ;
	uint8_t read_rc = 0;
	int uuid_len = strlen(uuid);
	int readn, read_len = 0;
	int fd;
	char * buffer;

	nid_log_error("msc_check_partition: %s:%u, uuid:%s, offset:%lu, len:%d", priv_p->p_ipstr, priv_p->p_port, uuid, offset, len);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	fd = open(fname, O_RDWR|O_CREAT);
	write(sfd, &msc_cmd, 1);
	write(sfd, &uuid_len, sizeof(uuid_len));
	write(sfd, uuid, uuid_len);
	write(sfd, &offset, sizeof(offset));
	write(sfd, &len, sizeof(len));
	read(sfd, &read_rc, sizeof(read_rc));
	if(read_rc){
		rc =1;
		printf("cant get the device, error: %d \n", read_rc);
	} else {
		readn = read_n(sfd, &read_len, sizeof(read_len));
		assert(readn == sizeof(read_len));
		buffer = malloc(read_len * sizeof(char));
		readn = read_n(sfd, buffer, read_len);
		assert(readn == read_len);
		nid_log_error("msc_check_partition: buffer: %s ",buffer);
		write(fd, buffer, read_len);
		close(fd);
		printf("\n");
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}

	return rc;
}

static int
msc_set_log_level(struct msc_interface *msc_p, int level)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char cmd = NID_REQ_SET_LOG_LEVEL;
	char nothing;

	assert(priv_p);
	nid_log_debug("msc_set_log_level %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &cmd, sizeof(cmd));
	write(sfd, &level, sizeof(level));
	read(sfd, &nothing, sizeof(nothing));
	close(sfd);

out:
	return rc;
}

static int
msc_get_version(struct msc_interface *msc_p)
{
	struct msc_private *priv_p = msc_p->m_private;
	int sfd = -1, rc = 0;
	char cmd = NID_REQ_GET_VERSION;
	char buffer[16];

	assert(priv_p);
	nid_log_debug("msc_get_version %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	write(sfd, &cmd, sizeof(cmd));

	read(sfd, buffer, 16);
	printf("Server build time:  %s ", buffer);
	read(sfd, buffer, 16);
	printf("%s\n", buffer);

	read(sfd, buffer, 16);
	printf("Server log level:   %s\n", buffer);
	close(sfd);

out:
	return rc;
}

static int
msc_check_server(struct msc_interface *p_msc)
{
	struct msc_private *priv_p = p_msc->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message nid_msg;
	char buf[32];
	int len, sfd = -1, rc = -1;

	nid_log_debug("msc_check_server: %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		goto out;
	}

	len = 32;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHECK_SERVER;
	mpk_p->m_op->m_encode(mpk_p, buf, &len, (struct nid_message_hdr *)&nid_msg);
	write(sfd, buf, len);

	memset(buf, 0, 32);
	if (read_n(sfd, buf, 5) == 5) {
		len = be32toh(*(uint32_t *)(buf + 1));
		if (len > 5 && len < 32) {
			if (read_n(sfd, buf + 5, len - 5) < 0) {
				rc = -1;
				close(sfd);
				goto out;
			}
		}
		memset(&nid_msg, 0, sizeof(nid_msg));
		mpk_p->m_op->m_decode(mpk_p, buf, &len, (struct nid_message_hdr *)&nid_msg);
		if (nid_msg.m_req == NID_REQ_CHECK_SERVER) {
			rc = 0;
		}
	}
	close(sfd);

out:
	if (!rc) {
		printf("The server is alive\n");
		printf("%s\n",NID_VERSION);
	}
	else {
		printf("The server does not exist\n");
	}
	return rc;
}

static int
msc_bio_release_start(struct msc_interface *msc_p, char *chan_uuid)
{
	char *log_header = "msc_bio_release_start";
	struct msc_private *priv_p = msc_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message_bio_release nid_msg;
	char buf[32];
	int len, sfd = -1;
	char nothing;
	int rc = 0;

	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	len = sizeof(buf);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_BIO;
	nid_msg.m_req_code = NID_REQ_BIO_RELEASE_START;
	strcpy(nid_msg.m_chan_uuid, chan_uuid);
	nid_msg.m_len = strlen(chan_uuid);
	mpk_p->m_op->m_encode(mpk_p, buf, &len, (struct nid_message_hdr *)&nid_msg);
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
msc_bio_release_stop(struct msc_interface *msc_p, char *chan_uuid)
{
	char *log_header = "msc_bio_release_stop";
	struct msc_private *priv_p = msc_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message_bio_release nid_msg;
	char buf[32];
	int len, sfd = -1;
	char nothing;
	int rc = 0;

	nid_log_debug("%s: start ...", log_header);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	len = sizeof(buf);
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_BIO;
	nid_msg.m_req_code = NID_REQ_BIO_RELEASE_STOP;
	strcpy(nid_msg.m_chan_uuid, chan_uuid);
	nid_msg.m_len = strlen(chan_uuid);
	mpk_p->m_op->m_encode(mpk_p, buf, &len, (struct nid_message_hdr *)&nid_msg);
	write(sfd, buf, len);

	read(sfd, &nothing, 1);
	printf("%s: done\n", log_header);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}


struct msc_operations msc_op = {
	.m_stop = msc_stop,
	.m_stat_reset = msc_stat_reset,
	.m_stat_get = msc_stat_get,
	.m_update = msc_update,
	.m_check_conn = msc_check_conn,
	.m_set_log_level = msc_set_log_level,
	.m_get_version = msc_get_version,
	.m_check_server = msc_check_server,
	.m_bio_fast_flush = msc_bio_fast_flush,
	.m_bio_stop_fast_flush = msc_bio_stop_fast_flush,
	.m_bio_vec_start = msc_bio_vec_start,
	.m_bio_vec_stop = msc_bio_vec_stop,
	.m_bio_vec_stat = msc_bio_vec_stat,
	.m_bio_release_start = msc_bio_release_start,
	.m_bio_release_stop = msc_bio_release_stop,
	.m_mem_size = msc_mem_size,
	.m_do_read = msc_do_read,
};

int
msc_initialization(struct msc_interface *msc_p, struct msc_setup *setup)
{
	struct msc_private *priv_p;

	nid_log_debug("msc_initialization start ...");
	if (!setup) {
		nid_log_error("msc_initialization got null setup");
		return -1;
	}

	priv_p = x_calloc(1, sizeof(*priv_p));
	msc_p->m_private = priv_p;
	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_mpk = setup->mpk;

	msc_p->m_op = &msc_op;
	stat_unpack[NID_REQ_STAT_SAC] = _get_sac_stat;
	stat_unpack[NID_REQ_STAT_WTP] = _get_tp_stat;
	stat_unpack[NID_REQ_STAT_IO] = _get_io_stat;
	return 0;
}
