#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "bit_if.h"
#include "ddn_if.h"
#include "rtn_if.h"
#include "btn_if.h"
#include "rtree_if.h"

static struct rtree_setup rtree_setup;
static struct rtree_interface ut_rtree;
static struct btn_interface ut_btn;
static struct btn_setup ut_btn_setup;
static struct ddn_interface ut_ddn;
static struct ddn_setup ut_ddn_setup;

int
bit_initialization(struct bit_interface *bit_p, struct bit_setup *setup)
{
	assert(bit_p && setup);
	return 0;
}

static void
_rtree_extend_cb(void *target_slot, void *parent)
{
	struct btn_node *slot = (struct btn_node *)target_slot;
	slot->n_parent = parent;
}

static void *
_rtree_insert_cb(void *target_slot, void *new_node, void *parent)
{
	struct btn_node *slot = (struct btn_node *)target_slot;
	struct btn_interface *btn_p = &ut_btn;
	struct data_description_node *np, *new_np = (struct data_description_node *)new_node;				 

	int did_it = 0;

	if (!slot) {
		//slot = calloc(1, sizeof(*slot));
		slot = (struct btn_node *)btn_p->bt_op->bt_get_node(btn_p); 
		INIT_LIST_HEAD(&slot->n_ddn_head);
	}
	slot->n_parent = parent;

	list_for_each_entry(np, struct data_description_node, &slot->n_ddn_head, d_list) {
		assert(np->d_offset != new_np->d_offset);
		if (np->d_offset > new_np->d_offset) {
			assert(np->d_offset > new_np->d_offset + new_np->d_len - 1);
			__list_add(&new_np->d_list, np->d_list.prev, &np->d_list);
			did_it = 1;
			break;
		}
	}

	if (!did_it)
		list_add_tail(&new_np->d_list, &slot->n_ddn_head);
	new_np->d_parent = slot;
	return slot;
}

static void *
_rtree_remove_cb(void *target_slot, void *target_node)
{
	struct btn_node *slot = (struct btn_node *)target_slot;
	struct data_description_node *np = (struct data_description_node *)target_node;
	struct btn_interface *btn_p = &ut_btn;

	if (!target_node)
		return NULL;

	assert(np->d_parent == slot);
	list_del(&np->d_list);	// removed from slot->n_ddn_head
	np->d_parent = NULL;
	if (list_empty(&slot->n_ddn_head)) {
		btn_p->bt_op->bt_put_node(btn_p, (struct btn_node *)slot);
		slot = NULL;
	}
	return slot;
}


int
main()
{
	struct data_description_node *np, *np2, *np3, *np4;
	static struct rtree_interface *rtree_p = &ut_rtree;
	static struct ddn_interface *ddn_p = &ut_ddn;
	struct btn_node *slot;
	nid_log_open();
	nid_log_info("rtree ut module start ...");

	btn_initialization(&ut_btn, &ut_btn_setup);
	ddn_initialization(&ut_ddn, &ut_ddn_setup);
       		
	rtree_setup.extend_cb = _rtree_extend_cb;
	rtree_setup.insert_cb = _rtree_insert_cb;
	rtree_setup.remove_cb = _rtree_remove_cb;
	rtree_initialization(&ut_rtree, &rtree_setup);

	np = ddn_p->d_op->d_get_node(ddn_p);
	np->d_offset = 0;
	np->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 0, np);

	np2 = ddn_p->d_op->d_get_node(ddn_p);
	np2->d_offset = 1024*16;
	np2->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 1, np2);

	np3 = ddn_p->d_op->d_get_node(ddn_p);
	np3->d_offset = 1024*32;
	np3->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 2, np3);

	np4 = ddn_p->d_op->d_get_node(ddn_p);
	np4->d_offset = 1024*36;
	np4->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 2, np4);

	slot = (struct btn_node *)np->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 0, np, slot->n_parent);

	slot = (struct btn_node *)np2->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 1, np2, slot->n_parent);

	slot = (struct btn_node *)np3->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 2, np3, slot->n_parent);

	slot = (struct btn_node *)np4->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 2, np4, slot->n_parent);

	nid_log_info("rtree ut module end...");
	nid_log_close();

	return 0;
}
