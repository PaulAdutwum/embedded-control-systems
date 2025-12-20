/**
 * @file hal.h
 * @brief Hardware Abstraction Layer for Lidar Control System
 * @author Paul Adutwum
 * @date 2025
 *
 * Provides hardware-independent interface for GPIO, Timers, ADC, and Interrupts.
 */

#ifndef HAL_H
#define HAL_H

#include "config.h"

/*============================================================================
 *                         GPIO INTERFACE
 *============================================================================*/

typedef enum
{
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT_PULLUP
} gpio_mode_t;

typedef enum
{
    GPIO_STATE_LOW = 0,
    GPIO_STATE_HIGH
} gpio_state_t;

/**
 * @brief Initialize a GPIO pin
 * @param pin Pin number
 * @param mode GPIO mode (input/output)
 */
void hal_gpio_init(uint8_t pin, gpio_mode_t mode);

/**
 * @brief Write to a GPIO pin
 * @param pin Pin number
 * @param state GPIO state (high/low)
 */
void hal_gpio_write(uint8_t pin, gpio_state_t state);

/**
 * @brief Read from a GPIO pin
 * @param pin Pin number
 * @return GPIO state
 */
gpio_state_t hal_gpio_read(uint8_t pin);

/**
 * @brief Toggle a GPIO pin
 * @param pin Pin number
 */
void hal_gpio_toggle(uint8_t pin);

/*============================================================================
 *                         TIMER INTERFACE
 *============================================================================*/

typedef void (*timer_callback_t)(void);

/**
 * @brief Initialize system timer
 */
void hal_timer_init(void);

/**
 * @brief Get current system time in milliseconds
 * @return System uptime in milliseconds
 */
uint32_t hal_timer_get_ms(void);

/**
 * @brief Get current system time in microseconds
 * @return System uptime in microseconds
 */
uint32_t hal_timer_get_us(void);

/**
 * @brief Delay for specified milliseconds
 * @param ms Delay in milliseconds
 */
void hal_delay_ms(uint32_t ms);

/**
 * @brief Delay for specified microseconds
 * @param us Delay in microseconds
 */
void hal_delay_us(uint32_t us);

/**
 * @brief Register a periodic timer callback
 * @param callback Function to call periodically
 * @param period_ms Period in milliseconds
 */
void hal_timer_register_callback(timer_callback_t callback, uint32_t period_ms);

/*============================================================================
 *                         ADC INTERFACE
 *============================================================================*/

/**
 * @brief Initialize ADC subsystem
 */
void hal_adc_init(void);

/**
 * @brief Read analog value from a pin
 * @param channel Analog channel number
 * @return 10-bit ADC value (0-1023)
 */
uint16_t hal_adc_read(uint8_t channel);

/**
 * @brief Read analog value with averaging
 * @param channel Analog channel number
 * @param samples Number of samples to average
 * @return Averaged ADC value
 */
uint16_t hal_adc_read_avg(uint8_t channel, uint8_t samples);

/*============================================================================
 *                         INTERRUPT INTERFACE
 *============================================================================*/

/**
 * @brief Enable global interrupts
 */
void hal_interrupts_enable(void);

/**
 * @brief Disable global interrupts
 */
void hal_interrupts_disable(void);

/**
 * @brief Enter critical section (save and disable interrupts)
 * @return Saved interrupt state
 */
uint8_t hal_critical_enter(void);

/**
 * @brief Exit critical section (restore interrupts)
 * @param state Previously saved interrupt state
 */
void hal_critical_exit(uint8_t state);

/*============================================================================
 *                         SYSTEM INTERFACE
 *============================================================================*/

/**
 * @brief Initialize all hardware
 */
void hal_system_init(void);

/**
 * @brief Perform system reset
 */
void hal_system_reset(void);

/**
 * @brief Enter low-power sleep mode
 */
void hal_system_sleep(void);

#endif /* HAL_H */
