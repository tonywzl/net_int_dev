/*
 * ppc.c
 * 	Implementation of Page Pool Channel Module
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "list.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_pp_if.h"
#include "ppc_if.h"
#include "ppg_if.h"
#include "pp_if.h"

struct ppc_private {
	struct ppg_interface	*p_ppg;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
ppc_accept_new_channel(struct ppc_interface *ppc_p, int sfd)
{
	struct ppc_private *priv_p = ppc_p->ppc_private;
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__ppc_stat(struct ppc_private *priv_p, char *msg_buf, struct umessage_pp_stat *stat_msg)
{
	char *log_header = "__ppc_stat";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	struct pp_interface *pp_p = NULL, **pps;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_pp_hdr *msghdr;
	int ctype = NID_CTYPE_PP, rc, len, i;
	struct umessage_pp_stat_resp stat_resp;
	char nothing_back;
	char *pp_name;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_pp_hdr *)stat_msg;
	assert(msghdr->um_req == UMSG_PP_CMD_STAT);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_PP_HEADER_LEN);
	if (cmd_len > UMSG_PP_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_PP_HEADER_LEN, cmd_len - UMSG_PP_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, pp_name:%s",
		log_header, cmd_len, msghdr->um_req_code, stat_msg->um_pp_name);

	switch (msghdr->um_req_code) {
	case UMSG_PP_CODE_STAT:
		pp_name = stat_msg->um_pp_name;
		pp_p = ppg_p->pg_op->pg_search_pp(ppg_p, pp_name);
		
		if(!pp_p) {
			goto out;
		}
		msghdr = (struct umessage_pp_hdr *)&stat_resp;
		msghdr->um_req = UMSG_PP_CMD_STAT;
		msghdr->um_req_code = UMSG_PP_CODE_RESP_STAT;
		
		stat_resp.um_pp_name_len = stat_msg->um_pp_name_len;
		memcpy(stat_resp.um_pp_name, stat_msg->um_pp_name, stat_msg->um_pp_name_len);
		stat_resp.um_pp_poolsz = pp_p->pp_op->pp_get_poolsz(pp_p);
		stat_resp.um_pp_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
		stat_resp.um_pp_poollen = pp_p->pp_op->pp_get_poollen(pp_p);
		stat_resp.um_pp_nfree = pp_p->pp_op->pp_get_nfree(pp_p);

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);

		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);
		break;

	case UMSG_PP_CODE_STAT_ALL:
		pps = ppg_p->pg_op->pg_get_all_pp(ppg_p, &len);
		for (i = 0; i < len; i++) {
			pp_p = pps[i];

			if (!pp_p)
				continue;

			msghdr = (struct umessage_pp_hdr *)&stat_resp;
			msghdr->um_req = UMSG_PP_CMD_STAT;
			msghdr->um_req_code = UMSG_PP_CODE_RESP_STAT_ALL;

			pp_name = pp_p->pp_op->pp_get_name(pp_p);
			stat_resp.um_pp_name_len = strlen(pp_name);
			//nid_log_warning("%s: pp_name:%s", log_header, pp_name);
			memcpy(stat_resp.um_pp_name, pp_name, stat_resp.um_pp_name_len);
			stat_resp.um_pp_poolsz = pp_p->pp_op->pp_get_poolsz(pp_p);
			stat_resp.um_pp_pagesz = pp_p->pp_op->pp_get_pagesz(pp_p);
			stat_resp.um_pp_poollen = pp_p->pp_op->pp_get_poollen(pp_p);
			stat_resp.um_pp_nfree = pp_p->pp_op->pp_get_nfree(pp_p);

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);

			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		msghdr = (struct umessage_pp_hdr *)&stat_resp;
		msghdr->um_req = UMSG_PP_CMD_STAT;
		msghdr->um_req_code = UMSG_PP_CODE_RESP_END;
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
__ppc_add_pp(struct ppc_private *priv_p, char *msg_buf, struct umessage_pp_add *add_msg)
{
	char *log_header = "__ppc_add_pp";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_pp_hdr *msghdr;
	int ctype = NID_CTYPE_PP, rc;
	struct umessage_pp_add_resp add_resp;
	char nothing_back;
	char *pp_name;
	int set_id;
	uint32_t pool_size, page_size;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_pp_hdr *)add_msg;
	assert(msghdr->um_req == UMSG_PP_CMD_ADD);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_PP_HEADER_LEN);
	if (cmd_len > UMSG_PP_HEADER_LEN)
		util_nw_read_n(sfd, msg_buf + UMSG_PP_HEADER_LEN, cmd_len - UMSG_PP_HEADER_LEN);

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc)
		goto out;

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, pp_name:%s",
		log_header, cmd_len, msghdr->um_req_code, add_msg->um_pp_name);
	pp_name = add_msg->um_pp_name;
	set_id = (int)add_msg->um_set_id;
	pool_size = add_msg->um_pool_size;
	page_size = add_msg->um_page_size;

	msghdr = (struct umessage_pp_hdr *)&add_resp;
	msghdr->um_req = UMSG_PP_CMD_ADD;
	msghdr->um_req_code = UMSG_PP_CODE_RESP_ADD;
	memcpy(add_resp.um_pp_name, add_msg->um_pp_name, add_msg->um_pp_name_len);
	add_resp.um_pp_name_len  = add_msg->um_pp_name_len;

	add_resp.um_resp_code = ppg_p->pg_op->pg_add_pp(ppg_p, pp_name, set_id, pool_size, page_size);

	rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
	if (rc)
		goto out;
	write(sfd, msg_buf, cmd_len);
	read(sfd, &nothing_back, 1);

out:
	close(sfd);
}

static void
__ppc_delete_pp(struct ppc_private *priv_p, char *msg_buf, struct umessage_pp_delete *delete_msg)
{
	char *log_header = "__ppc_delete";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_pp_hdr *msghdr;
	int ctype = NID_CTYPE_PP, rc;
	char nothing_back;
	struct umessage_pp_delete_resp delete_resp;
	struct ppg_interface *ppg_p = priv_p->p_ppg;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_pp_hdr *)delete_msg;
	assert(msghdr->um_req == UMSG_PP_CMD_DELETE);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_PP_HEADER_LEN);

	if (cmd_len > UMSG_PP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_PP_HEADER_LEN, cmd_len - UMSG_PP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, pp_name:%s", log_header, cmd_len, msghdr->um_req_code, delete_msg->um_pp_name);

	switch (msghdr->um_req_code) {
	case UMSG_PP_CODE_DELETE:
		delete_resp.um_pp_name_len = strlen(delete_msg->um_pp_name);
		strcpy(delete_resp.um_pp_name, delete_msg->um_pp_name);
		delete_resp.um_resp_code = ppg_p->pg_op->pg_delete_pp(ppg_p, delete_msg->um_pp_name);

		msghdr = (struct umessage_pp_hdr *)&delete_resp;
		msghdr->um_req = UMSG_PP_CMD_DELETE;
		msghdr->um_req_code = UMSG_PP_CODE_RESP_DELETE;

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
__ppc_display(struct ppc_private *priv_p, char *msg_buf, struct umessage_pp_display *dis_msg)
{
	char *log_header = "__ppc_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct ppg_interface *ppg_p = priv_p->p_ppg;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_pp_hdr *msghdr;
	int ctype = NID_CTYPE_PP, rc;
	char nothing_back;
	struct umessage_pp_display_resp dis_resp;
	struct pp_interface *pp_p;
	struct pp_setup *pp_setup_p;
	int *working_pp = NULL, cur_index = 0;
	int num_pp = 0, num_working_pp = 0, get_it = 0, found_pp = 0;
	int i;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_pp_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_PP_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_PP_HEADER_LEN);

	if (cmd_len > UMSG_PP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_PP_HEADER_LEN, cmd_len - UMSG_PP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, pp_name: %s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_pp_name);

	switch (msghdr->um_req_code){
	case UMSG_PP_CODE_S_DISP:
		if (dis_msg->um_pp_name == NULL)
			goto out;
		pp_setup_p = ppg_p->pg_op->pg_get_all_pp_setup(ppg_p, &num_pp);
		if (pp_setup_p == NULL)
			goto out;
		for (i = 0; i < PPG_MAX_PP; i++, pp_setup_p++) {
			if (found_pp == num_pp)
				break;
			if (strlen(pp_setup_p->pp_name) != 0)  {
				found_pp++;
				if (!strcmp(pp_setup_p->pp_name ,dis_msg->um_pp_name)) {
					get_it = 1;
					break;
				}
			}
		}
		if (get_it) {
			dis_resp.um_pp_name_len = strlen(pp_setup_p->pp_name);
			memcpy(dis_resp.um_pp_name, pp_setup_p->pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_set_id = pp_setup_p->set_id;
			dis_resp.um_page_size = pp_setup_p->page_size;
			dis_resp.um_pool_size = pp_setup_p->pool_size;

			msghdr = (struct umessage_pp_hdr *)&dis_resp;
			msghdr->um_req = UMSG_PP_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_PP_CODE_S_RESP_DISP;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
			read(sfd, &nothing_back, 1);
		}
		break;

	case UMSG_PP_CODE_S_DISP_ALL:
		pp_setup_p = ppg_p->pg_op->pg_get_all_pp_setup(ppg_p, &num_pp);
		for (i = 0; i < PPG_MAX_PP; i++, pp_setup_p++) {
			if (found_pp == num_pp)
				break;
			if (strlen(pp_setup_p->pp_name) != 0) {
				memset(&dis_resp, 0, sizeof(dis_resp));
				dis_resp.um_pp_name_len = strlen(pp_setup_p->pp_name);
				memcpy(dis_resp.um_pp_name, pp_setup_p->pp_name, dis_resp.um_pp_name_len);
				dis_resp.um_set_id = pp_setup_p->set_id;
				dis_resp.um_page_size = pp_setup_p->page_size;
				dis_resp.um_pool_size = pp_setup_p->pool_size;

				msghdr = (struct umessage_pp_hdr *)&dis_resp;
				msghdr->um_req = UMSG_PP_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_PP_CODE_S_RESP_DISP_ALL;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);
				found_pp++;
			}
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_pp_hdr *)&dis_resp;
		msghdr->um_req = UMSG_PP_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_PP_CODE_DISP_END;

		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_PP_CODE_W_DISP:
		if (dis_msg->um_pp_name == NULL)
			goto out;
		pp_p = ppg_p->pg_op->pg_search_pp(ppg_p, dis_msg->um_pp_name);

		if (pp_p == NULL)
			goto out;

		pp_setup_p = ppg_p->pg_op->pg_get_all_pp_setup(ppg_p, &num_pp);
		for (i = 0; i <num_pp; i++, pp_setup_p++) {
			if (!strcmp(pp_setup_p->pp_name, dis_msg->um_pp_name)) {
				dis_resp.um_pp_name_len = strlen(pp_setup_p->pp_name);
				memcpy(dis_resp.um_pp_name, pp_setup_p->pp_name, dis_resp.um_pp_name_len);
				dis_resp.um_set_id = pp_setup_p->set_id;
				dis_resp.um_page_size = pp_setup_p->page_size;
				dis_resp.um_pool_size = pp_setup_p->pool_size;

				msghdr = (struct umessage_pp_hdr *)&dis_resp;
				msghdr->um_req = UMSG_PP_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_PP_CODE_W_RESP_DISP;

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

	case UMSG_PP_CODE_W_DISP_ALL:
		pp_setup_p = ppg_p->pg_op->pg_get_all_pp_setup(ppg_p, &num_pp);
		working_pp = ppg_p->pg_op->pg_get_working_pp_index(ppg_p, &num_working_pp);

		for (i = 0; i < num_working_pp; i++) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			cur_index = working_pp[i];
			dis_resp.um_pp_name_len = strlen(pp_setup_p[cur_index].pp_name);
			memcpy(dis_resp.um_pp_name, pp_setup_p[cur_index].pp_name, dis_resp.um_pp_name_len);
			dis_resp.um_set_id = pp_setup_p[cur_index].set_id;
			dis_resp.um_page_size = pp_setup_p[cur_index].page_size;
			dis_resp.um_pool_size = pp_setup_p[cur_index].pool_size;

			msghdr = (struct umessage_pp_hdr *)&dis_resp;
			msghdr->um_req = UMSG_PP_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_PP_CODE_W_RESP_DISP_ALL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);
		}

		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_pp_hdr *)&dis_resp;
		msghdr->um_req = UMSG_PP_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_PP_CODE_DISP_END;

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
	free(working_pp);
	close(sfd);
}

static void
__ppc_hello(struct ppc_private *priv_p, char *msg_buf, struct umessage_pp_hello *hello_msg)
{
	char *log_header = "__ppc_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_pp_hdr *msghdr;
	int ctype = NID_CTYPE_PP, rc;
	char nothing_back;
	struct umessage_pp_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_pp_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_PP_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_PP_HEADER_LEN);

	if (cmd_len > UMSG_PP_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_PP_HEADER_LEN, cmd_len - UMSG_PP_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_PP_CODE_HELLO:
		msghdr = (struct umessage_pp_hdr *)&hello_resp;
		msghdr->um_req = UMSG_PP_CMD_HELLO;
		msghdr->um_req_code = UMSG_PP_CODE_RESP_HELLO;

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
ppc_do_channel(struct ppc_interface *ppc_p, struct scg_interface *scg_p)
{
	char *log_header = "ppc_do_channel";
	struct ppc_private *priv_p = (struct ppc_private *)ppc_p->ppc_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_pp pp_msg;
	struct umessage_pp_hdr *msghdr = (struct umessage_pp_hdr *)&pp_msg;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_PP_HEADER_LEN);
	msghdr->um_req = *p++;
	msghdr->um_req_code = *p++;
	msghdr->um_len = *(uint32_t *)p;
	nid_log_error("um_req: %u", msghdr->um_req);

	if (!priv_p->p_ppg) {
		nid_log_warning("%s support_pp should be set to 1 to support this operation", log_header);
		close(sfd);
		return;
	}

	switch (msghdr->um_req) {
	case UMSG_PP_CMD_ADD:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__ppc_add_pp(priv_p, msg_buf, (struct umessage_pp_add *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_PP_CMD_DELETE:
		scg_p->sg_op->sg_lock_dis_lck(scg_p);
		__ppc_delete_pp(priv_p, msg_buf, (struct umessage_pp_delete *)msghdr);
		scg_p->sg_op->sg_unlock_dis_lck(scg_p);
		break;

	case UMSG_PP_CMD_STAT:
		__ppc_stat(priv_p, msg_buf, (struct umessage_pp_stat *)msghdr);
		break;

	case UMSG_PP_CMD_DISPLAY:
		__ppc_display(priv_p, msg_buf, (struct umessage_pp_display *)msghdr);
		break;

	case UMSG_PP_CMD_HELLO:
		__ppc_hello(priv_p, msg_buf, (struct umessage_pp_hello *)msghdr);
		break;

	default:
		nid_log_error("%s: got wrong req:%d", log_header, msghdr->um_req);
	}
}

static void
ppc_cleanup(struct ppc_interface *ppc_p)
{
	nid_log_debug("ppc_cleanup start, ppc_p:%p", ppc_p);
	if (ppc_p->ppc_private != NULL) {
		free(ppc_p->ppc_private);
		ppc_p->ppc_private = NULL;
	}
}

struct ppc_operations ppc_op = {
	.ppc_accept_new_channel = ppc_accept_new_channel,
	.ppc_do_channel = ppc_do_channel,
	.ppc_cleanup = ppc_cleanup,
};

int
ppc_initialization(struct ppc_interface *ppc_p, struct ppc_setup *setup)
{
	char *log_header = "ppc_initialization";
	struct ppc_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	ppc_p->ppc_private = priv_p;
	ppc_p->ppc_op = &ppc_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_ppg = setup->ppg;
	return 0;
}
