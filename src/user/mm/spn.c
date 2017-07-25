/*
 * spn.c
 * 	Implementation of Space Node Module
 *	Space Node Module allocates space in a large piece, preferred with alignment.
 */

#include <stdlib.h>
#include <pthread.h>
#include "nid_log.h"
#include "allocator_if.h"
#include "spn_if.h"

#define SPN_SEGMENT_SIZE	128


struct spn_segment {
	void			*a_handle;
	void			*a_spn_header_handle;
	void			*a_segment_handle;
	uint32_t		nfree;
	struct list_head	list;
	struct spn_header	*spn_headers;
	void			*segment;
};

struct spn_private {
	struct allocator_interface	*p_allocator;
	int				p_a_set_id;
	struct list_head		p_seg_head;
	struct list_head		p_free_head;
	uint32_t			p_nseg;
	uint32_t			p_nfree;
	pthread_mutex_t			p_lck;
	uint32_t			p_spn_size;
	uint32_t			p_seg_size;
	size_t				p_alignment;
};

static void
__spn_extend(struct spn_interface *spn_p)
{
	char *log_header = "__spn_extend";
	struct spn_private *priv_p = (struct spn_private *)spn_p->sn_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	int a_set_id = priv_p->p_a_set_id;
	uint32_t i, seg_size = priv_p->p_seg_size;
	struct spn_segment *seg_p;
	struct spn_header *snhp;
	void *a_handle;
	void *sn_data;

	if (allocator_p) {
		a_handle = allocator_p->a_op->a_calloc(allocator_p, a_set_id, 1, sizeof(*seg_p));
		seg_p = (struct spn_segment *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
		seg_p->a_handle = a_handle;

		if (!priv_p->p_alignment)
			a_handle = allocator_p->a_op->a_malloc(allocator_p, a_set_id,
				seg_size * sizeof(*snhp));
		else
			a_handle = allocator_p->a_op->a_memalign(allocator_p, a_set_id,
				priv_p->p_alignment, seg_size * priv_p->p_spn_size);
		seg_p->spn_headers = (struct spn_header *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
		seg_p->a_spn_header_handle = a_handle;

		if (!priv_p->p_alignment)
			a_handle = allocator_p->a_op->a_malloc(allocator_p, a_set_id,
				seg_size * priv_p->p_spn_size);
		else
			a_handle = allocator_p->a_op->a_memalign(allocator_p, a_set_id,
				priv_p->p_alignment, seg_size * priv_p->p_spn_size);
		seg_p->segment = (void *)allocator_p->a_op->a_get_data(allocator_p, a_handle);
		seg_p->a_segment_handle = a_handle;
	} else {
		seg_p = calloc(1, sizeof(*seg_p));
		if (!priv_p->p_alignment) {
			seg_p->spn_headers = malloc(seg_size * sizeof(*snhp));
			seg_p->segment = malloc(seg_size * priv_p->p_spn_size);
		} else {
			posix_memalign((void **)&seg_p->spn_headers, priv_p->p_alignment,
				seg_size * sizeof(*snhp));
			posix_memalign((void **)&seg_p->segment, priv_p->p_alignment,
				seg_size * priv_p->p_spn_size);
		}
	}
	nid_log_debug("%s: p_nfree:%u, p_nseg:%u", log_header, priv_p->p_nfree, priv_p->p_nseg);
	seg_p->nfree = priv_p->p_seg_size;
	for (i = 0, snhp = &seg_p->spn_headers[0], sn_data = seg_p->segment; i < seg_size; i++ ) {
		list_add(&snhp->sn_list, &priv_p->p_free_head);
		snhp->sn_owner = (void *)seg_p;
		snhp->sn_data = sn_data;
		sn_data = (void *)((char *)sn_data + priv_p->p_spn_size);
		snhp = &seg_p->spn_headers[i+1];
	}

	priv_p->p_nfree += seg_size;
	list_add(&seg_p->list, &priv_p->p_seg_head);
	priv_p->p_nseg++;
}

static struct spn_header *
spn_get_node(struct spn_interface *spn_p)
{
	struct spn_private *priv_p = (struct spn_private *)spn_p->sn_private;
	struct spn_segment *seg_p;
	struct spn_header *snhp = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	if (list_empty(&priv_p->p_free_head)) {
		__spn_extend(spn_p);
	}
	snhp = list_first_entry(&priv_p->p_free_head, struct spn_header, sn_list);
	list_del(&snhp->sn_list);
	priv_p->p_nfree--;
	seg_p = snhp->sn_owner;
	seg_p->nfree--;
	pthread_mutex_unlock(&priv_p->p_lck);
	return snhp;
}

/*
 * Notice:
 * 	the caller should hold p_lck
 */
static void
__spn_shrink(struct spn_interface *spn_p, struct spn_segment *seg_p)
{
	char *log_header = "__spn_shrink";
	struct spn_private *priv_p = (struct spn_private *)spn_p->sn_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	uint32_t i, seg_size = priv_p->p_seg_size;
	struct spn_header *snhp;

	nid_log_debug("%s: p_nfree:%u, p_nseg:%u", log_header, priv_p->p_nfree, priv_p->p_nseg);
	assert(seg_p->nfree == seg_size);
	for (i = 0, snhp = &seg_p->spn_headers[0]; i < seg_size; i++) {
		list_del(&snhp->sn_list);
		snhp = &seg_p->spn_headers[i+1];
	}
	priv_p->p_nfree -= seg_size;
	list_del(&seg_p->list);
	priv_p->p_nseg--;
	if (allocator_p) {
		allocator_p->a_op->a_free(allocator_p, seg_p->a_segment_handle);
		allocator_p->a_op->a_free(allocator_p, seg_p->a_spn_header_handle);
		allocator_p->a_op->a_free(allocator_p, seg_p->a_handle);
	} else {
		free(seg_p->segment);
		free(seg_p->spn_headers);
		free(seg_p);
	}
}

static void
spn_put_node(struct spn_interface *spn_p, struct spn_header *snhp)
{
	struct spn_private *priv_p = (struct spn_private *)spn_p->sn_private;
	struct spn_segment *seg_p = (struct spn_segment *)snhp->sn_owner;

	pthread_mutex_lock(&priv_p->p_lck);
	list_add(&snhp->sn_list, &priv_p->p_free_head);
	priv_p->p_nfree++;
	seg_p->nfree++;
	nid_log_debug("spn_put_node: put one spn");
	if ((seg_p->nfree == priv_p->p_seg_size) && (priv_p->p_nfree >= priv_p->p_seg_size * 2)) {
		__spn_shrink(spn_p, seg_p);
	}
	pthread_mutex_unlock(&priv_p->p_lck);
}

static void
spn_destroy(struct spn_interface *spn_p)
{
	struct spn_private *priv_p = (struct spn_private *)spn_p->sn_private;
	struct allocator_interface *allocator_p = priv_p->p_allocator;
	struct spn_segment *seg_p = NULL;
	struct spn_segment *seg_p1 = NULL;

	pthread_mutex_lock(&priv_p->p_lck);
	if (allocator_p) {
		list_for_each_entry_safe(seg_p, seg_p1, struct spn_segment, &priv_p->p_seg_head, list) {
			list_del(&seg_p->list);
			allocator_p->a_op->a_free(allocator_p, seg_p->a_segment_handle);
			allocator_p->a_op->a_free(allocator_p, seg_p->a_spn_header_handle);
			allocator_p->a_op->a_free(allocator_p, seg_p->a_handle);
		}
	} else {
		list_for_each_entry_safe(seg_p, seg_p1, struct spn_segment, &priv_p->p_seg_head, list) {
			list_del(&seg_p->list);
			free(seg_p->segment);
			free(seg_p->spn_headers);
			free(seg_p);
		}
	}
	pthread_mutex_unlock(&priv_p->p_lck);
	pthread_mutex_destroy(&priv_p->p_lck);
	free((void *)priv_p);
}

struct spn_operations spn_op = {
	.sn_get_node = spn_get_node,
	.sn_put_node = spn_put_node,
	.sn_destroy = spn_destroy,
};

int
spn_initialization(struct spn_interface *spn_p, struct spn_setup *setup)
{
	char *log_header = "spn_initialization";
	struct spn_private *priv_p;

	nid_log_info("%s: start ...", log_header);
	assert(setup);
	spn_p->sn_op = &spn_op;
	priv_p = calloc(1, sizeof(*priv_p));
	spn_p->sn_private = priv_p;

	priv_p->p_allocator = setup->allocator;
	priv_p->p_a_set_id = setup->a_set_id;
	priv_p->p_alignment = setup->alignment;
	priv_p->p_spn_size = setup->spn_size;
	priv_p->p_seg_size = setup->seg_size;
	if (!priv_p->p_seg_size)
		priv_p->p_seg_size = SPN_SEGMENT_SIZE;
	INIT_LIST_HEAD(&priv_p->p_seg_head);
	INIT_LIST_HEAD(&priv_p->p_free_head);
	pthread_mutex_init(&priv_p->p_lck, NULL);
	return 0;
}

