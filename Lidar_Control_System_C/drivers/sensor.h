/**
 * @file sensor.h
 * @brief Ultrasonic Sensor Driver
 * @author Paul Adutwum
 * @date 2025
 * 
 * Provides interface for HC-SR04 ultrasonic distance sensor.
 */

#ifndef SENSOR_H
#define SENSOR_H

#include "../include/config.h"

/*============================================================================
 *                         SENSOR CONFIGURATION
 *============================================================================*/

typedef struct {
    uint8_t trig_pin;
    uint8_t echo_pin;
    uint16_t max_distance_cm;
    uint16_t timeout_us;
} ultrasonic_config_t;

typedef struct {
    uint16_t distance_cm;
    uint32_t echo_duration_us;
    bool     valid;
    uint32_t timestamp;
} ultrasonic_reading_t;

/*============================================================================
 *                         ULTRASONIC SENSOR FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize ultrasonic sensor
 * @param trig_pin Trigger pin number
 * @param echo_pin Echo pin number
 */
void ultrasonic_init(uint8_t trig_pin, uint8_t echo_pin);

/**
 * @brief Initialize with custom configuration
 * @param config Sensor configuration
 * @return 0 on success, -1 on error
 */
int ultrasonic_init_config(const ultrasonic_config_t *config);

/**
 * @brief Trigger a distance measurement
 * @return Distance in centimeters (0 if out of range or error)
 */
uint16_t ultrasonic_measure(void);

/**
 * @brief Get detailed measurement reading
 * @param reading Pointer to reading structure
 * @return 0 on success, -1 on timeout/error
 */
int ultrasonic_get_reading(ultrasonic_reading_t *reading);

/**
 * @brief Measure distance with averaging
 * @param samples Number of samples to average
 * @return Averaged distance in centimeters
 */
uint16_t ultrasonic_measure_avg(uint8_t samples);

/**
 * @brief Calibrate sensor (measure at known distance)
 * @param known_distance_cm Known distance for calibration
 * @return Calibration factor (1.0 = perfect)
 */
float ultrasonic_calibrate(uint16_t known_distance_cm);

/**
 * @brief Check if sensor is responding
 * @return true if sensor is working, false otherwise
 */
bool ultrasonic_test(void);

/*============================================================================
 *                         ANALOG SENSOR FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize analog sensors (LDR, Potentiometer)
 */
void analog_sensors_init(void);

/**
 * @brief Read light level from LDR
 * @return Light level (0-1023, higher = brighter)
 */
uint16_t ldr_read(void);

/**
 * @brief Check if it's dark (night mode)
 * @param threshold Darkness threshold (0-1023)
 * @return true if dark, false if bright
 */
bool ldr_is_dark(uint16_t threshold);

/**
 * @brief Read potentiometer value
 * @return Raw ADC value (0-1023)
 */
uint16_t potentiometer_read(void);

/**
 * @brief Get alarm threshold from potentiometer
 * @param min_cm Minimum threshold value
 * @param max_cm Maximum threshold value
 * @return Threshold in centimeters
 */
uint16_t potentiometer_get_threshold(uint16_t min_cm, uint16_t max_cm);

/*============================================================================
 *                         SENSOR FUSION
 *============================================================================*/

/**
 * @brief Initialize all sensors
 */
void sensors_init_all(void);

/**
 * @brief Read all sensor values
 * @param distance Pointer to store distance
 * @param light Pointer to store light level
 * @param threshold Pointer to store alarm threshold
 */
void sensors_read_all(uint16_t *distance, uint16_t *light, uint16_t *threshold);

/**
 * @brief Apply moving average filter to distance readings
 * @param new_reading New distance reading
 * @return Filtered distance value
 */
uint16_t sensor_filter_distance(uint16_t new_reading);

/**
 * @brief Validate sensor reading
 * @param distance Distance reading to validate
 * @return true if reading is valid, false if anomalous
 */
bool sensor_validate_reading(uint16_t distance);

#endif /* SENSOR_H */

