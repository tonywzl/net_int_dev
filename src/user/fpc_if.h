/*
 * fpc_if.h
 *
 *  Created on: Aug 28, 2015
 *      Author: Rick Xiong
 */

#ifndef FPC_IF_H_
#define FPC_IF_H_

#include <stdint.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <sys/uio.h>
#include "nid_log.h"
#include "list.h"

typedef enum _fpc_algorithm {
	FPC_SHA256,
	FPC_SHA224,
	FPC_MD5,
} fpc_algorithm ;

struct fpn_interface;
struct fpc_interface;

struct fpc_operations {
	void		(*fpc_calculate)(struct fpc_interface *, const void *, size_t, unsigned char *);
	uint64_t	(*fpc_hvcalculate)(struct fpc_interface *, const char *);
	int		(*fpc_cmp)(struct fpc_interface *, const void *, const void *);
	uint8_t		(*fpc_get_fp_len)(struct fpc_interface *);
	void		(*fpc_set_wgt)(struct fpc_interface *, uint8_t);
	uint8_t		(*fpc_get_wgt_from_fp)(struct fpc_interface *, const char *);
	void		(*fpc_set_wgt_to_fp)(struct fpc_interface *, char *, uint8_t);
	void    	(*fpc_bcalculate)(struct fpc_interface *, struct fpn_interface  *, void *buf, size_t count, size_t blocksz, struct list_head *fp_head);
	void    	(*fpc_bcalculatev)(struct fpc_interface *, struct fpn_interface *, struct iovec *iovs, int iovcnt, size_t blocksz, struct list_head *fp_head);
	void		(*fpc_destroy)(struct fpc_interface *);
};

struct fpc_interface {
	void					*fpc_private;
	struct fpc_operations	*fpc_op;
};

struct fpc_setup {
	fpc_algorithm fpc_algrm;
};

extern int fpc_initialization(struct fpc_interface *, struct fpc_setup *);

#endif /* FPC_IF_H_ */
