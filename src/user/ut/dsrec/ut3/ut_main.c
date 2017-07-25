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
#include "dsmgr_if.h"
#include "brn_if.h"
#include "fpc_if.h"
#include "allocator_if.h"

#define DSMGR_LARGE_SPACE	4096	// more than 4096 is belong large space

#define LRU_MAX_INDEX 9300
#define RC_SIZE 1000
#define TEST_POOL_SIZE 10000


// test simulating a crc_flush_content() function

static struct dsmgr_setup dsmgr_setup;
static struct dsrec_setup dsrec_setup;
static struct dsrec_interface ut_dsrec;
static struct cdn_setup cdn_setup;
static struct cdn_interface ut_cdn;
static struct brn_setup brn_setup;
static struct brn_interface ut_brn;
static struct fpc_setup fpc_setup;
static struct fpc_interface ut_fpc;
static struct allocator_setup alloc_setup;
static struct allocator_interface ut_alloc;



static struct list_head dsmgr_list_head;
static struct list_head list_nodes_to_reclaim;
static struct list_head list_to_reclaim;

static int max_reclaim_index;
static int out_of_space;

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
rc_touch_block(struct rc_interface *rc_p, void *old_block, void *new_block)
{
  assert( rc_p && old_block && new_block );
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


struct dsmgr_interface ut_dsmgr;




int compare_reclaim_space_lists(  )
{
	struct block_size_node *np1, *np2;
	struct list_head* list_marker_2;
	int first_element_list_2 = 1;
	int num_matches = 0;
  	list_for_each_entry( np1, struct block_size_node, &dsmgr_list_head   , bsn_list ) {
	  if ( first_element_list_2 ) {
	    	list_marker_2 = &list_nodes_to_reclaim;
		first_element_list_2 = 0;
	  } else {
	    	list_marker_2 = &np2->bsn_list;
	  }
		
	  np2 = list_first_entry( list_marker_2, struct block_size_node, bsn_list );
	  if  ( ( np2->bsn_size == np1->bsn_size ) && (np2->bsn_index == np1->bsn_index ) ) {
	    	num_matches++;
	    	nid_log_debug( "match on node of size %lu and index %lu, num_matches = %d\n", np2->bsn_size, np2->bsn_index, num_matches );
	    	printf( "match on node of size %lu and index %lu, num_matches = %d\n", np2->bsn_size, np2->bsn_index, num_matches );
	    //list_del( &np2->bsn_list );
	  } else if ( (unsigned long)(&np2->bsn_list) != (unsigned long)(&list_nodes_to_reclaim) ) {
	    	nid_log_debug(  " no match on node of size %lu and index %lu\n", np2->bsn_size, np2->bsn_index );
	    	assert( 0 );
	  }
  	}
	assert ( num_matches >= max_reclaim_index );

	return 1;
}


void get_some_space( uint64_t max_get_size )
{

	long int get_factor = RAND_MAX / max_get_size;
	uint64_t size_needed = random(  ) / get_factor;
	struct list_head space_head;

	INIT_LIST_HEAD( &space_head );
	struct block_size_node* bk_np, *bk_np_temp;


	uint64_t flush_size = size_needed;
	uint64_t page_expecting = 0;
  	while ((page_expecting = flush_size)) {
    		ut_dsmgr.dm_op->dm_get_space(&ut_dsmgr, &page_expecting, &space_head);
    		assert(flush_size >= page_expecting);
    		flush_size -= page_expecting;
  	}

	list_for_each_entry_safe(bk_np, bk_np_temp, struct block_size_node, &space_head, bsn_list) {
	  	// add the space to the list of space nodes
	  	list_del( &bk_np->bsn_list );
	  	list_add_tail( &bk_np->bsn_list, &list_to_reclaim );
		nid_log_debug( "getting space of size = %lu", bk_np->bsn_size );
	}
	nid_log_debug( "leaving get_some_space()" );
	if ( flush_size != 0 ) {
	  out_of_space = 1;
	}
}

void put_exact_space_to_dsrec( uint64_t size_needed )
{
  	struct block_size_node *bn, *temp_bn;
	nid_log_debug( "put_exact_space_to_dsmgr()\n" );
	// uint64_t size;
	uint64_t orig_size_needed = size_needed;
	uint64_t size_retrieved = 0;
	nid_log_debug( "put_space_to_dsmgr() - size_needed = %lu\n", (unsigned long)size_needed );

	int iteration = 0;
	int found_block = 0;

	list_for_each_entry_safe( bn,  temp_bn, struct block_size_node, &list_to_reclaim, bsn_list ) {
	  	nid_log_debug( "the size of the block is %lu, offset is %lu, iteration = %d\n",  bn->bsn_size, bn->bsn_index, iteration );
		if ( ( size_needed > 0 ) && ( ! found_block ) ) {
	  		if ( size_needed < bn->bsn_size ) {
			  	nid_log_debug( "put_exact_space_to_dsrec() - putting  block size is %lu, offset is %lu\n",  bn->bsn_size, bn->bsn_index );
				list_del( &bn->bsn_list );
				uint64_t total_size_insert = 0;
				while ( total_size_insert < bn->bsn_size ) {
  					struct content_description_node* cd_np;
					cd_np = ut_cdn.cn_op->cn_get_node(&ut_cdn);
					cd_np->cn_data = (unsigned long)bn->bsn_index + total_size_insert;
					cd_np->cn_is_ondisk = 1;
					ut_dsrec.sr_op->sr_insert_block( &ut_dsrec, cd_np ); 
					total_size_insert ++;
				}

				size_retrieved += bn->bsn_size;
				if ( size_retrieved > size_needed ) {
				  	size_needed = 0;
				} else {
					size_needed -= size_retrieved;
				}
				assert( size_needed < orig_size_needed );
				found_block = 1;
	  		}
		} else { 
		  	break;
		}
	}
	
	/*
	 * Did not find a block with greater size, iterate through list backwards
	 * to find blocks of smaller size and then put them until size requirement is
	 * fulfilled.
	 */
	if ( ! found_block ) {
	  	list_for_each_entry_safe_reverse( bn,  struct block_size_node, temp_bn, struct block_size_node, &list_to_reclaim, bsn_list ) {
		  	if ( ( size_needed > 0 ) && ( ! found_block ) ) {
			  nid_log_debug( "put_space_to_dsmgr() - putting  block size is %lu, offset is %lu, size_needed = %lu\n",  bn->bsn_size, bn->bsn_index, size_needed );

				list_del( &bn->bsn_list );
				uint64_t total_size_insert = 0;
				while ( total_size_insert < bn->bsn_size ) {
  					struct content_description_node* cd_np;
					cd_np = ut_cdn.cn_op->cn_get_node(&ut_cdn);
					cd_np->cn_data = (unsigned long)bn->bsn_index + total_size_insert;
					cd_np->cn_is_ondisk = 1;
					ut_dsrec.sr_op->sr_insert_block( &ut_dsrec, cd_np ); 
					total_size_insert ++;
				}

				size_retrieved += bn->bsn_size;
				if ( size_needed < size_retrieved ) {
					  size_needed = 0;
				} else {
					size_needed -= size_retrieved;
				}

			} else {
			  	break;
			}
		}
	}
	if ( size_needed > 0 ) {
	  	nid_log_info( "unable to put total size needed, %lu left\n", size_needed );
	}
	if ( out_of_space ) {
	  out_of_space = 0;
	}

}

void insert_some_blocks(  uint64_t max_put_size )
{
  	long int put_factor = RAND_MAX / max_put_size;
  	uint64_t size_needed = random() / put_factor;

	put_exact_space_to_dsrec( size_needed );
}

#define MAX_GET_SIZE 5000
#define MAX_PUT_SIZE 3000

#define PATTERN_ZEROES  0 // 0000000000000000
#define PATTERN_SEPARATOR 1 // 0000000000000001

#define LARGE_CHUNK_SIZE    160000
#define LARGE_CHUNKS_NUMBER   40

#define BITMAP_SIZE 64


void initialize_large_zeroed_bitmap( uint64_t** p_bitmap )
{

  	*p_bitmap = calloc( LARGE_CHUNKS_NUMBER * LARGE_CHUNK_SIZE , BITMAP_SIZE );

	uint64_t* p_bitmap_index = *p_bitmap;

	uint64_t ul_pattern_zeroes = PATTERN_ZEROES;
	int i = 0, j = 0;
	while ( i < LARGE_CHUNKS_NUMBER ) {
	  	while ( j < LARGE_CHUNK_SIZE ) {
		  	memset( p_bitmap_index, ul_pattern_zeroes, sizeof( uint64_t ) );
			j++;
			p_bitmap_index ++;
		}
		// memset( p_bitmap_index ++, PATTERN_SEPARATOR, sizeof( uint64_t ) );
		i ++;
	}


}

void initialize_dsmgr(struct blksn_setup *blksn_setup)
{

  	uint64_t* p_bitmap = NULL;

	dsmgr_setup.size = ( LARGE_CHUNKS_NUMBER * LARGE_CHUNK_SIZE  * BITMAP_SIZE ) + 1;
	initialize_large_zeroed_bitmap( &p_bitmap );

	struct blksn_interface*  p_blksn = calloc(1, sizeof( struct blksn_interface ));
        brn_initialization(&ut_brn, &brn_setup);


	blksn_initialization(p_blksn, blksn_setup);

	dsmgr_setup.bitmap = p_bitmap;
	dsmgr_setup.blksn = p_blksn;
	dsmgr_setup.brn = &ut_brn;
	dsmgr_setup.allocator = &ut_alloc;

	dsmgr_initialization(&ut_dsmgr, &dsmgr_setup);

}

int
main()
{

	nid_log_open();
	nid_log_info("dsrec ut module start ...");

	INIT_LIST_HEAD( &dsmgr_list_head );
	INIT_LIST_HEAD( &list_nodes_to_reclaim );
	INIT_LIST_HEAD( &list_to_reclaim);
	//	ut_dsmgr.dm_op = &dsmgr_op;
	allocator_initialization(&ut_alloc, &alloc_setup);

	cdn_setup.fp_size = 0;
	cdn_setup.allocator = &ut_alloc;
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
	blksn_setup.allocator = &ut_alloc;
	blksn_initialization(p_blksn, &blksn_setup);

	initialize_dsmgr(&blksn_setup);

	dsrec_setup.cdn = &ut_cdn;
	dsrec_setup.rc_size = RC_SIZE;
	dsrec_setup.rc = put_rc;
	dsrec_setup.blksn = p_blksn;
	dsrec_setup.dsmgr = &ut_dsmgr;
	dsrec_setup.fpc = &ut_fpc;
	dsrec_initialization(&ut_dsrec, &dsrec_setup);

	int lru_max = ( RC_SIZE * 7 ) / 10;
	max_reclaim_index = TEST_POOL_SIZE - lru_max;
	out_of_space = 0;
	int i = 0;
	while  ( i < TEST_POOL_SIZE ) {
	  	get_some_space( MAX_GET_SIZE );
	  	usleep(  20 );
		insert_some_blocks( MAX_PUT_SIZE );
	  	usleep(  20 );
		i++;
	}
	sleep( 25 );
	//	compare_reclaim_space_lists();
	// nid_log_info("compare_reclaim_space_lists() succeeded");
	nid_log_info("dsrec ut module end...");
	nid_log_close();

	return 0;
}
