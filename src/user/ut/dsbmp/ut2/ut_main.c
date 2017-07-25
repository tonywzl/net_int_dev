#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"

#include "rtree_if.h"
#include "dsbmp_if.h"
#include "blksn_if.h"
#include "node_if.h"
#include "brn_if.h"
#include "bit_if.h"
#include "dsmgr_if.h"
#include "btn_if.h"
#include "rtree_if.h"

#include "bit_if.h"
#include "ddn_if.h"
#include "rtn_if.h"

struct blksn_setup blksn_setup;
struct blksn_interface*  p_blksn;

// static struct dsmgr_setup dsmgr_setup;
static struct dsmgr_interface ut_dsmgr;
static struct btn_interface ut_btn;
static struct btn_setup ut_btn_setup;

static struct dsbmp_setup dsbmp_setup;
static struct dsbmp_interface ut_dsbmp;
static struct list_head test_list;

static struct rtree_setup rtree_setup;
static struct rtree_interface ut_rtree;

static struct rtree_interface *rtree_p_dsbmp;

static struct ddn_interface ut_ddn;
static struct ddn_setup ut_ddn_setup;

static struct brn_setup brn_setup;
static struct brn_interface brn; 


/*
 * unit test which allows a bitmap to be passed into dsbmp so an rtree could be 
 * created with a particular pattern.  Then verify the rtree has the correct nodes.
 * Then perform operations on the dsbmp - dsbmp_put_node(), dsbmp_get_node().
 * Then verify after all the nodes were put back, that all the offsets are the
 * same as at the beginning of the test.
 */



static void
_rtree_extend_cb(void *target_slot, void *parent)
{
  struct bridge_node *slot = (struct bridge_node *)target_slot;
  slot->bn_parent = parent;

}

static void *
_rtree_insert_cb(void *target_slot, void *new_node, void *parent)
{
  struct bridge_node *slot = (struct bridge_node *)target_slot;
  if (!slot) {
    slot = new_node;
    slot->bn_parent = parent;
  }
  struct block_size_node* bsn_np = (struct block_size_node*)(slot->bn_data);
	// debug
	nid_log_debug( "dsbmp-ut2__rtree_insert_cb() - insert node of offset %lu\n", bsn_np->bsn_index );

  return slot;

}

static void *
_rtree_remove_cb(void *target_slot, void *target_node)
{
	assert(target_slot == target_node);
	return NULL;
}

void traverse_rtree( struct rtree_interface* rtree_p )
{

  	rtree_p->rt_op->rt_iter_start( rtree_p );

  	struct bridge_node* bnp = NULL;
  	struct block_size_node* dsnp = NULL;
  	do {
    		bnp = (struct bridge_node*) rtree_p->rt_op->rt_iter_next( rtree_p );
    		if (bnp == NULL ) {
      			break;
    		}
    		dsnp = bnp->bn_data;
    		nid_log_debug( "block size node - index is %lu, size of the node is %lu\n", dsnp->bsn_index,  dsnp->bsn_size );
  	} while ( bnp != NULL );
  	rtree_p->rt_op->rt_iter_stop( rtree_p );

}

void initialize_rtree() 
{
  struct data_description_node *np; 

	rtree_p_dsbmp = &ut_rtree;
	static struct ddn_interface *ddn_p = &ut_ddn;
	//struct btn_node *slot;
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
	// rtree_p_dsbmp->rt_op->rt_insert(rtree_p_dsbmp, 0, np);

	struct rtree_stat* stat = calloc( 1, sizeof( stat ) );
	rtree_p_dsbmp->rt_op->rt_get_stat( rtree_p_dsbmp, stat );
	nid_log_debug( "rtree stat after np insert: nseg = %d,  segsz = %d, nfree = %d, nused = %d\n", 
		stat->s_rtn_nseg,
		stat->s_rtn_segsz,
		stat->s_rtn_nfree,
		stat->s_rtn_nused );

}


static void
dsmgr_add_new_node(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
  assert( dsmgr_p );
  assert( np );

  list_add_tail( &np->bsn_list, &test_list );

  // struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
  // __dsmgr_add_node(priv_p, np);
}

static void put_elements_into_rtree()
{
  struct block_size_node *cur_np;

  list_for_each_entry( cur_np,  struct block_size_node, &test_list, bsn_list ){
	nid_log_debug( "put_eirt() - cur_np - bsn_size = %lu, bsn_index = %lu\n", cur_np->bsn_size, cur_np->bsn_index );
	// if ( cur_np->bsn_index == 0 ) continue;
	struct bridge_node* slot_np;
	struct block_size_node* bsn_np;
	bsn_np = p_blksn->bsn_op->bsn_get_node( p_blksn );
	bsn_np->bsn_index = cur_np->bsn_index;
	bsn_np->bsn_size = cur_np->bsn_size;
	slot_np = brn.bn_op->bn_get_node( &brn );
	slot_np->bn_data = bsn_np;
	bsn_np->bsn_slot = (void*)slot_np;
	rtree_p_dsbmp->rt_op->rt_insert( rtree_p_dsbmp, bsn_np->bsn_index, slot_np );
  }


}


static void return_elements_to_dsbmp()
{
  struct block_size_node *cur_np;

  list_for_each_entry( cur_np,  struct block_size_node, &test_list, bsn_list ){
    nid_log_debug( "return_elements_to_dsbmp() - cur_np - bsn_size = %lu, bsn_index = %lu\n", cur_np->bsn_size, cur_np->bsn_index );

	INIT_LIST_HEAD( &cur_np->bsn_olist );
	ut_dsbmp.sb_op->sb_put_node( &ut_dsbmp, cur_np );
  }

  nid_log_debug( "after returning the elements - tree traversal for dsbmp\n" );
  traverse_rtree( rtree_p_dsbmp );

  unsigned long free_blocks = ut_dsbmp.sb_op->sb_get_free_blocks( &ut_dsbmp );
  nid_log_debug( "after returning elements - free blocks is %lu\n", free_blocks );


}

struct dsmgr_operations dsmgr_op = {
  	.dm_add_new_node = dsmgr_add_new_node,
};

int
bit_initialization(struct bit_interface *bit_p, struct bit_setup *setup)
{
  assert(bit_p && setup);
  return 0;
  } 

/* int
sdn_initialization(struct sdn_interface *sdn_p, struct sdn_setup *setup)
{
    assert(sdn_p && setup);
    return 0;
    } */

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
void initialize_bitmap( uint64_t* p_bitmap)
{
  	uint64_t ul_pattern1 = PATTERN1;
	uint64_t ul_pattern2 = PATTERN2;
  	uint64_t ul_pattern3 = PATTERN3;
	uint64_t ul_pattern4 = PATTERN4;
  	uint64_t ul_pattern5 = PATTERN5;
	// uint64_t ul_pattern6 = PATTERN6;
  	uint64_t ul_pattern7 = PATTERN7;
	uint64_t ul_pattern8 = PATTERN8;

        // p_bitmap = calloc( NUM_BITMAPS, BITMAP_SIZE );

	uint64_t* p_bitmap_index = p_bitmap;
	//	struct blksn_setup blksn_setup;
	// struct blksn_interface*  p_blksn = calloc(1, sizeof( struct blksn_interface ));

	int i = 0;
	while ( i < ( (NUM_BITMAPS ) / 8 ) ) {
		// use memset to create patterns in the bitmap
	  	*p_bitmap_index  = ul_pattern1;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern2;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern3;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern4;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern5;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern5;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern7;
		p_bitmap_index ++;
	  	*p_bitmap_index  = ul_pattern8;
		p_bitmap_index ++;


		//memset( p_bitmap_index ++, ul_pattern1, sizeof( uint64_t ) );
		// memset( p_bitmap_index ++, ul_pattern2, sizeof( uint64_t ) );
		//	memset( p_bitmap_index ++, ul_pattern3, sizeof( uint64_t ) );
		// memset( p_bitmap_index ++, ul_pattern4, sizeof( uint64_t ) );
		// memset( p_bitmap_index ++, ul_pattern5, sizeof( uint64_t ) );
		// memset( p_bitmap_index ++, ul_pattern5, sizeof( uint64_t ) );
		// memset( p_bitmap_index ++, ul_pattern7, sizeof( uint64_t ) );
		// memset( p_bitmap_index ++, ul_pattern8, sizeof( uint64_t ) );

		i ++;
	}
	// return p_bitmap;
}

struct dsbmp_private {
  struct allocator_interface      *p_allocator;
  struct dsmgr_interface          *p_dsmgr;
  struct brn_interface            *p_brn;
  struct blksn_interface          *p_blksn;
  uint64_t                        p_size;         // in number of uint64_t
  uint64_t                        *p_bitmap;
  uint64_t                        p_free;
  struct rtree_interface          p_rtree;
  struct list_head                p_space_head;   // search list in space order
  uint64_t                        p_num_free_blocks;
};



void check_rtree()
{
  struct dsbmp_private *priv_p = (struct dsbmp_private *)(ut_dsbmp.sb_private);
  struct rtree_interface *rtree_p = &priv_p->p_rtree;


  traverse_rtree( rtree_p );

}

void compare_dsbmp_rtree_w_list()
{
  struct dsbmp_private *priv_p = (struct dsbmp_private *)(ut_dsbmp.sb_private);
  struct rtree_interface *rtree_p = &priv_p->p_rtree;

  struct block_size_node* cur_np;

  list_for_each_entry( cur_np,  struct block_size_node, &test_list, bsn_list ){
	void* pVal = rtree_p->rt_op->rt_search( rtree_p_dsbmp, cur_np->bsn_index );
	if ( ! pVal ) {
	  nid_log_debug( "after returning nodes() - error did not findd element of offset %lu\n", cur_np->bsn_index );
	  exit( 1 );
	} else {
	  nid_log_debug( "after returning nodes() - found element of offset %lu\n", cur_np->bsn_index );
	}

  }




}


// get nodes from dsbmp and put them into  a linked list
// have to make sure it knows the correct offset
void get_nodes()
{
  // struct list_head node_list;
  struct block_size_node* cur_np; //,*temp_np ;
  // struct dsbmp_private * priv_p  = (struct dsbmp_private*)ut_dsbmp.sb_private;

  uint64_t free_blocks = ut_dsbmp.sb_op->sb_get_free_blocks( &ut_dsbmp );

  nid_log_debug( "before get_nodes, free_blocks is %lu\n", free_blocks );

  traverse_rtree( rtree_p_dsbmp );
  list_for_each_entry( cur_np,  struct block_size_node, &test_list, bsn_list ){
    // ut_dsbmp.sb_op->sb_get_node( &ut_dsbmp, cur_np );
	void* pVal = rtree_p_dsbmp->rt_op->rt_search( rtree_p_dsbmp, cur_np->bsn_index );
	if ( ! pVal ) {
	  nid_log_debug( "get_nodes() - error did not findd element of offset %lu\n", cur_np->bsn_index );
	  exit( 1 );
	} else {
	  nid_log_debug( "before get_nodes() - found element of offset %lu\n", cur_np->bsn_index );
	}

  }
  list_for_each_entry( cur_np,  struct block_size_node, &test_list, bsn_list ){
     ut_dsbmp.sb_op->sb_get_node( &ut_dsbmp, cur_np );

  }
  nid_log_debug( "after getting the nodes - tree traversal\n" );
  traverse_rtree( rtree_p_dsbmp );
  list_for_each_entry( cur_np,  struct block_size_node, &test_list, bsn_list ){
	void* pVal = rtree_p_dsbmp->rt_op->rt_search( rtree_p_dsbmp, cur_np->bsn_index );
	if ( ! pVal ) {
	  nid_log_debug( "after get_nodes() - error did not findd element of offset %lu\n", cur_np->bsn_index );
	  exit( 1 );
	} else {
	  nid_log_debug( "after get_nodes() - found element of offset %lu\n", cur_np->bsn_index );
	}

  }

  free_blocks = ut_dsbmp.sb_op->sb_get_free_blocks( &ut_dsbmp );

  nid_log_debug( "after get_nodes, free_blocks is %lu\n", free_blocks );

}

int
main()
{
 //   int i;
   // uint64_t bmp_size = 5*64*4096*sizeof(uint64_t);

  	INIT_LIST_HEAD( &test_list );
	p_blksn = calloc(1, sizeof( struct blksn_interface ));

  	uint64_t* p_bitmap = NULL;
	p_bitmap = calloc( NUM_BITMAPS, BITMAP_SIZE );
      	initialize_bitmap( p_bitmap );

        blksn_initialization(p_blksn, &blksn_setup);
	brn_initialization( &brn, &brn_setup );
	ut_dsmgr.dm_op = &dsmgr_op;

	dsbmp_setup.size = NUM_BITMAPS * 1;

	dsbmp_setup.bitmap = p_bitmap;
    
	dsbmp_setup.blksn = p_blksn;
	dsbmp_setup.brn = &brn;
	dsbmp_setup.dsmgr = &ut_dsmgr;
	nid_log_open();
	nid_log_info("dsbmp ut module start ...");

	initialize_rtree();
	dsbmp_initialization(&ut_dsbmp, &dsbmp_setup);

	check_rtree();

	put_elements_into_rtree();
	get_nodes();

	return_elements_to_dsbmp();
	compare_dsbmp_rtree_w_list();

	nid_log_info("dsbmp ut module end...");
	nid_log_close();

	return 0;
}
