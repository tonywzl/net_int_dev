/*
 * mgrdriver.c
 *	Implementation of Manager to Driver Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "util_nw.h"
#include "nid.h"
#include "nid_log.h"
#include "umpka_if.h"
#include "umpk_driver_if.h"
#include "mgrdriver_if.h"
#include "nid_shared.h"
#include "manager.h"

struct mgrdriver_private {
	char			p_ipstr[16];
	u_short			p_port;
	struct umpka_interface	*p_umpka;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_DRIVER;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);
	sfd = util_nw_make_connection(ipstr, port);

	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d", ipstr, port, errno);
	} else {
		if (util_nw_write_two_byte(sfd, chan_type) != NID_SIZE_CTYPE_MSG){
			nid_log_error("%s: failed to send chan_type, errno:%d", log_header, errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

static int
mgrdriver_version(struct mgrdriver_interface *mgrdriver_p)
{
	char *log_header = "mgrdriver_version";
	struct mgrdriver_private *priv_p = mgrdriver_p->drv_private;
	struct umpka_interface *umpka_p = priv_p->p_umpka;
	struct umessage_driver_version nid_msg;
	struct umessage_driver_version_resp nid_msg_resp;
	struct umessage_driver_hdr *msghdr;
	char *p, buf[4096];
	int sfd = -1;
	uint32_t len, nread;
	int rc = 0, ctype = NID_CTYPE_DRIVER;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		sfd = -1;
		goto out;
	}

	memset(&nid_msg, 0, sizeof(nid_msg));
	msghdr = (struct umessage_driver_hdr *)&nid_msg;
	msghdr->um_req = UMSG_DRIVER_CMD_VERSION;
	msghdr->um_req_code = UMSG_DRIVER_CODE_VERSION;
	umpka_p->um_op->um_encode(umpka_p, buf, &len, ctype, (void *)&nid_msg);
	write(sfd, buf, len);

	msghdr = (struct umessage_driver_hdr *)&nid_msg_resp;
	nread = util_nw_read_n(sfd, buf, UMSG_DRIVER_HEADER_LEN);
	if (nread != UMSG_DRIVER_HEADER_LEN) {
		rc = -1;
		goto out;
	}

	p = buf;
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	p += 4;
	nid_log_debug("%s: req:%d, code: %d, resp_len:%d, done", log_header, msghdr->um_req, msghdr->um_req_code, msghdr->um_len);

	assert(msghdr->um_req == UMSG_DRIVER_CMD_VERSION);
	assert(msghdr->um_req_code == UMSG_DRIVER_CODE_RESP_VERSION);
	assert(msghdr->um_len >= UMSG_DRIVER_HEADER_LEN);
	
	nread = util_nw_read_n(sfd, p, msghdr->um_len - UMSG_DRIVER_HEADER_LEN);
	if (nread < (msghdr->um_len - UMSG_DRIVER_HEADER_LEN)) {
		rc = -1;
		goto out;
	}
	umpka_p->um_op->um_decode(umpka_p, buf, msghdr->um_len, NID_CTYPE_DRIVER, msghdr);
	printf("version: %s\n", nid_msg_resp.um_version);
	
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;

}

struct mgrdriver_operations mgrdriver_op = {
	.driver_version = mgrdriver_version,
};

int 
mgrdriver_initialization(struct mgrdriver_interface *mgrdriver_p, struct mgrdriver_setup *setup)
{
	char *log_header = "mgrdriver_initialization";
	struct mgrdriver_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrdriver_p->drv_private = priv_p;
	mgrdriver_p->drv_op = &mgrdriver_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpka = setup->umpka;
	return 0;
}
