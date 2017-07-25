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

void get_stat( )
{
  	static struct rtree_interface *rtree_p = &ut_rtree;
	struct rtree_stat* stat = calloc( 1, sizeof( stat ) );
	rtree_p->rt_op->rt_get_stat( rtree_p, stat );
	printf( "rtree stat after np insert: nseg = %d,  segsz = %d, nfree = %d, nused = %d\n", 
		stat->s_rtn_nseg,
		stat->s_rtn_segsz,
		stat->s_rtn_nfree,
		stat->s_rtn_nused );
}

int search_for_node( off_t soffset )
{
	static struct rtree_interface *rtree_p = &ut_rtree;
	struct btn_node* slot;
	struct data_description_node* np;
	struct list_head *pre_list;

	// void* pVal = NULL;
	slot = rtree_p->rt_op->rt_search( rtree_p, soffset );
	if ( NULL == slot ) {
	  	printf( "error - not able to find the node for npa at offset %lu \n", soffset );
		// exit ( 1 );
		return 3;
	}
	printf( "search_for_node() - found something\n" );
	pre_list = slot->n_ddn_head.next;
	np = list_entry(pre_list, struct data_description_node, d_list);
	pre_list = &np->d_slist;
	if ( np->d_offset == soffset ) {
	  	printf( "found the node with the offset - %lu\n", soffset ); 
		return 0;
	}else {
	  	printf( "found the node with the offset - %lu, but soffset was %lu\n", np->d_offset, soffset ); 
		return 0;

	}
	int rc = 2;
	do {
		np = list_entry(pre_list, struct data_description_node, d_list);
		if ( np->d_offset == soffset ) {
			printf( "found the node with the offset - %lu\n", soffset ); 
			rc = 0;
		  	break;
		}
		pre_list = pre_list->prev;
	} while (pre_list != &slot->n_ddn_head );
	return rc;
	  
	
}


int
main()
{
  struct data_description_node *np, *np2, *np3, *np4, *np5, *np6;
	struct data_description_node* npa[ 1024 ];
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

	struct rtree_stat* stat = calloc( 1, sizeof( stat ) );
	rtree_p->rt_op->rt_get_stat( rtree_p, stat );
	printf( "rtree stat after np insert: nseg = %d,  segsz = %d, nfree = %d, nused = %d\n", 
		stat->s_rtn_nseg,
		stat->s_rtn_segsz,
		stat->s_rtn_nfree,
		stat->s_rtn_nused );

	np2 = ddn_p->d_op->d_get_node(ddn_p);
	np2->d_offset = 1024*16;
	np2->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 1, np2);


	rtree_p->rt_op->rt_get_stat( rtree_p, stat );
	printf( "rtree stat after np2 insert: nseg = %d,  segsz = %d, nfree = %d, nused = %d\n", 
		stat->s_rtn_nseg,
		stat->s_rtn_segsz,
		stat->s_rtn_nfree,
		stat->s_rtn_nused );

	np3 = ddn_p->d_op->d_get_node(ddn_p);
	np3->d_offset = 1024*32;
	np3->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 2, np3);

	np4 = ddn_p->d_op->d_get_node(ddn_p);
	np4->d_offset = 1024*36;
	np4->d_len = 4096;
	rtree_p->rt_op->rt_insert(rtree_p, 2, np4);


	np5 = ddn_p->d_op->d_get_node( ddn_p );
	np5->d_offset = 1024 * 24;
	np5->d_len = 4096;
	slot= (struct btn_node *)np->d_parent;
	rtree_p->rt_op->rt_hint_insert( rtree_p, 2, np5,  3, slot->n_parent );

	rtree_p->rt_op->rt_get_stat( rtree_p, stat );
	printf( "rtree stat after np5 insert: nseg = %d,  segsz = %d, nfree = %d, nused = %d\n", 
		stat->s_rtn_nseg,
		stat->s_rtn_segsz,
		stat->s_rtn_nfree,
		stat->s_rtn_nused );

  	if ( search_for_node( 3 ) ) {
    		printf( "unable to find np5 node in rtree \n" );
		exit( 1 );
	}

	np6 = ddn_p->d_op->d_get_node( ddn_p );
	np6->d_offset = 1024 * 28;
	np6->d_len = 4096;
	slot= (struct btn_node *)np->d_parent;
	rtree_p->rt_op->rt_hint_insert( rtree_p, 2, np6,  7, slot->n_parent );

	rtree_p->rt_op->rt_get_stat( rtree_p, stat );
	printf( "rtree stat after np5 insert: nseg = %d,  segsz = %d, nfree = %d, nused = %d\n", 
		stat->s_rtn_nseg,
		stat->s_rtn_segsz,
		stat->s_rtn_nfree,
		stat->s_rtn_nused );

  	if ( search_for_node( 7 ) ) {
    		printf( "unable to find np5 node in rtree \n" );
		exit( 1 );
	}


	int i = 0;
	while ( i < 1025 ) {
	  	npa[ i ] = ddn_p->d_op->d_get_node(ddn_p);
	  	npa[ i ]->d_offset = (1024 * (40 + i) * 4 );
	  	npa[i]->d_len = 4096;
		uint64_t index_key = (npa[ i ]->d_offset) / 4096;
		rtree_p->rt_op->rt_insert(rtree_p, index_key, npa[ i ]);
	  	printf( "stat after insert of array member in npa - %d\n", i );
	  	get_stat();
	  	i ++;
	}

	i = 0;
	while ( i < 1025 )
	{
	  	uint64_t index_key = (npa[ i ]->d_offset) / 4096;
	  	if ( search_for_node( index_key ) ) {
	    		printf( "unable to find node at index %d\n", i );
			exit( 1 );
		}
		i ++;
	}
	printf( "found all the nodes that were inserted from the array\n" );


	i = 1025;
	while ( i < 1029 ) {
	  	npa[ i ] = ddn_p->d_op->d_get_node(ddn_p);
	  	npa[ i ]->d_offset = (1024 * (40 + i )* 4 );
	  	npa[i]->d_len = 4096;
		slot = (struct btn_node *)npa[ 1024 ]->d_parent;
		struct rtree_node *nphi = (struct rtree_node *)(slot->n_parent);
		printf( "height of nphi = %d\n", nphi->n_height );
	  	uint64_t index_key = (npa[ i ]->d_offset) / 4096;
		rtree_p->rt_op->rt_hint_insert(rtree_p, 1025, npa[ i ], index_key, slot->n_parent);
	  	printf( "stat after hint_insert of array member in npa - %d\n", i );
	  	get_stat();
	  	i ++;
	}

	i = 1025;
	while ( i < 1029 )
	{
	  	uint64_t index_key = (npa[ i ]->d_offset) / 4096;
	  	if ( search_for_node( index_key ) ) {
	    		printf( "unable to find node at index %d\n", i );
			exit( 1 );
		}
		i ++;
	}
	printf( "found all the nodes that were hint_inserted from the array\n" );

	i = 0;
	while ( i < 1029 )
	{
	  	uint64_t index_key = (npa[ i ]->d_offset) / 4096;
		void* pVal = rtree_p->rt_op->rt_remove( rtree_p, index_key, npa[ i ] );
	  	if ( ! pVal ) {
		  	printf( "did not remove anything from rtree of npa at index %d, index_key %lu\n", i, index_key );
			exit( 1 );
		} else {
		  	printf( "removed the node  from rtree of npa at index %d, index_key %lu\n", i, index_key );
		}
		i ++;
	} 
	printf( "found all the nodes that were hint inserted from the array\n" );
		
	// exit( 1 );
		

	slot = (struct btn_node *)np->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 0, np, slot->n_parent);

	// verify this is removed
	if  ( !search_for_node( 0 ) ) {
	  	printf( "found the node after it was removed with rt_hint_remove, for np" );	
		exit( 1 );
	}

	slot = (struct btn_node *)np2->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 1, np2, slot->n_parent);

	// verify this is removed
	if  ( !search_for_node( 1 ) ) {
	  	printf( "found the node after it was removed with rt_hint_remove, for np2" );	
		exit( 1 );
	}

	slot = (struct btn_node *)np3->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 2, np3, slot->n_parent);

	// verify this is removed
	if  ( !search_for_node( 2 ) ) {
	  	printf( "found the node after it was removed with rt_hint_remove, for np3" );	
		exit( 1 );
	}

	
	slot = (struct btn_node *)np4->d_parent;
	rtree_p->rt_op->rt_hint_remove(rtree_p, 2, np4, slot->n_parent);

	// verify this is removed
	if  ( !search_for_node( 2 ) ) {
	  	printf( "found the node after it was removed with rt_hint_remove, for np4" );	
		exit( 1 );
	}

	nid_log_info("rtree ut module end...");
	nid_log_close();

	return 0;
}
