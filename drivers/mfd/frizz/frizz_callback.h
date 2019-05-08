#ifndef __FRIZZ_CALLBACK_H__
#define __FRIZZ_CALLBACK_H__

typedef void(* INTERRUPT_TIMER_P)(unsigned int, unsigned int[]);
typedef void(* INTERRUPT_POLL_P)(void);
typedef void(* INTERRUPT_TIMEOUT_P)(void);
typedef void(* INTERRUPT_GET_SENSOR_DATA_P)(void);

/*!
 * update sampling time
 *
 * @param[in] header
 * @param[in] packet data
 */
void interrupt_timer(unsigned int, unsigned int[]);

/*!
 * unlock polling process
 *
 */
void interrupt_poll(void);

/*!
 * unlock polling process
 *
 */
void interrupt_timeout(void);

/*!
 * notify when sensor data update
 */
void interrupt_get_sensor_data(void);


#endif
