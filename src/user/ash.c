/*
 * ash.c
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
#include <time.h>

#include "nid_log.h"
#include "nid.h"
#include "nid_shared.h"
#include "ini_if.h"
#include "ash_if.h"
#include "util_nw.h"

#define ASH_RW_TIMEOUT	10

struct ash_private {
	struct ini_interface	*p_ini;
	int			p_role;				// server or client
	int			p_rsfd;				// control socket fd
	int			p_dsfd;				// data socket fd
	int			p_retry;
	u_short			p_dport;			// data port
	char			p_uuid[NID_MAX_UUID];		// uuid
	char			p_sip[NID_MAX_IP];		// servic ip str
	char			p_exportname[NID_MAX_PATH];	// exportname
	char			p_sync;
};

static int
_recv_next_ha_messag(int sfd, char *buf)
{
	int nread;
	uint16_t msg_len;
	char *p = buf, cmd;

        nread = util_nw_read_n_timeout(sfd, p, 3, ASH_RW_TIMEOUT);
        if (nread != 3) {
               nid_log_error("get_one_message: read error, errno:%d", errno);
	       return -1;
        }
        cmd = *p++;
        msg_len = (*p++) << 8;
	msg_len += *p++;

        nread = util_nw_read_n_timeout(sfd, p, msg_len, ASH_RW_TIMEOUT);
        if (nread != msg_len) {
               nid_log_error("get_one_message: read error, errno:%d", errno);
	       return -1;
        }
	p[nread] = 0;
        nid_log_debug("get_one_message: got cmd:%d, len:%d, msg:{%s}", cmd, msg_len, p);
	return nread + 3;
}

static int
_make_request_connection(char *ipstr, u_short port)
{
	int sfd;
	struct sockaddr_in saddr;
	int opt = 1;
	socklen_t len = sizeof(opt);
	uint16_t chan_type = NID_CTYPE_REG;
	int flags, ret, error = 0;
	int nsec = 1;
	fd_set rset, wset;
	struct timeval	tval;
//	char buf;

	//nid_log_info("client _make_request_connection start (%s, %d)", ipstr, port);
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = inet_addr(ipstr);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (sfd < 0) {
		nid_log_error("cannot open a socket");
		return -1;
	}
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len) < 0) {
		nid_log_error("cannot do TCP_NODELAY, errno:%d", errno);
	}
	flags = fcntl(sfd, F_GETFL, 0);
	fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

	ret = connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret == 0) {
		goto done;
	}
	if (ret < 0) {
		nid_log_warning("_make_request_connection: connect return error (errno:%d)", errno);
		if (errno != EINPROGRESS) {
			close(sfd);
			return -1;
		}
	}

	nid_log_warning("_make_request_connection: got error EINPROGRESS");
	FD_ZERO(&rset);
	FD_SET(sfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ((ret = select(sfd + 1, &rset, &wset, NULL, nsec ? &tval : NULL)) <= 0) {
		nid_log_error("cannot connect to the server (%s:%d), errno:%d",
			ipstr, port, errno);
		close(sfd);		/* timeout */
		errno = ETIMEDOUT;
		return -1;
	}

	if (FD_ISSET(sfd, &rset) || FD_ISSET(sfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			nid_log_error("getsockopt return error: %d.", errno);
			close(sfd);
			return -1;			/* Solaris pending error */
		}
	} else {
		nid_log_error("select error: sockfd not set");
		close(sfd);
		return -1;
	}

done:
	//fcntl(sfd, F_SETFL, flags);	/* restore file status flags */
	if (error) {
		nid_log_error("_make_request_connection: error:%d", error);
		close(sfd);		/* just in case */
		errno = error;
		return -1;
	}

//	buf = (chan_type >> 8) | 0x80;
//	util_nw_write_timeout(sfd, &buf, 1, ASH_RW_TIMEOUT);
//	buf = chan_type;
//	util_nw_write_timeout(sfd, &buf, 1, ASH_RW_TIMEOUT);
	util_nw_write_two_byte_timeout(sfd, chan_type, ASH_RW_TIMEOUT);
	return sfd;
}

static int
_make_data_connection(u_short port)
{
	struct sockaddr_in saddr;
	int sfd;
	int opt = 1;
	socklen_t len = sizeof(opt);

	//nid_log_info("client _make_data_connection start (port:%d)", port);
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);

	sfd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (sfd < 0) {
		nid_log_error("_make_data_connection: cannot open socket, errno:%d", errno);
		return -1;
	}
	if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		close(sfd);
		return -1;
	}

	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
	if (listen(sfd, 10) < 0) {
		close(sfd);
		return -1;
	}
	return sfd;
}

static int
ash_exp_generator(struct ash_interface *ash_p, char *exportname, char *msg_buf)
{
	char cmd, len1, len2;
	uint16_t len;
	char *msg = exportname;

	assert(ash_p);
	cmd = NID_HS_EXP;
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
ash_port_generator(struct ash_interface *ash_p, char *ipstr, u_short port, char *msg_buf)
{
	char cmd, len1, len2;
	uint16_t len;
	char buf[128];

	assert(ash_p);
	cmd = NID_HS_PORT;
	sprintf(buf, "%s:%c%c", ipstr, port>>8, port%256);
       	len = strlen(buf);
	len1 = len >> 8;
	len2 = len % 256;
	sprintf(msg_buf, "%c%c%c%s", cmd, len1, len2, buf);
	len += 3;
	nid_log_debug("port:%d, port1:%d, port2:%d", port, port>>8, port%256);
	return len;
}

static int
ash_create_channel(struct ash_interface *ash_p, struct ash_channel_info *info_p)
{
	struct ash_private *priv_p = ash_p->h_private;
	char buf[1024];
	int len, nwrite, rc, a_sfd, opt = 1;
	uint32_t size_h, size_l;
	uint64_t exp_size;
	u_short port;
	socklen_t clen;
	struct sockaddr_in caddr;
	struct timeval tv;
	fd_set rfds;

	nid_log_debug("ash_create_channel: start(uuid:%s, ip:%s) ...",
		priv_p->p_uuid, priv_p->p_sip);

	assert(ash_p);
	priv_p->p_rsfd = _make_request_connection(priv_p->p_sip, NID_SERVICE_PORT);
	if (priv_p->p_rsfd < 0) {
		nid_log_debug("ash_create_channel(uuid:%s): _make_request_connection return error",
			priv_p->p_uuid);
		return -1;
	}

	/* read helo message from the service */
	rc = _recv_next_ha_messag(priv_p->p_rsfd, buf);
	if (rc < 0) {
		close(priv_p->p_rsfd);
		priv_p->p_rsfd = -1;
		return -1;
	}

	len = ash_p->h_op->h_exp_generator(ash_p, priv_p->p_uuid, buf);
	nwrite = util_nw_write_timeout(priv_p->p_rsfd, buf, len, ASH_RW_TIMEOUT);
	if (nwrite != len) {
		buf[len] = 0;
		nid_log_error("ash_create_channel(uuid:%s): write(%s) incorrectly (len:%d, nwrite:%d",
			priv_p->p_uuid, buf, len, nwrite);
		close(priv_p->p_rsfd);
		return -1;
	}

	/* setup size */
	if (util_nw_read_n_timeout(priv_p->p_rsfd, (char *)&size_h, 4, ASH_RW_TIMEOUT) != 4 ||
	    util_nw_read_n_timeout(priv_p->p_rsfd, (char *)&size_l, 4, ASH_RW_TIMEOUT) != 4) {
		nid_log_debug("channel_establish(uuid:%s): fd:%d read_n error",
			priv_p->p_uuid, priv_p->p_rsfd);
		close(priv_p->p_rsfd);
		return -1;
	}
	size_l = ntohl(size_l);
	size_h = ntohl(size_h);
	exp_size = size_h;
	exp_size <<= 32;
	exp_size += size_l;
	nid_log_debug("channel_establish(uuid:%s): size:%lu, size_h:%u, size_l:%u",
		priv_p->p_uuid, exp_size, size_h, size_l);
	info_p->i_size = exp_size;

	port = NID_SERVICE_PORT;
	do {
		port++;
		a_sfd = _make_data_connection(port);
	} while (a_sfd < 0);
	nid_log_debug("channel_establish(uuid:%s): _make_data_connection successful at port:%d",
		priv_p->p_uuid, port);
	clen = sizeof(caddr);
	getsockname(priv_p->p_rsfd, (struct sockaddr *)&caddr, &clen);
	nid_log_debug("channel_establish(uuid:%s): my ipstr:%s, clen:%d, errno:%d",
		priv_p->p_uuid, inet_ntoa(caddr.sin_addr), clen, errno);

	len = ash_p->h_op->h_port_generator(ash_p, inet_ntoa(caddr.sin_addr), port, buf);
	nwrite = util_nw_write_timeout(priv_p->p_rsfd, buf, len, ASH_RW_TIMEOUT);
	if (nwrite != len) {
		nid_log_error("channel_establish(uuid:%s): write(%s) incorrectly (len:%d, nwrite:%d",
			priv_p->p_uuid, buf, len, nwrite);
		close(a_sfd);
		close(priv_p->p_rsfd);
		return -1;
	}

	clen = sizeof(caddr);

	FD_ZERO(&rfds);
	FD_SET(a_sfd, &rfds);

	tv.tv_sec = 8;
	tv.tv_usec = 0;

	rc = select(a_sfd + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);
	if(rc > 0 && FD_ISSET(a_sfd, &rfds)) {
		priv_p->p_dsfd = accept(a_sfd, (struct sockaddr *)&caddr, &clen);
		if (fcntl(priv_p->p_dsfd, F_SETFD, fcntl(priv_p->p_dsfd, F_GETFL)|FD_CLOEXEC) < 0) {
			nid_log_warning("fcntl(FD_CLOEXEC) failed while create channel, error: %d\n", errno);
		}
	} else {
		nid_log_error("channel_establish(uuid:%s): accept incorrectly",
			priv_p->p_uuid);
		close(a_sfd);
		close(priv_p->p_rsfd);
		return -1;
	}

	nid_log_info("channel_establish(uuid:%s) got connect from the server, fd:%d",
		priv_p->p_uuid, priv_p->p_dsfd);
	close(a_sfd);

	if (setsockopt(priv_p->p_dsfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0) {
		nid_log_error("channel_establish(uuid:%s): cannot do TCP_NODELAY, errno:%d",
			priv_p->p_uuid, errno);
		close(priv_p->p_rsfd);
		priv_p->p_rsfd = -1;
		close(priv_p->p_dsfd);
		priv_p->p_dsfd = -1;
		return -1;
	}

	info_p->i_rsfd = priv_p->p_rsfd;
	info_p->i_dsfd = priv_p->p_dsfd;
	return 0;
}

static void
ash_cleanup(struct ash_interface *ash_p)
{
	struct ash_private *priv_p = ash_p->h_private;
	free(priv_p);
	ash_p->h_private = NULL;
}

struct ash_operations ash_op = {
	.h_create_channel = ash_create_channel,
	.h_exp_generator = ash_exp_generator,
	.h_port_generator = ash_port_generator,
	.h_cleanup = ash_cleanup,
};

int
ash_initialization(struct ash_interface *ash_p, struct ash_setup *setup)
{
	struct ash_private *priv_p;

	nid_log_debug("ash_initialization start (ip:%s, uuid:%s)...",
		setup->sip, setup->uuid);
	if (!setup) {
		nid_log_error("ash_initialization got null setup");
		return -1;
	}

	priv_p = x_calloc(1, sizeof(*priv_p));
	ash_p->h_private = priv_p;
	priv_p->p_ini = setup->ini;
	strcpy(priv_p->p_uuid, setup->uuid);
	strcpy(priv_p->p_sip, setup->sip);
	ash_p->h_op = &ash_op;
	return 0;
}
