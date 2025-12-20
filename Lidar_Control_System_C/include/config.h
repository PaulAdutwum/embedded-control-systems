/**
 * @file config.h
 * @brief System Configuration for Autonomous Lidar Control System
 * @author Paul Adutwum
 * @date 2025
 *
 * Hardware configuration, timing parameters, and system constants.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 *                         HARDWARE CONFIGURATION
 *============================================================================*/

/* MCU Clock Configuration */
#define F_CPU 16000000UL /* 16 MHz system clock */
#define SYSCLK_FREQ F_CPU

/* UART Configuration */
#define UART_BAUD_RATE 9600
#define UART_BUFFER_SIZE 64
#define UART_TIMEOUT_MS 100

/* PWM Configuration for Servo Motor */
#define PWM_FREQUENCY 50 /* 50 Hz (20ms period) for servo */
#define PWM_TIMER_PRESCALER 64
#define PWM_MIN_PULSE_US 1000    /* 1ms = 0 degrees */
#define PWM_MAX_PULSE_US 2000    /* 2ms = 180 degrees */
#define PWM_CENTER_PULSE_US 1500 /* 1.5ms = 90 degrees */

/* Servo Configuration */
#define SERVO_MIN_ANGLE 15     /* Minimum sweep angle */
#define SERVO_MAX_ANGLE 165    /* Maximum sweep angle */
#define SERVO_STEP_DELAY_MS 30 /* Delay between angle steps */
#define SERVO_PWM_PIN 2        /* Servo control pin */

/* Ultrasonic Sensor Configuration */
#define TRIG_PIN 12
#define ECHO_PIN 13
#define ECHO_TIMEOUT_US 30000    /* 30ms timeout (~5m range) */
#define SPEED_OF_SOUND_CM 0.0343 /* cm/microsecond at 20°C */
#define MAX_DISTANCE_CM 400      /* Maximum measurable distance */
#define MIN_DISTANCE_CM 2        /* Minimum measurable distance */

/* Alarm System Configuration */
#define LED_PIN 3
#define BUZZER_PIN 4
#define MOTOR_PIN 8
#define LDR_PIN 0 /* Analog pin A0 */
#define POT_PIN 1 /* Analog pin A1 */

/* Detection Thresholds */
#define DEFAULT_ALARM_THRESHOLD_CM 20
#define MIN_ALARM_THRESHOLD_CM 5
#define MAX_ALARM_THRESHOLD_CM 50
#define NIGHT_MODE_THRESHOLD 900 /* LDR reading threshold */

/*============================================================================
 *                         TIMING CONFIGURATION
 *============================================================================*/

/* State Machine Timing */
#define STATE_TRANSITION_DELAY_MS 10
#define ALARM_CHECK_INTERVAL_MS 100
#define SENSOR_READ_INTERVAL_MS 50
#define MAX_RESPONSE_TIME_MS 50 /* Maximum allowed response time */

/* Debounce Configuration */
#define DEBOUNCE_SAMPLES 3
#define DEBOUNCE_THRESHOLD 2

/*============================================================================
 *                         SYSTEM STATES
 *============================================================================*/

typedef enum
{
    STATE_IDLE = 0,
    STATE_INIT,
    STATE_SCANNING,
    STATE_TARGET_DETECTED,
    STATE_ALARM_ACTIVE,
    STATE_CALIBRATING,
    STATE_ERROR,
    STATE_COUNT
} system_state_t;

typedef enum
{
    SCAN_DIR_LEFT_TO_RIGHT = 0,
    SCAN_DIR_RIGHT_TO_LEFT
} scan_direction_t;

typedef enum
{
    ERROR_NONE = 0,
    ERROR_SENSOR_TIMEOUT,
    ERROR_UART_OVERFLOW,
    ERROR_SERVO_FAULT,
    ERROR_SYSTEM_CRITICAL
} error_code_t;

/*============================================================================
 *                         DATA STRUCTURES
 *============================================================================*/

/* Sensor Data Structure */
typedef struct
{
    uint16_t distance_cm;
    uint16_t angle_deg;
    uint32_t timestamp_ms;
    bool valid;
} sensor_reading_t;

/* System Status Structure */
typedef struct
{
    system_state_t current_state;
    system_state_t previous_state;
    scan_direction_t scan_direction;
    uint16_t current_angle;
    uint16_t alarm_threshold;
    uint16_t light_level;
    bool is_night_mode;
    bool target_locked;
    error_code_t last_error;
    uint32_t uptime_ms;
    uint32_t scan_count;
} system_status_t;

/* Configuration Structure */
typedef struct
{
    uint16_t min_angle;
    uint16_t max_angle;
    uint16_t step_delay_ms;
    uint16_t default_threshold;
    bool buzzer_enabled;
    bool night_mode_only;
} system_config_t;

/*============================================================================
 *                         MACROS
 *============================================================================*/

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define CLAMP(x, low, high) (((x) < (low)) ? (low) : (((x) > (high)) ? (high) : (x)))
#define MAP(x, in_min, in_max, out_min, out_max) \
    (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) + (out_min))

/* Bit manipulation macros */
#define BIT_SET(reg, bit) ((reg) |= (1 << (bit)))
#define BIT_CLR(reg, bit) ((reg) &= ~(1 << (bit)))
#define BIT_TGL(reg, bit) ((reg) ^= (1 << (bit)))
#define BIT_CHK(reg, bit) ((reg) & (1 << (bit)))

/* GPIO macros (AVR specific) */
#define GPIO_OUTPUT(port, pin) BIT_SET(port, pin)
#define GPIO_INPUT(port, pin) BIT_CLR(port, pin)
#define GPIO_HIGH(port, pin) BIT_SET(port, pin)
#define GPIO_LOW(port, pin) BIT_CLR(port, pin)

/*============================================================================
 *                         DEBUG CONFIGURATION
 *============================================================================*/

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define DEBUG_LOG(msg) printf("[DEBUG] %s\n", msg)
#else
#define DEBUG_PRINT(fmt, ...)
#define DEBUG_LOG(msg)
#endif

#endif /* CONFIG_H */
