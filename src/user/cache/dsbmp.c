/*
 * dsbmp.c
 * 	Implementation of Device Space Bitmap Module
 * 	Maintain free disk space base on name space
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nid_log.h"
#include "blksn_if.h"
#include "brn_if.h"
#include "rtree_if.h"
#include "dsmgr_if.h"
#include "dsbmp_if.h"

#define	DSBMP_UNIT_SIZE		(1 << 6)	

static uint64_t bit_masks[DSBMP_UNIT_SIZE];
static uint64_t num_bridge_nodes_dsbmp = 0;
static uint64_t num_blksn_nodes_dsbmp = 0;

struct dsbmp_private {
	struct allocator_interface	*p_allocator;
	struct dsmgr_interface	 	*p_dsmgr;
	struct brn_interface 		*p_brn;
	struct blksn_interface 		*p_blksn;
	uint64_t			p_size;		// in number of uint64_t
	uint64_t			*p_bitmap;
	uint64_t			p_free;
	struct rtree_interface		p_rtree;
	struct list_head		p_space_head;	// search list in space order
  	uint64_t 			p_num_free_blocks;
};

static uint64_t dsbmp_get_free_blocks( struct dsbmp_interface *dsbmp_p )
{
	struct dsbmp_private *priv_p = (struct dsbmp_private *)dsbmp_p->sb_private;
	return priv_p->p_num_free_blocks;
}


struct list_head*  __dsbmp_traverse_rtree( struct dsbmp_private* priv_p )
{
  	struct rtree_interface* rtree_p = NULL;
  	rtree_p = &(priv_p->p_rtree);

  	rtree_p->rt_op->rt_iter_start( rtree_p );

	struct list_head* rtree_list = (struct list_head*)x_calloc( 1, sizeof( struct list_head ));
  	struct bridge_node* bnp = NULL;
  	struct block_size_node* bsnp = NULL;
	INIT_LIST_HEAD( rtree_list );
  	do {
    		bnp = (struct bridge_node*) rtree_p->rt_op->rt_iter_next( rtree_p );
    		if (bnp == NULL ) {
			nid_log_debug( "__dsbmp_traverse_rtree() - bnp is null " );
      			break;
		}
    		bsnp = bnp->bn_data;
		struct block_size_node* new_bsnp = (struct block_size_node*)x_calloc( 1, sizeof( struct block_size_node ) );
		memset( new_bsnp, 0, sizeof( struct block_size_node)); // duplicate the block_size_node so it can be used for multiple lists if necessary and the new one can persist as a point in time replica of the rtree
		new_bsnp->bsn_size = bsnp->bsn_size;
		new_bsnp->bsn_index = bsnp->bsn_index;
		list_add_tail( &new_bsnp->bsn_olist, rtree_list ); 
		nid_log_debug( "__dsbmp_traverse_rtree() - added one node - index = %lu, size = %lu ", bsnp->bsn_index, bsnp->bsn_size );
  	} while ( bnp != NULL );
   	rtree_p->rt_op->rt_iter_stop( rtree_p );
	return rtree_list;

}

struct list_head* dsbmp_traverse_rtree( struct dsbmp_interface* dsbmp_p ) 
{
  	struct dsbmp_private *priv_p = (struct dsbmp_private *)dsbmp_p->sb_private;
	//  	struct rtree_interface* rtree_p = NULL;

	struct list_head* rtree_list;
	rtree_list = __dsbmp_traverse_rtree( priv_p );
	return rtree_list;
}


/*
 * Algorithm:
 * 	1> shrink the old space block node (old_np) by new_size, keep it in the free space pool 
 * 	2> create a new space node with new_size, but not keep it in the space pool
 */
static struct block_size_node * 
dsbmp_split_node(struct dsbmp_interface *dsbmp_p, struct block_size_node *old_np, uint64_t new_size)
{
	struct dsbmp_private *priv_p = (struct dsbmp_private *)dsbmp_p->sb_private;
	struct blksn_interface *blksn_p = priv_p->p_blksn;
	struct block_size_node *new_np;

	old_np->bsn_size -= new_size;
	new_np = blksn_p->bsn_op->bsn_get_node(blksn_p);
	num_blksn_nodes_dsbmp ++;
	nid_log_debug( "dsbmp_split_node() - # blksn_nodes = %lu ", num_blksn_nodes_dsbmp );
	new_np->bsn_index = old_np->bsn_index + old_np->bsn_size;
	new_np->bsn_size = new_size;
	priv_p->p_num_free_blocks -= new_size;
	return new_np;	
}

/*
 * Algorithm:
 * 	There are three scenarios: 
 * 	1> If np is not concatenated with any other node, just add np
 * 	2> if np is np is concatenated with its left neighbor, extend it's left neighbor.
 * 	   If the new extended left neighbor concatenated with its right neighbor,
 * 	   concatenate these two neighbors by extending the left neighbor again
 * 	   and deleting the right neighbor
 * 	3> If np is concatenated with its right neighbor, not its left bighbor,
 * 	   add np and extend np by its right neighbor size and delete its right neighbor
 */
static struct block_size_node *
dsbmp_put_node(struct dsbmp_interface *dsbmp_p, struct block_size_node *np)
{
	struct dsbmp_private *priv_p = (struct dsbmp_private *)dsbmp_p->sb_private;
	struct dsmgr_interface *dsmgr_p = priv_p->p_dsmgr;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct brn_interface *brn_p = priv_p->p_brn;
	struct bridge_node *br_np;
	struct blksn_interface *blksn_p = priv_p->p_blksn;
	struct block_size_node *nbr_np = NULL, *left_np = NULL, *right_np = NULL;
	struct block_size_node *ret_node = NULL;

	br_np = rtree_p->rt_op->rt_search_around(rtree_p, np->bsn_index);
	if (br_np) {
		nbr_np = (struct block_size_node *)br_np->bn_data;
	} else if (!list_empty(&priv_p->p_space_head)) {
		nbr_np = list_first_entry(&priv_p->p_space_head, struct block_size_node, bsn_olist); 
	}

	if (!nbr_np) {
		/*
		 * There is no neighbor, I'm the first node in the space pool
		 * So just insert it into the pool
		 */
		br_np = brn_p->bn_op->bn_get_node(brn_p);
		num_bridge_nodes_dsbmp ++;
		nid_log_debug( "dsbmp_put_node() - # bridge_nodes = %lu ", num_bridge_nodes_dsbmp );
		br_np->bn_data = (void *)np;
		np->bsn_slot = br_np;
		rtree_p->rt_op->rt_insert(rtree_p, np->bsn_index, br_np);
		list_add( &np->bsn_olist, &priv_p->p_space_head );
		priv_p->p_num_free_blocks += np->bsn_size; 
		goto out;
	}

	/*
	 * Got a neighbor. Try to concatenate it with its neighbor(s)
	 * We know that the np does not overlap with any neighbor
	 */
	while ((nbr_np->bsn_index < np->bsn_index) && (nbr_np->bsn_olist.next != &priv_p->p_space_head)) {
		left_np = list_entry(nbr_np->bsn_olist.next, struct block_size_node, bsn_olist);
		if (left_np->bsn_index > np->bsn_index)
			break;
		nbr_np = list_entry(nbr_np->bsn_olist.next, struct block_size_node, bsn_olist);
	}

	while ((nbr_np->bsn_index > np->bsn_index) && (nbr_np->bsn_olist.prev != &priv_p->p_space_head)) {
		nbr_np = list_entry(nbr_np->bsn_olist.prev, struct block_size_node, bsn_olist);
	}

	/*
	 * Now, we know that if the neighbor we just found, nbr_np, is a left neighbor,
	 * it must be the closest left neighbor. If it is a right neighbor, it must be
	 * the closest right neighbor
	 */
	if (nbr_np->bsn_index < np->bsn_index && nbr_np->bsn_index + nbr_np->bsn_size == np->bsn_index) {
		/*
		 * left concatenated neighbor. Extending this left neighbor,
		 * without adding this new node
		 */
		
		/* don't extend size here, let dsmgr do it */
		dsmgr_p->dm_op->dm_extend_space(dsmgr_p, nbr_np, np->bsn_size);
		priv_p->p_num_free_blocks += np->bsn_size; 
		ret_node = np;	// the np is not added, return to the caller
		left_np = nbr_np;
		if ( left_np->bsn_olist.next != &priv_p->p_space_head )
			right_np = list_entry(left_np->bsn_olist.next, struct block_size_node, bsn_olist);

	} else {
		/*
		 * The neighbor nbr_np is not a concatenated left neighbor
		 */
		br_np = brn_p->bn_op->bn_get_node(brn_p);
		num_bridge_nodes_dsbmp ++;
		nid_log_debug( "dsbmp_put_node() - # bridge_nodes = %lu ", num_bridge_nodes_dsbmp );
		br_np->bn_data = (void *)np;
		np->bsn_slot = br_np;
		rtree_p->rt_op->rt_insert(rtree_p, np->bsn_index, br_np);
		if ( nbr_np->bsn_index < np->bsn_index ) {
		  	left_np = np;
		  	// add node after the neighbor
		  	list_add( &np->bsn_olist, &nbr_np->bsn_olist );
			if ( left_np->bsn_olist.next != &priv_p->p_space_head )
				right_np = list_entry( left_np->bsn_olist.next, struct block_size_node, bsn_olist);

		} else {
		  	// add node before the neighbor - neighbor is a right neighbor
		  	list_add_tail( &np->bsn_olist, &nbr_np->bsn_olist );
		  	left_np = np;
			right_np = nbr_np;
		}
		priv_p->p_num_free_blocks += np->bsn_size; 
	}

	if (right_np && (right_np->bsn_index == left_np->bsn_index + left_np->bsn_size)) {
		/*
		 * Concatnate left_np and right_np
		 * 1> remove the birdge node of right_np from rtree
		 * 2> remove right_np from space pool (p_space_head)
		 * 3> extending left_np
		 */
		br_np = (struct bridge_node *)right_np->bsn_slot;
		assert(br_np->bn_data == right_np && br_np == right_np->bsn_slot);
		rtree_p->rt_op->rt_hint_remove(rtree_p, right_np->bsn_index, br_np, br_np->bn_parent);
		list_del(&right_np->bsn_olist);		// dropped from p_space_head
		dsmgr_p->dm_op->dm_delete_node(dsmgr_p, right_np);

		brn_p->bn_op->bn_put_node(brn_p, br_np);
		num_bridge_nodes_dsbmp --;
		nid_log_debug( "dsbmp_put_node() - # bridge_nodes = %lu ", num_bridge_nodes_dsbmp );
		dsmgr_p->dm_op->dm_extend_space(dsmgr_p, left_np, right_np->bsn_size);
		blksn_p->bsn_op->bsn_put_node(blksn_p,right_np);
	}

out:
	return ret_node;
}

/*
 * Algorithm:
 * 	The caller (dsmgr) knows which node it wants to get
 * 	Don't worry about informing dsmgr
 */
static struct block_size_node *
dsbmp_get_node(struct dsbmp_interface *dsbmp_p, struct block_size_node *np)
{
	struct dsbmp_private *priv_p = (struct dsbmp_private *)dsbmp_p->sb_private;
	struct brn_interface *brn_p = priv_p->p_brn;
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct bridge_node *br_np = (struct bridge_node *)np->bsn_slot;

	rtree_p->rt_op->rt_hint_remove(rtree_p, np->bsn_index, br_np, br_np->bn_parent);
	list_del(&np->bsn_olist);
	priv_p->p_num_free_blocks -= np->bsn_size;
	brn_p->bn_op->bn_put_node(brn_p, br_np);

	return np;
}

struct dsbmp_operations dsbmp_op = {
	.sb_put_node = dsbmp_put_node,
	.sb_split_node = dsbmp_split_node, 
	.sb_get_node = dsbmp_get_node,
	.sb_get_free_blocks = dsbmp_get_free_blocks,
	.sb_traverse_rtree = dsbmp_traverse_rtree,
};

static void
__dsbmp_rtree_extend_cb(void *target_slot, void *parent)
{
	struct bridge_node *slot = (struct bridge_node *)target_slot;
	slot->bn_parent = parent;
}

static void *
__dsbmp_rtree_insert_cb(void *target_slot, void *new_node, void *parent)
{
	struct bridge_node *slot = (struct bridge_node *)target_slot;
	if (!slot) {
		slot = new_node; 
		slot->bn_parent = parent;
	}
	return slot;
}

static void *
__dsbmp_rtree_remove_cb(void *target_slot, void *target_node)
{
	assert(target_slot == target_node);	
	return NULL;
}

static void 
__dsbmp_rtree_shrink_to_root_cb(void *node )
{
  	assert ( node );
  	struct bridge_node* bn = (struct bridge_node*)( node );
	bn->bn_parent = NULL;
}

/*
 * Algorithm:
 * 	Establish rtree based on name space. 
 * 	Find all contigous 0-bit segment in the biotmap
 */
static void
__bitmap_scan(struct dsbmp_private *priv_p)
{
	struct rtree_interface *rtree_p = &priv_p->p_rtree;
	struct dsmgr_interface *dsmgr_p = priv_p->p_dsmgr;
	struct brn_interface *brn_p = priv_p->p_brn;
	struct blksn_interface *blksn_p = priv_p->p_blksn;
	uint64_t *cur_unit = priv_p->p_bitmap;	// start from the first unit
	uint64_t cur_size = 0,			// current unit position 
		 seg_count;			// bit number of current free segment		
	uint64_t bit_pos = 0;			// bit position in current unit (uint64_t), [0 63]
	uint64_t new_index_key;
	uint64_t first_bit;
	struct bridge_node *slot_np;
	struct block_size_node *np;

next_seg:
	seg_count = 0;	// start a new segment scanning
	if (cur_size == priv_p->p_size)
		goto out;

	/*
	 * Now, we start a new 0-bit segment
	 */
	assert(bit_pos < DSBMP_UNIT_SIZE);
	first_bit = cur_size * DSBMP_UNIT_SIZE + bit_pos;
	new_index_key = first_bit;

	if (bit_pos) {
		/* not start from the first bit of current bitmap unit (uint64_t) */
		while (bit_pos < DSBMP_UNIT_SIZE && !(*cur_unit & bit_masks[bit_pos])) {
			seg_count++;
			bit_pos++;
		}

		if (bit_pos == DSBMP_UNIT_SIZE) {
			cur_unit++;	// move to next unit
			cur_size++;
			bit_pos = 0;	// start a new bitmap unit
		}
	}

	/*
	 * Current free segment scanning start/continue on a new unit (uint64_t)
	 */
	while ((cur_size < priv_p->p_size)) {
		seg_count += DSBMP_UNIT_SIZE;	// all 64 bits are off
		cur_unit++;			// move to next unit
		cur_size++;			// update current unit position
		bit_pos = 0;			// start a new bitmap unit
	}

	/*
	 * we know this segment scanning will stop at some bit of this unit
	 */
	if (cur_size < priv_p->p_size) {
		while (bit_pos < DSBMP_UNIT_SIZE && !(*cur_unit & bit_masks[bit_pos])) {
			seg_count++;
			bit_pos++;
		}
		assert(bit_pos < DSBMP_UNIT_SIZE);
	}

	/*
	 *  create and insert this segment
	 */
	assert(seg_count);

	/*
	 * create a node and insert to the space order list (p_space_head)
	 */
	np = blksn_p->bsn_op->bsn_get_node(blksn_p);
	num_blksn_nodes_dsbmp++;
	nid_log_debug( "__bitmap_scan() - # blksn_nodes = %lu ", num_blksn_nodes_dsbmp );
	np->bsn_index = new_index_key;
	np->bsn_size = seg_count;
	list_add_tail(&np->bsn_olist, &priv_p->p_space_head);

	/* 
	 * create a slot and insert to the rtree
	 */
	slot_np = brn_p->bn_op->bn_get_node(brn_p);
	num_bridge_nodes_dsbmp ++;
	nid_log_debug( "__bitmap_scan() - # bridge_nodes = %lu ", num_bridge_nodes_dsbmp );
	slot_np->bn_data = np;
	np->bsn_slot = (void*)slot_np;
	rtree_p->rt_op->rt_insert(rtree_p, np->bsn_index, slot_np);
	priv_p->p_num_free_blocks += seg_count;

	/* inform my owner */
	dsmgr_p->dm_op->dm_add_new_node(dsmgr_p, np);
	goto next_seg;

out:
	return;
}

int
dsbmp_initialization(struct dsbmp_interface *dsbmp_p, struct dsbmp_setup *setup)
{
	char *log_header = "dsbmp_initialization";
	struct dsbmp_private *priv_p;
	struct rtree_setup rtree_setup;
	int i;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	dsbmp_p->sb_op = &dsbmp_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	dsbmp_p->sb_private = priv_p;

	INIT_LIST_HEAD(&priv_p->p_space_head);
	for (i = 0; i < DSBMP_UNIT_SIZE; i++) {
		bit_masks[i] = (1UL <<i);
	}

	priv_p->p_allocator = setup->allocator;;
	priv_p->p_dsmgr = setup->dsmgr;
	priv_p->p_size = setup->size;
	priv_p->p_bitmap = setup->bitmap;
	priv_p->p_brn = setup->brn;
	priv_p->p_blksn = setup->blksn;

	priv_p->p_num_free_blocks = 0;
	rtree_setup.allocator = priv_p->p_allocator;
	rtree_setup.extend_cb = __dsbmp_rtree_extend_cb;
	rtree_setup.insert_cb = __dsbmp_rtree_insert_cb;
	rtree_setup.remove_cb = __dsbmp_rtree_remove_cb;
	rtree_setup.shrink_to_root_cb = __dsbmp_rtree_shrink_to_root_cb;
	rtree_initialization(&priv_p->p_rtree, &rtree_setup);
	__bitmap_scan(priv_p);
	return 0;
}
