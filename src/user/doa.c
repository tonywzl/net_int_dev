/*
 * doa.c
 * 	Implementation of Delegation of Authority Module
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#include "nid_shared.h"
#include "nid_log.h"
#include "scg_if.h"
#include "doa_if.h"
#include "umpk_if.h"
#include "umpk_doa_if.h"

struct doa_private {
	struct scg_interface	*p_scg;
	struct umpk_interface	*p_umpk;
	int			p_sfd;			// socket fd
    char                    p_id[DOA_MAX_ID];       // doa id
    char                    p_owner[DOA_MAX_ID];    // owner of this doa
//	int			p_req_code;			// command of the request
	int			p_res;			// result of the requset
	struct timeval 		p_time_renew;		// timestamp of renew
	struct timeval		p_time_init;		//timestamp of init
	struct timeval		p_time_last_renew;	//timestamp of last time renew
	time_t			p_time_out;		//time_out for next time
	time_t			p_hold_time;
};

static void
doa_init_info(struct doa_interface *doa_p, struct umessage_doa_information *res)
{
	char *log_header = "doa_init_info";
	struct doa_private *priv_p = (struct doa_private *)doa_p->d_private;
	struct timeval 		tv;
	nid_log_debug("%s: start ...", log_header);
	gettimeofday(&tv, NULL);

	memcpy(priv_p->p_id, res->um_doa_lid, res->um_doa_lid_len);
	priv_p->p_id[res->um_doa_lid_len] = 0;
	memcpy(priv_p->p_owner, res->um_doa_vid, res->um_doa_vid_len);
	priv_p->p_owner[res->um_doa_vid_len] = 0;

	priv_p->p_hold_time = 0;
	priv_p->p_time_renew = tv;
	priv_p->p_time_init = tv;
	priv_p->p_time_last_renew =  tv;
	priv_p->p_time_out = res->um_doa_time_out;
	priv_p->p_res = 1 ;
}


static void
doa_get_info_stat(struct doa_interface *doa_p, struct umessage_doa_information *stat_p)
{
	char *log_header = "doa_get_info_stat";
	struct doa_private *priv_p = (struct doa_private *)doa_p->d_private;
	struct timeval tv;
	//time_t	hold_time;
	nid_log_debug("%s: start ...", log_header);
	memset(stat_p, 0, sizeof(struct umessage_doa_information));

	gettimeofday(&tv,  NULL);

	stat_p->um_doa_vid_len = strlen(priv_p->p_owner);
	stat_p->um_doa_lid_len = strlen(priv_p->p_id);
	memcpy(stat_p->um_doa_vid, priv_p->p_owner, stat_p->um_doa_vid_len);
	memcpy(stat_p->um_doa_lid, priv_p->p_id, stat_p->um_doa_lid_len);


	stat_p->um_doa_hold_time = tv.tv_sec - priv_p->p_time_renew.tv_sec;
	stat_p->um_doa_time_out = priv_p->p_time_out;
	stat_p->um_doa_lock = priv_p->p_res;


}

static struct doa_interface *
doa_compare(struct doa_interface *old_doa_p, struct doa_interface *new_doa_p)
{
	char *log_header = "doa_compare";
	struct doa_private *new_priv_p = (struct doa_private *)new_doa_p->d_private;
	struct doa_private *old_priv_p = (struct doa_private *)old_doa_p->d_private;

	char *new_owner, *old_owner;
	struct timeval	tv;
	time_t		hold_time;

	nid_log_debug("%s: start ...", log_header);
	gettimeofday(&tv, NULL);
	old_owner = old_priv_p->p_owner;
	new_owner = new_priv_p->p_owner;
	nid_log_debug("%s: old owner is %s, new owner is %s", log_header, old_owner, new_owner);
	if (!strcmp(old_owner, new_owner)){
		nid_log_debug("%s: same vid, renew the lock",log_header);
		//new_priv_p->p_hold_time = 1;
		new_priv_p->p_time_renew = tv;
		new_priv_p->p_time_last_renew = old_priv_p->p_time_renew;
		new_priv_p->p_time_init = old_priv_p->p_time_init;
		new_priv_p->p_res = 1;
		return new_doa_p;
	} else {
		hold_time = tv.tv_sec - old_priv_p->p_time_renew.tv_sec;
		if (hold_time > old_priv_p->p_time_out)
		{
			nid_log_debug("%s: got the lock for the old one", log_header);
			new_priv_p->p_res = 1;
			return new_doa_p;
		} else {
			nid_log_debug("%s: did not got the lock for the old one", log_header);
			old_priv_p->p_res = 0;
			return NULL;
		}
	}


}



static char *
doa_get_id(struct doa_interface *doa_p)
{
	struct doa_private *priv_p = (struct doa_private *)doa_p->d_private;
	return priv_p->p_id;
}

static char *
doa_get_owner(struct doa_interface *doa_p)
{
	struct doa_private *priv_p = (struct doa_private *)doa_p->d_private;
	return priv_p->p_owner;
}

static time_t
doa_get_timestamp(struct doa_interface *doa_p)
{
	struct doa_private *priv_p = (struct doa_private *)doa_p->d_private;
	return priv_p->p_time_init.tv_sec;
}

static void
doa_free(struct doa_interface *doa_p)
{
	free (doa_p->d_private);
	doa_p->d_private = NULL;

	doa_p->d_op = NULL;
}

struct doa_operations doa_op = {
	.d_get_info_stat = doa_get_info_stat,
	.d_init_info = doa_init_info,
	.d_compare = doa_compare,
	.d_get_id = doa_get_id,
	.d_get_owner = doa_get_owner,
	.d_get_timestamp = doa_get_timestamp,
	.d_free = doa_free,
};

int
doa_initialization(struct doa_interface *doa_p, struct doa_setup *setup)
{
	char *log_header = "doa_initialization";
	struct doa_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	doa_p->d_op = &doa_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	doa_p->d_private = priv_p;
	priv_p->p_umpk = setup->umpk;
	return 0;
}
