/*
 * mac.c
 * 	Implementation of Manage Service Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "nid_shared.h"
#include "nid.h"
#include "nid_log.h"
#include "mpk_if.h"
#include "dsm_if.h"
#include "mac_if.h"
#include "nid_shared.h"
#include "util_nw.h"

#define MAC_STAT_ALL	1
#define MAC_STAT_WD	2
#define MAC_STAT_WU	3
#define MAC_STAT_WUD	4
#define MAC_STAT_RWWD	5
#define MAC_STAT_RWWU	6
#define MAC_STAT_RWWUD	7
#define MAC_STAT_UD	8

struct mac_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct mpk_interface	*p_mpk;
};

typedef void (*stat_unpack_func_t)(struct nid_message *, int what);
stat_unpack_func_t stat_unpack[NID_REQ_MAX];


static int
make_connection(char *ipstr, u_short port)
{
	struct sockaddr_in saddr;
	int sfd;
	//char chan_type = NID_CTYPE_ADM;
	uint16_t chan_type = NID_CTYPE_ADM;
        struct timeval timeout;

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
		timeout.tv_sec = SOCKET_TIME_OUT;
		timeout.tv_usec = 0;
		if (setsockopt(sfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			nid_log_warning("set SO_SNDTIMEO failed, errno:%d", errno);
		}
		if (setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
			nid_log_warning("set SO_RCVTIMEO failed, errno:%d", errno);
		}
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

static void
_get_asc_stat(struct nid_message *nid_msg, int what)
{
	char uuid[NID_MAX_UUID], devname[NID_MAX_DEVNAME], ip[NID_MAX_IP];
	char *dent = "   ";

	nid_log_info("_get_asc_stat: what:%d", what);
	if (what == MAC_STAT_WD) {
		if (nid_msg->m_has_devname &&
		    nid_msg->m_has_state && nid_msg->m_state == DSM_STATE_WORK) {
			memcpy(devname, nid_msg->m_devname, nid_msg->m_devnamelen);
			devname[nid_msg->m_devnamelen] = 0;
			printf("%s\n", devname);
		}
		return;
	}

	if (what == MAC_STAT_WU) {
		if (nid_msg->m_has_uuid &&
		    nid_msg->m_has_state && nid_msg->m_state == DSM_STATE_WORK) {
			memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
			uuid[nid_msg->m_uuidlen] = 0;
			printf("%s\n", uuid);
		}
		return;
	}

	if (what == MAC_STAT_WUD) {
		if (nid_msg->m_has_uuid && nid_msg->m_has_devname &&
		    nid_msg->m_has_state && nid_msg->m_state == DSM_STATE_WORK) {
			memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
			uuid[nid_msg->m_uuidlen] = 0;
			memcpy(devname, nid_msg->m_devname, nid_msg->m_devnamelen);
			devname[nid_msg->m_devnamelen] = 0;
			printf("%s %s\n", uuid, devname);
		}
		return;
	}

	if (what == MAC_STAT_RWWD) {
		if (nid_msg->m_has_devname &&
		    nid_msg->m_has_state && nid_msg->m_state == DSM_STATE_WORK &&
		    nid_msg->m_has_alevel && nid_msg->m_alevel >= NID_ALEVEL_RDWR) {
			memcpy(devname, nid_msg->m_devname, nid_msg->m_devnamelen);
			devname[nid_msg->m_devnamelen] = 0;
			printf("%s\n", devname);
		}
		return;
	}

	if (what == MAC_STAT_RWWU) {
		if (nid_msg->m_has_uuid &&
		    nid_msg->m_has_state && nid_msg->m_state == DSM_STATE_WORK &&
		    nid_msg->m_has_alevel && nid_msg->m_alevel >= NID_ALEVEL_RDWR) {
			memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
			uuid[nid_msg->m_uuidlen] = 0;
			printf("%s\n", uuid);
		}
		return;
	}

	if (what == MAC_STAT_RWWUD) {
		if (nid_msg->m_has_uuid && nid_msg->m_has_devname &&
		    nid_msg->m_has_state && nid_msg->m_state == DSM_STATE_WORK &&
		    nid_msg->m_has_alevel && nid_msg->m_alevel >= NID_ALEVEL_RDWR) {
			memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
			uuid[nid_msg->m_uuidlen] = 0;
			memcpy(devname, nid_msg->m_devname, nid_msg->m_devnamelen);
			devname[nid_msg->m_devnamelen] = 0;
			printf("%s %s\n", uuid, devname);
		}
		return;
	}

	if (what == MAC_STAT_UD) {
		if (nid_msg->m_has_uuid && nid_msg->m_has_devname) {
			memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
			uuid[nid_msg->m_uuidlen] = 0;
			memcpy(devname, nid_msg->m_devname, nid_msg->m_devnamelen);
			devname[nid_msg->m_devnamelen] = 0;
			printf("%s %s\n", uuid, devname);
		}
		return;
	}

	printf("Service Agent Channel:\n");
	if (nid_msg->m_has_uuid) {
		memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
		uuid[nid_msg->m_uuidlen] = 0;
		printf("%suuid:%s\n", dent, uuid);
	}

	if (nid_msg->m_has_ip) {
		memcpy(ip, nid_msg->m_ip, nid_msg->m_iplen);
		ip[nid_msg->m_iplen] = 0;
		printf("%sip:%s\n", dent, ip);
	}

	if (nid_msg->m_has_devname) {
		memcpy(devname, nid_msg->m_devname, nid_msg->m_devnamelen);
		devname[nid_msg->m_devnamelen] = 0;
		printf("%sdevname:%s\n", dent, devname);
	}

	if (nid_msg->m_has_state) {
		printf("%sstate:%s\n", dent, nid_state_to_str(nid_msg->m_state));
	}

	if (nid_msg->m_has_alevel) {
		printf("%salevel:%s\n", dent, nid_alevel_to_str(nid_msg->m_alevel));
	}

	if (nid_msg->m_has_rcounter) {
		printf("%srcounter:%lu\n", dent, nid_msg->m_rcounter);
	}

	if (nid_msg->m_has_r_rcounter) {
		printf("%srrcounter:%lu\n", dent, nid_msg->m_r_rcounter);
	}

	if (nid_msg->m_has_wcounter) {
		printf("%swcounter:%lu\n", dent, nid_msg->m_wcounter);
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
		printf("%sread_sequence:%lu\n", dent, nid_msg->m_recv_sequence);
	}

	if (nid_msg->m_has_wait_sequence) {
		printf("%swrite_sequence:%lu\n", dent, nid_msg->m_wait_sequence);
	}

	if (nid_msg->m_has_live_time) {
		printf("%slive_time:%dsec\n", dent, nid_msg->m_live_time);
	}

}

static void
_get_boc_stat(struct nid_message *nid_msg, int what)
{
	char uuid[NID_MAX_UUID];
	char *dent = "   ";
	char *channels;

	assert(what == MAC_STAT_ALL);
	printf("Bundle of Channel Information:\n");
	if (nid_msg->m_has_uuid) {
		memcpy(uuid, nid_msg->m_uuid, nid_msg->m_uuidlen);
		uuid[nid_msg->m_uuidlen] = 0;
		printf("%suuid:%s\n", dent, uuid);
	}
	if (nid_msg->m_has_size) {
		printf("%ssize:%lu\n", dent, nid_msg->m_size);
	}
	if (nid_msg->m_has_devname) {
		channels = x_malloc(nid_msg->m_devnamelen + 1);
		memcpy(channels, nid_msg->m_devname, nid_msg->m_devnamelen);
		channels[nid_msg->m_devnamelen] = 0;
		printf("%schannels:%s\n", dent, channels);
		free(channels);
	}
}

static int
mac_stop(struct mac_interface *mac_p)
{
	struct mac_private *priv_p = mac_p->m_private;
	int sfd = -1, rc = 0;
	char mac_cmd = NID_REQ_STOP;
	char nothing;

	nid_log_debug("mac_stop %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	util_nw_write_n(sfd, &mac_cmd, 1);
	util_nw_read_n(sfd, &nothing, 1);
	printf("nidagent stop successfully\n");

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mac_ioerror(struct mac_interface *mac_p, char *devname)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	int sfd = -1, rc = -1, len = 0;
	char msg_buf[512], nothing;
	struct nid_message nid_msg;
	char code = 0;
	int nread = -1;

	nid_log_debug("mac_ioerror(%s:%u): %s", priv_p->p_ipstr, priv_p->p_port, devname);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	len = 512;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_IOERROR_DEVICE;
	nid_msg.m_devname = devname;
	nid_msg.m_devnamelen = strlen(devname);
	nid_msg.m_has_devname = 1;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	util_nw_write_n(sfd, msg_buf, len);
	nread = util_nw_read_n(sfd, &code, 1);
	if (nread == 1)
		rc = code;
	util_nw_read_n(sfd, &nothing, 1);
	if (rc == 0)
		printf("nidagent ioerror %s successfully\n", devname);
	else
		printf("nidagent ioerror %s fail\n", devname);
out:
	return rc;
}

static int
mac_delete(struct mac_interface *mac_p, char *devname)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	int sfd = -1, rc = 0, len = 0;
	char msg_buf[512], nothing;
	struct nid_message nid_msg;

	nid_log_debug("mac_delete (%s:%u): %s", priv_p->p_ipstr, priv_p->p_port, devname);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	len = 512;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_DELETE;
	nid_msg.m_devname = devname;
	nid_msg.m_devnamelen = strlen(devname);
	nid_msg.m_has_devname = 1;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	util_nw_write_n(sfd, msg_buf, len);
	util_nw_read_n(sfd, &nothing, 1);
	printf("nidagent delete %s successfully\n", devname);
out:
	return rc;
}

static int
_mac_stat_get(struct mac_interface *mac_p, struct mac_stat *stat_p, int what)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message res;
	int sfd = -1, rc = 0;
	char mac_cmd = NID_REQ_STAT_GET;
	char *msg_buf, msg_hdr[5];
	int len, idx;

	assert(stat_p);
	nid_log_debug("mac_stat_get %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	util_nw_write_n(sfd, &mac_cmd, 1);
	while (util_nw_read_n(sfd, &msg_hdr, 6) == 6) {
		len = be32toh(*(uint32_t *)(msg_hdr + 2));
		if (len > 6) {
			msg_buf = x_malloc(len);
			if (util_nw_read_n(sfd, msg_buf + 6, len - 6) < 0) {
				rc = -1;
				goto out;
			}
			memcpy(msg_buf, msg_hdr, 6);
		}
		mpk_p->m_op->m_decode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&res);
		idx = msg_buf[0];
		stat_unpack[idx](&res, what);
	}
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mac_stat_get(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_ALL);
}

static int
mac_stat_get_wd(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_WD);
}

static int
mac_stat_get_wu(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_WU);
}

static int
mac_stat_get_wud(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_WUD);
}

static int
mac_stat_get_rwwd(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_RWWD);
}

static int
mac_stat_get_rwwu(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_RWWU);
}

static int
mac_stat_get_rwwud(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_RWWUD);
}

static int
mac_stat_get_ud(struct mac_interface *mac_p, struct mac_stat *stat_p)
{
	return _mac_stat_get(mac_p, stat_p, MAC_STAT_UD);
}

static int
mac_stat_reset(struct mac_interface *mac_p)
{
	struct mac_private *priv_p = mac_p->m_private;
	int sfd = -1, rc = 0;
	char mac_cmd = NID_REQ_STAT_RESET;
	char nothing;

	assert(priv_p);
	nid_log_debug("mac_stat_reset %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	util_nw_write_n(sfd, &mac_cmd, 1);
	util_nw_read_n(sfd, &nothing, 1);
	printf("nids reset stat successfully\n");
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mac_update(struct mac_interface *mac_p)
{
	struct mac_private *priv_p = mac_p->m_private;
	int sfd = -1, rc = 0;
	char mac_cmd = NID_REQ_UPDATE;
	char nothing;

	assert(priv_p);
	nid_log_debug("mac_update %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		nid_log_error("mac_update: make_connection failed");
		rc = 1;
		goto out;
	}

	if (util_nw_write_n(sfd, &mac_cmd, 1) != 1) {
		nid_log_error("mac_update: failed to write command (NID_REQ_UPDATE)");
		rc = 1;
		goto out;
	}

	util_nw_read_n(sfd, &nothing, 1);
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
mac_check_agent(struct mac_interface *mac_p)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	struct nid_message nid_msg;
	char *msg_buf, *resp_buf, msg_hdr[8];
	int len, sfd = -1, rc = -1;

	nid_log_debug("mac_check_agent: %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		goto out;
	}

	msg_buf = x_malloc(32);
	len = 32;
	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_CHECK_AGENT;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	util_nw_write_n(sfd, msg_buf, len);

	if (util_nw_read_n(sfd, &msg_hdr, 5) == 5) {
		len = be32toh(*(uint32_t *)(msg_hdr + 1));
		resp_buf = x_malloc(len);
		if (len > 5) {
			if (util_nw_read_n(sfd, resp_buf + 5, len - 5) < 0) {
				rc = -1;
				goto out;
			}
		}
		memcpy(resp_buf, msg_hdr, 5);
		memset(&nid_msg, 0, sizeof(nid_msg));
		mpk_p->m_op->m_decode(mpk_p, resp_buf, &len, (struct nid_message_hdr *)&nid_msg);
		if (nid_msg.m_req == NID_REQ_CHECK_AGENT) {
			rc = 0;
		}
	}
	close(sfd);

out:
	if (!rc)
		printf("The agent is alive\n");
	else
		printf("The agent does not exist\n");
	return rc;
}

static int
mac_check_conn(struct mac_interface *mac_p, char *uuid)
{
	struct mac_private *priv_p = mac_p->m_private;
	int sfd = -1, rc = 0;
	char mac_cmd = NID_REQ_CHECK_CONN;
	char nothing, got_it = 1, sac_exists, len = strlen(uuid);

	nid_log_debug("mac_conn_check: %s:%u, uuid:%s", priv_p->p_ipstr, priv_p->p_port, uuid);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	util_nw_write_n(sfd, &mac_cmd, 1);
	util_nw_write_n(sfd, &len, 1);
	util_nw_write_n(sfd, uuid, len);
	util_nw_read_n(sfd, &sac_exists, 1);
	printf("nids check conn successfully (sac_exists:%d)\n", sac_exists);
	if (!sac_exists)
		rc = 1;
	util_nw_write_n(sfd, &got_it, 1);
	util_nw_read_n(sfd, &nothing, 1);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mac_upgrade(struct mac_interface *mac_p, char *devname)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	int sfd = -1, rc = 0;
	struct nid_message nid_msg;
	char msg_buf[512], code;
	int len = 512;
	fd_set rfds;
	struct timeval tv;

	assert(priv_p);
	nid_log_debug("mac_upgrade %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		nid_log_error("mac_update: make_connection failed");
		rc = EFAULT;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_UPGRADE;
	nid_msg.m_devname = devname;
	nid_msg.m_devnamelen = strlen(devname);
	nid_msg.m_has_devname = 1;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	util_nw_write_n(sfd, msg_buf, len);

	FD_ZERO(&rfds);
	FD_SET(sfd, &rfds);

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	rc = select(sfd + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);
	if(rc > 0 && FD_ISSET(sfd, &rfds)) {
		util_nw_read_n(sfd, &code, 1);
		rc = code;
	} else {
		rc = ETIMEDOUT;
	}

	close(sfd);

out:
	if (!rc)
		printf("nids upgrade successfully\n");
	else
		printf("nids upgrade failed: %d\n", rc);

	return rc;
}

static int
mac_set_log_level(struct mac_interface *mac_p, int level)
{
	struct mac_private *priv_p = mac_p->m_private;
	int sfd = -1, rc = 0;
	char mac_cmd = NID_REQ_SET_LOG_LEVEL;
	char nothing;

	assert(priv_p);
	nid_log_debug("mac_set_log_level %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	util_nw_write_n(sfd, &mac_cmd, sizeof(mac_cmd));
	util_nw_write_n(sfd, &level, sizeof(level));
	util_nw_read_n(sfd, &nothing, sizeof(nothing));

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mac_get_version(struct mac_interface *mac_p)
{
	struct mac_private *priv_p = mac_p->m_private;
	int sfd = -1, rc = 0;
	char cmd = NID_REQ_GET_VERSION;
	char buffer[16];

	assert(priv_p);
	nid_log_debug("mac_get_version %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	util_nw_write_n(sfd, &cmd, sizeof(cmd));

	util_nw_read_n(sfd, buffer, 16);
	printf("Agent build time:   %s ", buffer);
	util_nw_read_n(sfd, buffer, 16);
	printf("%s\n", buffer);

	util_nw_read_n(sfd, buffer, 16);
	printf("Agent log level:    %s\n", buffer);

out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

static int
mac_upgrade_force(struct mac_interface *mac_p, char *devname)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	int sfd = -1, rc = 0;
	struct nid_message nid_msg;
	char msg_buf[512], code;
	int len = 512;

	assert(priv_p);
	nid_log_debug("mac_upgrade_force %s:%u", priv_p->p_ipstr, priv_p->p_port);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		nid_log_error("mac_update_force: make_connection failed");
		rc = EFAULT;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_UPGRADE;
	nid_msg.m_devname = devname;
	nid_msg.m_devnamelen = strlen(devname);
	nid_msg.m_has_devname = 1;
	nid_msg.m_has_upgrade_force = 1;
	nid_msg.m_upgrade_force = 1;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	util_nw_write_n(sfd, msg_buf, len);

	util_nw_read_n(sfd, &code, 1);
	rc = code;

	close(sfd);

out:
	printf("Forcely upgrade %s %s.\n", devname, (rc ? "failed" : "successful"));
	return rc;
}

static int
mac_set_hook(struct mac_interface *mac_p, int cmd)
{
	struct mac_private *priv_p = mac_p->m_private;
	struct mpk_interface *mpk_p = priv_p->p_mpk;
	int sfd = -1, rc = 0;
	struct nid_message nid_msg;
	char msg_buf[128], code;
	int len = 128;

	assert(priv_p);
	nid_log_debug("mac_set_hook: %d", cmd);
	sfd = make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		nid_log_error("mac_set_hook: make_connection failed");
		rc = EFAULT;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	nid_msg.m_req = NID_REQ_SET_HOOK;
	nid_msg.m_cmd = cmd;
	nid_msg.m_has_cmd = 1;
	mpk_p->m_op->m_encode(mpk_p, msg_buf, &len, (struct nid_message_hdr *)&nid_msg);
	util_nw_write_n(sfd, msg_buf, len);

	util_nw_read_n(sfd, &code, 1);
	rc = code;

	close(sfd);

out:
	printf("%s hook functions %s.\n", (cmd ? "enable" : "disable"),
		(rc ? "failed" : "successful"));
	return rc;
}

struct mac_operations mac_op = {
	.m_stop = mac_stop,
	.m_ioerror= mac_ioerror,
	.m_delete = mac_delete,
	.m_stat_reset = mac_stat_reset,
	.m_stat_get = mac_stat_get,
	.m_stat_get_wd = mac_stat_get_wd,
	.m_stat_get_wu = mac_stat_get_wu,
	.m_stat_get_wud = mac_stat_get_wud,
	.m_stat_get_rwwd = mac_stat_get_rwwd,
	.m_stat_get_rwwu = mac_stat_get_rwwu,
	.m_stat_get_rwwud = mac_stat_get_rwwud,
	.m_stat_get_ud = mac_stat_get_ud,
	.m_update = mac_update,
	.m_check_agent = mac_check_agent,
	.m_check_conn = mac_check_conn,
	.m_upgrade = mac_upgrade,
	.m_set_log_level = mac_set_log_level,
	.m_get_version = mac_get_version,
	.m_upgrade_force = mac_upgrade_force,
	.m_set_hook = mac_set_hook,
};

int
mac_initialization(struct mac_interface *mac_p, struct mac_setup *setup)
{
	struct mac_private *priv_p;

	nid_log_debug("mac_initialization start ...");
	if (!setup) {
		nid_log_error("mac_initialization got null setup");
		return -1;
	}

	priv_p = x_calloc(1, sizeof(*priv_p));
	mac_p->m_private = priv_p;
	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_mpk = setup->mpk;

	mac_p->m_op = &mac_op;
	stat_unpack[NID_REQ_STAT] = _get_asc_stat;
	stat_unpack[NID_REQ_STAT_BOC] = _get_boc_stat;
	return 0;
}
