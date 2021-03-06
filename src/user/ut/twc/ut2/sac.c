/*
 * sac.c
 *
 *  Created on: Dec 25, 2015
 *      Author: root
 */


#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "server.h"
#include "sac_if.h"
#include "ds_if.h"
#include "rc_if.h"
#include "dsrec_if.h"

extern struct ds_interface	ut_twc_ds;
extern int			ut_twc_in_ds;
extern struct io_interface 	ut_twc_io;
extern volatile int		ut_twc_respnose_left;


static void sa_add_response_node(struct sac_interface *sac, struct request_node *rn_p)
{
	(void)sac;
	ut_twc_ds.d_op->d_put_buffer(&ut_twc_ds, ut_twc_in_ds, rn_p->r_seq, rn_p->r_len, &ut_twc_io);
	free(rn_p);
	printf("Send response1!!rn_p->r_offset:%ld, rn_p->r_len:%u\n", rn_p->r_offset, rn_p->r_len);
	__sync_fetch_and_sub(&ut_twc_respnose_left, 1);

}

static void sa_add_response_list(struct sac_interface *sac, struct list_head *rn_head)
{
	(void)sac;
	struct request_node *rn_p, *rn_p1;

	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, rn_head, r_list) {
		list_del(&rn_p->r_list);

		ut_twc_ds.d_op->d_put_buffer(&ut_twc_ds, ut_twc_in_ds, rn_p->r_seq, rn_p->r_len, &ut_twc_io);
		__sync_sub_and_fetch(&ut_twc_respnose_left, 1);
		free(rn_p);

	}

}

void sa_remove_wc(struct sac_interface *sac)
{
	(void)sac;
}

struct sac_operations sac_op = {
	.sa_add_response_node = sa_add_response_node,
	.sa_add_response_list = sa_add_response_list,
	.sa_remove_wc = sa_remove_wc,
};

int sac_initialization(struct sac_interface *sac, struct sac_setup *sac_setup) {
	(void)sac_setup;
	sac->sa_op = &sac_op;
	return 0;
}
