#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "nid_log.h"
//#include "sdn_if.h"
#include "rtree_if.h"
#include "dsbmp_if.h"
#include "dsmgr_if.h"
#include "blksn_if.h"
#include "brn_if.h"


static struct dsmgr_interface ut_dsmgr;

static struct dsbmp_setup dsbmp_setup;
static struct dsbmp_interface ut_dsbmp;

struct blksn_setup blksn_setup;
struct blksn_interface*  p_blksn;


static void
dsmgr_add_new_node(struct dsmgr_interface *dsmgr_p, struct block_size_node *np)
{
  assert( dsmgr_p );
  assert( np );

  //  list_add_tail( &np->bsn_list, &test_list );

  // struct dsmgr_private *priv_p = (struct dsmgr_private *)dsmgr_p->dm_private;
  // __dsmgr_add_node(priv_p, np);
}

struct dsmgr_operations dsmgr_op = {
  	.dm_add_new_node = dsmgr_add_new_node,
};

/* int
rtree_initialization(struct rtree_interface *rtree_p, struct rtree_setup *setup)
{
    assert(rtree_p && setup);
    return 0;
    } */

/*int
sdn_initialization(struct sdn_interface *sdn_p, struct sdn_setup *setup)
{
    assert(sdn_p && setup);
    return 0;
    } */

int
main()
{
 //   int i;
   // uint64_t bmp_size = 5*64*4096*sizeof(uint64_t);
    uint64_t bmp[5];
   /* for (i = 0; i < 5; i++){
        bmp[i] = 2328230976;
    }
*/
   // uint64_t bmp_size = 5 * 64;
    dsbmp_setup.size = 5;
//    dsbmp_setup.bitmap =calloc(bmp_size, sizeof(char));;
    dsbmp_setup.bitmap = bmp;
    nid_log_open();
    nid_log_info("dsbmp ut module start ...");

	p_blksn = calloc(1, sizeof( struct blksn_interface ));


	static struct brn_setup brn_setup;
	static struct brn_interface brn; // = calloc( 1, sizeof( struct brn_interface ));

        blksn_initialization(p_blksn, &blksn_setup);
	brn_initialization( &brn, &brn_setup );
	ut_dsmgr.dm_op = &dsmgr_op;

	//	dsbmp_setup.size = NUM_BITMAPS * 1;

	//	dsbmp_setup.bitmap = p_bitmap;
    
	dsbmp_setup.blksn = p_blksn;
	dsbmp_setup.brn = &brn;
	dsbmp_setup.dsmgr = &ut_dsmgr;

    	dsbmp_initialization(&ut_dsbmp, &dsbmp_setup);

    nid_log_info("dsbmp ut module end...");
    nid_log_close();

    return 0;
}

