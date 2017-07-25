#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "node_if.h"

static struct node_setup node_setup;
static struct node_interface ut_node;

void get_nodes()
{
  struct node_header *nhp1 = NULL, *nhp2 = NULL;

  printf( "nhp1 before getting = %lu, nhp2 = %lu\n", (unsigned long)nhp1, (unsigned long)nhp2 );
  nhp1 = ut_node.n_op->n_get_node( &ut_node );

  nhp2 = ut_node.n_op->n_get_node( &ut_node );

  printf( "nhp1 after getting = %lu, nhp2 = %lu\n", (unsigned long)nhp1, (unsigned long)nhp2 );


  // put nodes
  ut_node.n_op->n_put_node( &ut_node, nhp1 );
  ut_node.n_op->n_put_node( &ut_node, nhp2 );

  printf( "after putting the nodes\n" );
}


int
main()
{
	nid_log_open();
	nid_log_info("node ut module start ...");

	node_setup.node_size = sizeof( struct node_header );
	node_initialization(&ut_node, &node_setup);

	get_nodes();
	nid_log_info("node ut module end...");
	nid_log_close();

	return 0;
}
