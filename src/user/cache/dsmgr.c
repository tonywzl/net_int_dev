/*
 * dsmgr.c
 * 	Implementation of Device Space Manager Module
 * 	Maintain disk free space based on size (number of contiguous blocks)
 */

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#include "nid_log.h"
#include "list.h"
#include "allocator_if.h"
#include "brn_if.h"
#include "blksn_if.h"
#include "rtree_if.h"
#include "dsbmp_if.h"
#include "dsmgr_if.h"
#include "umpk_crc_if.h"

#define DSMGR_BLOCKSZ_SHIFT	12	// 4K
#define DSMGR_LARGE_SPACE	4096	// more than 4096 is belong large space

#define min(x, y)	((x) <= (y) ? (x):(y))


struct dsmgr_space_node {
	struct list_head	sn_list;	// in p_sp_heads
	uint64_t		sn_size;
	void			*sn_slot;	// pointer to a bridge node
};

struct dsmgr_private {
	pthread_mutex_t			p_lck;
	struct dsmgr_space_node		p_sp_nodes[DSMGR_LARGE_SPACE];
	struct allocator_interface	*p_allocator;
	struct brn_interface		*p_brn;
	struct blksn_interface		*p_blksn;
	struct dsbmp_interface		p_dsbmp;
	struct rtree_interface		p_sp_rtree;				// space pool tree
	struct list_head		p_large_sp_head;			// large space pool, in space order
	struct list_head		p_sp_heads[DSMGR_LARGE_SPACE];		// space pool, in space order
	int				p_sp_heads_size[DSMGR_LARGE_SPACE+1];	// number of elements in p_sp_heads and p_large_sp_head
	struct list_head		p_space_head;				// in order of size
	int				p_size;					// number of blocks of the whole space
	int				p_block_shift;
  	uint64_t num_bridge_nodes_dsmgr;
  	uint64_t num_blksn_nodes_dsmgr;
};



static void
__dsmgr_rtree_extend_cb(void *target_slot, void *parent)
{
	struct bridge_node *slot = (struct bridge_node *)target_slot;
	slot->bn_parent = parent;
}

static void *
__dsmgr_rtree_insert_cb(void *target_slot, void *new_node, void *parent)
{
	struct bridge_node *slot = (struct bridge_node *)target_slot;
	if (!slot) {
		slot = new_node; 
		slot->bn_parent = parent;
	}
	return slot;
}

static void *
__dsmgr_rtree_remove_cb(void *target_slot, void *target_node)
{
	assert(!target_node || target_slot == target_node);	
	return NULL;
}

static void 
__dsmgr_rtree_shrink_to_root_cb(void *node )
{
  	assert ( node );
  	struct bridge_node* bn = (struct bridge_node*)( node );
	bn->bn_parent = NULL;
}

static void
__dsmgr_traverse_rtree(struct dsmgr_private* priv_p)
{
  	struct rtree_interface* rtree_p = NULL;
  	struct bridge_node* bnp = NULL;
  	struct dsmgr_space_node* dsnp = NULL;

  	rtree_p = &(priv_p->p_sp_rtree);
  	rtree_p->rt_op->rt_iter_start( rtree_p );
  	do {
    		bnp = (struct bridge_node*) rtree_p->rt_op->rt_iter_next( rtree_p );
    		if (bnp == NULL ) {
      			break;
		}
    		dsnp = bnp->bn_data;
    		nid_log_debug( "space node - size of the node is %lu\n", dsnp->sn_size );
  	} while ( bnp != NULL );
   	rtree_p->rt_op->rt_iter_stop( rtree_p );

}

struct umessage_crc_information_resp_freespace_dist *
__dsmgr_check_space(struct dsmgr_private* priv_p)
{
	struct list_head* curr_head;
	struct block_size_node *cur_np;
	int i = 0;
	unsigned long total_space = 0;
	int curr_size = 0;
	struct umessage_crc_information_resp_freespace_dist *um_rc_fdr;
	int total_num_sizes = 0;

	um_rc_fdr = x_calloc(1, sizeof(*um_rc_fdr));
	INIT_LIST_HEAD( &(um_rc_fdr->um_size_list_head ) );

	// iterate through the all the space heads for each size where they exist
	for (i = 0; i < DSMGR_LARGE_SPACE; i++) {
	  	curr_head = NULL;
		curr_head = &priv_p->p_sp_heads[ i  ];

		// iterate through the entries of that space size
		if ( curr_head ) {
		  	int total_entries = 0;
			curr_size = 0;
			list_for_each_entry(cur_np, struct block_size_node, curr_head, bsn_list) {
			  	total_entries++;
				if ( &(cur_np->bsn_list) == (curr_head) ) {
					break;
				}
				/* if ( total_entries > 10000 ) {
				  sleep(1);
				  } */
				if ( ! curr_size ) {
				  	curr_size = cur_np->bsn_size;
;
 					nid_log_debug( "_dsmgr_check_space() bsn_list head is %lu list_for_ent - bsn_list = %lu, size = %lu, index = %lu\n", 
						(unsigned long)(&(curr_head)), 
						(unsigned long)(&(cur_np->bsn_list)  ), 
						cur_np->bsn_size, 
						cur_np->bsn_index );
				}
			}
			if ( total_entries ) {
			  	total_num_sizes ++;
			  	nid_log_info( "total number of entries of size %lu, is %d\n", (unsigned long)(i + 1), total_entries );
				struct space_size_number* ssn =  x_calloc( 1, sizeof( struct space_size_number ) );
				ssn->um_size = curr_size;
				ssn->um_number = total_entries;
				list_add_tail( &ssn->um_ss_list, &(um_rc_fdr->um_size_list_head ) );

				total_space = total_space + (total_entries * ( i + 1 ));
			}
		}
	}

	// iterate through all the entries of the large space node
	unsigned long total_large_sp_size = 0;
	if (!list_empty(&priv_p->p_large_sp_head)){
	  	list_for_each_entry( cur_np,  struct block_size_node, &priv_p->p_large_sp_head, bsn_list ){
		  	nid_log_info( "large space entry of size %lu\n", cur_np->bsn_size );
		  	total_large_sp_size += cur_np->bsn_size;
			struct space_size_number* ssn =  x_calloc( 1, sizeof( struct space_size_number ) );
			ssn->um_size = cur_np->bsn_size;
			ssn->um_number = 1;  // need to change in the future
			list_add_tail( &ssn->um_ss_list, & (um_rc_fdr->um_size_list_head ) );
			total_num_sizes ++;
		}
	}
	um_rc_fdr->um_num_sizes = total_num_sizes;
	nid_log_info( "total large space entries size is %lu\n", total_large_sp_size );

	nid_log_info( "total size of space in space pool is %lu\n", total_space + total_large_sp_size);

	return um_rc_fdr;

}

/*
 * Algorithm:
 * 	Drop a space node from p_sp_heads
 * 	Doesn't inform dsbmp
 */
static void
__dsmgr_drop_node(struct dsmgr_private *priv_p, struct block_size_node *np)
{
	struct rtree_interface *rtree_p = &priv_p->p_sp_rtree;
	struct list_head *from_head;
	struct brn_interface *brn_p = priv_p->p_brn;
	struct bridge_node *br_np;
	struct dsmgr_space_node *sp_node;
	uint64_t node_size = np->bsn_size;

	list_del(&np->bsn_list);	// dropped from either space pool or large space pool
	if (node_size <= DSMGR_LARGE_SPACE) {
		__sync_sub_and_fetch(&priv_p->p_sp_heads_size[node_size - 1], 1);
		from_head = &priv_p->p_sp_heads[node_size - 1];

		if (list_empty(from_head)) {
			br_np = rtree_p->rt_op->rt_remove(rtree_p, node_size - 1, NULL);
			assert(br_np);
			sp_node = (struct dsmgr_space_node *)br_np->bn_data;
			assert(sp_node->sn_size == node_size);
			list_del(&sp_node->sn_list);	// dropped from p_space_head
			brn_p->bn_op->bn_put_node(brn_p, br_np);
			priv_p->num_bridge_nodes_dsmgr--;

			INIT_LIST_HEAD(from_head);  // from_head is still in the sp_nodes array so initialize it
		}
	} else {
		__sync_sub_and_fetch(&priv_p->p_sp_heads_size[DSMGR_LARGE_SPACE], 1);
	}
}

/*
 * Algorithm:
 * 	Add a space node to p_sp_heads/p_large_sp_head.
 * 	If this is the first node in p_sp_heads of its size, don't forget
 * 	insert a slot for this size into the rtree
 * 	Doesn't inform dsbmp
 */
static void
__dsmgr_add_node(struct dsmgr_private *priv_p, struct block_size_node *np)
{
	struct brn_interface *brn_p = priv_p->p_brn;
	struct rtree_interface *rtree_p = &priv_p->p_sp_rtree;
	struct list_head *to_head;
	uint64_t node_size = np->bsn_size;
	struct bridge_node *br_np, *br_np2 = NULL;
	struct dsmgr_space_node *sp_node, *nbr_node = NULL, *left_np = NULL;

	if (node_size > DSMGR_LARGE_SPACE) {
		__sync_add_and_fetch(&priv_p->p_sp_heads_size[DSMGR_LARGE_SPACE], 1);
		to_head = &priv_p->p_large_sp_head;
	} else {
		__sync_add_and_fetch(&priv_p->p_sp_heads_size[node_size - 1], 1);
		to_head = &priv_p->p_sp_heads[node_size - 1];
		if (list_empty(to_head)) {
			sp_node = &priv_p->p_sp_nodes[node_size - 1];
			br_np = brn_p->bn_op->bn_get_node(brn_p);
			priv_p->num_bridge_nodes_dsmgr++;
			br_np->bn_data = sp_node; 
			sp_node->sn_slot = br_np;
			br_np2 = rtree_p->rt_op->rt_search_around(rtree_p, node_size - 1);
			if (br_np2)
				nbr_node = (struct dsmgr_space_node *)br_np2->bn_data;
			assert(!nbr_node || nbr_node->sn_size != node_size);
			if (!nbr_node && !list_empty(&priv_p->p_space_head)) {
				nbr_node = list_first_entry(&priv_p->p_space_head, struct dsmgr_space_node, sn_list);
			}

			if (!nbr_node) {
				/*
				 * There is no neighbor, I'm the first node in the space pool
				 * So just insert me into the pool
				 */
				list_add(&sp_node->sn_list, &priv_p->p_space_head);
			} else {
				/*
				 * There is a neighbor
				 */

				while ((nbr_node->sn_size < node_size) && (nbr_node->sn_list.next != &priv_p->p_space_head)) {
					left_np = list_entry(nbr_node->sn_list.next, struct dsmgr_space_node, sn_list);
					if (left_np->sn_size > node_size)
						break;
					nbr_node = list_entry(nbr_node->sn_list.next, struct dsmgr_space_node, sn_list);
				}
				while ((nbr_node->sn_size > node_size) && (nbr_node->sn_list.prev != &priv_p->p_space_head)) {
					nbr_node = list_entry(nbr_node->sn_list.prev, struct dsmgr_space_node, sn_list);
				}

				/*
				 * Now, we know that if the neighbor we just found, nbr_node, is a left neighbor,
				 * it must be the closest left neighbor. If it is a right neighbor, it must br
				 * the closest right neighbor
				 */
				if (nbr_node->sn_size < node_size){
					list_add(&sp_node->sn_list, &nbr_node->sn_list);
				} else {
					list_add_tail(&sp_node->sn_list, &nbr_node->sn_list);
				}
			}

			rtree_p->rt_op->rt_insert(rtree_p, node_size - 1, br_np);
		}
	}

	list_add_tail(&np->bsn_list, to_head);
}

/*
 * Algorithm:
 * 	Inform dsbmp to do split first
 * 	split an block_size_node (old_np) into two nodes, shrunken old_np and create a new_np
 * 	return the new_np, keep the old_np in p_sp_heads/p_large_sp_head properly
 *
 * Return:
 * 	a new block_size_node which is not maintained in free space pool
 */
static struct block_size_node *
__dsmgr_space_split(struct dsmgr_private *priv_p, struct block_size_node *old_np, uint64_t new_size)
{
	struct dsbmp_interface *dsbmp_p = &priv_p->p_dsbmp;
	struct block_size_node *new_np;
	uint64_t old_size = old_np->bsn_size;

	assert(old_size > new_size);
	__dsmgr_drop_node(priv_p, old_np);
	new_np = dsbmp_p->sb_op->sb_split_node(dsbmp_p, old_np, new_size);	// old_np->bsn_size decreased by new_size
	assert((old_np->bsn_size == old_size - new_size) && (new_np->bsn_size == new_size));
	__dsmgr_add_node(priv_p, old_np);
	return new_np;
}


static struct umessage_crc_information_resp_freespace_dist *
dsmgr_check_space( struct dsmgr_interface* dsmgr_p )
{
  	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;

	return __dsmgr_check_space( priv_p );
}

static uint64_t
dsmgr_get_num_free_blocks(struct dsmgr_interface *dsmgr_p)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	struct dsbmp_interface *dsbmp_p = &priv_p->p_dsbmp;

	return dsbmp_p->sb_op->sb_get_free_blocks(dsbmp_p );
}

static int*
dsmgr_get_sp_heads_size(struct dsmgr_interface *dsmgr_p)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;

	return priv_p->p_sp_heads_size;
}

static void
dsmgr_display_space_nodes(struct dsmgr_interface* dsmgr_p) 
{
  	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	//  	struct rtree_interface* rtree_p = NULL;

	__dsmgr_traverse_rtree( priv_p );
}

static struct list_head* dsmgr_traverse_dsbmp_rtree( struct dsmgr_interface* dsmgr_p ) 
{
  	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	struct dsbmp_interface *dsbmp_p = &priv_p->p_dsbmp;
	//  	struct rtree_interface* rtree_p = NULL;

	// struct list_head* dsbmp_rtree_head;
	return dsbmp_p->sb_op->sb_traverse_rtree(dsbmp_p );

	// __dsmgr_traverse_rtree( priv_p );
}

/*
 * Algorithm:
 *
 * Input:
 * 	size: number of free blocks desired
 *
 * Output:
 * 	out_head: list of free blocks (block_size_node), sum of blocks should be the size requested by the caller
 * 	size: number of free blocks allocated in out_head 
 */
static int
dsmgr_get_space(struct dsmgr_interface *dsmgr_p, uint64_t *size, struct list_head *out_head)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	struct dsbmp_interface *dsbmp_p = &priv_p->p_dsbmp;
	struct rtree_interface *rtree_p = &priv_p->p_sp_rtree;
	struct block_size_node *new_np, *cur_np;
	struct dsmgr_space_node *sp_node = NULL, *cur_node, *largest_node = NULL;
	uint64_t largest_size = 0, expecting = *size;	// in number of blocks
	struct list_head *cur_head;
	struct bridge_node *br_np;

	pthread_mutex_lock(&priv_p->p_lck);
	if (!list_empty(&priv_p->p_space_head)){
		largest_node = list_entry(priv_p->p_space_head.prev, struct dsmgr_space_node, sn_list);
		largest_size = largest_node->sn_size;
	}

	/*
	 * If expecting space is too large to be satisfied by any space node in the space pool,
	 * try to find space inode from the large space node pool
	 */
	while (expecting > largest_size && !list_empty(&priv_p->p_large_sp_head)) {
		cur_np = list_first_entry(&priv_p->p_large_sp_head, struct block_size_node, bsn_list);
		if (cur_np->bsn_size > expecting) {
			new_np = __dsmgr_space_split(priv_p, cur_np, expecting);
			list_add_tail(&new_np->bsn_list, out_head);

			expecting = 0;
		} else {
			__dsmgr_drop_node(priv_p, cur_np);	// removed from p_large_sp_head
			dsbmp_p->sb_op->sb_get_node(dsbmp_p, cur_np);
			list_add_tail(&cur_np->bsn_list, out_head);

			expecting -= cur_np->bsn_size;
		}
	}

	if (!expecting)
		goto out;

	/*
	 * We are here because:
	 * 	1> either we are out of large space (p_large_sp_head)
	 * 	2> or one of space from p_sp_heads can satisfy expecting.
	 * No matter what, we should find space from p_sp_heads, not from p_large_sp_head any more
	 */
	br_np = rtree_p->rt_op->rt_search_around(rtree_p, min(expecting, DSMGR_LARGE_SPACE) - 1);
	if (br_np) {
		sp_node = (struct dsmgr_space_node *)br_np->bn_data;
	} else if (!br_np && !list_empty(&priv_p->p_space_head)) {
		sp_node = list_first_entry(&priv_p->p_space_head, struct dsmgr_space_node, sn_list);
	}
	if (sp_node) {
		while (sp_node->sn_size < expecting && sp_node->sn_list.next != &priv_p->p_space_head) {
			sp_node = list_entry(sp_node->sn_list.next, struct dsmgr_space_node, sn_list); 	
		}
	} else {
		/* can not find space in the space spool (p_sp_heads) */
		goto out;
	}

	/* 
	 * Good! we got some space from the space pool
	 */
	cur_node = &priv_p->p_sp_nodes[sp_node->sn_size - 1];
	cur_head = &priv_p->p_sp_heads[sp_node->sn_size - 1];
	if ( ! list_empty( cur_head ) ) {
	  	cur_np = list_first_entry(cur_head, struct block_size_node, bsn_list);
	} else {
	  	goto out;
	}
	while (expecting) {
		if (cur_np->bsn_size > expecting) {
			new_np = __dsmgr_space_split(priv_p, cur_np, expecting);
			list_add_tail(&new_np->bsn_list, out_head);

			expecting = 0;
		} else {
			__dsmgr_drop_node(priv_p, cur_np);
			dsbmp_p->sb_op->sb_get_node(dsbmp_p, cur_np);
			list_add_tail(&cur_np->bsn_list, out_head);

			expecting -= cur_np->bsn_size;

			if (expecting >= cur_np->bsn_size) {
				if (list_empty(cur_head)) {
				  	if ( (cur_node->sn_list.prev != &priv_p->p_space_head) && ( cur_node->sn_list.prev != NULL ) ) {
						cur_node = list_entry(cur_node->sn_list.prev, struct dsmgr_space_node, sn_list);
						cur_head = &priv_p->p_sp_heads[cur_node->sn_size - 1];
						cur_np = list_first_entry(cur_head, struct block_size_node, bsn_list);
					} else if ( cur_node->sn_list.prev == NULL ) {
						cur_node = NULL; 

					} else {
						cur_node = NULL;
					}
				} else {
					cur_np = list_first_entry(cur_head, struct block_size_node, bsn_list);
				}
			} else {
				br_np = rtree_p->rt_op->rt_search_around(rtree_p, expecting);
				if (br_np) {
					sp_node = (struct dsmgr_space_node *)br_np->bn_data;
					while (sp_node->sn_size < expecting && sp_node->sn_list.next != &priv_p->p_space_head) {
						sp_node = list_entry(sp_node->sn_list.next, struct dsmgr_space_node, sn_list); 	
					}
					cur_node = sp_node;
					cur_head = &priv_p->p_sp_heads[cur_node->sn_size - 1];
					cur_np = list_first_entry(cur_head, struct block_size_node, bsn_list);
				} else {
				  	// nothing left in rtree - break from the loop
				  	break;
				}
			}

			if (!cur_node)
				break;
		}
	}
out:
	pthread_mutex_unlock(&priv_p->p_lck);
	*size -= expecting;
	return 0;
}

/*
 * Algorithm:
 * 	Free some space described by snp
 * 	1> First search around in the rtree to find its neighbor
 * 	2> try to extend its neighbor if possible
 * 	3> add this node in both rtree and space order list (p_sp_heads)
 * 	   if it cannot join its neighbor
 */
static void
dsmgr_put_space(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	struct dsbmp_interface *dsbmp_p = &priv_p->p_dsbmp;
	struct blksn_interface *blksn_p = priv_p->p_blksn;
	struct block_size_node *np2;

	pthread_mutex_lock(&priv_p->p_lck);
	__dsmgr_add_node(priv_p, np);
	np2 = dsbmp_p->sb_op->sb_put_node(dsbmp_p, np);
	if (np2) {
		/* the np has not been added to dsbmp */
		assert(np2 == np);
		__dsmgr_drop_node(priv_p, np);
		blksn_p->bsn_op->bsn_put_node(blksn_p, np);
		priv_p->num_blksn_nodes_dsmgr++;
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

/*
 * Algorithm:
 * 	Do not inform dsbmp, we assume the caller should take care dsbmp update
 */
static void
dsmgr_add_new_node(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	__dsmgr_add_node(priv_p, np);
}

/*
 * Algorithm:
 * 	Do not inform dsbmp, we assume the caller should take care dsbmp update
 */
static void
dsmgr_delete_node(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	__dsmgr_drop_node(priv_p, np);
}

/*
 * Algorithm:
 * 	dsbmp wnats to extend a free space.
 * 	Does not worry about informing dsbmp. We assume dsbmp already knows this extending
 *	
 */
static void
dsmgr_extend_space(struct dsmgr_interface *dsmgr_p, struct block_size_node *np, uint64_t ext_size) 
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	uint64_t old_size = np->bsn_size; 

	if (old_size <= DSMGR_LARGE_SPACE) {
		__dsmgr_drop_node(priv_p, np);
		np->bsn_size += ext_size;	// extending
		__dsmgr_add_node(priv_p, np);
	} else {
		np->bsn_size += ext_size;	// extending
	}
}

static int
dsmgr_get_page_shift(struct dsmgr_interface *dsmgr_p)
{
	struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
	return priv_p->p_block_shift;
}

struct dsmgr_operations dsmgr_op = {
	.dm_get_space = dsmgr_get_space,
	.dm_put_space = dsmgr_put_space,
	.dm_extend_space = dsmgr_extend_space,
	.dm_add_new_node = dsmgr_add_new_node,
	.dm_delete_node = dsmgr_delete_node,
	.dm_get_page_shift = dsmgr_get_page_shift,
	.dm_display_space_nodes = dsmgr_display_space_nodes,
	.dm_check_space = dsmgr_check_space,
	.dm_get_num_free_blocks = dsmgr_get_num_free_blocks,
	.dm_get_sp_heads_size = dsmgr_get_sp_heads_size,
	.dm_traverse_dsbmp_rtree = dsmgr_traverse_dsbmp_rtree,
};

int
dsmgr_initialization(struct dsmgr_interface *dsmgr_p, struct dsmgr_setup *setup)
{
	char *log_header = "dsmgr_initialization";
	struct dsmgr_private *priv_p;
	struct dsbmp_setup dsbmp_setup;
	struct rtree_setup rtree_setup;
	int i;

	nid_log_info("%s: start ...", log_header);
	dsmgr_p->dm_op = &dsmgr_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	dsmgr_p->dm_private = priv_p;

	pthread_mutex_init(&priv_p->p_lck, NULL);
	priv_p->p_sp_heads_size[DSMGR_LARGE_SPACE] = 0;
	for (i = 0; i < DSMGR_LARGE_SPACE; i++) {
		INIT_LIST_HEAD(&priv_p->p_sp_heads[i]);
		priv_p->p_sp_heads_size[i] = 0;
		priv_p->p_sp_nodes[i].sn_size = i + 1;
	}
	INIT_LIST_HEAD(&priv_p->p_large_sp_head);
	INIT_LIST_HEAD(&priv_p->p_space_head);

	priv_p->p_allocator = setup->allocator;
	priv_p->p_brn = setup->brn;
	priv_p->p_blksn = setup->blksn;
	priv_p->p_size = setup->size;  
	priv_p->p_block_shift = setup->block_shift;  

	rtree_setup.allocator = priv_p->p_allocator;
	rtree_setup.extend_cb = __dsmgr_rtree_extend_cb;
	rtree_setup.insert_cb = __dsmgr_rtree_insert_cb;
	rtree_setup.remove_cb = __dsmgr_rtree_remove_cb;
	rtree_setup.shrink_to_root_cb = __dsmgr_rtree_shrink_to_root_cb;
	rtree_initialization(&priv_p->p_sp_rtree, &rtree_setup);

	assert(priv_p->p_size / 64);
	dsbmp_setup.size = priv_p->p_size / 64;	// in units (uint64_t)
	dsbmp_setup.bitmap = setup->bitmap;
	dsbmp_setup.dsmgr = dsmgr_p;
	dsbmp_setup.brn = priv_p->p_brn;
	dsbmp_setup.blksn = priv_p->p_blksn;
	dsbmp_setup.allocator = priv_p->p_allocator;
	dsbmp_initialization(&priv_p->p_dsbmp, &dsbmp_setup);

 	priv_p->num_bridge_nodes_dsmgr = 0;
	priv_p->num_blksn_nodes_dsmgr = 0;

	return 0;
}
