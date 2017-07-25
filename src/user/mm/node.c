/*
 * node.c
 * 	Implementation of Node Module
 */

#include <stdlib.h>
#include <pthread.h>
#include "nid_log.h"
#include "allocator_if.h"
#include "node_if.h"

#define NODE_SEGMENT_SIZE	128


struct node_segment {
	void			*a_handle;
	void			*a_seg_handle;
	uint32_t		nfree;
	struct list_head	list;
	struct node_header	*segment;
};

struct node_private {
	struct allocator_interface	*p_allocator;
	int				p_a_set_id;
	struct list_head		p_seg_head;
	struct list_head		p_free_head;
	uint32_t			p_nseg;
	uint32_t			p_nfree;
	pthread_mutex_t			p_lck;
	uint32_t			p_node_size;
	uint32_t			p_seg_size;
	size_t				p_alignment;
};

static void
__node_extend(struct node_interface *node_p)
{
	char *log_header = "__node_extend";
	struct node_private *priv_p = (struct node_private *)node_p->n_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	int a_set_id = priv_p->p_a_set_id;
	uint32_t i, seg_size = priv_p->p_seg_size;
	struct node_segment *seg_p;
	struct node_header *nhp;
	void *a_handle;

	if (allocator_p) {
		a_handle = allocator_p->a_op->a_calloc(allocator_p, a_set_id, 1, sizeof(*seg_p));
		seg_p = (struct node_segment *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
		seg_p->a_handle = a_handle;

		if (!priv_p->p_alignment)
			a_handle = allocator_p->a_op->a_malloc(allocator_p, a_set_id,
				seg_size * priv_p->p_node_size);
		else
			a_handle = allocator_p->a_op->a_memalign(allocator_p, a_set_id,
				priv_p->p_alignment, seg_size * priv_p->p_node_size);
		seg_p->segment = (struct node_header *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
		seg_p->a_seg_handle = a_handle;
	} else {
		seg_p = x_calloc(1, sizeof(*seg_p));
		if (!seg_p) {
			nid_log_error("%s: x_calloc failed.", log_header);
		}
		if (!priv_p->p_alignment) {
			seg_p->segment = x_malloc(seg_size * priv_p->p_node_size);
			if (!seg_p->segment ) {
				nid_log_error("%s: x_malloc failed.", log_header);
			}
		}
		else {
			if (x_posix_memalign((void **)&seg_p->segment, priv_p->p_alignment, seg_size * priv_p->p_node_size) != 0) {
				nid_log_error("%s: x_posix_memalign failed.", log_header);
			}
		}
	}
	nid_log_debug("%s: p_nfree:%u, p_nseg:%u", log_header, priv_p->p_nfree, priv_p->p_nseg);
	seg_p->nfree = priv_p->p_seg_size;
	for (i = 0, nhp = &seg_p->segment[0]; i < seg_size; i++  ) {
		nhp->n_owner = (void *)seg_p;
		nhp = (struct node_header*)((char*)nhp + priv_p->p_node_size );
	}

	for (i = 0, nhp = &seg_p->segment[0]; i < seg_size; i++  ) {
		list_add(&nhp->n_list, &priv_p->p_free_head);
		nhp = (struct node_header*)((char*)nhp + priv_p->p_node_size );
	}
	priv_p->p_nfree += seg_size;
	list_add(&seg_p->list, &priv_p->p_seg_head);
	priv_p->p_nseg++;
}

static struct node_header *
node_get_node(struct node_interface *node_p)
{
	struct node_private *priv_p = (struct node_private *)node_p->n_private;
	struct node_segment *seg_p;
	struct node_header *nhp = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	if (list_empty(&priv_p->p_free_head)) {
		__node_extend(node_p);
	}
	nhp = list_first_entry(&priv_p->p_free_head, struct node_header, n_list);
	list_del(&nhp->n_list);
	priv_p->p_nfree--;
	seg_p = nhp->n_owner;
	seg_p->nfree--;
	pthread_mutex_unlock(&priv_p->p_lck);
	return nhp;
}

/*
 * Notice:
 * 	the caller should hold p_lck
 */
static void
__node_shrink(struct node_interface *node_p, struct node_segment *seg_p)
{
	char *log_header = "__node_shrink";
	struct node_private *priv_p = (struct node_private *)node_p->n_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	uint32_t i, seg_size = priv_p->p_seg_size;
	struct node_header *nhp;

	nid_log_debug("%s: p_nfree:%u, p_nseg:%u", log_header, priv_p->p_nfree, priv_p->p_nseg);
	assert(seg_p->nfree == seg_size);
	for (i = 0, nhp = &seg_p->segment[0]; i < seg_size; i++) {
		list_del(&nhp->n_list);
		nhp = (struct node_header*)((char*)nhp + priv_p->p_node_size );
	}
	priv_p->p_nfree -= seg_size;
	list_del(&seg_p->list);
	priv_p->p_nseg--;
	if (allocator_p) {
		allocator_p->a_op->a_free(allocator_p, seg_p->a_seg_handle);
		allocator_p->a_op->a_free(allocator_p, seg_p->a_handle);
	} else {
		free(seg_p->segment);
		free(seg_p);
	}
}

static void
node_put_node(struct node_interface *node_p, struct node_header *nhp)
{
	struct node_private *priv_p = (struct node_private *)node_p->n_private;
	struct node_segment *seg_p = (struct node_segment *)nhp->n_owner;

	pthread_mutex_lock(&priv_p->p_lck);
	list_add(&nhp->n_list, &priv_p->p_free_head);
	priv_p->p_nfree++;
	seg_p->nfree++;
	nid_log_debug("node_put_node: put one node");
	if ((seg_p->nfree == priv_p->p_seg_size) && (priv_p->p_nfree >= priv_p->p_seg_size * 2)) {
		__node_shrink(node_p, seg_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
node_destroy(struct node_interface *node_p)
{
	struct node_private *priv_p = (struct node_private *)node_p->n_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct node_segment *seg_p = NULL;
	struct node_segment *seg_p1 = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	list_for_each_entry_safe(seg_p, seg_p1, struct node_segment, &priv_p->p_seg_head, list) {
		list_del(&seg_p->list);
		if(allocator_p) {
			allocator_p->a_op->a_free(allocator_p, seg_p->a_seg_handle);
			allocator_p->a_op->a_free(allocator_p, seg_p->a_handle);
		} else {
			free(seg_p->segment);
			free(seg_p);
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	pthread_mutex_destroy(&priv_p->p_lck);
	free((void *)priv_p);
}

struct node_operations node_op = {
	.n_get_node = node_get_node,
	.n_put_node = node_put_node,
	.n_destroy = node_destroy,
};

int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
	char *log_header = "node_initialization";
	struct node_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	node_p->n_op = &node_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	node_p->n_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_a_set_id = setup->a_set_id;
	priv_p->p_alignment = setup->alignment;
	priv_p->p_node_size = setup->node_size;
	priv_p->p_seg_size = setup->seg_size;
	if (!priv_p->p_seg_size)
		priv_p->p_seg_size = NODE_SEGMENT_SIZE;
	INIT_LIST_HEAD(&priv_p->p_seg_head);
	INIT_LIST_HEAD(&priv_p->p_free_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	return 0;
}
