/*
 * doa.c
 * 	Implementation of Delegation of Authority Module
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "util_nw.h"
#include "nid_shared.h"
#include "nid_log.h"
#include "scg_if.h"
#include "doac_if.h"
#include "doa_if.h"
#include "doacg_if.h"
#include "umpk_if.h"
#include "umpk_doa_if.h"

struct doac_private {
	struct doacg_interface  *p_doacg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
	uint8_t			p_is_old_command;
};

static void
__doac_sub(struct doac_private *priv_p, char *msg_buf, struct umessage_doa_information *doa_msg)
{
	char *log_header = "__doac_sub";
	struct doacg_interface *doacg_p = priv_p->p_doacg;
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_doa_hdr *msghdr;
	int ctype = NID_CTYPE_DOA;
	int rc = 0;
	uint8_t is_old_command = priv_p->p_is_old_command;

	nid_log_debug("%s: start ...", log_header);
	msghdr = (struct umessage_doa_hdr *)doa_msg;
	assert(msghdr->um_req == UMSG_DOA_CMD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DOA_HEADER_LEN);
	if (cmd_len > UMSG_DOA_HEADER_LEN){
		rc = util_nw_read_n(sfd, msg_buf + UMSG_DOA_HEADER_LEN, cmd_len - UMSG_DOA_HEADER_LEN);
		if (rc < 0){
			nid_log_error("%s read error", log_header);

		}

	} else {
		nid_log_error ("%s: command length is too short", log_header);

	}

	if (is_old_command)
		*(msg_buf + 1) += 3;//conver UMSG_DOA_CODE_XXX to UMSG_DOA_CODE_OLD_XXX

	umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);
	nid_log_debug ("%s: vid:%s, vid_len:%u, lid:%s, lid_len:%u, hold_time:%u, get_lck:%u",
			log_header, doa_msg->um_doa_vid, doa_msg->um_doa_vid_len, doa_msg->um_doa_lid, doa_msg->um_doa_lid_len, doa_msg->um_doa_hold_time, doa_msg->um_doa_lock );

	switch (msghdr->um_req_code){

	case UMSG_DOA_CODE_REQUEST:
	case UMSG_DOA_CODE_OLD_REQUEST:
		doacg_p->dg_op->dg_request(doacg_p, doa_msg, is_old_command);
		break;
	case UMSG_DOA_CODE_RELEASE:
	case UMSG_DOA_CODE_OLD_RELEASE:
		doacg_p->dg_op->dg_release(doacg_p, doa_msg, is_old_command);
		break;
	case UMSG_DOA_CODE_CHECK:
	case UMSG_DOA_CODE_OLD_CHECK:
		doacg_p->dg_op->dg_check(doacg_p, doa_msg, is_old_command);
		break;

	default:
		nid_log_error("%s: got wrong req_code:%d", log_header, msghdr->um_req_code);
		break;
	}
	msghdr = (struct umessage_doa_hdr *)doa_msg;
	msghdr->um_req = UMSG_DOA_CMD;
	umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, (void *)msghdr);
	if (priv_p->p_is_old_command) {
		msghdr->um_req_code -= 3;//conver UMSG_DOA_CODE_OLD_XXX to UMSG_DOA_CODE_XXX
		*(msg_buf + 1) -= 3;
	}
	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);
	nid_log_debug ("%s: vid:%s, vid_len:%u, lid:%s, lid_len:%u, hold_time:%u, get_lck:%u",
			log_header, doa_msg->um_doa_vid, doa_msg->um_doa_vid_len, doa_msg->um_doa_lid, doa_msg->um_doa_lid_len, doa_msg->um_doa_hold_time, doa_msg->um_doa_lock );
	write(sfd, msg_buf, cmd_len);
	//read(sfd, &nothing_back, 1);
	close(sfd);


}

static void
__doac_hello(struct doac_private *priv_p, char *msg_buf, struct umessage_doa_hello *hello_msg)
{
	char *log_header = "__doac_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_doa_hdr *msghdr;
	int ctype = NID_CTYPE_DOA, rc;
	char nothing_back;
	struct umessage_doa_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_doa_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_DOA_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_DOA_HEADER_LEN);

	if (cmd_len > UMSG_DOA_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_DOA_HEADER_LEN, cmd_len - UMSG_DOA_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_DOA_CODE_HELLO:
		msghdr = (struct umessage_doa_hdr *)&hello_resp;
		msghdr->um_req = UMSG_DOA_CMD_HELLO;
		msghdr->um_req_code = UMSG_DOA_CODE_RESP_HELLO;

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

/*
 * Algorithm:
 * 	setup p_sfd, p_id, p_owner
 */

static int
doac_accept_new_channel(struct doac_interface *doac_p, int sfd)
{
	struct doac_private *priv_p = doac_p->dc_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
doac_do_channel(struct doac_interface *doac_p)
{
	char *log_header = "doac_do_channel";
	struct doac_private *priv_p = (struct doac_private *)doac_p->dc_private;
	struct umessage_doa_information doa_msg;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;

	struct umessage_doa_hdr	*msghdr = (struct umessage_doa_hdr *)&doa_msg;
	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_DOA_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	if (priv_p->p_is_old_command)
		msghdr->um_len = be32toh(*(uint32_t *)p);
	else
		msghdr->um_len = *(uint32_t *)p;

	switch (msghdr->um_req) {

	case UMSG_DOA_CMD:
		__doac_sub(priv_p, msg_buf, &doa_msg);
		break;

	case UMSG_DOA_CMD_HELLO:
		__doac_hello(priv_p, msg_buf, (struct umessage_doa_hello *)&doa_msg);
		break;

	default:
		nid_log_error("%s: got wrong req: %u", log_header, msghdr->um_req);
		break;
	}


}

static void
doac_cleanup(struct doac_interface *doac_p)
{
	nid_log_debug("doac_cleanup start, doac_p:%p", doac_p);
	if (doac_p->dc_private != NULL) {
		free(doac_p->dc_private);
		doac_p->dc_private = NULL;
	}
}



struct doac_operations doac_op = {
	.dc_accept_new_channel = doac_accept_new_channel,
	.dc_do_channel = doac_do_channel,
	.dc_cleanup = doac_cleanup,

};

int
doac_initialization(struct doac_interface *doac_p, struct doac_setup *setup)
{
	char *log_header = "doac_initialization";
	struct doac_private *priv_p ;

	nid_log_info("%s: start ...", log_header);

	priv_p = x_calloc(1, sizeof(*priv_p));
	doac_p->dc_private = priv_p;
	doac_p->dc_op = &doac_op;
	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_doacg = setup->doacg;
	priv_p->p_is_old_command = setup->is_old_command;
	return 0;
}
