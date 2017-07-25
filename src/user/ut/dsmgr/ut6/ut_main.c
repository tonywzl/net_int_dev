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
#include "util_list.h"
#include "umpk_crc_if.h"
#include "allocator_if.h"

static struct dsmgr_setup dsmgr_setup;
static struct dsmgr_interface ut_dsmgr;
static struct list_head list_used_blocks_offset;
static struct list_head list_used_blocks_size;

static struct brn_setup brn_setup;
static struct brn_interface ut_brn;

static unsigned int rand_seed;

static int out_of_space = 0;

static uint64_t total_size_retrieved = 0;

/*
 *
 * dsmgr-dsbmp stress test
 *    - specifically gets a large pool of space initially
 *    - then does hte following:
 *   random - get 1000  (this means random number of times, randomly around 1000 bytes) 
 *   random - put 1000  (this means random number of times, randomly around 1000 bytes) 
 *   random - get 1000  (this means random number of times, randomly around 1000 bytes) 
 *   random - put 1000  (this means random number of times, randomly around 1000 bytes) 
 *    ... and so on - indefinitely
 *
 *     make sure that it does not get more than the space that is available
 *     - if it is about to get too much space, put alot of the space back (reclaiming)
 *
 *
 */

#define DSMGR_LARGE_SPACE       4096    // more than 4096 is belong large space

struct dsmgr_space_node {
  	struct list_head        sn_list;
  	uint64_t                sn_size;
  	void                    *sn_slot;
};

struct dsmgr_private {
  	struct dsmgr_space_node         p_sp_nodes[DSMGR_LARGE_SPACE];
  	struct allocator_interface      *p_allocator;
  	struct brn_interface            *p_brn;
  	struct blksn_interface          *p_blksn;
  	struct dsbmp_interface          p_dsbmp;
  	struct rtree_interface          p_sp_rtree;                     // space pool tree
  	struct list_head                p_large_sp_head;                // large space pool, in space order
  	struct list_head                p_sp_heads[DSMGR_LARGE_SPACE];  // space pool, in space order
  	struct list_head                p_space_head;
  	int                             p_size;                         // number of blocks of the whole space
  	int                             p_block_shift;
};



int
bit_initialization(struct bit_interface *bit_p, struct bit_setup *setup)
{
  assert(bit_p && setup);
  return 0;
}

void traverse_rtree()
{

  ut_dsmgr.dm_op->dm_display_space_nodes( &ut_dsmgr );

}




uint64_t get_num_free_blocks()
{
  	uint64_t num_free_blocks = ut_dsmgr.dm_op->dm_get_num_free_blocks( &ut_dsmgr );
  	return num_free_blocks;

}

void print_num_free_blocks()
{
  	uint64_t num_free_blocks = get_num_free_blocks( );
  	nid_log_info( "the number of free blocks is %lu\n", num_free_blocks );

}


struct umessage_crc_information_resp_freespace_dist* get_and_verify_space_from_dsmgr()
{
  	struct list_head out_list1, out_list2, out_list3;
	struct space_size_number *cur_np;

	struct list_head* list_dsmgr_space_size_ptr = NULL;
	struct umessage_crc_information_resp_freespace_dist* dist_result = NULL;
	INIT_LIST_HEAD( &out_list1 );
	INIT_LIST_HEAD( &out_list2 );
	INIT_LIST_HEAD( &out_list3 );

  	dist_result = ut_dsmgr.dm_op->dm_check_space( &ut_dsmgr );
	if ( dist_result ) {
	  	list_dsmgr_space_size_ptr = &(dist_result->um_size_list_head);
	} else {
	  	return NULL;
	}

	list_for_each_entry( cur_np, struct space_size_number, list_dsmgr_space_size_ptr, um_ss_list ) {
	  	nid_log_debug( "dsmgr_unit - get_and_verify..() size = %lu, number = %lu\n", cur_np->um_size, cur_np->um_number );

	}

	return dist_result;
	
}


void initialize_stress_test()
{
	INIT_LIST_HEAD( &list_used_blocks_offset );
	INIT_LIST_HEAD( &list_used_blocks_size );
	//	INIT_LIST_HEAD( list_dsmgr_space_size_ptr );
	srandom( rand_seed );
}


void add_all_space_back_to_dsmgr()
{
  	struct block_size_node *bn, *temp_bn;
	nid_log_debug( "add_all_space_back_to_dsmgr()\n" );
	int iteration = 0;
	list_for_each_entry_safe( bn,  temp_bn, struct block_size_node, &list_used_blocks_size, bsn_list ) {
	  nid_log_debug( "40k block,  the size of the block is %lu, offset is %lu, iteration = %d\n",  bn->bsn_size, bn->bsn_index, iteration );
		list_del( &bn->bsn_list );
		list_del( &bn->bsn_olist );
		ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, bn);
		iteration ++;
	}

	//traverse_rtree();
	// ut_dsmgr.dm_op->dm_check_space( &ut_dsmgr );

}


void put_exact_space_to_dsmgr( uint64_t size_needed )
{
  	struct block_size_node *bn, *temp_bn;
	nid_log_debug( "put_exact_space_to_dsmgr()\n" );
	// uint64_t size;
	uint64_t orig_size_needed = size_needed;
	uint64_t size_retrieved = 0;
	nid_log_debug( "put_space_to_dsmgr() - size_needed = %lu\n", (unsigned long)size_needed );

	int iteration = 0;
	int found_block = 0;

	list_for_each_entry_safe( bn,  temp_bn, struct block_size_node, &list_used_blocks_size, bsn_list ) {
	  	nid_log_debug( "the size of the block is %lu, offset is %lu, iteration = %d\n",  bn->bsn_size, bn->bsn_index, iteration );
		if ( ( size_needed > 0 ) && ( ! found_block ) ) {
	  		if ( size_needed < bn->bsn_size ) {
			  	nid_log_debug( "put_space_to_dsmgr() - putting  block size is %lu, offset is %lu\n",  bn->bsn_size, bn->bsn_index );
				list_del( &bn->bsn_list );
				list_del( &bn->bsn_olist );
				ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, bn);
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
	  	list_for_each_entry_safe_reverse( bn,  struct block_size_node, temp_bn, struct block_size_node, &list_used_blocks_size, bsn_list ) {
		  	if ( ( size_needed > 0 ) && ( ! found_block ) ) {
			  nid_log_debug( "put_space_to_dsmgr() - putting  block size is %lu, offset is %lu, size_needed = %lu\n",  bn->bsn_size, bn->bsn_index, size_needed );

				list_del( &bn->bsn_list );
				list_del( &bn->bsn_olist );
				ut_dsmgr.dm_op->dm_put_space( &ut_dsmgr, bn);
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


/*
 * puts in size or larger space back to the dsmgr
 */
void put_random_space_to_dsmgr( uint64_t max_put_size )
{
  // struct block_size_node *bn; //, *temp_bn;
	nid_log_debug( "put_space_to_dsmgr()\n" );
	// uint64_t size;
	long int put_factor = RAND_MAX / max_put_size;
	uint64_t size_needed = random(  ) / put_factor;

	nid_log_debug( "put_random_space_to_dsmgr() - size_needed = %lu\n", (unsigned long)size_needed );
	put_exact_space_to_dsmgr( size_needed );

}

void get_exact_space_from_dsmgr( uint64_t size )
{
  	struct block_size_node* bn, *bn2, *bn3, *temp_bn, *temp_bn2, *temp_bn3;
	uint64_t orig_size;
	orig_size = size;
	nid_log_debug( "get_space_from_dsmgr - size is %lu\n", (unsigned long) size );
	struct list_head out_list1;
	INIT_LIST_HEAD( &out_list1 );
	ut_dsmgr.dm_op->dm_get_space( &ut_dsmgr, &size, &out_list1 );
	// assert( 1 == size );

	/* list_for_each_entry( bn,  struct block_size_node, &out_list1, bsn_list ) {
	  	printf( "the size of the block is %lu, offset is %lu\n",  bn->bsn_size, bn->bsn_index );
		} */


	// insert the elements of the list in-order into the size list and offset list
	list_for_each_entry_safe( bn, temp_bn, struct block_size_node, &out_list1, bsn_list ) {
	  	int size_list_added = 0;
		struct list_head* list_size_2_prev_marker = NULL;
		uint64_t prev_size_index = 0;

	  	list_for_each_entry_safe( bn2, temp_bn2, struct block_size_node, &list_used_blocks_size, bsn_list ) {
		  	if ( bn2->bsn_size >= bn->bsn_size ) {
			  if ( bn->bsn_size == 2686 ) {
			    nid_log_debug( "get_space_from_dsmgr() - deleting element of size 2686 from the list\n" );
			  }
			  list_del( &bn->bsn_list );
			  if ( list_size_2_prev_marker ) {
				    nid_log_debug( "add to size list  new block size is %lu, offset is %lu, adding before block w/size %lu, after block w/size %lu\n",  bn->bsn_size, bn->bsn_index, bn2->bsn_size, prev_size_index );
				//list_del( &bn->bsn_list );
			  	list_add( &bn->bsn_list, list_size_2_prev_marker );
				    total_size_retrieved += bn->bsn_size;
				size_list_added = 1;
			  	break;
			  } else { // need to add to beginning of the list
				nid_log_debug( "add to beginning of size list  new block size is %lu, offset is %lu, adding before block w/index %lu\n",  bn->bsn_size, bn->bsn_index, bn2->bsn_index );
			    	list_add_tail( &bn->bsn_list, &bn2->bsn_list );
				total_size_retrieved += bn->bsn_size;
				size_list_added = 1;
				break;
			  }
		  	} else {
			  	list_size_2_prev_marker  = &bn2->bsn_list;
				prev_size_index = bn2->bsn_size;
			}

	  	}
		if ( ! size_list_added ) {
			  if ( bn->bsn_size == 2686 ) {
			    nid_log_debug( "get_space_from_dsmgr() - deleting element of size 2686 from the list\n" );
			  }

			list_del( &bn->bsn_list );
			list_add( &bn->bsn_list, &bn2->bsn_list );
			nid_log_debug( "add to size list - last -  block size is %lu, offset is %lu\n",  bn->bsn_size, bn->bsn_index );
		}

		int offset_list_added = 0;
		// insert into the offset list
		struct list_head* list_offset_2_prev_marker = NULL;
		uint64_t prev_offset_index = 0;
	  	list_for_each_entry_safe( bn3, temp_bn3, struct block_size_node, &list_used_blocks_offset, bsn_olist ) {
			  if ( ( list_used_blocks_offset.next == NULL ) || ( list_used_blocks_offset.next->next == NULL ) ) {
			  	nid_log_error( "list_used_blocks_offset has a NULL for one of its pointers\n" );
				assert( 0 );
		  	}
		  	if ( bn3->bsn_index > bn->bsn_index ) {
			  	if ( ( bn->bsn_olist.next != NULL ) && ( bn->bsn_olist.prev != NULL ) ) {
			    		assert( 0 );
			  		list_del( &bn->bsn_olist );
		  		}

			  	if ( list_offset_2_prev_marker ) {
			  		nid_log_debug( "add to offset list  new block size is %lu, offset is %lu, adding after block w/index %lu\n",  bn->bsn_size, bn->bsn_index, prev_offset_index );
			  		list_add( &bn->bsn_olist, list_offset_2_prev_marker );
					offset_list_added = 1;
			  		break;
			  	} else { // need to add to beginning of the list
					nid_log_debug( "add to beginning of offset list  new block size is %lu, offset is %lu, adding before block w/index %lu\n",  bn->bsn_size, bn->bsn_index, bn3->bsn_index );
			    		list_add_tail( &bn->bsn_olist, &bn3->bsn_olist );
					offset_list_added = 1;
					break;
			  	}
		  	} else {
			  	if ( ( &bn3->bsn_olist == &list_used_blocks_offset ) || ( bn3->bsn_olist.next == &list_used_blocks_offset ) ) {
				  //assert( 0 );
			    		break;
			  	}
			  	list_offset_2_prev_marker  = &bn3->bsn_olist;
				prev_offset_index = bn3->bsn_index;
			}
	  	}
		if ( ! offset_list_added )  {
		  	if ( ( bn->bsn_olist.next != NULL ) && ( bn->bsn_olist.prev != NULL ) ) {
		    		assert( 0 );
		  		list_del( &bn->bsn_olist );
		  	}
			list_add_tail( &bn->bsn_olist, &list_used_blocks_offset );
			nid_log_debug( "add to offset list - last-  block size is %lu, offset is %lu - prev index = %lu\n",  bn->bsn_size, bn->bsn_index, prev_offset_index );
		}
	}

	if ( size < orig_size ) {
	  nid_log_warning( "get_exact_space_from_dsmgr() - out of space, size = %lu, original size = %lu\n", size, orig_size );
	  out_of_space = 1;
	} 

	nid_log_debug( "get_exact_space_from_dsmgr() - total_size_retrieved = %lu\n", total_size_retrieved );
}

void get_random_space_from_dsmgr( uint64_t max_get_size )
{
	uint64_t size;
	long int get_factor = RAND_MAX / max_get_size;
	size = random(  ) / get_factor;

	nid_log_debug( "get_random_space_from_dsmgr - size is %lu\n", (unsigned long) size );
	get_exact_space_from_dsmgr( size );

}


void free_dsmgr_list_memory( struct list_head* dsmgr_list )
{
  	struct space_size_number* bn, *temp_bn;
  	list_for_each_entry_safe( bn, temp_bn, struct space_size_number, dsmgr_list, um_ss_list ) {
	  	list_del( &bn->um_ss_list );
	  	free(bn);
	}
	//free( dsmgr_list );  - TBD - memory leak - need to free the containing pointer 

}

void free_rtree_memory( struct list_head* rtree_list )
{
  	struct block_size_node* bn;
  	list_for_each_entry( bn, struct block_size_node, rtree_list, bsn_olist ) {
	  	free( bn);
	}
	free( rtree_list );

}

struct list_head* traverse_dsbmp_rtree()
{
  	struct list_head* rtree_list = NULL;

	rtree_list = ut_dsmgr.dm_op->dm_traverse_dsbmp_rtree( &ut_dsmgr );
	return rtree_list;

}


int contrast_dsbmp_rtree_lists( struct list_head* list_1, struct list_head* list_2 ) 
{
	struct block_size_node *np1, *np2;
	struct list_head* list_2_marker;
	// int first_element_list_2 = 1;
	int list1_behind_list2  = 0;
	int num_np2_comparisons = 0;
	nid_log_debug ( "contrast_dsbmp_rtree_lists\n" );

	np2 = list_first_entry( list_2, struct block_size_node, bsn_olist );
	list_2_marker = &np2->bsn_olist;

  	list_for_each_entry( np1, struct block_size_node, list_1   , bsn_olist ) {

	  if ( list1_behind_list2 ) {
	    // do not advance list2 marker - because need to catch up
	    if ( np2->bsn_index < np1->bsn_index ) {
	      // need to iterate to next entry of list2 - no match - unset list1_behind_list2
	      nid_log_debug( "list2 has index - %lu - which did not match in list1\n", np2->bsn_index );
	      list1_behind_list2 = 0;
	    } else if ( np2->bsn_index == np1->bsn_index ) {
	      // there is a match - this is bad - print error and exit
	      nid_log_debug( "contrast_dsbmp_rtree_lists() - match found with bsn_index %lu\n", np2->bsn_index );
	    } else {  // np2->bsn_index > np1->bsn_index  - no match yet - need to iterate to 
	      // next entry of np1 until it passes np2 index
	      // list1 still behind - keep set list1_behind_list2
	    }
	  }
	  if ( ! list1_behind_list2 ) {
	    num_np2_comparisons ++;
	    struct list_head* list_2_marker_temp = NULL;
	    list_for_each_entry( np2, struct block_size_node, list_2_marker, bsn_olist ) {
	      list_2_marker_temp = &np2->bsn_olist;
	      if ( np2->bsn_index < np1->bsn_index ) {
		// need to iterate to next entry of list_2 - no match 
	      	nid_log_debug( "list2 has index - %lu - which did not match in list1\n", np2->bsn_index );
	      } else if ( np2->bsn_index == np1->bsn_index ) {
		// there is a match, print error and exit - we don't want a match here
		nid_log_error( "found  a match at offset %lu - exitting\n", np2->bsn_index );
		exit( 1 );
	      } else { // np2->bsn_index > np1->bsn_index - no match yet - need to iterate to next
		// entry of np1 until it passes np2 index
		list1_behind_list2 = 1;
		break;
	      }
	    }
	    	if ( &np2->bsn_olist == list_2_marker ) {
	  		nid_log_debug( "iterated through list2\n" );
		} else {
		  list_2_marker = list_2_marker_temp;
		}

	  }
	  nid_log_debug( "moving past list1 entry - that does not match with index: %lu\n", np1->bsn_index );
	}
	if ( &np1->bsn_olist == list_1 ) {
	  nid_log_debug( "iterated through list1\n" );
	  if (num_np2_comparisons < 3 ) {
	    nid_log_debug( "iterate through list 2\n" );
	    list_for_each_entry( np2, struct block_size_node, list_2, bsn_olist ) {
	      nid_log_debug( "list 2 entry has index %lu\n", np2->bsn_index );
	    }
	  }
	}
	nid_log_info ( "no matches\n" );
	return 0;
}

/*
 *   Assume the offsets are in order in the space list returned 
 *
 */

int contrast_offsets_of_space_lists( struct list_head* list_1, uint64_t size, uint64_t offset ) 
{
  	struct block_size_node* cur_np = NULL;

	nid_log_debug( "contrast_offsets_of_space_lists() - size = %lu,offset = %lu\n", size, offset );
  	list_for_each_entry( cur_np, struct block_size_node, list_1, bsn_list ) {
	  nid_log_debug( "contrast_offsets..() - cur_np - offset = %lu, size = %lu\n", cur_np->bsn_index, cur_np->bsn_size );
	  	if ( ( cur_np->bsn_size == size ) && ( cur_np->bsn_index <= offset ) ) {
		  	if ( cur_np->bsn_index == offset ) { // assume offsets are sorted so no need to go through
		    		nid_log_error( "match found on size and offset of np2 and np1 - offset = %lu, size = %lu\n", cur_np->bsn_index, cur_np->bsn_size );
		    		exit( 1 );
		  	} else {
		    		nid_log_debug( "contrast_offsets..() - moving past list_1 element - of offset %lu, size %lu\n", cur_np->bsn_size, cur_np->bsn_index );
		  	}
		} else {
		  // no match for this size
		  break;
		}
	}
	return 0;
}


int contrast_dsmgr_space_lists( struct list_head* list_1, struct list_head* list_2 ) 
{
	struct block_size_node *np1, *np2;
	struct list_head* list_2_marker = NULL;
	// int first_element_list_2 = 1;
	int list1_behind_list2  = 0;
	int num_np2_comparisons = 0;
	nid_log_debug ( "contrast_dsmgr_space_lists\n" );

	//	np2 = list_first_entry( list_2, struct block_size_node, bsn_list );
	// list_2_marker = &np2->bsn_list;

  	list_for_each_entry( np1, struct block_size_node, list_1   , bsn_list ) {

	  nid_log_debug( "contrast_dsmgr_space - np1 -next entry - size = %lu, offset = %lu\n", np1->bsn_size, np1->bsn_index );
	  	if ( list1_behind_list2 ) {
	    		// do not advance list2 marker - because need to catch up
	    		if ( np2->bsn_size < np1->bsn_size ) {
	      			// need to iterate to next entry of list2 - no match - unset list1_behind_list2
	      			nid_log_debug( "list2 has size - %lu - which did not match in list1\n", np2->bsn_size );
	      			list1_behind_list2 = 0;
	    		} else if ( np2->bsn_size == np1->bsn_size ) {
	      			// there is a match - on the size - need to check index
	      			nid_log_debug( "contrast_dsbmp_rtree_lists() - match found with bsn_size %lu, check offsets\n", np2->bsn_size );
				if ( np2->bsn_index == np1->bsn_index ) {
				  	nid_log_error( "match found on size and offset of np2 and np1 - offset = %lu, size = %lu\n", np2->bsn_index, np2->bsn_size );
					exit( 0 );
				} else {
				  	contrast_offsets_of_space_lists( &np1->bsn_list, np2->bsn_size, np2->bsn_index );
					list1_behind_list2 = 0;
				}
	    		} else {  // np2->bsn_size > np1->bsn_size  - no match yet - need to iterate to 
	      			// next entry of np1 until it passes np2 index
	      			// list1 still behind - keep set list1_behind_list2
	    		}
	  	}
	  	if ( ! list1_behind_list2 ) {
		  	if ( ! list_2_marker ) {
		    		list_2_marker = list_2;
		  	}
	    		num_np2_comparisons ++;
	    		struct list_head* list_2_marker_temp = NULL;
	    		list_for_each_entry( np2, struct block_size_node, list_2_marker, bsn_list ) {
	      			list_2_marker_temp = &np2->bsn_list;
				nid_log_debug( "contrast_dsmgr_space - np2 -next entry - size = %lu, offset = %lu\n", np2->bsn_size, np2->bsn_index );
	      			if ( np2->bsn_size < np1->bsn_size ) {
					// need to iterate to next entry of list_2 - no match 
	      				nid_log_debug( "list2 has index - %lu - which did not match in list1\n", np2->bsn_index );
	      			} else if ( np2->bsn_size == np1->bsn_size ) {
					// there is a match, print error and exit - we don't want a match here
					nid_log_debug( "found  a match at size %lu - checking indexes and of other entries\n", np2->bsn_size );
					if ( np2->bsn_index == np1->bsn_index ) {
					  	nid_log_debug( "match found on size and offset of np2 and np1 - offset = %lu, size = %lu\n", np2->bsn_index, np2->bsn_size );
						exit( 0 );
					} else {
					  	contrast_offsets_of_space_lists( &np1->bsn_list, np2->bsn_size, np2->bsn_index );
					}

	      			} else { // np2->bsn_size > np1->bsn_size - no match yet - need to iterate to next
					// entry of np1 until it passes np2 index
					list1_behind_list2 = 1;
					break;
	      			}
			}
	    		if ( &np2->bsn_list == list_2 ) {
	  			nid_log_debug( "iterated through list2\n" );
				list_2_marker = NULL;
				break;
			} else {
		  		list_2_marker = list_2_marker_temp;
			}

	  	}
	  	nid_log_debug( "moving past list1 entry w/size %lu - that does not match list2 entry with size: %lu\n", np1->bsn_size, np2->bsn_size );
	}
	if ( &np1->bsn_list == list_1 ) {
	  	nid_log_debug( "iterated through list1\n" );
	  	if (num_np2_comparisons < 3 ) {
	    		nid_log_debug( "iterate through list 2\n" );
	    		list_for_each_entry( np2, struct block_size_node, list_2, bsn_list ) {
	      			nid_log_debug( "list 2 entry has size %lu, index %lu\n", np2->bsn_size, np2->bsn_index );
	    		}
	  	}
	}
	nid_log_info ( "no matches\n" );
	return 0;
}






// assume the lists are already in order
int compare_dsbmp_rtree_lists( struct list_head* list_1, struct list_head* list_2 )
{
	struct block_size_node *np1, *np2;
	struct list_head* list_marker_2;
	int first_element_list_2 = 1;
  	list_for_each_entry( np1, struct block_size_node, list_1   , bsn_olist ) {
	  if ( first_element_list_2 ) {
	    	list_marker_2 = list_2;
		first_element_list_2 = 0;
	  } else {
	    	list_marker_2 = &np2->bsn_olist;
	  }
		
	  np2 = list_first_entry( list_marker_2, struct block_size_node, bsn_olist );
	  if  ( ( np2->bsn_size == np1->bsn_size ) && (np2->bsn_index == np1->bsn_index ) ) {
	    nid_log_debug( "match on node of size %lu and index %lu\n", np2->bsn_size, np2->bsn_index );
	  } else {
	    nid_log_debug(  " no match on node of size %lu and index %lu\n", np2->bsn_size, np2->bsn_index );
	    return 0;
	  }
  	}

	return 1;
}


void get_small_chunks_add_big_chunks( uint64_t put_max_size, uint64_t get_max_size  )
{
  	int i = 0;

  	struct list_head* rtree_list_before, *rtree_list_after_1, * rtree_list_after_2, *rtree_list_after_3;
	// struct list_head* dsmgr_space_list = NULL;

	rtree_list_before = traverse_dsbmp_rtree();
  	for ( i = 0; i < 1000; i ++ ) {
    		get_random_space_from_dsmgr( get_max_size );
  	}


  	rtree_list_after_1 = traverse_dsbmp_rtree();

	contrast_dsbmp_rtree_lists( &list_used_blocks_offset, rtree_list_after_1 );

	for ( i = 0; i < 100; i ++ ) {
	  	put_random_space_to_dsmgr( put_max_size );
	}

  	rtree_list_after_2 = traverse_dsbmp_rtree();

	contrast_dsbmp_rtree_lists( &list_used_blocks_offset, rtree_list_after_2 );

  	add_all_space_back_to_dsmgr();

  	rtree_list_after_3 = traverse_dsbmp_rtree();

	compare_dsbmp_rtree_lists( rtree_list_before, rtree_list_after_3 );

 	free_rtree_memory( rtree_list_before );
	free_rtree_memory( rtree_list_after_1 );
	free_rtree_memory( rtree_list_after_2 );
	free_rtree_memory( rtree_list_after_3 );

}

void get_big_chunks_add_small_chunks( uint64_t put_max_size, uint64_t get_max_size  )
{
  	int i = 0;

	nid_log_debug( "get_big_chunks_add_small_chunks()\n" );

  	struct list_head* rtree_list_before, *rtree_list_after_1, * rtree_list_after_2, *rtree_list_after_3;
	// struct list_head* dsmgr_space_list = NULL;

	rtree_list_before = traverse_dsbmp_rtree();
  	for ( i = 0; i < 100; i ++ ) {
    		get_random_space_from_dsmgr( get_max_size );
  	}


  	rtree_list_after_1 = traverse_dsbmp_rtree();

	contrast_dsbmp_rtree_lists( &list_used_blocks_offset, rtree_list_after_1 );

	for ( i = 0; i < 100; i ++ ) {
	  	put_random_space_to_dsmgr( put_max_size );
	}

  	rtree_list_after_2 = traverse_dsbmp_rtree();

	contrast_dsbmp_rtree_lists( &list_used_blocks_offset, rtree_list_after_2 );

  	add_all_space_back_to_dsmgr();

  	rtree_list_after_3 = traverse_dsbmp_rtree();

	compare_dsbmp_rtree_lists( rtree_list_before, rtree_list_after_3 );

 	free_rtree_memory( rtree_list_before );
	free_rtree_memory( rtree_list_after_1 );
	free_rtree_memory( rtree_list_after_2 );
	free_rtree_memory( rtree_list_after_3 );
	//	exit( 0 );

}

#define POOL_SIZE_RANGE 90000
#define POOL_SIZE_BASE 10000

#define RANDOM_GET_RANGE 9000
#define RANDOM_PUT_RANGE 9000

#define GET_SIZE_BASE 1000
#define PUT_SIZE_BASE 1000

void run_stress_test( /*uint64_t put_max_size, uint64_t get_max_size */ )
{
  //int i = 0;
  	struct list_head* rtree_list_before;

	// get baseline of the rtree
	rtree_list_before = traverse_dsbmp_rtree();


	// initialize the test- get a pool of somewhere between 10,000 or 100,000 blocks
	long int pool_size_factor = RAND_MAX / POOL_SIZE_RANGE;
	int pool_size_to_add = random() / pool_size_factor;
	int pool_size = POOL_SIZE_BASE + pool_size_to_add;

	//	int blocks_taken = 0;
	get_exact_space_from_dsmgr( pool_size );

	uint64_t num_iterations = 0;
	while ( 1 ) {
		  int put_size_factor = RAND_MAX / RANDOM_PUT_RANGE;
		  int put_size_to_add = random() / put_size_factor;
		  int put_size = PUT_SIZE_BASE + put_size_to_add;
		  put_exact_space_to_dsmgr( put_size );

		  uint64_t num_free_blocks = get_num_free_blocks();

		  if ( num_free_blocks > ( GET_SIZE_BASE + RANDOM_GET_RANGE ) ) {

		  	int get_size_factor = RAND_MAX / RANDOM_GET_RANGE;
		  	int get_size_to_add = random() / get_size_factor;
		  	int get_size = GET_SIZE_BASE + get_size_to_add;
		  	get_exact_space_from_dsmgr( get_size );
		  }
		  num_iterations ++;

		  if ( num_iterations % 10 == 0 ) {
		    	// do rtree comparison
		    	struct list_head* rtree_list_after = traverse_dsbmp_rtree();
			contrast_dsbmp_rtree_lists( &list_used_blocks_offset, rtree_list_after );

			free_rtree_memory( rtree_list_after );

			struct list_head* dsmgr_space_list = NULL;
			struct umessage_crc_information_resp_freespace_dist* dist_result = NULL;
			dist_result = get_and_verify_space_from_dsmgr();
			if ( dist_result ) {
	  			dsmgr_space_list = &(dist_result->um_size_list_head);
			} else {
	  			return;
			}

			contrast_dsmgr_space_lists( dsmgr_space_list, &list_used_blocks_size );
			free_dsmgr_list_memory( dsmgr_space_list );
			free( dist_result );
		  }

		  if ( num_iterations % 1000 == 0 ) {
		    	// add all space back and do another comparison, then get the pool again
  			add_all_space_back_to_dsmgr();

  			struct list_head* rtree_list_after = traverse_dsbmp_rtree();
			compare_dsbmp_rtree_lists( rtree_list_before, rtree_list_after );

			// get the pool again
			pool_size_factor = RAND_MAX / POOL_SIZE_RANGE;
			pool_size_to_add = random() / pool_size_factor;
			pool_size = POOL_SIZE_BASE + pool_size_to_add;

			get_exact_space_from_dsmgr( pool_size );
		  }

	}


	exit( 0 );
}



#define BLOCK_SIZE 4096
#define UNIT_SIZE 2000
//
// looks like the size of the unit - should be 2000
//  - so should compute the index like that
//    offset / 2000
//


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

void initialize_large_chunk_bitmap( uint64_t** p_bitmap )
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
		memset( p_bitmap_index ++, PATTERN_SEPARATOR, sizeof( uint64_t ) );
		i ++;
	}


}

#define NUM_BITMAPS 4000

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
void initialize_patterned_bitmap( uint64_t** p_bitmap )
{
  	uint64_t ul_pattern1 = PATTERN1;
	uint64_t ul_pattern2 = PATTERN2;
  	uint64_t ul_pattern3 = PATTERN3;
	uint64_t ul_pattern4 = PATTERN4;
  	uint64_t ul_pattern5 = PATTERN5;
	// uint64_t ul_pattern6 = PATTERN6;
  	uint64_t ul_pattern7 = PATTERN7;
	uint64_t ul_pattern8 = PATTERN8;

	*p_bitmap = calloc( NUM_BITMAPS, BITMAP_SIZE );

	uint64_t* p_bitmap_index = *p_bitmap;

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
}


void initialize_dsmgr()
{


  	uint64_t* p_bitmap = NULL;

	nid_log_info( "dsmgr_unit - initialize_dsmgr() - enter" );
	// initialize_patterned_bitmap( &p_bitmap );
	// dsmgr_setup.size = ( NUM_BITMAPS  * BITMAP_SIZE ) + 1;
	// initialize_large_chunk_bitmap( &p_bitmap );

	dsmgr_setup.size = ( LARGE_CHUNKS_NUMBER * LARGE_CHUNK_SIZE  * BITMAP_SIZE ) + 1;

	initialize_large_zeroed_bitmap( &p_bitmap );

	struct allocator_setup alloc_setup;
	struct allocator_interface *p_alloc = calloc(1, sizeof(*p_alloc));
	allocator_initialization(p_alloc, &alloc_setup);

	struct blksn_setup blksn_setup;
	blksn_setup.allocator = p_alloc;
	blksn_setup.set_id = 2;
	struct blksn_interface* p_blksn = calloc(1,
			sizeof(struct blksn_interface));
	brn_initialization(&ut_brn, &brn_setup);

	blksn_initialization(p_blksn, &blksn_setup);

	dsmgr_setup.bitmap = p_bitmap;
	dsmgr_setup.blksn = p_blksn;
	dsmgr_setup.brn = &ut_brn;
	dsmgr_setup.allocator = p_alloc;


	dsmgr_initialization(&ut_dsmgr, &dsmgr_setup);
	nid_log_info( "dsmgr_unit - initialize_dsmgr() - exit" );

}



int
main()
{
  	struct list_head* dsmgr_space_list_main;
	struct umessage_crc_information_resp_freespace_dist* dist_result = NULL;
	nid_log_open();
	nid_log_info("dsmgr ut module start ...");
	//uint64_t* p_bitmap = calloc( 100000, 64 );

	initialize_dsmgr();
	dist_result = get_and_verify_space_from_dsmgr();
	assert( dist_result );
	dsmgr_space_list_main = &(dist_result->um_size_list_head);

	print_num_free_blocks();

	free_dsmgr_list_memory( dsmgr_space_list_main );
	free( dist_result );
	dist_result = NULL;
	initialize_stress_test();
	get_big_chunks_add_small_chunks( 1000, 50 );

	// exit( 0 );
	get_small_chunks_add_big_chunks(50, 1000);
	run_stress_test();

	dist_result = get_and_verify_space_from_dsmgr();
	assert( dist_result );
	dsmgr_space_list_main = &(dist_result->um_size_list_head);

	free_dsmgr_list_memory( dsmgr_space_list_main );
	free( dist_result );

	print_num_free_blocks();

	add_all_space_back_to_dsmgr();
	dist_result = get_and_verify_space_from_dsmgr();
	assert( dist_result );
	dsmgr_space_list_main = &(dist_result->um_size_list_head);
	free_dsmgr_list_memory( dsmgr_space_list_main );
	free( dist_result );

	print_num_free_blocks();

	nid_log_info("dsmgr ut module end...");
	nid_log_close();

	return 0;
}
