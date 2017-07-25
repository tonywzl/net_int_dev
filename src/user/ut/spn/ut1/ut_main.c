#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nid_log.h"
#include "spn_if.h"

static struct spn_setup spn_setup;
static struct spn_interface ut_spn;

void get_spns()
{
  struct spn_header *nhp1 = NULL, *nhp2 = NULL;

  printf( "nhp1 before getting = %lu, nhp2 = %lu\n", (unsigned long)nhp1, (unsigned long)nhp2 );
  nhp1 = ut_spn.sn_op->sn_get_spn( &ut_spn );

  nhp2 = ut_spn.sn_op->sn_get_spn( &ut_spn );

  printf( "nhp1 after getting = %lu, nhp2 = %lu\n", (unsigned long)nhp1, (unsigned long)nhp2 );
  printf( "nhp1 data after getting = %lu, nhp2 data = %lu\n", (unsigned long)nhp1->sn_data, (unsigned long)nhp2->sn_data );


  // put spns
  ut_spn.sn_op->sn_put_spn( &ut_spn, nhp1 );
  ut_spn.sn_op->sn_put_spn( &ut_spn, nhp2 );

  printf( "after putting the spns\n" );
}


int
main()
{
	nid_log_open();
	nid_log_info("spn ut module start ...");

	spn_setup.spn_size = 512;
	spn_initialization(&ut_spn, &spn_setup);

	get_spns();
	nid_log_info("spn ut module end...");
	nid_log_close();

	return 0;
}

