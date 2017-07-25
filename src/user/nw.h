#ifndef NID_NW_H
#define NID_NW_H

#include "list.h"
#include "tp_if.h"
#include "nid_shared.h"

struct nw_interface;
struct nw_job {
	struct tp_jobheader	j_header;
	struct nw_interface	*j_nw;
	struct list_head	j_list;
	char			j_cip[NID_MAX_IP];	// client ip
	int			j_sfd;			// socket fd
	int			j_res;
	char			j_new;
	void			*j_data;
};

extern void* module_accept_new_channel(void *module, int sfd, char *cip);
extern void module_do_channel(void *module, void *data);

#endif
