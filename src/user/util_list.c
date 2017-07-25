// utilities for linked lists


#include "util_list.h"

/*
 * Algorithm:
 * 	Verify if a particular linked list has a loop.  This is done by traversing the list
 *	with two different markers.  One traverses two nodes at a time and the other 
 *	traverses one node at a time (turtle and the hair).  If at any point the markers are on 
 *	the same node, then there is a loop in the list.
 * Return:
 *	Returns 1 if there is a loop.
 *	Returns 0 if there is no loop in the list.
 */
int check_for_loop( struct list_head* list_head )
{
	struct list_head* cur_np, *cur_np_loop_2;

  	cur_np = list_head;
	cur_np_loop_2 = list_head;
	if ( cur_np == NULL ) {
	  	return 0;
	} 
	cur_np = cur_np->next;
	if ( cur_np == NULL ) {
	  	return 0;
	} else {
	  	cur_np_loop_2 = cur_np->next;
	}
  	while ( ( cur_np != list_head ) && ( cur_np != NULL ) && ( cur_np_loop_2 != list_head ) && (cur_np_loop_2 != NULL ) ) {
	  	if ( cur_np == cur_np_loop_2 ) {
	    		// found loop 
		  	printf( "found loop in the list\n" );
			//assert( 0 );
	    		return 1;
	  	}
    		cur_np = cur_np->next;
		if ( cur_np_loop_2->next == NULL ) {
		  	break;
		}
		cur_np_loop_2 = cur_np_loop_2->next->next;
  	}
	if ( ( cur_np == list_head ) || ( cur_np_loop_2 == list_head ) )    {
	  	printf( "reached the head of the list\n" );
	} else {
	  	printf ( "found a NULL in the list\n" );
	}
	printf( "no loop in the list\n" );
	return 0;
}
