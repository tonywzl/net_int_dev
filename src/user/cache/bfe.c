/*
 * bfe.c
 * 	Implementation of BIO Flush Engine Module
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "bfe_if.h"

extern int bfes_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup);
extern int bfea_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup);

int
bfe_initialization(struct bfe_interface *bfe_p, struct bfe_setup *setup)
{
	if (!setup->rw_sync) {
		return bfea_initialization(bfe_p, setup);
	} else {
		return bfes_initialization(bfe_p, setup);
	}
	return -1;
}
