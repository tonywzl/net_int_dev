/*
 * mgrall.c
 * 	Implementation of Manager to ALL Channel Module
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
#include "mgrall_if.h"

#include "nid_shared.h"

struct mgrall_private {
	char    		p_ipstr[16];
	u_short			p_port;
	struct umpk_interface	*p_umpk;
};

static int
__make_connection(char *ipstr, u_short port)
{
	char *log_header = "__make_connection";
	int sfd;
	uint16_t chan_type = NID_CTYPE_ALL;

	nid_log_debug("%s: start (ip:%s port:%d)", log_header, ipstr, port);
	sfd = util_nw_make_connection(ipstr, port);

	if (sfd < 0) {
		nid_log_error("cannot connect to the client (%s:%d), errno:%d",
			ipstr, port, errno);
	} else {
		if (util_nw_write_two_byte(sfd, chan_type) != NID_SIZE_CTYPE_MSG) {
			nid_log_error("%s: failed to send chan_type, errno:%d", log_header, errno);
			close(sfd);
			sfd = -1;
		}
	}

	return sfd;
}

static int
mgrall_display(struct mgrall_interface *mgrall_p,
		struct mgrtp_interface *mgrtp_p,
		struct mgrpp_interface *mgrpp_p,
		struct mgrbwc_interface *mgrbwc_p,
		struct mgrtwc_interface *mgrtwc_p,
		struct mgrcrc_interface *mgrcrc_p,
		struct mgrsds_interface *mgrsds_p,
		struct mgrmrw_interface *mgrmrw_p,
		struct mgrdrw_interface *mgrdrw_p,
		struct mgrdx_interface *mgrdx_p,
		struct mgrcx_interface *mgrcx_p,
		struct mgrsac_interface *mgrsac_p)
{
	char *log_header = "mgrall_display";
	struct mgrall_private *priv_p = mgrall_p->al_private;
	int sfd = -1;
	char buf = 0;
	int rc = 0;

	nid_log_debug("%s: start ...", log_header);
	sfd = __make_connection(priv_p->p_ipstr, priv_p->p_port);
	if (sfd < 0) {
		rc = -1;
		goto out;
	}

	read(sfd, &buf, 1);
	if (buf != 0) {
		nid_log_warning("%s cannot get lock!", log_header);
		goto out;
	}
	
	mgrtp_p->t_op->tp_display_all(mgrtp_p, 1);
	mgrpp_p->pp_op->pp_display_all(mgrpp_p, 1);
	mgrbwc_p->bw_op->bw_display_all(mgrbwc_p, 1);
	mgrtwc_p->tw_op->tw_display_all(mgrtwc_p, 1);
	mgrcrc_p->mcr_op->mcr_display_all(mgrcrc_p, 1);
	mgrsds_p->sds_op->sds_display_all(mgrsds_p, 1);
	mgrmrw_p->mr_op->mr_display_all(mgrmrw_p, 1);
	mgrdrw_p->drw_op->drw_display_all(mgrdrw_p, 1);
	mgrdx_p->dx_op->dx_display_all(mgrdx_p, 1);
	mgrcx_p->cx_op->cx_display_all(mgrcx_p, 1);
	mgrsac_p->sa_op->sa_display_all(mgrsac_p, 1);

	write(sfd, 0, 1);
out:
	if (sfd >= 0) {
		close(sfd);
	}
	return rc;
}

struct mgrall_operations mgrall_op = {
	.al_display = mgrall_display,
};

int
mgrall_initialization(struct mgrall_interface *mgrall_p, struct mgrall_setup *setup)
{
	struct mgrall_private *priv_p;

	nid_log_debug("mgrall_initialization start ...");
	priv_p = x_calloc(1, sizeof(*priv_p));
	mgrall_p->al_private = priv_p;
	mgrall_p->al_op = &mgrall_op;

	strncpy(priv_p->p_ipstr, setup->ipstr, 16);
	priv_p->p_port = setup->port;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
