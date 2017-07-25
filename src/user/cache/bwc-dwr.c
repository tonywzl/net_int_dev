/*
 * bwc-dwr.c
 *
 * Delay write response for write back cache
 */

#include "bwc.h"

/**
 * Default values:
 * priv_p->p_write_delay_block_increase_step = 10;	// How many blocks reduce, make delay time increase a unit.
 * priv_p->p_write_delay_first_level = 300;	// Block left number
 * priv_p->p_write_delay_second_level = 100;	// Block left number
 * priv_p->p_write_delay_first_level_increase_interval = 10000(ms) / priv_p->p_write_delay_time_increase_step = 1000;
 * priv_p->p_write_delay_second_level_increase_interval = 50000(ms) / priv_p->p_write_delay_time_increase_step = 5000;
 * priv_p->p_write_delay_second_level_start_ms = (priv_p->p_write_delay_first_level - priv_p->p_write_delay_second_level)
 *				* priv_p->p_write_delay_first_level_increase_interval = 200000(ms)
 */
inline __useconds_t
_bwc_calculate_sleep_time(struct bwc_private *priv_p, uint16_t block_left)
{
	if (block_left >= priv_p->p_write_delay_first_level) {
		return 0;
	} else if (block_left < priv_p->p_write_delay_first_level && block_left >= priv_p->p_write_delay_second_level) {
		/**
		 * For default value, when less than 300 block, every 10 block reduce, the delay time increase 0.01 second.
		 * Max delay time is 0.2 second for every write.
		 */
		return (priv_p->p_write_delay_first_level - block_left)
				* priv_p->p_write_delay_first_level_increase_interval;
	} else if (block_left < priv_p->p_write_delay_second_level) {
		/**
		 * For default value, when less than 100 block, every 10 block reduce, the delay time increase 0.05 second.
		 * So max delay time is 0.7 second for every write.
		 */
		return priv_p->p_write_delay_second_level_start_ms +
				(priv_p->p_write_delay_second_level - block_left)
				* priv_p->p_write_delay_second_level_increase_interval;
	}
	/*Code should never reach here*/
	//assert(0);
	return 0;

}

void
__bwc_write_delay(struct bwc_private *priv_p)
{
	if (priv_p->p_write_delay_first_level == 0) return;

	uint16_t block_left = priv_p->p_flush_nblocks - priv_p->p_noccupied;
	__useconds_t sleep_time = _bwc_calculate_sleep_time(priv_p, block_left);
	struct mqtt_interface *mqtt_p = priv_p->p_mqtt;
	struct mqtt_message msg;

	if (sleep_time > 0) {
		if (!priv_p->p_record_delay) {
			msg.type = "nidserver";
			msg.module = "NIDFlowControl";
			msg.message = "The flow control threshold is reached, and delay is added";
			msg.status = "WARN";
			strcpy(msg.uuid, "NULL");
			strcpy(msg.ip, "NULL");
			mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
		}

		nid_log_debug("__bwc_write_delay: write delay: %u", sleep_time);
		usleep(sleep_time);
	} else if (sleep_time == 0 && priv_p->p_record_delay != 0) {
		msg.type = "nidserver";
		msg.module = "NIDFlowControl";
		msg.message = "The flow control delay has changed from non-zero to zero";
		msg.status = "OK";
		strcpy(msg.ip, "NULL");
		mqtt_p->mt_op->mt_publish(mqtt_p, &msg);
	}
	priv_p->p_record_delay = sleep_time;
}

