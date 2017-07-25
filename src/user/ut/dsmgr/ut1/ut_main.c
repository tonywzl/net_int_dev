#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "rtree_if.h"
#include "brn_if.h"
#include "blksn_if.h"
#include "dsbmp_if.h"
#include "dsmgr_if.h"
#include "bit_if.h"
#include "node_if.h"
#include "rtn_if.h"
#include "allocator_if.h"

static struct dsmgr_setup dsmgr_setup;
static struct dsmgr_interface ut_dsmgr;
static struct list_head list_used_blocks;
static struct brn_setup brn_setup;
static struct brn_interface ut_brn;


int
bit_initialization(struct bit_interface *bit_p, struct bit_setup *setup)
{
  assert(bit_p && setup);
  return 0;
}

void traverse_rtree()
{

  ut_dsmgr.dm_op->dm_display_space_nodes( &ut_dsmgr );

  /*dsmgr_traverse_rtree(&ut_dsmgr);
  struct dsmgr_private* priv_p = (struct dsmgr_private*)(ut_dsmgr.dm_private);
  struct rtree_interface* rtree_p = NULL;
  rtree_p = &(priv_p->p_sp_rtree);
  
  rtree_p->rt_op->rt_iter_start( rtree_p );
  struct rtree_node* np = NULL;
  do {
    np = (struct rtree_node*) rtree_p->rt_op->rt_iter_next( rtree_p );
    } while ( np != NULL ); */

  // print values of np

}

void print_num_free_blocks()
{
  uint64_t num_free_blocks = ut_dsmgr.dm_op->dm_get_num_free_blocks( &ut_dsmgr );
  nid_log_info( "the number of free blocks is %lu\n", num_free_blocks );

}


/*int
node_initialization(struct node_interface *node_p, struct node_setup *setup)
{
  assert(node_p && setup);
  return 0;
  } */


 /* int
rtn_initialization(struct rtn_interface *rtn_p, struct rtn_setup *setup)
{
  assert(rtn_p && setup);
  return 0;
  } */


/* int rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
	assert(rtree_p && setup);
	return 0;
	} */

 /* int dsbmp_initialization(struct dsbmp_interface *dsbmp_p, struct dsbmp_setup *setup)
{
	assert(dsbmp_p && setup);
	return 0;
	} */

  /* int brn_initialization(struct brn_interface *brn_p, struct brn_setup *setup)
{
	assert(brn_p && setup);
	return 0;
	} */

   /* int blksn_initialization(struct blksn_interface *blksn_p, struct blksn_setup *setup)
{
	assert(blksn_p && setup);
	return 0;
	} */


uint64_t index_key_1;
uint64_t index_key_2;
uint64_t index_key_3;

void get_and_verify_space_from_dsmgr()
{
  	struct list_head out_list1, out_list2, out_list3;
	// struct block_size_node *bn;

	INIT_LIST_HEAD( &out_list1 );
	INIT_LIST_HEAD( &out_list2 );
	INIT_LIST_HEAD( &out_list3 );


  	ut_dsmgr.dm_op->dm_check_space( &ut_dsmgr );

}

void get_initial_space_from_dsmgr()
{
  // struct list_head  out_list3;
	struct block_size_node *bn;

	// INIT_LIST_HEAD( &out_list1 );
	// INIT_LIST_HEAD( &out_list2 );
	// INIT_LIST_HEAD( &out_list3 );
	INIT_LIST_HEAD( &list_used_blocks );

	//
	// get a lot of space - all 100, 4k blocks
	//
	int i = 0;
	while ( i < 100 ) {
		uint64_t size;
		size = 1;
		struct list_head out_list1;
		INIT_LIST_HEAD( &out_list1 );
		ut_dsmgr.dm_op->dm_get_space( &ut_dsmgr, &size, &out_list1 );
		assert( 1 == size );

		list_for_each_entry( bn,  struct block_size_node, &out_list1, bsn_list ) {
		  	nid_log_debug( "i = %d, the size of the block is %lu, offset is %lu\n", i, bn->bsn_size, bn->bsn_index );
			// if ( (unsigned long)(&out_list1) == (unsigned long)bn->bsn_list ) break;
		}
		list_splice( &out_list1, &list_used_blocks );
		i ++;


	}

	//
	// get 100, 40k blocks
	//
	i = 0;
	while ( i < 100 ) {
		uint64_t size;
		size = 10;
		struct list_head out_list2;
		INIT_LIST_HEAD( &out_list2 );
		ut_dsmgr.dm_op->dm_get_space( &ut_dsmgr, &size, &out_list2 );
		assert( 10 == size );
		list_for_each_entry( bn,  struct block_size_node, &out_list2, bsn_list ) {
		  	nid_log_debug( "40k block, i = %d, the size of the block is %lu, offset is %lu\n", i, bn->bsn_size, bn->bsn_index );
		}
		i ++;
		list_splice( &out_list2, &list_used_blocks );

	}


	//
	// get 10, 400k blocks
	//
	i = 0;
	while ( i < 10 ) {
		uint64_t size;
		size = 100;
		struct list_head out_list3;
		INIT_LIST_HEAD( &out_list3 );
		ut_dsmgr.dm_op->dm_get_space( &ut_dsmgr, &size, &out_list3 );
		assert( 100 == size );
		list_for_each_entry( bn,  struct block_size_node, &out_list3, bsn_list ) {
		  	nid_log_debug( "400k block, i = %d, the size of the block is %lu, offset is %lu\n", i, bn->bsn_size, bn->bsn_index );
		}
		i ++;
		list_splice( &out_list3, &list_used_blocks );

	}

	nid_log_info( "listing all of the used blocks\n" );
	list_for_each_entry( bn,  struct block_size_node, &list_used_blocks, bsn_list ) {
	  	nid_log_info( "the size of the block is %lu, offset is %lu\n",  bn->bsn_size, bn->bsn_index );
	}

	//traverse_rtree();
}

#define BLOCK_SIZE 4096
#define UNIT_SIZE 2000
//
// looks like the size of the unit - should be 2000
//  - so should compute the index like that
//    offset / 2000
//
void add_space_to_dsmgr_crash()
{

	//np.bsn_size = 
  	// this will cause a crash- keep the function until fix the bug
  	uint64_t size1 = 2000;   
	uint64_t offset1 = 100200000;

	uint64_t size2 = 4000;
	uint64_t offset2 = 100204000;

	uint64_t size3 = 6000;
	uint64_t offset3 = 100208000;

	//	uint64_t index_key_1, index_key_2, index_key_3;

	index_key_1 = offset1 / UNIT_SIZE;
	index_key_2 = offset2 / UNIT_SIZE;
	index_key_3 = offset3 / UNIT_SIZE;



  	struct block_size_node* np = calloc( 1, sizeof( *np ) );
	np->bsn_size = size1;
	np->bsn_index = index_key_1;
  	ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, np);

	struct block_size_node* np2 = calloc( 1, sizeof( *np2 ) );
	np->bsn_size = size2;
	np->bsn_index = index_key_2;
  	ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, np2);

	struct block_size_node* np3 = calloc( 1, sizeof( *np3 ) );
	np->bsn_size = size3;
	np->bsn_index = index_key_3;
  	ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, np3);
}

void add_all_space_back_to_dsmgr()
{
  	struct block_size_node *bn, *temp_bn;
	nid_log_debug( "add_all_space_back_to_dsmgr()\n" );
	list_for_each_entry_safe( bn,  temp_bn, struct block_size_node, &list_used_blocks, bsn_list ) {
	  	nid_log_debug( "40k block,  the size of the block is %lu, offset is %lu\n",  bn->bsn_size, bn->bsn_index );
		list_del( &bn->bsn_list );
		ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, bn);
		traverse_rtree();
	}

}

#define NUM_BITMAPS 1000
#define BITMAP_SIZE 64

#define PATTERN1 972  // 00001111001100
#define PATTERN2 13107 // 0011001100110011
#define PATTERN3 3855 // 0000111100001111
#define PATTERN4 4080 // 0000111111110000
#define PATTERN5 0    // 0000000000000000
#define PATTERN6 1    // 0000000000000001
#define PATTERN7 64   // 0000000001000000
#define PATTERN8 255  // 0000000011111111

//
// 64000 blocks - 4k blocks
//
void initialize_dsmgr()
{
  	uint64_t ul_pattern1 = PATTERN1;
	uint64_t ul_pattern2 = PATTERN2;
  	uint64_t ul_pattern3 = PATTERN3;
	uint64_t ul_pattern4 = PATTERN4;
  	uint64_t ul_pattern5 = PATTERN5;
	// uint64_t ul_pattern6 = PATTERN6;
  	uint64_t ul_pattern7 = PATTERN7;
	uint64_t ul_pattern8 = PATTERN8;

	uint64_t* p_bitmap = calloc( NUM_BITMAPS, BITMAP_SIZE );

	uint64_t* p_bitmap_index = p_bitmap;
	struct blksn_setup blksn_setup;
	struct blksn_interface*  p_blksn = calloc(1, sizeof( struct blksn_interface ));

	struct allocator_setup alloc_setup;
	struct allocator_interface *p_alloc = calloc(1, sizeof(*p_alloc));

	int i = 0;
	while ( i < ( (NUM_BITMAPS ) / 8 ) ) {
		// use memset to create patterns in the bitmap
		memset( p_bitmap_index ++, ul_pattern1, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern2, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern3, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern4, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern5, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern5, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern7, sizeof( uint64_t ) );
		memset( p_bitmap_index ++, ul_pattern8, sizeof( uint64_t ) );

		i ++;
	}

	allocator_initialization(p_alloc, &alloc_setup);

	brn_setup.allocator = p_alloc;
	brn_setup.set_id = 1;
        brn_initialization(&ut_brn, &brn_setup);
        blksn_setup.allocator = p_alloc;
        blksn_setup.set_id = 2;
	blksn_initialization(p_blksn, &blksn_setup);

	dsmgr_setup.size = (1000 * 64 ) + 1;
	dsmgr_setup.bitmap = p_bitmap;
	dsmgr_setup.blksn = p_blksn;
	dsmgr_setup.brn = &ut_brn;
	dsmgr_setup.allocator = p_alloc;

	dsmgr_initialization(&ut_dsmgr, &dsmgr_setup);

}


int
main()
{
	nid_log_open();
	nid_log_info("dsmgr ut module start ...");
	//uint64_t* p_bitmap = calloc( 100000, 64 );

	initialize_dsmgr();
	print_num_free_blocks();

	get_initial_space_from_dsmgr();
	// add_space_to_dsmgr();
	print_num_free_blocks();

	// traverse_rtree();
	add_all_space_back_to_dsmgr();
	//get_and_verify_space_from_dsmgr();
	print_num_free_blocks();

	nid_log_info("dsmgr ut module end...");
	nid_log_close();

	return 0;
}
