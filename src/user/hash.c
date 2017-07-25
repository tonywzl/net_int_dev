/*
 * hash.c
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nid_log.h"
#include "ini_if.h"
#include "hash_if.h"

#define HASH_DJB2	1
#define HASH_SDBM	2

struct hash_private {
	uint32_t	hash_idx;
	uint32_t	hash_mod;
	uint64_t	*hash_hit;
};

/*
 * djb2
 *
 * this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c.
 * another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i];
 * the magic of number 33 (why it works better than many other constants, prime or not) has never
 * been adequately explained.
 *
 */
static uint32_t
hash_djb2(const char *str, uint32_t dlen)
{
	uint32_t i, hash = 5381;
	uint8_t c;

	for(i = 0; i< dlen; i++) {
		c = (uint8_t)*str++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

/*
 * sdbm
 *
 * this algorithm was created for sdbm (a public-domain reimplementation of ndbm) database library.
 * it was found to do well in scrambling bits, causing better distribution of the keys and fewer splits.
 * it also happens to be a good general hashing function with good distribution. the actual function is
 * hash(i) = hash(i - 1) * 65599 + str[i]; what is included below is the faster version used in gawk.
 * the magic constant 65599 was picked out of thin air while experimenting with different constants,
 * and turns out to be a prime. this is one of the algorithms used in berkeley db (see sleepycat) and
 * elsewhere.
 *
 */

static uint32_t
hash_sdbm(const char *str, uint32_t dlen)
{
	uint32_t i, hash = 0;
	uint8_t c;

	for(i = 0; i< dlen; i++) {
		c = (uint8_t)*str++;
		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

static uint32_t
hash_get_name(struct hash_setup *setup) {
	char *log_header = "hash_get_name";
	struct ini_interface *ini_p;
	struct ini_section_content *sc_p;
	struct ini_key_content *the_key;
	char *hash_nm;
	uint32_t hash_idx = HASH_DJB2;

	if(setup->ini == NULL) goto out;
	ini_p = *(setup->ini);
	hash_nm = NULL;
	ini_p->i_op->i_rollback(ini_p);
	while ((sc_p = ini_p->i_op->i_next_section(ini_p))) {
		the_key = ini_p->i_op->i_search_key(ini_p, sc_p, "hash_type");
		if(the_key == NULL) continue;
		hash_nm = (char *)(the_key->k_value);
		if(hash_nm == NULL) continue;
		break;
	}

	if(hash_nm && (strcmp(hash_nm, "sdbm") == 0)) {
		hash_idx = HASH_SDBM;
	}

out:
	nid_log_debug("%s: got hash type %d", log_header, hash_idx);
	return hash_idx;
}

static uint32_t
hash_calculate(struct hash_interface *hash_p, const char *dbuf, uint32_t dlen) {
	char *log_header = "hash_caculate";
	struct hash_private *priv_p = (struct hash_private *)hash_p->s_private;
	uint32_t rc;

	switch(priv_p->hash_idx) {
	case HASH_DJB2:
		rc = hash_djb2(dbuf, dlen);
		if(priv_p->hash_mod) {
			rc = rc % priv_p->hash_mod;
			priv_p->hash_hit[rc]++;
		}
		break;

	case HASH_SDBM:
		rc = hash_sdbm(dbuf, dlen);
		if(priv_p->hash_mod) {
			rc = rc % priv_p->hash_mod;
			priv_p->hash_hit[rc]++;
		}
		break;

	default:
		nid_log_error("%s: got wrong hash type %d", log_header, priv_p->hash_idx);
		rc = UINT32_MAX;
		break;
	}

	return rc;
}

static void
hash_performance(struct hash_interface *hash_p) {
	char *log_header = "hash_performance";
	struct hash_private *priv_p = (struct hash_private *)hash_p->s_private;
	uint32_t idx;

	for(idx = 0; idx < priv_p->hash_mod; idx++) {
		nid_log_info("%s: %d hash hit %lu times", log_header, idx, priv_p->hash_hit[idx]);
	}
}

static struct hash_operations hash_op = {
	.hash_calc = hash_calculate,
	.hash_perf = hash_performance,
};

int hash_initialization(struct hash_interface *hash_p, struct hash_setup *setup) {
	struct hash_private *priv_p = x_calloc(1, sizeof(*priv_p));

	hash_p->s_private = priv_p;
	priv_p->hash_idx = hash_get_name(setup);
	hash_p->s_op = &hash_op;
	priv_p->hash_mod = setup->hash_mod;
	priv_p->hash_hit = x_calloc(1, sizeof(uint64_t)*(setup->hash_mod));

	return 0;
}


