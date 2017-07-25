/*
 * rtree.c
 * 	Implementation of Raidx Tree Module
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "nid_log.h"
#include "list.h"
#include "bit_if.h"
#include "rtn_if.h"
#include "rtree_if.h"

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define	RTREE_MAX_HEIGHT	8
#define RTREE_INDEX_BITS	64
#define RTREE_MAP_SHIFT		6
#define RTREE_MAP_SIZE		(1 << RTREE_MAP_SHIFT)
#define	RTREE_MAP_MASK		(RTREE_MAP_SIZE - 1)
#define RTREE_MAX_PATH		(DIV_ROUND_UP(RTREE_INDEX_BITS, RTREE_MAP_SHIFT))

struct rtree_root {
	uint16_t		r_height;
	struct rtree_node	*r_rnode;
};

struct rtree_iterator {
  	int			cursor_height;
	struct rtree_node*	parent_stack[ RTREE_MAX_HEIGHT ];
  	uint16_t		marker_stack[  RTREE_MAX_HEIGHT ];
  	int 			marker_stack_index; // pointer to top of stack - above first entry
  	int	 		parent_stack_index; // pointer to top of stack - above first entry
  	int			done_traversing; // indicates if traversal is finished

};

void push_iter_stack_values( struct rtree_iterator* rtree_it_p, struct rtree_node* rt_node, uint16_t curr_marker )
{
  	rtree_it_p->parent_stack[ ++ ( rtree_it_p->parent_stack_index ) ] = rt_node;
  	rtree_it_p->marker_stack[ ++ (rtree_it_p->marker_stack_index)  ] = curr_marker;
	// rtree_it_p->parent_stack_index ++;
	// rtree_it_p->marker_stack_index ++;
}

void pop_iter_stack_values( struct rtree_iterator* rtree_it_p, struct rtree_node** rt_node, uint16_t* curr_marker )
{
  	assert( rtree_it_p->marker_stack_index >= 0 );
	assert( rtree_it_p->parent_stack_index >= 0 );
  	*curr_marker = rtree_it_p->marker_stack[ rtree_it_p->marker_stack_index ];
  	*rt_node = rtree_it_p->parent_stack[ rtree_it_p->parent_stack_index ];
	rtree_it_p->parent_stack[ rtree_it_p->parent_stack_index ] = NULL;
	rtree_it_p->marker_stack[ rtree_it_p->marker_stack_index ] = -1;
	rtree_it_p->parent_stack_index --;
	rtree_it_p->marker_stack_index --;
}


struct rtree_private {
	struct bit_interface	p_bit;
	struct rtree_root	p_root;
	struct rtn_interface	p_rtn;
	uint16_t		p_index_bits;
	uint16_t		p_map_shift;
	uint64_t		p_map_size;
	uint64_t		p_map_mask;
	uint64_t		p_height_to_maxindex[RTREE_MAX_PATH + 1];
	uint64_t		p_bitmap_mask[RTREE_INDEX_BITS];
	uint64_t		p_bitmap_bit[RTREE_INDEX_BITS];
	uint64_t		p_high_mask[RTREE_INDEX_BITS];
	uint64_t		p_little_mask[RTREE_INDEX_BITS];
	void			(*p_extend_cb)(void *, void *);
	void*			(*p_insert_cb)(void *, void *, void *);
	void*			(*p_remove_cb)(void *, void *);
	void			(*p_shrink_to_root_cb)(void * );
	int			p_iter_start;
  	struct rtree_iterator* 	rtree_iter_p;
};

/*
 * Note:
 * 	Lets assume RTREE_INDEX_BITS == 64, p_map_shift == 6
 * 	The bottom layer (leaf) is of level 0. The maxindex is 0, It can represent only one leaf node (2**0 == 1).
 *	The layer above the bottom layer is of level 1. The maxindex is 63. It can represent 64 leaf nodes (2**6 == 64)  
 *	The layer above is of level 2. The index is 2**12-1. It can represent 2**12 leaf nodes.
 *	The layer above is of level 3. The index is 2**18-1. It can represent 2**18 leaf nodes.
 *	The layer above is of level 4. The index is 2**24-1. It can represent 2**24 leaf nodes.
 *	The layer above is of level 5. The index is 2**30-1. It can represent 2**30 leaf nodes.
 *	The layer above is of level 6. The index is 2**36-1. It can represent 2**36 leaf nodes.
 *	...
 */
static uint64_t
__maxindex(struct rtree_interface *rtree_p, uint16_t height)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	uint64_t width = height * priv_p->p_map_shift;
	int shift = RTREE_INDEX_BITS - width;

	if (shift < 0)
		return ~0UL;
	if (shift >= RTREE_INDEX_BITS)
		return 0UL;
	return ~0UL >> shift;
}

static void
__rtree_init_maxindex(struct rtree_interface *rtree_p)
{
	char *log_header = "__rtree_init_maxindex";
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	int i;
	for (i = 0; i < RTREE_MAX_PATH + 1; i++) {
		priv_p->p_height_to_maxindex[i] = __maxindex(rtree_p, i);
		nid_log_debug("%s: i:%d, max:0x%lx", log_header, i, priv_p->p_height_to_maxindex[i]);
	}
}

/*
 * Algorithm:
 * 	The caller knows the rtree should extend at least one level 
 * 	If the rtree->r_rnode == NULL at input, just update the rtree height.
 * 	When extending a new level, the old tree will be the most left slot (slot 0)
 * 	of the new node
 */
static int
__rtree_extend(struct rtree_interface *rtree_p, uint64_t index_key)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *np, *slot;
	uint16_t height, newheight;

	height = root_p->r_height;
	while (index_key > priv_p->p_height_to_maxindex[height])
		height++;

	if (root_p->r_rnode == NULL) {
		root_p->r_height = height;
		assert(root_p->r_height <= RTREE_MAX_HEIGHT);
		goto out;
	}

	do {
		np = rtn_p->rt_op->rt_get_node(rtn_p);
		memset(np->n_slots, 0, sizeof(np->n_slots));
		newheight = root_p->r_height + 1;
		assert(newheight <= RTREE_MAX_HEIGHT);
		np->n_height = newheight;
		np->n_bitmap = 1;	// the first slot (most left one) will be set.
		np->n_count = 1;	// one slot(the most left one) will be set
		np->n_parent = NULL;	// i'm the root node (r_rnode)
		slot = root_p->r_rnode;
		if (newheight > 1)
			slot->n_parent = np;
		else if (slot)
			priv_p->p_extend_cb(slot, np);

		np->n_slots[0] = slot;
		root_p->r_rnode = np;
		root_p->r_height = newheight;
		assert(root_p->r_height <= RTREE_MAX_HEIGHT);
	} while (height > root_p->r_height);

out:
	return 0;
}

/*
 * Algorithm:
 * 	Prepare/extend the rtree properly to hold the new item as a leaf node
 * 	After insert a new item, don't forget to call insert callback function.
 *
 * Input:
 * 	index_key: the position of item in the whole range 
 * 	item: only the caller (application) understands what it is. It will be
 * 	inserted as a leaf node (level 0) in the rtree
 */
static void * 
rtree_insert(struct rtree_interface *rtree_p, uint64_t index_key, void *item)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *np = NULL, *slot;
	uint64_t offset;
	uint16_t height, shift;
	void *rc_slot = NULL;

	if (index_key > priv_p->p_height_to_maxindex[root_p->r_height]) {
		if (__rtree_extend(rtree_p, index_key))
			goto out;
	}

	slot = root_p->r_rnode;
	height = root_p->r_height;
	shift = (height - 1) * priv_p->p_map_shift;
	offset = 0;
	while (height > 0) {
		if (slot == NULL) {
			slot = rtn_p->rt_op->rt_get_node(rtn_p);
			memset(slot->n_slots, 0, sizeof(slot->n_slots));
			slot->n_bitmap = 0;
			slot->n_count = 0;
			slot->n_height = height;
			assert(slot->n_height <= 8);
			slot->n_parent = np;
			if (np) {
				np->n_slots[offset] = slot;
				np->n_bitmap |= priv_p->p_bitmap_bit[offset];
				np->n_count++;
				assert(np->n_count <= 64);
			} else {
				root_p->r_rnode = slot;
			}
		}

		/* Go a level down */
		offset = (index_key >> shift) & priv_p->p_map_mask;
		np = slot;
		slot = np->n_slots[offset];
		shift -= priv_p->p_map_shift;
		height--;
	}

	if (np) {
		assert(root_p->r_height);
		assert((np->n_height == 1) && (np->n_count <= 64));
		if (!np->n_slots[offset]) {
			np->n_bitmap |= priv_p->p_bitmap_bit[offset];
			np->n_count++;
		}
		np->n_slots[offset] = priv_p->p_insert_cb(np->n_slots[offset], item, np);
		rc_slot = np->n_slots[offset];
	} else {
		assert(root_p->r_height == 0);
		root_p->r_rnode = priv_p->p_insert_cb(root_p->r_rnode, item, NULL);
		rc_slot = root_p->r_rnode;
	}

out:
	return rc_slot;
}

/*
 * Algorithm:
 * 	Insert a node whose parent very likely be the given hint parent (hint_parent) which contains hint_key
 */
static void * 
rtree_hint_insert(struct rtree_interface *rtree_p, uint64_t index_key, void *item, uint64_t hint_key, void *hint_parent)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_node *np = (struct rtree_node *)hint_parent;
	void *rc_slot = NULL;
	int off;

	assert(priv_p);
	assert(!np || np->n_height == 1);
	if (np && (hint_key >> RTREE_MAP_SHIFT) == (index_key >> RTREE_MAP_SHIFT)) {
		off = (hint_key & RTREE_MAP_MASK);
		void* prev_slot_val =  np->n_slots[ off ];
		np->n_slots[off] = priv_p->p_insert_cb(np->n_slots[off], item, np);		
		rc_slot = np->n_slots[off];
		assert(  NULL != rc_slot   );
		if ( NULL == prev_slot_val  ) {
			np->n_bitmap |= priv_p->p_bitmap_bit[off];
			np->n_count++;
		} else {
			assert( prev_slot_val == rc_slot );
		}

		assert(np->n_count <= 64);
	} else {
		rc_slot = rtree_insert(rtree_p, index_key, item);
	}

	return rc_slot;
}

/*
 * Agorithm:
 * 	Start from the root node (r_rnode), remove one node if it only contains
 * 	the most left slot.
 */
static void
__rtree_shrink(struct rtree_interface *rtree_p)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *to_free = root_p->r_rnode;
	struct rtree_node *slot;

	if (root_p->r_height > 0 && !to_free->n_count) {
		root_p->r_height = 0;
		root_p->r_rnode = NULL;
		rtn_p->rt_op->rt_put_node(rtn_p, to_free);
		return;
	}

	while (root_p->r_height > 0) {
		to_free = root_p->r_rnode;

		if (to_free->n_count != 1)
			break;
		if (!to_free->n_slots[0])
			break;

		slot = to_free->n_slots[0];
		if (root_p->r_height > 1)
			slot->n_parent = NULL;
		root_p->r_rnode = slot;
		root_p->r_height--;

		rtn_p->rt_op->rt_put_node(rtn_p, to_free);
	}
	if ( 0 == root_p->r_height ) {
	  	// assign the parent of the remaining bridge node to null 
	  	if ( ( root_p->r_rnode ) && ( priv_p->p_shrink_to_root_cb ) ) {
		  	priv_p->p_shrink_to_root_cb( root_p->r_rnode );
		} else if ( ( root_p->r_rnode ) && ( ! priv_p->p_shrink_to_root_cb ) )  {
		  	nid_log_error( "shrink_to_root issue" );
		}

	}
	assert(root_p->r_height <= RTREE_MAX_HEIGHT);
	assert(!root_p->r_height || root_p->r_rnode->n_height == root_p->r_height);
	assert(!root_p->r_height || root_p->r_rnode->n_count <= RTREE_INDEX_BITS);
}

/*
 * Algorithm:
 * 	1> search the leaf node based on index_key, start from the root node (r_rnode)
 * 	2> remove the item from the leaf node by remove call back
 * 	3> shrinking from bottom
 *
 * Input:
 * 	index_key: based on this index to find out the leaf node. Only the user
 * 	understands what the leaf node is.
 * 	item: only the user knows what it is.
 */
static void *
rtree_remove(struct rtree_interface *rtree_p, uint64_t index_key, void *item)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *np, *slot = NULL, *to_free;
	uint64_t shift;
	uint64_t offset;

	if (index_key > priv_p->p_height_to_maxindex[root_p->r_height])
		goto out;

	slot = root_p->r_rnode;
	if (root_p->r_height == 0) {
		slot = priv_p->p_remove_cb(root_p->r_rnode, item);
		if ( ! slot ) {
			slot = root_p->r_rnode;
			root_p->r_rnode = NULL;
		} 
		goto out;
	}

	shift = root_p->r_height * priv_p->p_map_shift;
	do {
		if (slot == NULL)
			goto out;

		shift -= priv_p->p_map_shift;
		offset = (index_key >> shift) & priv_p->p_map_mask;
		np = slot;
		slot = slot->n_slots[offset];
	} while (shift);

	if (slot == NULL)
		goto out;

	/*
	 * Now free the nodes we do not need anymore
	 */
	to_free = NULL;
	while (np) {
		//np->n_slots[offset] = NULL;
		np->n_slots[offset] = priv_p->p_remove_cb(np->n_slots[offset], item); 
		item = NULL;
		if (!np->n_slots[offset]) {
			np->n_bitmap &= priv_p->p_bitmap_mask[offset];
			np->n_count--;
			assert(!np->n_count || np->n_bitmap);
			assert(np->n_count <= RTREE_INDEX_BITS); 
		}

		/*
		 * Queue the node for deferred freeing after the
		 * last reference to it disappears (set NULL, above).
		 */
		if (to_free)
			rtn_p->rt_op->rt_put_node(rtn_p, to_free);

		if (np->n_count) {
			if (np == root_p->r_rnode)
				__rtree_shrink(rtree_p);
			goto out;
		}

		/* Node with zero n_slots in use so free it */
		to_free = np;

		index_key >>= priv_p->p_map_shift;
		offset = index_key & priv_p->p_map_mask;
		np = np->n_parent;
	}

	root_p->r_height = 0;
	root_p->r_rnode = NULL;
	if (to_free)
		rtn_p->rt_op->rt_put_node(rtn_p, to_free);

out:
	return slot;
}

static void *
rtree_hint_remove(struct rtree_interface *rtree_p, uint64_t index_key, void *item, void *n_parent)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *np = (struct rtree_node *)n_parent, *to_free;
	void *slot = NULL;
	uint64_t offset;

	if (np == NULL) {
		assert(root_p->r_height == 0);
		slot = priv_p->p_remove_cb(root_p->r_rnode, item);
		root_p->r_rnode = slot;
		goto out;
	}
	assert(np->n_height == 1);

	offset = index_key & priv_p->p_map_mask;
	assert(np->n_slots[offset]);
	slot = priv_p->p_remove_cb(np->n_slots[offset], item);
	np->n_slots[offset] = slot;
	if (!np->n_slots[offset]) {
		assert(np->n_count > 0);
		np->n_bitmap &= priv_p->p_bitmap_mask[offset];
		np->n_count--;
		assert(!np->n_count || np->n_bitmap);
		assert(np->n_count <= 64);
	}

	while (!np->n_count && np->n_parent) {
		to_free = np;
		np = np->n_parent;
		index_key >>= priv_p->p_map_shift;
		offset = index_key & priv_p->p_map_mask;
		assert(np->n_slots[offset] == to_free);
		assert(np->n_height <= root_p->r_height);
		assert(np->n_count > 0 && np->n_count <= RTREE_INDEX_BITS);
		np->n_slots[offset] = NULL;
		np->n_bitmap &= priv_p->p_bitmap_mask[offset];
		np->n_count--;
		assert(!np->n_count || np->n_bitmap);
		assert(np->n_count <= 64);
		rtn_p->rt_op->rt_put_node(rtn_p, to_free);
	}

	if (!np->n_parent) {
		assert(np->n_height == root_p->r_height);
		__rtree_shrink(rtree_p);
	}

out:
	return slot;
	
}

/*
 * Algorithm:
 * 	Start searching from the root node (r_rnode).
 * 	The shift for level n is (n-1)*priv_p->p_map_shift
 */
static void *
rtree_search(struct rtree_interface *rtree_p, uint64_t index_key)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *slot = NULL;
	uint64_t shift;
	uint64_t offset;

	if (index_key > priv_p->p_height_to_maxindex[root_p->r_height])
		goto out;

	slot = root_p->r_rnode;
	if (root_p->r_height == 0)
		goto out;

	shift = root_p->r_height * priv_p->p_map_shift;
	do {
		if (slot == NULL)
			goto out;

		shift -= priv_p->p_map_shift;
		offset = (index_key >> shift) & priv_p->p_map_mask;
		slot = slot->n_slots[offset];
	} while (shift);

out:
	return (void *)slot;
}

/*
 * Algorithm:
 * 	Search a node whose parent very likely be the given hint parent (hint_parent) which contains hint_key
 */
static void * 
rtree_hint_search(struct rtree_interface *rtree_p, uint64_t index_key, uint64_t hint_key, void *hint_parent)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_node *np = (struct rtree_node *)hint_parent;
	void *rc_slot = NULL;
	int index_off;

	assert(priv_p);
	assert(!np || np->n_height == 1);
	if (np && (hint_key >> RTREE_MAP_SHIFT) == (index_key >> RTREE_MAP_SHIFT)) {
		index_off = (index_key & RTREE_MAP_MASK);
		rc_slot = np->n_slots[index_off];
	} else {
		rc_slot = rtree_search(rtree_p, index_key);
	}

	return rc_slot;
}

/*
 * Algorithm:
 * 	the same as rtree_search if the target item is at its slot
 * 	If the slot of the index_key does not exist, we'll looking around in the
 * 	node which is supposed to contain this non-existent slot but not.
 * 	1> If the index_key large than the crrent height can hold, return NULL
 * 	2> If this node contains a slot on the left side of the postion of this non-existent
 * 	slot, return the nearest left slot.
 * 	3> If the node is the root node, this means there is no left slot in the whole rtree,
 * 	return NULL
 * 	4> Otherwise, (we don't know if there is left slot, but don't want to waste time to check it)
 * 	just return the nearest right item 
 *
 * Return:
 * 	NULL: either smaller or larger than all index in the rtree
 *	
 * Notice:
 * 	left side means smaller offsite
 */
static void *
rtree_search_around(struct rtree_interface *rtree_p, uint64_t index_key)
{
	char *log_header = "rtree_search_around";
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node *slot = NULL, *np = NULL;
	uint64_t shift;
	uint64_t side_mask, side_bitmap, offset, offset2;

	/* if it's definitly larger than all index in the rtree */
	if (index_key > priv_p->p_height_to_maxindex[root_p->r_height])
		goto out;

	slot = root_p->r_rnode;
	if (root_p->r_height == 0) {
		nid_log_debug("%s: index_key:0x%lx got root", log_header, index_key);
		goto out;
	}

	shift = root_p->r_height * priv_p->p_map_shift;
	do {
		if (slot == NULL)
			break;

		shift -= priv_p->p_map_shift;
		offset = (index_key >> shift) & priv_p->p_map_mask;
		np = slot;
		slot = slot->n_slots[offset];
	} while (shift);

	if (slot) {
		nid_log_debug("%s: don't need searching around", log_header);
		goto out;
	}
	assert(np->n_count);

	/*
	 * looking around, start on left side
	 */
	offset2 = offset;
	side_mask = priv_p->p_high_mask[offset];
	side_bitmap = np->n_bitmap & side_mask;
	if (side_bitmap) {
		offset2 = RTREE_INDEX_BITS - __builtin_clzll(side_bitmap) - 1;
		assert(np->n_slots[offset2]);
		while (np->n_height > 1) {
			np = np->n_slots[offset2];
			offset2 = RTREE_INDEX_BITS - __builtin_clzll(np->n_bitmap) - 1;
			assert(np->n_slots[offset2]);
		}
		slot = np->n_slots[offset2];
		assert(slot);
		nid_log_debug("%s: searching around left success", log_header);
		goto out;
	} else if (np->n_height == root_p->r_height) {
		/* 
		 * It's easy to tell that there is no slot on the left side of me in the whole rtree
		 * Don't search the right side. Just return NULL
		 */
		goto out;
	}

	/*
	 * searching on right side
	 */
	offset2 = offset;
	offset2 = __builtin_ffsll(np->n_bitmap) - 1;
	assert(np->n_slots[offset2]);
	while (np->n_height > 1) {
		np = np->n_slots[offset2];
		offset2 = __builtin_ffsll(np->n_bitmap) - 1;
		assert(np->n_slots[offset2]);
	}
	slot = np->n_slots[offset2];
	assert(slot);
	nid_log_debug("%s: searching around right success", log_header);
	goto out;

out:
	return (void *)slot;
}

/*
 * Algorithm:
 * 	Search around a node whose parent very likely be the given hint parent (hint_parent) which contains hint_key
 */
static void *
rtree_hint_search_around(struct rtree_interface *rtree_p,
	uint64_t index_key, uint64_t hint_key, void *hint_parent)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_node *np = (struct rtree_node *)hint_parent;
	void *rc_slot = NULL;
	int off, hint_off;

	assert(priv_p);
	assert(!np || np->n_height == 1);
	if (np && (hint_key >> RTREE_MAP_SHIFT) == (index_key >> RTREE_MAP_SHIFT)) {
		off = (index_key & RTREE_MAP_MASK);
		if (!(rc_slot = np->n_slots[off])) {
			hint_off = (hint_key & RTREE_MAP_MASK);
			rc_slot = np->n_slots[hint_off];
		}
	} else {
		rc_slot = rtree_search_around(rtree_p, index_key);
	}

	return rc_slot;
}


static void
rtree_get_stat(struct rtree_interface *rtree_p, struct rtree_stat *stat)
{
	char *log_header = "rtree_get_stat";
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct rtn_stat rtn_stat;
	nid_log_debug("%s: start", log_header);
	rtn_p->rt_op->rt_get_stat(rtn_p, &rtn_stat);
	stat->s_rtn_nseg = rtn_stat.s_nseg;	
	stat->s_rtn_segsz = rtn_stat.s_segsz;	
	stat->s_rtn_nfree = rtn_stat.s_nfree;	
	stat->s_rtn_nused = rtn_stat.s_nused;	
}

static void
rtree_close(struct rtree_interface *rtree_p)
{
	char *log_header = "rtree_close";
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtn_interface *rtn_p = &priv_p->p_rtn;
	struct bit_interface *bit_p = &priv_p->p_bit;
	nid_log_notice("%s: start (rtree_p:%p) ...", log_header, rtree_p);
	rtn_p->rt_op->rt_close(rtn_p);
	bit_p->b_op->b_close(bit_p);
	free(priv_p);
	rtree_p->rt_private = NULL;
}

static void
rtree_iter_start(struct rtree_interface *rtree_p)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_iterator* rtree_it_p = (struct rtree_iterator*)priv_p->rtree_iter_p;
	assert(priv_p);
	priv_p->p_iter_start = 1;
	rtree_it_p->cursor_height = priv_p->p_root.r_height; // start at bottom of the tree
	memset( rtree_it_p->parent_stack, 0, sizeof( struct rtree_node* ) );
	memset( rtree_it_p->marker_stack, 0, sizeof( uint16_t ) );
	nid_log_debug( "rtree_iter_start() - starting" );
	rtree_it_p->marker_stack_index = -1;
	rtree_it_p->parent_stack_index = -1;
	rtree_it_p->done_traversing = 0;
}

static void
rtree_iter_stop(struct rtree_interface *rtree_p)
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;

	assert(priv_p);
	priv_p->p_iter_start = 0;	

}

static void *
rtree_iter_next(struct rtree_interface *rtree_p )
{
	struct rtree_private *priv_p = (struct rtree_private *)rtree_p->rt_private;
	struct rtree_iterator* rtree_it_p = (struct rtree_iterator*)priv_p->rtree_iter_p;
	struct rtree_root *root_p = &priv_p->p_root;
	struct rtree_node* curr_node_p = NULL; // *old_node_p = NULL;
	// uint16_t done_traversing = rtree_it_p->done_traversing;
	uint16_t curr_marker = 0;

	nid_log_debug( "rtree_iter_next() - starting - done_traversing = %d", rtree_it_p->done_traversing );
	assert( priv_p->p_iter_start );
	assert(priv_p);
	if ( rtree_it_p->done_traversing ) {
	  	return NULL;
	}
	if ( rtree_it_p->marker_stack_index >= 0 ) {
		pop_iter_stack_values( rtree_it_p, &curr_node_p, &curr_marker );
	} else { // start from scratch
	  curr_node_p = root_p->r_rnode;
	}
	if ( ! curr_node_p ) {
	  	nid_log_debug( "rtree_iter_next() - return NULL - curr_node_p == NULL" );
		return NULL;
	}

	uint16_t tree_height = root_p->r_height;

	while ( ! rtree_it_p->done_traversing ) {
	  	while ( ( rtree_it_p->cursor_height > 1 ) && ( curr_marker < 64 ) ) {
	  		while ( curr_marker < 64 ) {
		  		if ( curr_node_p->n_slots[ curr_marker ] ) {
		    			nid_log_debug( "rtree_iter_next() - found a slot" );
		    			// traverse this slot
		    			curr_marker ++;
		    			push_iter_stack_values( rtree_it_p, curr_node_p, curr_marker );
					curr_node_p = curr_node_p->n_slots[ curr_marker - 1 ];
					curr_marker = 0; // reset the marker to 0, since at new level
					rtree_it_p->cursor_height --;
					break;
		  		}
		  		curr_marker ++;
			}
	  	}
	  	if ( ( rtree_it_p->cursor_height == 1 ) && ( curr_marker < 64 ) ) { // at the leaves
	    		while ( curr_marker < 64 ) {
	      			if ( curr_node_p->n_slots[ curr_marker ] ) {
					push_iter_stack_values( rtree_it_p, curr_node_p, ++ curr_marker );
					// curr_marker ++; // increment marker for the next time
					return curr_node_p->n_slots[ curr_marker - 1 ];
	      			}
	      			curr_marker ++;
	    		}
	  	} else if ( rtree_it_p->cursor_height == 0 ) {
		  	rtree_it_p->done_traversing = 1;
	  		nid_log_debug( "rtree_iter_next() - return root node as curr_node_p" );
		  	return curr_node_p; // this should be the only node on the tree

		} 
		nid_log_debug( "rtree_iter_next() - past checking the leaves" );
	  	if ( curr_marker >= 64 )  { // not at the leaves, but at the left most node - need to traverse back to parent
	    		nid_log_debug( "rtree_iter_next() - level greater than 64" );
	    		if ( rtree_it_p->cursor_height < tree_height ){
		  		pop_iter_stack_values( rtree_it_p, &curr_node_p, &curr_marker );
				rtree_it_p->cursor_height ++;
	    		} else {
	      			// no more values - at bottom of the stack
		  		rtree_it_p->done_traversing = 1;
				break;
			}
	  	} else {
	    		rtree_it_p->done_traversing = 1;
	  	}
	}
	return NULL;	
}


struct rtree_operations rtree_op = {
	.rt_insert = rtree_insert,
	.rt_hint_insert = rtree_hint_insert,
	.rt_remove = rtree_remove,
	.rt_hint_remove = rtree_hint_remove,
	.rt_search = rtree_search,
	.rt_hint_search = rtree_hint_search,
	.rt_hint_search_around = rtree_hint_search_around,
	.rt_search_around = rtree_search_around,
	.rt_get_stat = rtree_get_stat,
	.rt_close = rtree_close,
	.rt_iter_start = rtree_iter_start,
	.rt_iter_stop = rtree_iter_stop,
	.rt_iter_next = rtree_iter_next,
};

int
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	struct rtree_private *priv_p;
	struct rtn_interface *rtn_p;
	struct rtn_setup rtn_setup;
	struct bit_interface *bit_p;
	struct bit_setup bit_setup;
	int i;

	nid_log_notice("rtree_initialization start ...");
	assert(setup);
	rtree_p->rt_op = &rtree_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	priv_p->rtree_iter_p = (struct rtree_iterator*)x_calloc( 1, sizeof(struct rtree_iterator) );
	rtree_p->rt_private = priv_p;
	priv_p->p_index_bits = RTREE_INDEX_BITS;// cannot larger than RTREE_INDEX_BITS 
	priv_p->p_map_shift = RTREE_MAP_SHIFT;	// cannot larger than RTREE_MAP_SHIFT 
	priv_p->p_map_size = (1UL << priv_p->p_map_shift);
	priv_p->p_map_mask = priv_p->p_map_size - 1;
	priv_p->p_extend_cb = setup->extend_cb;
	priv_p->p_insert_cb = setup->insert_cb;
	priv_p->p_remove_cb = setup->remove_cb;
	priv_p->p_shrink_to_root_cb = setup->shrink_to_root_cb;
	__rtree_init_maxindex(rtree_p);

	rtn_p = &priv_p->p_rtn;
	rtn_initialization(rtn_p, &rtn_setup);
	bit_p = &priv_p->p_bit;
	bit_initialization(bit_p, &bit_setup);

	for (i = 0; i < RTREE_INDEX_BITS; i++) {
		priv_p->p_bitmap_bit[i] = (1UL << i);
		priv_p->p_bitmap_mask[i] = ~(priv_p->p_bitmap_bit[i]);
	}
	for (i = 1; i < RTREE_INDEX_BITS; i++) {
		priv_p->p_high_mask[i] = priv_p->p_high_mask[i-1] | (1UL << (i - 1));
	}
	for (i = RTREE_INDEX_BITS - 2; i >= 0; i--) {
		priv_p->p_little_mask[i] = priv_p->p_little_mask[i+1] | (1UL << (i + 1));
	}

	for (i=0; i < RTREE_INDEX_BITS; i++)
		nid_log_debug("p_high_mask[%d]: 0x%lx", i, priv_p->p_high_mask[i]);
	for (i=0; i < RTREE_INDEX_BITS; i++)
		nid_log_debug("p_little_mask[%d]: 0x%lx", i, priv_p->p_little_mask[i]);
		         
	return 0;
}
