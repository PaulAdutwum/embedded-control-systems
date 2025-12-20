/**
 * @file state_machine.h
 * @brief State Machine for Lidar Control System
 * @author Paul Adutwum
 * @date 2025
 * 
 * Implements a finite state machine for system control with <50ms transitions.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "../include/config.h"

/*============================================================================
 *                         STATE MACHINE EVENTS
 *============================================================================*/

typedef enum {
    EVENT_NONE = 0,
    EVENT_INIT_COMPLETE,
    EVENT_SCAN_COMPLETE,
    EVENT_TARGET_DETECTED,
    EVENT_TARGET_LOST,
    EVENT_ALARM_TRIGGERED,
    EVENT_ALARM_CLEARED,
    EVENT_ERROR_DETECTED,
    EVENT_ERROR_CLEARED,
    EVENT_CALIBRATE_REQUEST,
    EVENT_CALIBRATE_COMPLETE,
    EVENT_TIMEOUT,
    EVENT_COUNT
} system_event_t;

/*============================================================================
 *                         STATE MACHINE INTERFACE
 *============================================================================*/

/**
 * @brief Initialize state machine
 */
void sm_init(void);

/**
 * @brief Process a state machine event
 * @param event Event to process
 */
void sm_process_event(system_event_t event);

/**
 * @brief Run one iteration of the state machine
 * @return Current state after processing
 */
system_state_t sm_run(void);

/**
 * @brief Get current state
 * @return Current system state
 */
system_state_t sm_get_state(void);

/**
 * @brief Get previous state
 * @return Previous system state
 */
system_state_t sm_get_previous_state(void);

/**
 * @brief Check if system is in error state
 * @return true if error, false otherwise
 */
bool sm_is_error(void);

/**
 * @brief Get time spent in current state (ms)
 * @return Time in current state
 */
uint32_t sm_get_state_time(void);

/**
 * @brief Get system status structure
 * @param status Pointer to status structure to fill
 */
void sm_get_status(system_status_t *status);

/**
 * @brief Set error code
 * @param error Error code
 */
void sm_set_error(error_code_t error);

/**
 * @brief Clear error state
 */
void sm_clear_error(void);

/**
 * @brief Get string name of state
 * @param state State to get name for
 * @return State name string
 */
const char* sm_state_name(system_state_t state);

/**
 * @brief Get string name of event
 * @param event Event to get name for
 * @return Event name string
 */
const char* sm_event_name(system_event_t event);

#endif /* STATE_MACHINE_H */

