/*
 * olap_if.h
 * 	Interface of Overlapping Module
 */
#ifndef NID_OLAP_IF_H
#define NID_OLAP_IF_H

struct olap_interface;
struct list_head;
struct olap_operations {
	void	(*ol_get_overlap)(struct olap_interface *, struct list_head *, struct list_head *);
};

struct olap_interface {
	void			*ol_private;
	struct olap_operations	*ol_op;
};

struct olap_setup {
	struct dcn_interface *dcn;
};

extern int olap_initialization(struct olap_interface *, struct olap_setup *);
#endif
