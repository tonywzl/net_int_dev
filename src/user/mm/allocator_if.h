/*
 * allocator_if.h
 * 	Interface of Allocator Module
 */
#ifndef NID_ALLOCATOR_IF_H
#define NID_ALLOCATOR_IF_H

#define ALLOCATOR_ROLE_AGENT		1
#define ALLOCATOR_ROLE_SERVER		2

#define	ALLOCATOR_SET_SCG_SRN		2
#define	ALLOCATOR_SET_CRC_CDN		3
#define	ALLOCATOR_SET_CRC_BLKSN		4
#define	ALLOCATOR_SET_BSE_BTN		5
#define	ALLOCATOR_SET_BSE_BSN		6
#define	ALLOCATOR_SET_SCG_BRN		7
#define	ALLOCATOR_SET_SCG_FPN		8
#define	ALLOCATOR_SET_SCG_PP		9
#define	ALLOCATOR_SET_SERVER_LSTN	10
#define ALLOCATOR_SET_BFE_D2CN		13
#define ALLOCATOR_SET_BFE_D1AN		14
#define ALLOCATOR_SET_RC_PP		15
#define ALLOCATOR_SET_MRW_WN		16
#define ALLOCATOR_SET_MRW_WSN		17
#define ALLOCATOR_SET_MRW_RN		18
#define ALLOCATOR_SET_DRC_PP		19
#define ALLOCATOR_SET_NSE_CRC_CB	22
#define ALLOCATOR_SET_CSE_CRC_CB	23
#define ALLOCATOR_SET_NSE_MDS		24
#define ALLOCATOR_SET_NSE_MDS_CB	25
#define ALLOCATOR_SET_CSE_MDS_CB	26
#define ALLOCATOR_SET_TWC_D2AN		27
#define ALLOCATOR_SET_TWC_D2BN		28
#define ALLOCATOR_SET_TWC_RCTWC_CB	29
#define ALLOCATOR_SET_TWC_RCTWC		30
#define ALLOCATOR_SET_TWC_DDN		31
#define ALLOCATOR_SET_SDS_PP		32
#define ALLOCATOR_SET_CRC_PP		33
#define ALLOCATOR_SET_SERVER_PP2	34
#define ALLOCATOR_SET_SERVER_DYN_PP2	35
#define ALLOCATOR_SET_CXP_TXN		36
#define ALLOCATOR_SET_BWC_D2AN		37
#define ALLOCATOR_SET_BWC_D2BN		38
#define ALLOCATOR_SET_BWC_RCBWC_CB	39
#define ALLOCATOR_SET_BWC_RCBWC	40
#define	ALLOCATOR_SET_BWC_DDN		41
#define	ALLOCATOR_SET_RSM_RSN		42
#define	ALLOCATOR_SET_RSM_RSUN		43
#define ALLOCATOR_SET_BWC_RBN		44
//#define	ALLOCATOR_SET_RSM_RSN		45
#define ALLOCATOR_SET_NSE_LSTN		45
#define ALLOCATOR_SET_SERVER_REP	46
#define ALLOCATOR_SET_SERVER_MAX	47

#define ALLOCATOR_SET_AGENT_PP2		1
#define ALLOCATOR_SET_AGENT_DYN_PP2	2
#define ALLOCATOR_SET_AGENT_DYN2_PP2	3
#define ALLOCATOR_SET_AGENT_LSTN	4
#define ALLOCATOR_SET_AGENT_MAX		5

#if ALLOCATOR_SET_AGENT_MAX > ALLOCATOR_SET_SERVER_MAX
	#define ALLOCATOR_SET_MAX		ALLOCATOR_SET_AGENT_MAX
#else 
	#define ALLOCATOR_SET_MAX		ALLOCATOR_SET_SERVER_MAX
#endif

struct allocator_interface;
struct allocator_operations {
	void*	(*a_calloc)(struct allocator_interface *, int, size_t, size_t);
	void*	(*a_malloc)(struct allocator_interface *, int, size_t);
	void*	(*a_memalign)(struct allocator_interface *, int, size_t, size_t);
	void	(*a_free)(struct allocator_interface *, void *);
	void*	(*a_get_data)(struct allocator_interface *, void *);
	void	(*a_get_size)(struct allocator_interface *);
	void	(*a_display)(struct allocator_interface *);
};

struct allocator_interface {
	void				*a_private;
	struct allocator_operations	*a_op;
};

struct allocator_setup {
	int				a_role;
};

extern int allocator_initialization(struct allocator_interface *, struct allocator_setup *);
#endif
