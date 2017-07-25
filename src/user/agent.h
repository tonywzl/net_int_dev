#ifndef _NID_AGENT_H
#define _NID_AGENT_H

#include "ini_if.h"

#define AGENT_RESERVED_DEV	"/dev/nid0"

#define AGENT_TYPE_ASC		"asc"
#define AGENT_DEV_SIZE		"size"
#define AGENT_DEVICE_TYPE	"device"
#define AGENT_NONEXISTED_DEV	"non-existed-dev"
#define AGENT_NONEXISTED_IP	"0.0.0.0"

#define AGENT_CHAN_DELIMITED	';'

#define AGENT_MAX_CHANNELS	4

struct lstn_interface;

extern void conf_global_registration(struct list_head *, struct lstn_interface *);
extern void conf_asc_registration(struct list_head *, struct lstn_interface *);
//extern struct ini_key_desc agent_keys[];
//extern struct ini_key_desc * agent_generate_conf_tmplate(struct list_head *, struct lstn_interface *);
extern void agent_generate_conf_tmplate(struct list_head *, struct lstn_interface *);
extern void agent_stop();
extern int agent_is_stop();

#endif
