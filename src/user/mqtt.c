/*
 * mqtt.c
 * 	Implementation of MQTT (mosquitto) Module
 */

#include <string.h>

#ifdef HAS_MQTT
#include <mosquitto.h>
#endif

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "nid_log.h"
#include "nid.h"
#include "mqtt_if.h"
#include "pp2_if.h"


#ifdef HAS_MQTT
struct mqtt_private {
	struct mosquitto 	*p_mosq;
	struct pp2_interface    *p_pp2;
	char			p_client_id[MQTT_MAX_ID];
	char			p_topic[MQTT_MAX_TOPIC];
	char			p_clean_session;
	uint32_t		p_seq;
	char			p_loop_start;
};

static void
_mqtt_assemble_message(char *buf, struct mqtt_message *msg_p)
{
	sprintf(buf, "{\"channel_uuid\":\"%s\","
			"\"name\":\"%s\","
			"\"description\":\"%s\","
			"\"service\":\"NID\","
			"\"createtime\":\"%lu\","
			"\"status\":\"%s\","
			"\"ip\":\"%s\"}",
			msg_p->uuid,
			msg_p->module,
			msg_p->message,
			msg_p->time,
			msg_p->status,
			msg_p->ip);
}

static void
_mqtt_assemble_topic(char *topic, struct mqtt_message *msg_p)
{
	sprintf(topic, "/alert/USXOPS/%s/%s",
			msg_p->uuid, msg_p->module);
}

static int
mqtt_publish(struct mqtt_interface *mt_p, struct mqtt_message *msg_p)
{
	char *log_header = "mqtt_publish";
	struct mqtt_private *priv_p = mt_p->mt_private;
	struct mosquitto *mosq_p = priv_p->p_mosq;
	char buf[MQTT_MAX_MSG];
	char topic[MQTT_MAX_MSG];
	int rc = 0;

	nid_log_debug("%s", log_header);
	msg_p->seq = priv_p->p_seq;
	priv_p->p_seq++;
	msg_p->time = time(NULL);
	memset(buf, 0, MQTT_MAX_MSG);
	_mqtt_assemble_topic(topic, msg_p);
	_mqtt_assemble_message(buf, msg_p);
	strcpy(priv_p->p_topic, topic);
	rc = mosquitto_publish(mosq_p, NULL, priv_p->p_topic, strlen(buf), buf, 1, false);

	return rc;
}

static void
mqtt_cleanup(struct mqtt_interface *mt_p)
{
	struct mqtt_private *priv_p = mt_p->mt_private;
	struct mosquitto *mosq_p = priv_p->p_mosq;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	mosquitto_disconnect(mosq_p);
	mosquitto_loop_stop(mosq_p, 0);
	mosquitto_destroy(mosq_p);
	mosquitto_lib_cleanup();
	pp2_p->pp2_op->pp2_put(pp2_p, (void*)priv_p);
	mt_p->mt_private = NULL;
}

struct mqtt_operations mt_op = {
	.mt_publish = mqtt_publish,
	.mt_cleanup = mqtt_cleanup,
};

int
mqtt_initialization(struct mqtt_interface *mt_p, struct mqtt_setup *setup)
{
	struct mqtt_private *priv_p;
	struct pp2_interface *pp2_p = setup->pp2;
	struct mosquitto *mosq_p = NULL;
	int rc;

	if (!setup) {
		return -1;
	}

	nid_log_notice("mqtt_initialization start ...");

	priv_p = (struct mqtt_private *) pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	mt_p->mt_private = (void *)priv_p;
	mt_p->mt_op = &mt_op;

	strcpy(priv_p->p_client_id, setup->client_id);
	priv_p->p_clean_session = setup->clean_session;
	priv_p->p_pp2 = pp2_p;
	priv_p->p_seq = 0;
	if (setup->role == NID_ROLE_SERVICE)
		strcpy(priv_p->p_topic, "nidserver");
	else if (setup->role == NID_ROLE_AGENT)
		strcpy(priv_p->p_topic, "nidagent");
	mosquitto_lib_init();
	mosq_p = mosquitto_new(priv_p->p_client_id, priv_p->p_clean_session, mt_p);
	if (!mosq_p) {
		nid_log_warning("fail to create mqtt instance");
		pp2_p->pp2_op->pp2_put(pp2_p, (void *)priv_p);
		return 1;
	}
	priv_p->p_mosq = mosq_p;

	rc = mosquitto_connect(mosq_p, "localhost", MQTT_PORT, MQTT_KEEPALIVE);
	if (rc)
		nid_log_warning("fail to connect to mosquitto");
	else
		mosquitto_loop_start(mosq_p);

	return 0;
}

#else
struct mqtt_private {
	struct pp2_interface    *p_pp2;
	char			p_client_id[MQTT_MAX_ID];
	char			p_topic[MQTT_MAX_TOPIC];
	char			p_clean_session;
	uint32_t		p_seq;
};

static int
mqtt_publish(struct mqtt_interface *mt_p, struct mqtt_message *msg_p)
{
	struct mqtt_private *priv_p = mt_p->mt_private;
	int rc = 0;

	msg_p->seq = priv_p->p_seq;
	priv_p->p_seq++;
	msg_p->time = time(NULL);

	return rc;
}

static void
mqtt_cleanup(struct mqtt_interface *mt_p)
{
	struct mqtt_private *priv_p = mt_p->mt_private;
	struct pp2_interface *pp2_p = priv_p->p_pp2;

	pp2_p->pp2_op->pp2_put(pp2_p, (void*)priv_p);
	mt_p->mt_private = NULL;
}

struct mqtt_operations mt_op = {
	.mt_publish = mqtt_publish,
	.mt_cleanup = mqtt_cleanup,
};

int
mqtt_initialization(struct mqtt_interface *mt_p, struct mqtt_setup *setup)
{
	struct mqtt_private *priv_p;
	struct pp2_interface *pp2_p = setup->pp2;

	if (!setup) {
		return -1;
	}

	nid_log_notice("mqtt_initialization start ...");

	priv_p = (struct mqtt_private *) pp2_p->pp2_op->pp2_get_zero(pp2_p, sizeof(*priv_p));
	mt_p->mt_private = (void *)priv_p;
	mt_p->mt_op = &mt_op;

	strcpy(priv_p->p_client_id, setup->client_id);
	priv_p->p_clean_session = setup->clean_session;
	priv_p->p_pp2 = pp2_p;
	priv_p->p_seq = 0;
	if (setup->role == NID_ROLE_SERVICE)
		strcpy(priv_p->p_topic, "nidserver");
	else if (setup->role == NID_ROLE_AGENT)
		strcpy(priv_p->p_topic, "nidagent");
	nid_log_warning("mosquitto is not enable");

	return 0;
}
#endif
