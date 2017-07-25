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

extern struct ds_interface	ut_bwc_ds;
extern int			ut_bwc_in_ds;
extern struct io_interface 	ut_bwc_io;
extern volatile int		ut_bwc_respnose_left;

static void sa_add_response_node(struct sac_interface *sac, struct request_node *rn_p)
{
	(void)sac;
	ut_bwc_ds.d_op->d_put_buffer(&ut_bwc_ds, ut_bwc_in_ds, rn_p->r_seq, rn_p->r_len, &ut_bwc_io);
	free(rn_p);
	printf("Send response1!!rn_p->r_offset:%ld, rn_p->r_len:%u\n", rn_p->r_offset, rn_p->r_len);
	__sync_fetch_and_sub(&ut_bwc_respnose_left, 1);

}

static void sa_add_response_list(struct sac_interface *sac, struct list_head *rn_head)
{
	(void)sac;
	struct request_node *rn_p, *rn_p1;
	int left;
	list_for_each_entry_safe(rn_p, rn_p1, struct request_node, rn_head, r_list) {
		list_del(&rn_p->r_list);
		ut_bwc_ds.d_op->d_put_buffer(&ut_bwc_ds, ut_bwc_in_ds, rn_p->r_seq, rn_p->r_len, &ut_bwc_io);
		left = __sync_sub_and_fetch(&ut_bwc_respnose_left, 1);
		printf("Send response:rn_p->r_offset:%ld, rn_p->r_len:%u left:%d\n",
				rn_p->r_offset, rn_p->r_len, left);
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
