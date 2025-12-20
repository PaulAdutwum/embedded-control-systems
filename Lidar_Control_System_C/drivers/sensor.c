/**
 * @file sensor.c
 * @brief Ultrasonic Sensor Driver Implementation
 * @author Paul Adutwum
 * @date 2025
 */

#include "sensor.h"
#include "../include/hal.h"
#include <stddef.h> /* For NULL */

/*============================================================================
 *                         PRIVATE VARIABLES
 *============================================================================*/

static uint8_t trig_pin_g = TRIG_PIN;
static uint8_t echo_pin_g = ECHO_PIN;
static uint16_t max_distance_g = MAX_DISTANCE_CM;
static uint32_t timeout_us_g = ECHO_TIMEOUT_US;
static float calibration_factor = 1.0f;

/* Moving average filter buffer */
#define FILTER_SIZE 5
static uint16_t filter_buffer[FILTER_SIZE];
static uint8_t filter_index = 0;
static bool filter_initialized = false;

/*============================================================================
 *                         ULTRASONIC SENSOR FUNCTIONS
 *============================================================================*/

void ultrasonic_init(uint8_t trig_pin, uint8_t echo_pin)
{
    trig_pin_g = trig_pin;
    echo_pin_g = echo_pin;

    /* Configure pins */
    hal_gpio_init(trig_pin, GPIO_MODE_OUTPUT);
    hal_gpio_init(echo_pin, GPIO_MODE_INPUT);

    /* Ensure trigger is low */
    hal_gpio_write(trig_pin, GPIO_STATE_LOW);

    /* Initialize filter buffer */
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        filter_buffer[i] = 0;
    }
    filter_index = 0;
    filter_initialized = false;
}

int ultrasonic_init_config(const ultrasonic_config_t *config)
{
    if (config == NULL)
    {
        return -1;
    }

    trig_pin_g = config->trig_pin;
    echo_pin_g = config->echo_pin;
    max_distance_g = config->max_distance_cm;
    timeout_us_g = config->timeout_us;

    ultrasonic_init(trig_pin_g, echo_pin_g);

    return 0;
}

uint16_t ultrasonic_measure(void)
{
    ultrasonic_reading_t reading;

    if (ultrasonic_get_reading(&reading) == 0 && reading.valid)
    {
        return reading.distance_cm;
    }

    return 0; /* Error or out of range */
}

int ultrasonic_get_reading(ultrasonic_reading_t *reading)
{
    if (reading == NULL)
    {
        return -1;
    }

    /* Initialize reading */
    reading->valid = false;
    reading->distance_cm = 0;
    reading->echo_duration_us = 0;
    reading->timestamp = hal_timer_get_ms();

    /* Ensure trigger is low */
    hal_gpio_write(trig_pin_g, GPIO_STATE_LOW);
    hal_delay_us(2);

    /* Send 10us trigger pulse */
    hal_gpio_write(trig_pin_g, GPIO_STATE_HIGH);
    hal_delay_us(10);
    hal_gpio_write(trig_pin_g, GPIO_STATE_LOW);

    /* Wait for echo to start (go HIGH) */
    uint32_t timeout_start = hal_timer_get_us();
    while (hal_gpio_read(echo_pin_g) == GPIO_STATE_LOW)
    {
        if ((hal_timer_get_us() - timeout_start) > timeout_us_g)
        {
            return -1; /* Timeout waiting for echo start */
        }
    }

    /* Measure echo pulse duration */
    uint32_t echo_start = hal_timer_get_us();

    while (hal_gpio_read(echo_pin_g) == GPIO_STATE_HIGH)
    {
        if ((hal_timer_get_us() - echo_start) > timeout_us_g)
        {
            return -1; /* Timeout waiting for echo end */
        }
    }

    uint32_t echo_duration = hal_timer_get_us() - echo_start;

    /* Calculate distance: distance = (time * speed_of_sound) / 2 */
    /* Speed of sound = 343 m/s = 0.0343 cm/us */
    uint16_t distance = (uint16_t)((echo_duration * SPEED_OF_SOUND_CM) / 2.0f);

    /* Apply calibration factor */
    distance = (uint16_t)(distance * calibration_factor);

    /* Validate range */
    if (distance >= MIN_DISTANCE_CM && distance <= max_distance_g)
    {
        reading->distance_cm = distance;
        reading->echo_duration_us = echo_duration;
        reading->valid = true;
        return 0;
    }

    return -1; /* Out of range */
}

uint16_t ultrasonic_measure_avg(uint8_t samples)
{
    if (samples == 0)
        samples = 1;
    if (samples > 10)
        samples = 10;

    uint32_t sum = 0;
    uint8_t valid_count = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        uint16_t distance = ultrasonic_measure();
        if (distance > 0)
        {
            sum += distance;
            valid_count++;
        }
        hal_delay_ms(10); /* Small delay between readings */
    }

    if (valid_count == 0)
        return 0;

    return (uint16_t)(sum / valid_count);
}

float ultrasonic_calibrate(uint16_t known_distance_cm)
{
    uint16_t measured = ultrasonic_measure_avg(5);

    if (measured > 0 && known_distance_cm > 0)
    {
        calibration_factor = (float)known_distance_cm / (float)measured;
    }

    return calibration_factor;
}

bool ultrasonic_test(void)
{
    /* Perform a test measurement */
    ultrasonic_reading_t reading;

    for (int i = 0; i < 3; i++)
    {
        if (ultrasonic_get_reading(&reading) == 0)
        {
            return true; /* Sensor responded */
        }
        hal_delay_ms(50);
    }

    return false; /* Sensor not responding */
}

/*============================================================================
 *                         ANALOG SENSOR FUNCTIONS
 *============================================================================*/

void analog_sensors_init(void)
{
    hal_adc_init();
}

uint16_t ldr_read(void)
{
    return hal_adc_read_avg(LDR_PIN, 3);
}

bool ldr_is_dark(uint16_t threshold)
{
    uint16_t light_level = ldr_read();
    return (light_level < threshold);
}

uint16_t potentiometer_read(void)
{
    return hal_adc_read_avg(POT_PIN, 3);
}

uint16_t potentiometer_get_threshold(uint16_t min_cm, uint16_t max_cm)
{
    uint16_t pot_value = potentiometer_read();

    /* Map ADC value (0-1023) to threshold range */
    return MAP(pot_value, 0, 1023, min_cm, max_cm);
}

/*============================================================================
 *                         SENSOR FUSION FUNCTIONS
 *============================================================================*/

void sensors_init_all(void)
{
    /* Initialize ultrasonic sensor */
    ultrasonic_init(TRIG_PIN, ECHO_PIN);

    /* Initialize analog sensors */
    analog_sensors_init();

    /* Allow sensors to stabilize */
    hal_delay_ms(100);
}

void sensors_read_all(uint16_t *distance, uint16_t *light, uint16_t *threshold)
{
    if (distance != NULL)
    {
        *distance = ultrasonic_measure();
    }

    if (light != NULL)
    {
        *light = ldr_read();
    }

    if (threshold != NULL)
    {
        *threshold = potentiometer_get_threshold(MIN_ALARM_THRESHOLD_CM,
                                                 MAX_ALARM_THRESHOLD_CM);
    }
}

uint16_t sensor_filter_distance(uint16_t new_reading)
{
    /* Add new reading to filter buffer */
    filter_buffer[filter_index] = new_reading;
    filter_index = (filter_index + 1) % FILTER_SIZE;

    if (!filter_initialized)
    {
        /* Fill buffer with initial readings */
        static uint8_t init_count = 0;
        init_count++;
        if (init_count >= FILTER_SIZE)
        {
            filter_initialized = true;
        }
        return new_reading;
    }

    /* Calculate median (simple bubble sort for small array) */
    uint16_t sorted[FILTER_SIZE];
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        sorted[i] = filter_buffer[i];
    }

    for (int i = 0; i < FILTER_SIZE - 1; i++)
    {
        for (int j = 0; j < FILTER_SIZE - i - 1; j++)
        {
            if (sorted[j] > sorted[j + 1])
            {
                uint16_t temp = sorted[j];
                sorted[j] = sorted[j + 1];
                sorted[j + 1] = temp;
            }
        }
    }

    /* Return median value */
    return sorted[FILTER_SIZE / 2];
}

bool sensor_validate_reading(uint16_t distance)
{
    /* Check for valid range */
    if (distance < MIN_DISTANCE_CM || distance > MAX_DISTANCE_CM)
    {
        return false;
    }

    /* Check for sudden jumps (might indicate noise) */
    static uint16_t last_valid_reading = 0;
    const uint16_t max_change = 50; /* Maximum expected change in cm */

    if (last_valid_reading > 0)
    {
        int16_t change = (int16_t)distance - (int16_t)last_valid_reading;
        if (change < 0)
            change = -change;

        if (change > max_change)
        {
            /* Possible noise, but still update for next comparison */
            last_valid_reading = distance;
            return false;
        }
    }

    last_valid_reading = distance;
    return true;
}
