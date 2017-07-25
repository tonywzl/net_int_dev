/*
 * allocator.c
 * 	Implementation of Allocator Module
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include "nid_log.h"
#include "list.h"
#include "lck_if.h"
#include "allocator_if.h"

struct allocator_handle {
	struct list_head	h_list;		// join the set list
	int			h_set_id;	// index of the set
	void			*h_data;	// real memory
	uint64_t		h_size;		// memory size test
};


struct allocator_private {
	int			p_role;
	int			p_set_max;
	struct list_head	p_set_heads[ALLOCATOR_SET_MAX];
	struct lck_interface	p_set_lck;
	struct lck_node		p_set_lnodes[ALLOCATOR_SET_MAX];
	uint64_t		p_size[ALLOCATOR_SET_MAX];
	uint64_t		p_getcounter[ALLOCATOR_SET_MAX];
	uint64_t		p_putcounter[ALLOCATOR_SET_MAX];
};

static void *
allocator_calloc(struct allocator_interface *allocator_p, int set_id, size_t nmemb, size_t size)
{
	char *log_header = "allocator_calloc";
	struct allocator_private *priv_p = (struct allocator_private *)allocator_p->a_private;
	struct lck_interface *lck_p = &priv_p->p_set_lck;
	struct allocator_handle *handle;

	assert(set_id < priv_p->p_set_max);
	handle = x_calloc(1, sizeof(*handle));
	if (!handle) {
		nid_log_error("%s: x_calloc failed.", log_header);
	}
	handle->h_set_id = set_id;
	handle->h_data = x_calloc(nmemb, size);
	assert(handle->h_data);
	handle->h_size = nmemb * size;
	__sync_add_and_fetch(&priv_p->p_size[set_id], handle->h_size);
       	lck_p->l_op->l_wlock(lck_p, &priv_p->p_set_lnodes[set_id]);
	priv_p->p_getcounter[set_id]++;
	list_add_tail(&handle->h_list, &priv_p->p_set_heads[set_id]);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_set_lnodes[set_id]);
	return handle;
}

static void
allocator_get(struct  allocator_interface *allocator_p)
{
	nid_log_debug("get_memory_start");
	struct allocator_private *priv_p = (struct allocator_private *)allocator_p->a_private;
	//struct lck_interface *lck_p = &priv_p->p_set_lck;
	struct allocator_handle *handle;
	int i;
	uint64_t mem_size;

	for ( i = 0 ; i < priv_p->p_set_max; i++){
		mem_size =0;
		list_for_each_entry( handle, struct allocator_handle, &priv_p->p_set_heads[i], h_list){
			if (handle != NULL){
				nid_log_debug("found a handle");
				mem_size += handle->h_size;
			}
		}
		if (mem_size != 0){
			nid_log_error("set_id = %d ",i);
			nid_log_error("memory_size = %lu ", mem_size);
		}
	}
}

static void *
allocator_malloc(struct allocator_interface *allocator_p, int set_id, size_t size)
{
	char *log_header = "allocator_malloc";
	struct allocator_private *priv_p = (struct allocator_private *)allocator_p->a_private;
	struct lck_interface *lck_p = &priv_p->p_set_lck;
	struct allocator_handle *handle;

	assert(set_id < priv_p->p_set_max);
	handle = x_calloc(1, sizeof(*handle));
	if (!handle) {
		nid_log_error("%s: x_calloc failed.", log_header);
	}
	handle->h_set_id = set_id;
	handle->h_data = x_malloc(size);
	assert(handle->h_data);
	handle->h_size = size;
	__sync_add_and_fetch(&priv_p->p_size[set_id], handle->h_size);
        lck_p->l_op->l_wlock(lck_p, &priv_p->p_set_lnodes[set_id]);
	priv_p->p_getcounter[set_id]++;
	list_add_tail(&handle->h_list, &priv_p->p_set_heads[set_id]);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_set_lnodes[set_id]);

	return handle;
}

static void *
allocator_memalign(struct allocator_interface *allocator_p, int set_id, size_t boundary, size_t size)
{
	char *log_header = "allocator_memalign";
	struct allocator_private *priv_p = (struct allocator_private *)allocator_p->a_private;
	struct lck_interface *lck_p = &priv_p->p_set_lck;
	struct allocator_handle *handle;

	assert(set_id < priv_p->p_set_max);
	handle = x_calloc(1, sizeof(*handle));
	if (!handle) {
		nid_log_error("%s: x_calloc failed.", log_header);
	}
	handle->h_set_id = set_id;
	if (x_posix_memalign((void **)&handle->h_data, boundary, size) !=0 ) {
		nid_log_error("%s: x_posix_memalign failed.", log_header);
	}
	assert(handle->h_data);
	if (size % boundary == 0){
		handle->h_size = size;
	} else {
		handle->h_size = boundary * ((size / boundary) + 1);
	}
	__sync_add_and_fetch(&priv_p->p_size[set_id], handle->h_size);
        lck_p->l_op->l_wlock(lck_p, &priv_p->p_set_lnodes[set_id]);
	priv_p->p_getcounter[set_id]++;
	list_add_tail(&handle->h_list, &priv_p->p_set_heads[set_id]);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_set_lnodes[set_id]);
	return handle;
}

static void
allocator_free(struct allocator_interface *allocator_p, void *handle)
{
	struct allocator_private *priv_p = (struct allocator_private *)allocator_p->a_private;
	struct lck_interface *lck_p = &priv_p->p_set_lck;
	struct allocator_handle *handle_p = (struct allocator_handle *)handle;
	int set_id = handle_p->h_set_id;

	lck_p->l_op->l_wlock(lck_p, &priv_p->p_set_lnodes[set_id]);
        priv_p->p_putcounter[set_id]++;
	list_del(&handle_p->h_list);
	lck_p->l_op->l_wunlock(lck_p, &priv_p->p_set_lnodes[set_id]);
	free(handle_p->h_data);
	__sync_sub_and_fetch(&priv_p->p_size[set_id], handle_p->h_size);
	free(handle_p);
}

static void *
allocator_get_data(struct allocator_interface *allocator_p, void *handle)
{
	struct allocator_private *priv_p = (struct allocator_private *)allocator_p->a_private;
	struct allocator_handle *handle_p = (struct allocator_handle *)handle;
	assert(!list_empty(&priv_p->p_set_heads[handle_p->h_set_id]));
	return handle_p->h_data;
}

static void
allocator_display(struct allocator_interface *allocator_p)
{
	struct allocator_private *priv_p = (struct allocator_private *) allocator_p->a_private;
        int i;
	char * showrole;

	if (priv_p->p_role == ALLOCATOR_ROLE_AGENT){
		showrole = "AGENT";
	} else if (priv_p->p_role == ALLOCATOR_ROLE_SERVER){
		showrole = "SERVER";
	}

	for (i = 1; i < priv_p->p_set_max; i++) {
		nid_log_warning("The size of %s set %d: %ld", showrole, i, priv_p->p_size[i]);
		nid_log_warning("The number of get in %s set %d: %ld", showrole, i, priv_p->p_getcounter[i]);
		nid_log_warning("The number of put in %s set %d: %ld", showrole, i, priv_p->p_putcounter[i]);
	}
}

struct allocator_operations allocator_op = {
	.a_calloc = allocator_calloc,
	.a_malloc = allocator_malloc,
	.a_memalign = allocator_memalign,
	.a_free = allocator_free,
	.a_get_data = allocator_get_data,
	.a_get_size = allocator_get,
	.a_display = allocator_display,
};

int
allocator_initialization(struct allocator_interface *allocator_p, struct allocator_setup *setup)
{
	char *log_header = "allocator_initialization";
	struct allocator_private *priv_p;
	struct lck_interface *lck_p;
	struct lck_setup lck_setup;
	int i;

	nid_log_warning("%s: start ...", log_header);
	assert(setup);
	allocator_p->a_op = &allocator_op;
	priv_p = x_calloc(1, sizeof(*priv_p));
	allocator_p->a_private = priv_p;

	priv_p->p_role = setup->a_role;
	lck_p = &priv_p->p_set_lck;
	lck_initialization(lck_p, &lck_setup);
	if (setup->a_role == ALLOCATOR_ROLE_SERVER) {
		priv_p->p_set_max = ALLOCATOR_SET_SERVER_MAX;
	} else if (setup->a_role == ALLOCATOR_ROLE_AGENT) {
		priv_p->p_set_max = ALLOCATOR_SET_AGENT_MAX;
	}

	for (i = 0; i < priv_p->p_set_max; i++) {
		INIT_LIST_HEAD(&priv_p->p_set_heads[i]);
		lck_node_init(&priv_p->p_set_lnodes[i]);
		priv_p->p_size[i] = 0;
		priv_p->p_getcounter[i] = 0;
		priv_p->p_putcounter[i] = 0;
	}

	return 0;
}
