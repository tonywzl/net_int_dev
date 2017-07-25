/*
 * olap.c
 * 	Implementation of  Overlapping Module
 */

#include <stdint.h>
#include <stdlib.h>
#include "nid_log.h"
#include "dcn_if.h"
#include "olap_if.h"

struct olap_private {
	struct dcn_interface *p_dcn;
};

/*
 * Input:
 * 	sorted_head: a list of dcn sorted by n_offset
 * output:
 * 	olap_head: a list of dcn sorted by n_offset
 */
static void
olap_get_overlap(struct olap_interface *olap_p, struct list_head *sorted_head, struct list_head *olap_head)
{
	struct olap_private *priv_p = (struct olap_private *)olap_p->ol_private;	
	struct dcn_interface *dcn_p = priv_p->p_dcn;
	struct dcn_node *np, *np2, *new_np;
	off_t cur_off = 0, olap_off;
	uint32_t cur_len = 0, olap_len = 0, power_of_2;
	assert(priv_p && olap_head);
	list_for_each_entry(np, struct dcn_node, sorted_head, n_list) {
		assert(np->n_offset >= cur_off);
		if (np->n_offset > cur_off + cur_len - 1) {
			/* no overlapping */
			cur_off = np->n_offset;
			cur_len = np->n_len;
		} else {
			if (np->n_offset + np->n_len > cur_off + cur_len) {
				/* np partially overlapped */ 
				olap_off = np->n_offset;
				olap_len = (cur_off + cur_len - 1) - np->n_offset + 1;
				cur_off = np->n_offset;
				cur_len = np->n_len;
			} else {
				/* the whole np got overlapped */
				olap_off = np->n_offset;
				olap_len = np->n_len;
			}

			if (!list_empty(olap_head)) {
				np2 = list_entry(olap_head->prev, struct dcn_node, n_list);
				if (np2->n_offset + np2->n_len - 1 >= olap_off) {
					if (np2->n_offset + np2->n_len >= olap_off + olap_len) {
						olap_len = 0;
					} else {
						olap_len -= (np2->n_offset + np2->n_len - 1) - olap_off + 1;
						olap_off = np2->n_offset + np2->n_len;
					}
				}
			}

			if (olap_len) {
				power_of_2 = 31 - __builtin_clz(olap_len);
				if (olap_len > (1U << power_of_2))
					power_of_2++;
				new_np = dcn_p->dc_op->dc_get_node(dcn_p, power_of_2);
				new_np->n_len = olap_len;
				new_np->n_offset = olap_off;
				nid_log_debug("olap_get_overlap: olap_off:%lu, olap_len:%u", olap_off, olap_len);
				list_add_tail(&new_np->n_list, olap_head);
				assert(!list_empty(olap_head));
			}
		}
	}
}

struct olap_operations olap_op = {
	.ol_get_overlap = olap_get_overlap,
};

int
olap_initialization(struct olap_interface *olap_p, struct olap_setup *setup)
{
	struct olap_private *priv_p;

	nid_log_info("olap_initialization start ...");
	olap_p->ol_op = &olap_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	olap_p->ol_private = priv_p;
	priv_p->p_dcn = setup->dcn;
	return 0;
}
