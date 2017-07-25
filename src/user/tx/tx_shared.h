/*
 * tx_shared.h
 *
 */

#ifndef _TX_SHARED_H_
#define _TX_SHARED_H_

#include <stdint.h>

#if 0
#define DO_HOUSEKEEPING		1
#endif

#if 1
#define SEQ_ACK_ENABLE		1
#endif

#define UMSG_TYPE_ACK		0xFF
#define UMSG_LEVEL_APP		0
#define UMSG_LEVEL_TX		1

#define TX_RW_TIMEOUT		10

struct umessage_tx_hdr {
	uint8_t		um_req;
	uint8_t		um_req_code;
	uint8_t		um_level;   // package is used for dxa/dxp/cxa/cxp or user of tx
	uint8_t		um_type;	// different active send package or ack package
	uint32_t	um_len;		// the whole message len, including the header
	uint32_t	um_dlen;	// data length
	uint64_t	um_seq;		// seq of control message
	uint64_t	um_dseq;	// seq of data message
	uint32_t	um_dummy;	// dummy variable to make structure length to 32bytes
}__attribute__ ((__packed__));

#define UMSG_TX_HEADER_LEN			sizeof(struct umessage_tx_hdr)

struct list_node;
extern char* encode_tx_hdr(char *, uint32_t *, struct umessage_tx_hdr *);
extern char* decode_tx_hdr(char *, uint32_t *, struct umessage_tx_hdr *);
extern uint8_t __tx_get_pkg_level(struct umessage_tx_hdr *);

#endif /* _TX_SHARED_H_ */
