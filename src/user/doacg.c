/*
 * doacg.c
 * 	Implementation of Delegation of Authority Guardian Module
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "list.h"
#include "nid_log.h"
#include "doac_if.h"
#include "doa_if.h"
#include "doacg_if.h"
#include "umpk_doa_if.h"

struct doacg_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct doa_channel {
	struct list_head	c_list;
	void			*c_data;
};

struct doacg_private {
	pthread_mutex_t		p_doa_mlck;
	pthread_mutex_t		p_doac_mlck;
	struct list_head	p_doac_head;
	struct umpk_interface	*p_umpk;
	struct list_head	p_all_doa_heads;
};

static void *
doacg_accept_new_channel(struct doacg_interface *doacg_p, int sfd, uint8_t is_old_command)
{
	char *log_header = "doacg_accept_new_channel";
	struct doacg_private *priv_p = (struct doacg_private *)doacg_p->dg_private;

	struct doac_interface *doac_p;
	struct doac_setup doac_setup;
	struct doacg_channel *chan_p;

	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	doac_p = x_calloc(1, sizeof(*doac_p));
	doac_setup.doacg = doacg_p;
	doac_setup.umpk = priv_p->p_umpk;
	doac_setup.is_old_command = is_old_command;
	doac_initialization(doac_p, &doac_setup);



	chan_p = x_calloc(1, sizeof(*chan_p));
	chan_p->c_data = doac_p;
	doac_p->dc_op->dc_accept_new_channel(doac_p, sfd);

	nid_log_debug("%s: end (chan_p:%p)", log_header, chan_p);
	return chan_p;
}
static void
__check_list(struct list_head *head)
{
	nid_log_warning("check list_head");
	nid_log_debug("address of head: %p, next address :%p, prev address :%p", head, head->next, head->prev );
}
/*
 * Algorithm:
 *
 */
static void
doacg_do_channel(struct doacg_interface *doacg_p, void *data)
{
	char *log_header = "doacg_do_channel";
	struct doacg_channel *chan_p = (struct doacg_channel *)data;
	struct doac_interface *doac_p = (struct doac_interface *)chan_p->c_data;
	struct doacg_private *priv_p = (struct doacg_private *)doacg_p->dg_private;

	nid_log_warning("%s:start ...", log_header);
	__check_list(&priv_p->p_doac_head);
	pthread_mutex_lock(&priv_p->p_doac_mlck);
	list_add_tail(&chan_p->c_list, &priv_p->p_doac_head);
	pthread_mutex_unlock(&priv_p->p_doac_mlck);

	doac_p->dc_op->dc_do_channel(doac_p);


	pthread_mutex_lock(&priv_p->p_doac_mlck);
	list_del(&chan_p->c_list);
	pthread_mutex_unlock(&priv_p->p_doac_mlck);
	__check_list(&priv_p->p_doac_head);
	doac_p->dc_op->dc_cleanup(doac_p);

}
static struct doa_channel *
__doacg_check_doa(struct doacg_private *priv_p, struct umessage_doa_information *doa_msg)
{
	char *log_header = "__doacg_check_doa";
	struct doa_channel *old_chan = NULL;
	struct doa_interface *old_doa = NULL;
	char new_lid[1024], *old_lid;
	int got_it = 0;


	nid_log_debug("%s: start ...", log_header);
	memcpy(new_lid, doa_msg->um_doa_lid, doa_msg->um_doa_lid_len);
	new_lid[doa_msg->um_doa_lid_len] = 0;

	list_for_each_entry(old_chan, struct doa_channel, &priv_p->p_all_doa_heads, c_list) {
		old_doa = (struct doa_interface *)old_chan->c_data;
		old_lid = old_doa->d_op->d_get_id(old_doa);
		nid_log_debug(" doa list entry : the old id = %s",old_lid);
		nid_log_debug(" doa list entry : the new id = %s",new_lid);
		if (!strcmp(new_lid, old_lid)) {
			got_it =1;
			break;
		}
	}

	if (got_it){
		return old_chan;
	} else{
		return NULL;
	}

}

static void
doacg_request(struct doacg_interface *doacg_p, struct umessage_doa_information *doa_msg, uint8_t is_old_command)
{
	char *log_header = "doacg_request";
	struct doacg_private *priv_p = (struct doacg_private *)doacg_p->dg_private;
	struct doa_interface *new_doa_p;
	struct doa_interface *old_doa_p = NULL, *ret_doa = NULL;
	struct doa_setup doa_setup;
	struct doa_channel *chan_p = NULL;


	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	new_doa_p = x_calloc(1, sizeof(*new_doa_p));
	doa_setup.doacg = doacg_p;
	doa_setup.umpk = priv_p->p_umpk;
	doa_initialization(new_doa_p, &doa_setup);

	new_doa_p->d_op->d_init_info(new_doa_p, doa_msg);
	pthread_mutex_lock(&priv_p->p_doa_mlck);
	chan_p = __doacg_check_doa(priv_p, doa_msg);

	if (!chan_p){
		chan_p = x_calloc(1, sizeof(*chan_p));
		chan_p->c_data = new_doa_p;
		list_add_tail(&chan_p->c_list, &priv_p->p_all_doa_heads);
		ret_doa = new_doa_p;

	} else {
		nid_log_debug("%s: found old doa", log_header);
		old_doa_p =  chan_p->c_data;
		ret_doa = old_doa_p->d_op->d_compare(old_doa_p, new_doa_p);
		if (ret_doa) {

			chan_p->c_data = ret_doa;
			old_doa_p->d_op->d_free(old_doa_p);
			free(old_doa_p);


		} else {
			ret_doa = old_doa_p;
			new_doa_p->d_op->d_free(new_doa_p);
			free(new_doa_p);
			doa_msg->um_doa_lock = 0;
		}

	}
	ret_doa->d_op->d_get_info_stat(ret_doa, doa_msg);
	pthread_mutex_unlock(&priv_p->p_doa_mlck);
	struct umessage_doa_hdr *msghdr;
	msghdr = (struct umessage_doa_hdr *)doa_msg;
	msghdr->um_req = UMSG_DOA_CMD;
	if (is_old_command)
		msghdr->um_req_code = UMSG_DOA_CODE_OLD_REQUEST;
	else
		msghdr->um_req_code = UMSG_DOA_CODE_REQUEST;
}

static void
doacg_release(struct doacg_interface *doacg_p, struct umessage_doa_information *doa_msg, uint8_t is_old_command)
{

	char *log_header = "doacg_release";
	struct doacg_private *priv_p = (struct doacg_private *)doacg_p->dg_private;
	struct doa_interface *ret_doa = NULL;
	struct doa_channel *chan_p = NULL;
	char *old_owner = NULL, new_owner[1024];
	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	pthread_mutex_lock(&priv_p->p_doa_mlck);
	chan_p = __doacg_check_doa(priv_p, doa_msg);

	if(chan_p){
		memcpy(new_owner, doa_msg->um_doa_vid, doa_msg->um_doa_vid_len);
		new_owner[doa_msg->um_doa_vid_len] = 0;
		ret_doa = chan_p->c_data;
		old_owner = ret_doa->d_op->d_get_owner(ret_doa);
		if (!strcmp(old_owner, new_owner)){
			nid_log_debug("%s: I am the lock owner (%s), release the lock", log_header, new_owner);
			ret_doa->d_op->d_free(ret_doa);
			free(ret_doa);
			list_del(&chan_p->c_list);
			free(chan_p);
			doa_msg->um_doa_lock = 1;
		} else{
			nid_log_debug ("%s: not the lock owner, failed to release the lock", log_header);
			doa_msg->um_doa_lock = 0;
		}

	} else {
		nid_log_debug("%s: did not found the lock, failed to release the lock", log_header);
		doa_msg->um_doa_lock = 0;
	}
	pthread_mutex_unlock(&priv_p->p_doa_mlck);
	struct umessage_doa_hdr *msghdr;
	msghdr = (struct umessage_doa_hdr *)doa_msg;
	msghdr->um_req = UMSG_DOA_CMD;
	if (is_old_command)
		msghdr->um_req_code = UMSG_DOA_CODE_OLD_RELEASE;
	else
		msghdr->um_req_code = UMSG_DOA_CODE_RELEASE;
}

static void
doacg_check(struct doacg_interface *doacg_p, struct umessage_doa_information *doa_msg, uint8_t is_old_command)
{

	char *log_header = "doacg_check";
	struct doacg_private *priv_p = (struct doacg_private *)doacg_p->dg_private;
	struct doa_interface *ret_doa = NULL;
	struct doa_channel *chan_p = NULL;

	nid_log_debug("%s: start ...", log_header);
	assert(priv_p);
	pthread_mutex_lock(&priv_p->p_doa_mlck);
	chan_p = __doacg_check_doa(priv_p, doa_msg);
	if (chan_p){
		nid_log_debug("%s: found the lock", log_header);
		ret_doa = chan_p->c_data;
		ret_doa->d_op->d_get_info_stat(ret_doa, doa_msg);
		doa_msg->um_doa_lock = 1;

	} else {
		nid_log_debug ("%s:did not found the lock", log_header);
		doa_msg->um_doa_lock = 0;
	}
	pthread_mutex_unlock(&priv_p->p_doa_mlck);
	struct umessage_doa_hdr *msghdr;
	msghdr = (struct umessage_doa_hdr *)doa_msg;
	msghdr->um_req = UMSG_DOA_CMD;
	if (is_old_command)
		msghdr->um_req_code = UMSG_DOA_CODE_OLD_CHECK;
	else
		msghdr->um_req_code = UMSG_DOA_CODE_CHECK;
}




struct doacg_operations doacg_op = {
	.dg_accept_new_channel = doacg_accept_new_channel,
	.dg_do_channel = doacg_do_channel,
	.dg_request = doacg_request,
	.dg_release = doacg_release,
	.dg_check = doacg_check,
};

int
doacg_initialization(struct doacg_interface *doacg_p, struct doacg_setup *setup)
{
	char *log_header = "doacg_initialization";
	struct doacg_private *priv_p;
	struct doa_interface *doa_p;
	struct doa_setup doa_setup;
	nid_log_info("%s: start ...", log_header);
	assert(setup);

	doacg_p->dg_op = &doacg_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	doacg_p->dg_private = priv_p;
	priv_p->p_umpk = setup->umpk;
	pthread_mutex_init(&(priv_p->p_doa_mlck), NULL);
	pthread_mutex_init(&(priv_p->p_doac_mlck), NULL);
	INIT_LIST_HEAD(&priv_p->p_all_doa_heads);
	INIT_LIST_HEAD(&priv_p->p_doac_head);
	doa_p = x_calloc(1, sizeof(*doa_p));
	doa_setup.doacg = doacg_p;
	doa_setup.umpk = priv_p->p_umpk;
	doa_initialization(doa_p, &doa_setup);
	return 0;
}
