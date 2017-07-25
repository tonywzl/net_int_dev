/*
 * mqtt_if.h
 * 	Interface of MQTT (mosquitto) Module
 */

#ifndef NID_MQTT_IF_H
#define NID_MQTT_IF_H

#define MQTT_MAX_ID	128
#define MQTT_PORT 	1883
#define MQTT_MAX_MSG	4096
#define MQTT_KEEPALIVE	60
#define MQTT_MAX_TOPIC	1024
#include "nid_shared.h"

struct mqtt_interface;

struct mqtt_message {
	char		*type;
	char		*module;
	time_t		time;		// will be filled by mqtt
	uint32_t	seq;		// will be filled by mqtt
	char		*message;
	char		*status;
	char		uuid[NID_MAX_UUID];
	char		ip[NID_MAX_IP];
};

struct mqtt_operations {
	int		(*mt_publish)(struct mqtt_interface *, struct mqtt_message *);
	void		(*mt_cleanup)(struct mqtt_interface *);
};

struct mqtt_interface {
	void			*mt_private;	// used internal
	struct mqtt_operations	*mt_op;		// operations
};

struct mqtt_setup {
	char			*client_id;
	char			clean_session;
	struct pp2_interface	*pp2;
	char			role;
};

extern int mqtt_initialization(struct mqtt_interface *, struct mqtt_setup *);

#endif
