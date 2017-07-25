/*
 * inic.c
 *	Implementation of ini Channel Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "util_nw.h"
#include "nid_log.h"
#include "umpk_if.h"
#include "umpk_ini_if.h"
#include "ini_if.h"
#include "inic_if.h"
#include "lstn_if.h"

struct inic_private {
	struct ini_interface	*p_ini;
	int			p_rsfd;
	struct umpk_interface	*p_umpk;
};

static int
inic_accept_new_channel(struct inic_interface *inic_p, int sfd)
{
	char *log_header = "inic_accept_new_channel";
	struct inic_private *priv_p = inic_p->i_private;

	nid_log_debug("%s: start", log_header);
	priv_p->p_rsfd = sfd;
	return 0;
}

static void
__inic_display(struct inic_private *priv_p, char *msg_buf, struct umessage_ini_display *dis_msg)
{
	char *log_header = "__inic_display";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	struct ini_interface *ini_p = priv_p->p_ini;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_ini_hdr *msghdr;
	int ctype = NID_CTYPE_INI, rc;
	char nothing_back;
	char str[16];
	char *s_name;
	struct list_head *key_list_head, *conf_list_head;
	struct list_node *key_node;
	struct ini_key_desc *ini_key;
	struct ini_key_content *the_kc;
	struct ini_section_content *the_sc;
	struct umessage_ini_resp_display dis_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_ini_hdr *)dis_msg;
	assert(msghdr->um_req == UMSG_INI_CMD_DISPLAY);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_INI_HEADER_LEN);

	if (cmd_len > UMSG_INI_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_INI_HEADER_LEN, cmd_len - UMSG_INI_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d, key_name:%s", log_header, cmd_len, msghdr->um_req_code, dis_msg->um_key);
	switch (msghdr->um_req_code) {
	case UMSG_INI_CODE_T_SECTION:
		key_list_head = ini_p->i_op->i_get_template_key_list(ini_p);
		list_for_each_entry(key_node, struct list_node, key_list_head, ln_list) {
        	        ini_key = (struct ini_key_desc *)key_node->ln_data;
			dis_resp.um_key_len = strlen(ini_key->k_name);
			memcpy(dis_resp.um_key, ini_key->k_name, dis_resp.um_key_len);
			dis_resp.um_desc_len = strlen(ini_key->k_description);
			memcpy(dis_resp.um_desc, ini_key->k_description, dis_resp.um_desc_len);
			if (ini_key->k_value && ini_key->k_type == KEY_TYPE_STRING) {
				dis_resp.um_value_len = strlen((char *)ini_key->k_value);
				memcpy(dis_resp.um_value, ini_key->k_value, dis_resp.um_value_len);
			}
			else {
				dis_resp.um_value_len = strlen("NULL");
				memcpy(dis_resp.um_value, "NULL", dis_resp.um_value_len);
			}

			msghdr = (struct umessage_ini_hdr *)&dis_resp;
			msghdr->um_req = UMSG_INI_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_INI_CODE_T_RESP_SECTION;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}
			write(sfd, msg_buf, cmd_len);

		}

		msghdr = (struct umessage_ini_hdr *)&dis_resp;
		msghdr->um_req = UMSG_INI_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_INI_CODE_T_SECTION_DETAIL:
		key_list_head = ini_p->i_op->i_get_template_key_list(ini_p);
		list_for_each_entry(key_node, struct list_node, key_list_head, ln_list) {
        	        ini_key = (struct ini_key_desc *)key_node->ln_data;
			if (!strcmp((char *)ini_key->k_value, dis_msg->um_key)) {
				while (ini_key->k_name) {
					dis_resp.um_key_len = strlen(ini_key->k_name);
					memcpy(dis_resp.um_key, ini_key->k_name, dis_resp.um_key_len);
					dis_resp.um_desc_len = strlen(ini_key->k_description);
					memcpy(dis_resp.um_desc, ini_key->k_description, dis_resp.um_desc_len);
					if (ini_key->k_value && ini_key->k_type == KEY_TYPE_STRING) {
						dis_resp.um_value_len = strlen((char *)ini_key->k_value);
						memcpy(dis_resp.um_value, ini_key->k_value, dis_resp.um_value_len);
					}
					else if (ini_key->k_value && ini_key->k_type == KEY_TYPE_INT) {
						sprintf(str, "%d", *(int *)ini_key->k_value);
						dis_resp.um_value_len = strlen(str);
						memcpy(dis_resp.um_value, str, dis_resp.um_value_len);
					}
					else if (!ini_key->k_value){
						dis_resp.um_value_len = strlen("NULL");
						memcpy(dis_resp.um_value, "NULL", dis_resp.um_value_len);
					}
					else {
						dis_resp.um_value_len = strlen("unknown type");
						memcpy(dis_resp.um_value, "unknown type", dis_resp.um_value_len);
					}

					msghdr = (struct umessage_ini_hdr *)&dis_resp;
					msghdr->um_req = UMSG_INI_CMD_DISPLAY;
					msghdr->um_req_code = UMSG_INI_CODE_T_RESP_SECTION_DETAIL;

					rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
					if (rc) {
						goto out;
					}
					write(sfd, msg_buf, cmd_len);

					ini_key++;
				}
			}
		}


		memset(&dis_resp, 0, sizeof(dis_resp));
		msghdr = (struct umessage_ini_hdr *)&dis_resp;
		msghdr->um_req = UMSG_INI_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_INI_CODE_T_ALL:
		key_list_head = ini_p->i_op->i_get_template_key_list(ini_p);
		list_for_each_entry(key_node, struct list_node, key_list_head, ln_list) {
        	        ini_key = (struct ini_key_desc *)key_node->ln_data;
			while (ini_key->k_name) {
				dis_resp.um_key_len = strlen(ini_key->k_name);
				memcpy(dis_resp.um_key, ini_key->k_name, dis_resp.um_key_len);
				dis_resp.um_desc_len = strlen(ini_key->k_description);
				memcpy(dis_resp.um_desc, ini_key->k_description, dis_resp.um_desc_len);
				if (ini_key->k_value && ini_key->k_type == KEY_TYPE_STRING) {
					dis_resp.um_value_len = strlen((char *)ini_key->k_value);
					memcpy(dis_resp.um_value, ini_key->k_value, dis_resp.um_value_len);
				}
				else if (ini_key->k_value && ini_key->k_type == KEY_TYPE_INT) {
					sprintf(str, "%d", *(int *)ini_key->k_value);
					dis_resp.um_value_len = strlen(str);
					memcpy(dis_resp.um_value, str, dis_resp.um_value_len);
				}
				else if (!ini_key->k_value)
				{
					dis_resp.um_value_len = strlen("NULL");
					memcpy(dis_resp.um_value, "NULL", dis_resp.um_value_len);
				}
				else {
					dis_resp.um_value_len = strlen("unknown type");
					memcpy(dis_resp.um_value, "unknown type", dis_resp.um_value_len);
				}

				msghdr = (struct umessage_ini_hdr *)&dis_resp;
				msghdr->um_req = UMSG_INI_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_INI_CODE_T_RESP_ALL;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}
				write(sfd, msg_buf, cmd_len);

				ini_key++;
			}
		}

		msghdr = (struct umessage_ini_hdr *)&dis_resp;
		msghdr->um_req = UMSG_INI_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_INI_CODE_C_SECTION:
		conf_list_head = ini_p->i_op->i_get_conf_key_list(ini_p);
		list_for_each_entry(the_sc, struct ini_section_content, conf_list_head, s_list) {
			the_kc = container_of(the_sc->s_head.next, struct ini_key_content, k_list);
			dis_resp.um_key_len = strlen(the_kc->k_name);
			memcpy(dis_resp.um_key, the_kc->k_name, dis_resp.um_key_len);
			dis_resp.um_desc_len = strlen(the_sc->s_name);
			memcpy(dis_resp.um_desc, the_sc->s_name, dis_resp.um_desc_len);
			if (the_kc->k_value && the_kc->k_type == KEY_TYPE_STRING) {
				dis_resp.um_value_len = strlen((char *)the_kc->k_value);
				memcpy(dis_resp.um_value, the_kc->k_value, dis_resp.um_value_len);
			}
			else
			{
				dis_resp.um_value_len = strlen("NULL");
				memcpy(dis_resp.um_value, "NULL", dis_resp.um_value_len);
			}
			msghdr = (struct umessage_ini_hdr *)&dis_resp;
			msghdr->um_req = UMSG_INI_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_INI_CODE_C_RESP_SECTION;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}

			write(sfd, msg_buf, cmd_len);
		}

		msghdr = (struct umessage_ini_hdr *)&dis_resp;
		msghdr->um_req = UMSG_INI_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
		
		if (rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);	
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_INI_CODE_C_SECTION_DETAIL:
		s_name = dis_msg->um_key;
		the_sc = ini_p->i_op->i_search_section(ini_p, s_name);
		if (!the_sc) {
			goto out;
		}
		list_for_each_entry(the_kc, struct ini_key_content, &the_sc->s_head, k_list) {
			memset(&dis_resp, 0, sizeof(dis_resp));
			dis_resp.um_key_len = strlen(the_kc->k_name);
			memcpy(dis_resp.um_key, the_kc->k_name, dis_resp.um_key_len);
			if (!strcmp(the_kc->k_name, "type")) {
				dis_resp.um_desc_len = strlen(the_sc->s_name);
				memcpy(dis_resp.um_desc, the_sc->s_name, dis_resp.um_desc_len);
			}

			if (the_kc->k_value && the_kc->k_type == KEY_TYPE_STRING) {
				dis_resp.um_value_len = strlen((char *)the_kc->k_value);
				memcpy(dis_resp.um_value, the_kc->k_value, dis_resp.um_value_len);
			}
			else if (the_kc->k_value && the_kc->k_type == KEY_TYPE_INT) {
				sprintf(str, "%d", *(int *)the_kc->k_value);
				dis_resp.um_value_len = strlen(str);
				memcpy(dis_resp.um_value, str, dis_resp.um_value_len);
			}
			else if (!the_kc->k_value)
			{
				dis_resp.um_value_len = strlen("NULL");
				memcpy(dis_resp.um_value, "NULL", dis_resp.um_value_len);
			}

			msghdr = (struct umessage_ini_hdr *)&dis_resp;
			msghdr->um_req = UMSG_INI_CMD_DISPLAY;
			msghdr->um_req_code = UMSG_INI_CODE_C_RESP_SECTION_DETAIL;

			rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
			if (rc) {
				goto out;
			}

			write(sfd, msg_buf, cmd_len);
		}

		msghdr = (struct umessage_ini_hdr *)&dis_resp;
		msghdr->um_req = UMSG_INI_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_END;
		rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);

		if(rc) {
			goto out;
		}
		write(sfd, msg_buf, cmd_len);
		read(sfd, &nothing_back, 1);

		break;

	case UMSG_INI_CODE_C_ALL:
		conf_list_head = ini_p->i_op->i_get_conf_key_list(ini_p);
		list_for_each_entry(the_sc, struct ini_section_content, conf_list_head, s_list) {
			list_for_each_entry(the_kc, struct ini_key_content, &the_sc->s_head, k_list) {
				memset(&dis_resp, 0, sizeof(dis_resp));
				dis_resp.um_key_len = strlen(the_kc->k_name);
				memcpy(dis_resp.um_key, the_kc->k_name, dis_resp.um_key_len);
				if (!strcmp(the_kc->k_name, "type")) {
					dis_resp.um_desc_len = strlen(the_sc->s_name);
					memcpy(dis_resp.um_desc, the_sc->s_name, dis_resp.um_desc_len);
				}

				if (the_kc->k_value && the_kc->k_type == KEY_TYPE_STRING) {
					dis_resp.um_value_len = strlen((char *)the_kc->k_value);
					memcpy(dis_resp.um_value, the_kc->k_value, dis_resp.um_value_len);
				}
				else if (the_kc->k_value && the_kc->k_type == KEY_TYPE_INT) {
					sprintf(str, "%d", *(int *)the_kc->k_value);
					dis_resp.um_value_len = strlen(str);
					memcpy(dis_resp.um_value, str, dis_resp.um_value_len);
				}
				else if (!the_kc->k_value)
				{
					dis_resp.um_value_len = strlen("NULL");
					memcpy(dis_resp.um_value, "NULL", dis_resp.um_value_len);
				}

				msghdr = (struct umessage_ini_hdr *)&dis_resp;
				msghdr->um_req = UMSG_INI_CMD_DISPLAY;
				msghdr->um_req_code = UMSG_INI_CODE_C_RESP_ALL;

				rc = umpk_p->um_op->um_encode(umpk_p, msg_buf, &cmd_len, ctype, msghdr);
				if (rc) {
					goto out;
				}

				write(sfd, msg_buf, cmd_len);
			}
		}

		msghdr = (struct umessage_ini_hdr *)&dis_resp;
		msghdr->um_req = UMSG_INI_CMD_DISPLAY;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_END;
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
__inic_hello(struct inic_private *priv_p, char *msg_buf, struct umessage_ini_hello *hello_msg)
{
	char *log_header = "__inic_hello";
	struct umpk_interface *umpk_p = priv_p->p_umpk;
	int sfd = priv_p->p_rsfd;
	uint32_t cmd_len;
	struct umessage_ini_hdr *msghdr;
	int ctype = NID_CTYPE_INI, rc;
	char nothing_back;
	struct umessage_ini_hello hello_resp;

	nid_log_warning("%s: start ...", log_header);
	msghdr = (struct umessage_ini_hdr *)hello_msg;
	assert(msghdr->um_req == UMSG_INI_CMD_HELLO);
	cmd_len = msghdr->um_len;
	assert(cmd_len <= 4096);
	assert(cmd_len >= UMSG_INI_HEADER_LEN);

	if (cmd_len > UMSG_INI_HEADER_LEN) {
		util_nw_read_n(sfd, msg_buf + UMSG_INI_HEADER_LEN, cmd_len - UMSG_INI_HEADER_LEN);
	}

	rc = umpk_p->um_op->um_decode(umpk_p, msg_buf, cmd_len, ctype, msghdr);
	if (rc) {
		goto out;
	}

	nid_log_warning("%s: cmd_len:%d, cmd_code:%d", log_header, cmd_len, msghdr->um_req_code);

	switch (msghdr->um_req_code) {
	case UMSG_INI_CODE_HELLO:
		msghdr = (struct umessage_ini_hdr *)&hello_resp;
		msghdr->um_req = UMSG_INI_CMD_HELLO;
		msghdr->um_req_code = UMSG_INI_CODE_RESP_HELLO;

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
inic_do_channel(struct inic_interface *inic_p)
{
	char *log_header = "inic_do_channel";
	struct inic_private *priv_p = (struct inic_private *)inic_p->i_private;
	int sfd = priv_p->p_rsfd;
	char msg_buf[4096], *p = msg_buf;
	union umessage_ini ini_msg;
	struct umessage_ini_hdr *msghdr_p;

	nid_log_warning("%s: start ...", log_header);
	util_nw_read_n(sfd, msg_buf, UMSG_INI_HEADER_LEN);

	msghdr_p = (struct umessage_ini_hdr *) &ini_msg;
	msghdr_p->um_req = *p++;
	msghdr_p->um_req_code = *p++;
	msghdr_p->um_len = *(uint32_t *)p;
	nid_log_warning("%s: req:%d, req_code:%d, um_len:%d", log_header, msghdr_p->um_req, msghdr_p->um_req_code, msghdr_p->um_len);

	switch (msghdr_p->um_req) {
	case UMSG_INI_CMD_DISPLAY:
		__inic_display(priv_p, msg_buf, (struct umessage_ini_display *)msghdr_p);
		break;

	case UMSG_INI_CMD_HELLO:
		__inic_hello(priv_p, msg_buf, (struct umessage_ini_hello *)msghdr_p);
		break;

	default:
		nid_log_warning("%s: got wrong req:%d", log_header, msghdr_p->um_req);
		break;
	}
}

static void
inic_cleanup(struct inic_interface *inic_p)
{
	char *log_header = "inic_cleanup";

	nid_log_debug("%s start, inic_p: %p", log_header, inic_p);
	if (inic_p->i_private != NULL) {
		free(inic_p->i_private);
		inic_p->i_private = NULL;
	}
}

struct inic_operations inic_op = {
	.i_accept_new_channel = inic_accept_new_channel,
	.i_do_channel = inic_do_channel,
	.i_cleanup = inic_cleanup,
};

int
inic_initialization(struct inic_interface *inic_p, struct inic_setup *setup)
{
	char *log_header = "inic_initialization";
	struct inic_private *priv_p;

	nid_log_debug("%s start ...", log_header);
	priv_p = x_calloc(1, sizeof(*priv_p));
	inic_p->i_private = priv_p;
	inic_p->i_op = &inic_op;

	priv_p->p_rsfd = -1;
	priv_p->p_umpk = setup->umpk;
	priv_p->p_ini = setup->ini;

	return 0;
}
