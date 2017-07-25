/*
 * nid.h
 */
#ifndef _NID_H
#define _NID_H

#define NID_NAME_SIZE		128
#define NID_REQ_READ		1
#define NID_REQ_WRITE		2
#define NID_REQ_DISC		3
#define NID_REQ_FLUSH		4
#define NID_REQ_TRIM		5
#define NID_REQ_KEEPALIVE	6

/*
 * sizeof nid_request should be 32
 */
struct nid_request {
	__be16		magic;
	char		code;
	char		cmd;
	__be32		len;		// data len
	__be64		offset;
	__be64		dseq;		// input data sequence number, high 32 bits
	char 		handle[8];
};

#endif
