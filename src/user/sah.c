/*
 * sah.c
 * 	Implementation of Agent Service Handshake Module
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "nid_log.h"
#include "nid.h"
#include "nid_shared.h"
#include "ini_if.h"
#include "rw_if.h"
#include "rwg_if.h"
#include "sah_if.h"
#include "sac_if.h"
#include "sacg_if.h"

struct sah_private {
	struct sacg_interface	*p_sacg;
	struct rwg_interface	*p_rwg;
	struct sac_info		*p_sac_info;
	int			p_rsfd;				// control socket fd
	int			p_dsfd;				// data socket fd
	u_short			p_dport;			// data port
	char			p_uuid[NID_MAX_UUID];		// uuid
	char			p_exportname[NID_MAX_PATH];
	char			p_ipaddr[NID_MAX_IP];
};

static uint64_t
_find_export_size(char *exportname)
{
	struct stat stat_buf;
	uint64_t size_bytes = 0;
	int fd;

	fd = open(exportname, O_RDONLY);
	if (fd < 0) {
		nid_log_error("_find_export_size: cannot open %s, errno:%d",
			exportname, errno);
		goto out;
	}

	if (!ioctl(fd, BLKGETSIZE64, &size_bytes) && size_bytes) {
		nid_log_debug("_find_export_size: ioctl %s to setup size %lu",
			exportname, size_bytes);

		close(fd);
		goto out;
	}
	close(fd);

	if (!stat(exportname, &stat_buf)) {
		nid_log_debug("_find_export_size: found size by stat");
		size_bytes = stat_buf.st_size;
	} else {
		nid_log_error("_find_export_size: stat %s error, errno:%d",
			exportname, errno);
	}

out:
	nid_log_debug("_find_export_size: size:%lu", size_bytes);
	return size_bytes;
}

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
_recv_next_hs_messag(int sfd, char *buf)
{
	int nread;
	uint16_t msg_len;
	char *p = buf, cmd;

        nread = read_n(sfd, p, 3);
        if (nread != 3) {
               nid_log_error("get_one_message: read error, errno:%d", errno);
	       return -1;
        }
        cmd = *p++;
        msg_len = (*p++) << 8;
	msg_len += *p++;

        nread = read_n(sfd, p, msg_len);
        if (nread < 0) {
               nid_log_error("get_one_message: read error, errno:%d", errno);
	       return -1;
        }
	p[nread] = 0;
        nid_log_debug("get_one_message: got cmd:%d, len:%d, msg:{%s}", cmd, msg_len, p);
	return nread + 3;
}

static int
_make_data_connection(struct sah_private *priv_p, char *ipstr, u_short port)
{
	char *log_header = "_make_data_connection";
	struct sockaddr_in saddr;
	int sfd;
	int flags;
	int opt = 1;
	socklen_t len = sizeof(opt);
	struct timeval timeout;

	nid_log_notice("%s:  start (ip:%s port:%d)", log_header, ipstr, port);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);
	sfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) {
		nid_log_error("cannot connect to the client (%s:%u), errno:%d", ipstr, port, errno);
	} else {
		//bug 6195
		if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len) < 0) {
			nid_log_error("cannot do TCP_NODELAY, errno:%d", errno);
		}
		timeout.tv_sec = SOCKET_WRITE_TIME_OUT;
		timeout.tv_usec = 0;
		if (setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			nid_log_error("set SO_SNDTIMEO failed, errno:%d", errno);
		}
		flags = fcntl(sfd, F_GETFL, 0);
		fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

		priv_p->p_dsfd = sfd;
		priv_p->p_dport = port;
		strncpy(priv_p->p_ipaddr, ipstr, NID_MAX_IP);

		nid_log_error("_make_data_connection successful: %s, %s:%u, sfd:%d",
			priv_p->p_uuid, priv_p->p_ipaddr, priv_p->p_dport, priv_p->p_dsfd);
	}

	return 0;
}

static int
sah_helo_generator(struct sah_interface *sah_p, char *msg_buf)
{
	char cmd, len1, len2;
	uint16_t len;
	char *msg = "Hello, I'm an nid server";

	assert(sah_p);
	cmd = NID_HS_HELO;
	len = strlen(msg);
	len1 = len >> 8;
	len2 = len % 256;
	sprintf(msg_buf, "%c%c%c%s", cmd, len1, len2, msg);
	len += 3;
	nid_log_debug("helo_generator: cmd:%d, len1:%d, len2:%d",
		cmd, len1, len2);
	return len;
}

static int
sah_channel_establish(struct sah_interface *sah_p)
{
	char *log_header = "sah_channel_establish";
	struct sah_private *priv_p = sah_p->h_private;
	struct sacg_interface *sacg_p = priv_p->p_sacg;
	struct rwg_interface *rwg_p = priv_p->p_rwg;
	struct rw_interface *rw_p;
	struct sac_info *sac_info_p;
	char msg_buf[1024], *p;
	unsigned char *p1;
	int msg_len;
	u_short port;
	uint64_t exp_size;
	uint32_t exp_size_l, exp_size_h;
	char *rw_name;

	nid_log_notice("%s: start ...", log_header);

	msg_len = sah_p->h_op->h_helo_generator(sah_p, msg_buf);
	write(priv_p->p_rsfd, msg_buf, msg_len);

	msg_len = _recv_next_hs_messag(priv_p->p_rsfd, msg_buf);
	if (msg_len < 0) {
		nid_log_error("%s: cannot recv hs message", log_header);
		return -1;
	}
	msg_buf[msg_len] = 0;
	sprintf(priv_p->p_uuid, "%s", msg_buf+3);
	nid_log_debug("%s: got uuid {%s}", log_header, priv_p->p_uuid);

	sac_info_p = sacg_p->sag_op->sag_get_sac_info(sacg_p, priv_p->p_uuid);
	if (!sac_info_p) {
		nid_log_error("%s: no uuid matched (uuid:%s)", log_header, priv_p->p_uuid);
		return -1;
	}
	if (!sac_info_p->ready) {
		nid_log_error("%s: sac not ready (uuid:%s)", log_header, priv_p->p_uuid);
		return -1;
	}

	priv_p->p_sac_info = sac_info_p;

	rw_name = sac_info_p->dev_name;
	rw_p = rwg_p->rwg_op->rwg_search(rwg_p, rw_name);
	assert(rw_p);
	if (!rw_p->rw_op->rw_get_device_ready(rw_p)){
		nid_log_warning("%s: rw:%s not ready", log_header, rw_name);
		return -1;
	}

	/* setup rw_type */
	strcpy(priv_p->p_exportname, sac_info_p->export_name);
	if (sac_info_p->dev_size) {
		/* mrw case */
		exp_size = (((uint64_t)sac_info_p->dev_size) << 30);
	} else {
		/* drw case */
		exp_size = _find_export_size(priv_p->p_exportname);
		if (exp_size == 0) {
			nid_log_error("%s: cannot find size of %s", log_header, priv_p->p_exportname);
			return -1;
		}
	}
	nid_log_debug("%s: uuid matched!!!, (uuid:%s, exportname:%s, exportsize:%lu)",
			log_header, priv_p->p_uuid, priv_p->p_exportname, exp_size);

	exp_size_h = exp_size >> 32;
	exp_size_h = htonl(exp_size_h);
	exp_size_l = exp_size & 0xffffffff;
	exp_size_l = htonl(exp_size_l);
	write(priv_p->p_rsfd, &exp_size_h, 4);
	write(priv_p->p_rsfd, &exp_size_l, 4);
	nid_log_notice("%s: (uuid:%s): exp_size_h:%u, exp_size_l:%u",
		log_header, priv_p->p_uuid, exp_size_h , exp_size_l);

	msg_len = _recv_next_hs_messag(priv_p->p_rsfd, msg_buf);
	if (msg_len < 0) {
		nid_log_error("%s: (uuid:%s): cannot recv hs message", log_header, priv_p->p_uuid);
		return -1;
	}
	p = msg_buf + 3;
	p1 = (unsigned char *)strchr(p, ':');
	*p1++ = 0;
        port = 256 * (*p1++);
	port += *p1;

	_make_data_connection(priv_p, p, port);
	return 0;
}

static char*
sah_get_uuid(struct sah_interface *sah_p)
{
	struct sah_private *priv_p = (struct sah_private *)sah_p->h_private;
	return priv_p->p_uuid;
}

static char*
sah_get_exportname(struct sah_interface *sah_p)
{
	struct sah_private *priv_p = (struct sah_private *)sah_p->h_private;
	return priv_p->p_exportname;
}

static int
sah_get_dsfd(struct sah_interface *sah_p)
{
	struct sah_private *priv_p = (struct sah_private *)sah_p->h_private;
	return priv_p->p_dsfd;
}

static char*
sah_get_ipaddr(struct sah_interface *sah_p)
{
	struct sah_private *priv_p = (struct sah_private *)sah_p->h_private;
	return priv_p->p_ipaddr;
}

static void
sah_cleanup(struct sah_interface *sah_p)
{
	struct sah_private *priv_p = (struct sah_private *)sah_p->h_private;

	nid_log_notice("sah_cleanup: %s, %s:%u, dsfd:%d",
		priv_p->p_uuid, priv_p->p_ipaddr, priv_p->p_dport, priv_p->p_dsfd);
	free(priv_p);
	sah_p->h_private = NULL;
}

static struct sac_info *
sah_get_sac_info(struct sah_interface *sah_p)
{
	struct sah_private *priv_p = (struct sah_private *)sah_p->h_private;
	return priv_p->p_sac_info;
}


struct sah_operations sah_op = {
	.h_channel_establish = sah_channel_establish,
	.h_helo_generator = sah_helo_generator,
	.h_get_uuid = sah_get_uuid,
	.h_get_exportname = sah_get_exportname,
	.h_get_dsfd = sah_get_dsfd,
	.h_cleanup = sah_cleanup,
	.h_get_ipaddr = sah_get_ipaddr,
	.h_get_sac_info = sah_get_sac_info,
};

int
sah_initialization(struct sah_interface *sah_p, struct sah_setup *setup)
{
	struct sah_private *priv_p;

	nid_log_debug("sah_initialization start ...");
	if (!setup) {
		nid_log_error("sah_initialization got null setup");
		return -1;
	}

	priv_p = x_calloc(1, sizeof(*priv_p));
	sah_p->h_private = priv_p;
	priv_p->p_sacg = setup->sacg;
	priv_p->p_rwg = setup->rwg;
	priv_p->p_rsfd = setup->rsfd;
	priv_p->p_dsfd = -1;
	sah_p->h_op = &sah_op;
	return 0;
}
