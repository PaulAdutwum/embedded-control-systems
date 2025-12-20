/**
 * @file state_machine.c
 * @brief State Machine Implementation
 * @author Paul Adutwum
 * @date 2025
 *
 * Optimized state machine with <50ms response time guarantee.
 */

#include "state_machine.h"
#include "../include/hal.h"
#include "../drivers/uart.h"
#include <stddef.h> /* For NULL */

/*============================================================================
 *                         PRIVATE VARIABLES
 *============================================================================*/

static system_status_t system_status;
static uint32_t state_entry_time = 0;

/* State name strings */
static const char *state_names[] = {
    "IDLE",
    "INIT",
    "SCANNING",
    "TARGET_DETECTED",
    "ALARM_ACTIVE",
    "CALIBRATING",
    "ERROR"};

/* Event name strings */
static const char *event_names[] = {
    "NONE",
    "INIT_COMPLETE",
    "SCAN_COMPLETE",
    "TARGET_DETECTED",
    "TARGET_LOST",
    "ALARM_TRIGGERED",
    "ALARM_CLEARED",
    "ERROR_DETECTED",
    "ERROR_CLEARED",
    "CALIBRATE_REQUEST",
    "CALIBRATE_COMPLETE",
    "TIMEOUT"};

/*============================================================================
 *                         STATE HANDLER PROTOTYPES
 *============================================================================*/

typedef void (*state_handler_t)(void);
typedef system_state_t (*transition_handler_t)(system_event_t event);

static void state_idle_entry(void);
static void state_idle_run(void);
static system_state_t state_idle_transition(system_event_t event);

static void state_init_entry(void);
static void state_init_run(void);
static system_state_t state_init_transition(system_event_t event);

static void state_scanning_entry(void);
static void state_scanning_run(void);
static system_state_t state_scanning_transition(system_event_t event);

static void state_target_detected_entry(void);
static void state_target_detected_run(void);
static system_state_t state_target_detected_transition(system_event_t event);

static void state_alarm_active_entry(void);
static void state_alarm_active_run(void);
static system_state_t state_alarm_active_transition(system_event_t event);

static void state_calibrating_entry(void);
static void state_calibrating_run(void);
static system_state_t state_calibrating_transition(system_event_t event);

static void state_error_entry(void);
static void state_error_run(void);
static system_state_t state_error_transition(system_event_t event);

/*============================================================================
 *                         STATE TABLE
 *============================================================================*/

typedef struct
{
    state_handler_t entry;
    state_handler_t run;
    transition_handler_t transition;
} state_descriptor_t;

static const state_descriptor_t state_table[STATE_COUNT] = {
    [STATE_IDLE] = {state_idle_entry, state_idle_run, state_idle_transition},
    [STATE_INIT] = {state_init_entry, state_init_run, state_init_transition},
    [STATE_SCANNING] = {state_scanning_entry, state_scanning_run, state_scanning_transition},
    [STATE_TARGET_DETECTED] = {state_target_detected_entry, state_target_detected_run, state_target_detected_transition},
    [STATE_ALARM_ACTIVE] = {state_alarm_active_entry, state_alarm_active_run, state_alarm_active_transition},
    [STATE_CALIBRATING] = {state_calibrating_entry, state_calibrating_run, state_calibrating_transition},
    [STATE_ERROR] = {state_error_entry, state_error_run, state_error_transition},
};

/*============================================================================
 *                         PRIVATE FUNCTIONS
 *============================================================================*/

/**
 * @brief Transition to a new state
 */
static void transition_to(system_state_t new_state)
{
    if (new_state >= STATE_COUNT)
        return;
    if (new_state == system_status.current_state)
        return;

    system_status.previous_state = system_status.current_state;
    system_status.current_state = new_state;
    state_entry_time = hal_timer_get_ms();

    /* Log state transition */
    DEBUG_PRINT("[SM] %s -> %s\n",
                state_names[system_status.previous_state],
                state_names[system_status.current_state]);

    /* Call entry handler for new state */
    if (state_table[new_state].entry != NULL)
    {
        state_table[new_state].entry();
    }
}

/*============================================================================
 *                         STATE HANDLERS: IDLE
 *============================================================================*/

static void state_idle_entry(void)
{
    /* Nothing to do in idle */
}

static void state_idle_run(void)
{
    /* Wait for initialization event */
}

static system_state_t state_idle_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_INIT_COMPLETE:
        return STATE_SCANNING;
    case EVENT_CALIBRATE_REQUEST:
        return STATE_CALIBRATING;
    case EVENT_ERROR_DETECTED:
        return STATE_ERROR;
    default:
        return STATE_IDLE;
    }
}

/*============================================================================
 *                         STATE HANDLERS: INIT
 *============================================================================*/

static void state_init_entry(void)
{
    /* Initialization will be performed by main.c */
}

static void state_init_run(void)
{
    /* Check for initialization timeout */
    if (sm_get_state_time() > 5000)
    { /* 5 second timeout */
        sm_set_error(ERROR_SYSTEM_CRITICAL);
        sm_process_event(EVENT_ERROR_DETECTED);
    }
}

static system_state_t state_init_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_INIT_COMPLETE:
        return STATE_SCANNING;
    case EVENT_ERROR_DETECTED:
        return STATE_ERROR;
    default:
        return STATE_INIT;
    }
}

/*============================================================================
 *                         STATE HANDLERS: SCANNING
 *============================================================================*/

static void state_scanning_entry(void)
{
    system_status.target_locked = false;
}

static void state_scanning_run(void)
{
    /* Scanning logic is handled in main loop */
    system_status.scan_count++;
}

static system_state_t state_scanning_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_TARGET_DETECTED:
        return STATE_TARGET_DETECTED;
    case EVENT_CALIBRATE_REQUEST:
        return STATE_CALIBRATING;
    case EVENT_ERROR_DETECTED:
        return STATE_ERROR;
    default:
        return STATE_SCANNING;
    }
}

/*============================================================================
 *                         STATE HANDLERS: TARGET DETECTED
 *============================================================================*/

static void state_target_detected_entry(void)
{
    system_status.target_locked = true;
}

static void state_target_detected_run(void)
{
    /* Check if we should escalate to alarm */
}

static system_state_t state_target_detected_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_TARGET_LOST:
        return STATE_SCANNING;
    case EVENT_ALARM_TRIGGERED:
        return STATE_ALARM_ACTIVE;
    case EVENT_ERROR_DETECTED:
        return STATE_ERROR;
    default:
        return STATE_TARGET_DETECTED;
    }
}

/*============================================================================
 *                         STATE HANDLERS: ALARM ACTIVE
 *============================================================================*/

static void state_alarm_active_entry(void)
{
    /* Alarm outputs are controlled in main loop */
}

static void state_alarm_active_run(void)
{
    /* Alarm state processing */
}

static system_state_t state_alarm_active_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_ALARM_CLEARED:
    case EVENT_TARGET_LOST:
        return STATE_SCANNING;
    case EVENT_ERROR_DETECTED:
        return STATE_ERROR;
    default:
        return STATE_ALARM_ACTIVE;
    }
}

/*============================================================================
 *                         STATE HANDLERS: CALIBRATING
 *============================================================================*/

static void state_calibrating_entry(void)
{
    uart_tx_string("\r\n[CALIBRATING...]\r\n");
}

static void state_calibrating_run(void)
{
    /* Calibration timeout */
    if (sm_get_state_time() > 10000)
    { /* 10 second calibration timeout */
        sm_process_event(EVENT_CALIBRATE_COMPLETE);
    }
}

static system_state_t state_calibrating_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_CALIBRATE_COMPLETE:
        return STATE_SCANNING;
    case EVENT_ERROR_DETECTED:
        return STATE_ERROR;
    default:
        return STATE_CALIBRATING;
    }
}

/*============================================================================
 *                         STATE HANDLERS: ERROR
 *============================================================================*/

static void state_error_entry(void)
{
    uart_printf("\r\n[ERROR] Code: %d\r\n", system_status.last_error);
}

static void state_error_run(void)
{
    /* Flash LED as error indicator */
    static uint32_t last_toggle = 0;
    if ((hal_timer_get_ms() - last_toggle) > 250)
    {
        /* Toggle would happen here */
        last_toggle = hal_timer_get_ms();
    }
}

static system_state_t state_error_transition(system_event_t event)
{
    switch (event)
    {
    case EVENT_ERROR_CLEARED:
        return STATE_INIT;
    default:
        return STATE_ERROR;
    }
}

/*============================================================================
 *                         PUBLIC FUNCTIONS
 *============================================================================*/

void sm_init(void)
{
    /* Initialize status structure */
    system_status.current_state = STATE_INIT;
    system_status.previous_state = STATE_IDLE;
    system_status.scan_direction = SCAN_DIR_LEFT_TO_RIGHT;
    system_status.current_angle = SERVO_MIN_ANGLE;
    system_status.alarm_threshold = DEFAULT_ALARM_THRESHOLD_CM;
    system_status.light_level = 0;
    system_status.is_night_mode = false;
    system_status.target_locked = false;
    system_status.last_error = ERROR_NONE;
    system_status.uptime_ms = 0;
    system_status.scan_count = 0;

    state_entry_time = hal_timer_get_ms();

    /* Call init state entry */
    if (state_table[STATE_INIT].entry != NULL)
    {
        state_table[STATE_INIT].entry();
    }
}

void sm_process_event(system_event_t event)
{
    if (event >= EVENT_COUNT)
        return;

    system_state_t current = system_status.current_state;

    /* Get next state from transition handler */
    if (state_table[current].transition != NULL)
    {
        system_state_t next = state_table[current].transition(event);

        if (next != current)
        {
            transition_to(next);
        }
    }
}

system_state_t sm_run(void)
{
    system_state_t current = system_status.current_state;

    /* Update uptime */
    system_status.uptime_ms = hal_timer_get_ms();

    /* Run current state handler */
    if (state_table[current].run != NULL)
    {
        state_table[current].run();
    }

    return current;
}

system_state_t sm_get_state(void)
{
    return system_status.current_state;
}

system_state_t sm_get_previous_state(void)
{
    return system_status.previous_state;
}

bool sm_is_error(void)
{
    return (system_status.current_state == STATE_ERROR);
}

uint32_t sm_get_state_time(void)
{
    return hal_timer_get_ms() - state_entry_time;
}

void sm_get_status(system_status_t *status)
{
    if (status != NULL)
    {
        *status = system_status;
    }
}

void sm_set_error(error_code_t error)
{
    system_status.last_error = error;
}

void sm_clear_error(void)
{
    system_status.last_error = ERROR_NONE;
    sm_process_event(EVENT_ERROR_CLEARED);
}

const char *sm_state_name(system_state_t state)
{
    if (state >= STATE_COUNT)
        return "UNKNOWN";
    return state_names[state];
}

const char *sm_event_name(system_event_t event)
{
    if (event >= EVENT_COUNT)
        return "UNKNOWN";
    return event_names[event];
}
