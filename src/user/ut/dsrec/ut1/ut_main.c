#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "dsbmp_if.h"
#include "rtree_if.h"
#include "dsrec_if.h"
#include "cdn_if.h"
#include "blksn_if.h"
#include "rc_if.h"
#include "fpc_if.h"

#define DSMGR_LARGE_SPACE	4096	// more than 4096 is belong large space

static struct dsrec_setup dsrec_setup;
static struct dsrec_interface ut_dsrec;
static struct cdn_setup cdn_setup;
static struct cdn_interface ut_cdn;
static struct fpc_setup fpc_setup;
static struct fpc_interface ut_fpc;

static struct list_head dsmgr_list_head;



static void
rc_drop_block(struct rc_interface *rc_p, void *block_to_drop)
{
  assert( rc_p );
  assert( block_to_drop );
}

static void*
rc_create_channel(struct rc_interface *rc_p, void *owner, struct rc_channel_info *rc_info, char *res_p, int *new)
{
  long* val = calloc( 1, sizeof( long ) ) ;
  assert( rc_p );
  assert( owner );
  assert( rc_info);
  assert( res_p );
  assert( new );
  return (void*)val;
}

static void*
rc_prepare_channel(struct rc_interface *rc_p, struct rc_channel_info *rc_info, char *res_p)
{
  long* val = calloc( 1, sizeof( long ) ) ;
  assert( rc_info );
  assert( rc_p );
  assert( res_p );
  return (void*)val;
}


static void 
rc_search(struct rc_interface *rc_p, void *rc_handle, struct sub_request_node *rn_p, struct list_head *not_found_head)
{
  	assert( rc_p );
 	assert( rc_handle );
  	assert( rn_p );
  	assert( not_found_head );
}

static void 
rc_updatev(struct rc_interface *rc_p, void *rc_handle, struct iovec *iov, int iov_counter, off_t offset, struct list_head *ag_fp_head)
{
  assert( rc_p );
  assert( rc_handle );
  assert( iov );
  assert( iov_counter );
  assert( offset );
  assert( ag_fp_head );
}


static void
rc_update (struct rc_interface *rc_p, void *rc_handle, void *data, ssize_t size, off_t offset, struct list_head *ag_fp_head) 
{
  assert( rc_p );
  assert( rc_handle );
  assert( data );
  assert( size );
  assert( offset );
  assert( ag_fp_head );

}


static ssize_t
rc_setup_cache_mdata(struct rc_interface *rc_p, struct list_head *fp_head, struct list_head *space_head)
{
  assert( rc_p );
  assert( fp_head );
  assert( space_head );
  return sizeof( char );
}

static ssize_t
rc_setup_cache_data(struct rc_interface *rc_p, char *p, struct list_head *space_head)
{
  assert( rc_p );
  assert( p );
  assert( space_head );
  return sizeof( char );
}

static ssize_t
rc_setup_cache_bmp(struct rc_interface *rc_p, struct list_head *space_head)
{
  assert( rc_p );
  assert( space_head );
  return sizeof( char);
}

static void
rc_flush_content(struct rc_interface *rc_p, struct list_head *flush_head,
	char *data_p, uint64_t flush_size, struct list_head *space_head)
{
  assert( rc_p );
  assert( flush_head );
  assert( data_p );
  assert( flush_size );
  assert( space_head );
}

static void
rc_touch_block(struct rc_interface *rc_p, void *old_block, void *new_blcok)
{
  assert( rc_p && old_block && new_blcok );
}

static void
rc_insert_block(struct rc_interface *rc_p, void *block_to_insert)
{
  assert( rc_p && block_to_insert );
}

static int
rc_read_block(struct rc_interface *rc_p, char *buf, uint64_t block_index)
{
  assert( rc_p || buf || block_index );
  return 0;
}

static char * 
rc_get_uuid(struct rc_interface *rc_p)
{
  char* c = calloc( 1, sizeof( char ) );
  assert( rc_p );
  return c;
}

static int 
rc_dropcache_start(struct rc_interface *rc_p, char *chan_uuid, int do_sync)
{
  assert( rc_p || chan_uuid || do_sync );
  return 0;
}

static int 
rc_dropcache_stop(struct rc_interface *rc_p, char *chan_uuid )
{
  assert( rc_p || chan_uuid );
  return 0;
}



struct rc_operations rc_op = {
	.rc_create_channel = rc_create_channel,
	.rc_prepare_channel = rc_prepare_channel,
	.rc_search = rc_search,
	.rc_updatev = rc_updatev,
	.rc_update = rc_update,
	.rc_setup_cache_data = rc_setup_cache_data,
	.rc_setup_cache_mdata = rc_setup_cache_mdata,
	.rc_setup_cache_bmp = rc_setup_cache_bmp,
	.rc_flush_content = rc_flush_content,
	.rc_touch_block = rc_touch_block,
	.rc_insert_block = rc_insert_block,
	.rc_read_block = rc_read_block,
	.rc_drop_block = rc_drop_block,
	.rc_get_uuid = rc_get_uuid,
	.rc_dropcache_start = rc_dropcache_start,
	.rc_dropcache_stop = rc_dropcache_stop,
};


struct rc_operations rc_op1 = {
	.rc_drop_block = rc_drop_block,
};


static struct rc_interface* put_rc;

struct dsmgr_space_node {
	struct list_head	sn_list;
	uint64_t		sn_size;
	void			*sn_slot;				
};

struct dsmgr_private {
	pthread_mutex_t			p_lck;
	struct dsmgr_space_node		p_sp_nodes[DSMGR_LARGE_SPACE];
	struct allocator_interface	*p_allocator;
	struct brn_interface		*p_brn;
	struct blksn_interface		*p_blksn;
	struct dsbmp_interface		p_dsbmp;
	struct rtree_interface		p_sp_rtree;			// space pool tree
	struct list_head		p_large_sp_head;		// large space pool, in space order
	struct list_head		p_sp_heads[DSMGR_LARGE_SPACE];	// space pool, in space order
	struct list_head		p_space_head;
	int				p_size;				// number of blocks of the whole space
	int				p_block_shift;
};


/*
 *  For testing purpose, this will store it in a linked list to compare later
 *
 *
 */
static void
dsmgr_put_space(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
  // list_del( &np->bsn_list );
  	list_add( &np->bsn_list, &dsmgr_list_head );
  	nid_log_info( "dsmgr_put_space() - added element with index = %lu, with dsmgr = %lu",
		(unsigned long)( np->bsn_index),
		(unsigned long)dsmgr_p );
}


static int
dsmgr_get_space(struct dsmgr_interface *dsmgr_p, uint64_t *size, struct list_head *out_head)
{
  assert( dsmgr_p && size && out_head );
  return 0;
}

static void
dsmgr_extend_space(struct dsmgr_interface *dsmgr_p, struct block_size_node *np, uint64_t ext_size) 
{
  assert( dsmgr_p && np && ext_size );
}


static void
dsmgr_add_new_node(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
  assert( dsmgr_p && np  );
}

static void
dsmgr_delete_node(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
  assert( dsmgr_p && np );
}

static int
dsmgr_get_page_shift(struct dsmgr_interface *dsmgr_p)
{
  assert( dsmgr_p );
  return 0;
}

static void dsmgr_display_space_nodes( struct dsmgr_interface* dsmgr_p ) 
{
  assert( dsmgr_p );
}

static struct list_head* dsmgr_check_space( struct dsmgr_interface* dsmgr_p )
{
  assert( dsmgr_p );
  return NULL;
}

static uint64_t dsmgr_get_num_free_blocks( struct dsmgr_interface *dsmgr_p)
{
  assert( dsmgr_p );
  return 0;
}

struct dsmgr_operations {
	int	(*dm_get_space)(struct dsmgr_interface *, uint64_t *size, struct list_head *);
	void	(*dm_put_space)(struct dsmgr_interface *, struct block_size_node *);
	void	(*dm_extend_space)(struct dsmgr_interface *, struct block_size_node *, uint64_t);
	void	(*dm_add_new_node)(struct dsmgr_interface *, struct block_size_node *);
	void	(*dm_delete_node)(struct dsmgr_interface *, struct block_size_node *);
	int	(*dm_get_page_shift)(struct dsmgr_interface *);
	void    (*dm_display_space_nodes)( struct dsmgr_interface* dsmgr_p );
	struct list_head*    (*dm_check_space)( struct dsmgr_interface* dsmgr_p );
	uint64_t  (*dm_get_num_free_blocks)( struct dsmgr_interface* dsmgr_p );
};




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
};

struct dsmgr_interface {
	void			*dm_private;
	struct dsmgr_operations	*dm_op;
};

struct dsmgr_interface ut_dsmgr;


int
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	assert(rtree_p && setup);
	return 0;
}

int
main()
{

	nid_log_open();
	nid_log_info("dsrec ut module start ...");

	INIT_LIST_HEAD( &dsmgr_list_head );
	ut_dsmgr.dm_op = &dsmgr_op;
	cdn_setup.fp_size = 0;
	cdn_setup.allocator = NULL;
	cdn_setup.set_id = 0;
	cdn_setup.seg_size = 0;
	cdn_initialization( &ut_cdn, &cdn_setup );

	fpc_setup.fpc_algrm = FPC_SHA224;
	fpc_initialization(&ut_fpc, &fpc_setup);

	// rc_initialization( &ut_rc, &rc_setup );

	struct rc_operations* rc_pop = (struct rc_operations*)calloc( 1, sizeof( struct rc_operations ) );
	*rc_pop = rc_op1;

	put_rc = (struct rc_interface*)calloc( 1, sizeof( struct rc_interface ) );
	put_rc->rc_op = rc_pop;

	struct blksn_setup blksn_setup;
	struct blksn_interface*  p_blksn = calloc(1, sizeof( struct blksn_interface ));
	blksn_setup.allocator = NULL;
	blksn_initialization(p_blksn, &blksn_setup);

	dsrec_setup.cdn = &ut_cdn;
	dsrec_setup.rc_size = 1000;
	dsrec_setup.rc = put_rc;
	dsrec_setup.blksn = p_blksn;
	dsrec_setup.dsmgr = &ut_dsmgr;
	dsrec_setup.fpc = &ut_fpc;
	dsrec_initialization(&ut_dsrec, &dsrec_setup);

	int i = 0;
	while  ( i < 10000 ) {
	  	usleep(  2000 );
  		struct content_description_node* cd_np;
		cd_np = ut_cdn.cn_op->cn_get_node(&ut_cdn);
		cd_np->cn_data = (unsigned long)i;
		cd_np->cn_is_ondisk = 1;
		ut_dsrec.sr_op->sr_insert_block( &ut_dsrec, cd_np ); 
		i++;
	}
	sleep( 25 );
	nid_log_info("dsrec ut module end...");
	nid_log_close();

	return 0;
}
