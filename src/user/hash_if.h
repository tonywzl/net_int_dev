/*
 * hash_if.h
 *
 */

#ifndef SRC_USER_HASH_IF_H_
#define SRC_USER_HASH_IF_H_

#include <stdint.h>

struct hash_interface;
struct hash_operations {
        uint32_t	(*hash_calc)(struct hash_interface *, const char *, uint32_t);
        void		(*hash_perf)(struct hash_interface *);
};

struct hash_interface {
        void                    *s_private;
        struct hash_operations    *s_op;
};

struct ini_interface;

struct hash_setup {
        struct ini_interface    **ini;
        uint32_t		hash_mod;
};

extern int hash_initialization(struct hash_interface *, struct hash_setup *);
#endif /* SRC_USER_HASH_IF_H_ */
