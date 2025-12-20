/**
 * @file pwm.h
 * @brief PWM Driver for Motor Control
 * @author Paul Adutwum
 * @date 2025
 *
 * Provides PWM signal generation for servo motor and actuator control.
 */

#ifndef PWM_H
#define PWM_H

#include "../include/config.h"

/*============================================================================
 *                         PWM CHANNEL CONFIGURATION
 *============================================================================*/

typedef enum
{
    PWM_CHANNEL_SERVO = 0,
    PWM_CHANNEL_MOTOR,
    PWM_CHANNEL_BUZZER,
    PWM_CHANNEL_COUNT
} pwm_channel_t;

typedef struct
{
    uint8_t pin;
    uint16_t frequency;
    uint16_t duty_cycle; /* 0-1000 (0.0% - 100.0%) */
    bool enabled;
} pwm_config_t;

/*============================================================================
 *                         PWM FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize PWM subsystem
 */
void pwm_init(void);

/**
 * @brief Configure a PWM channel
 * @param channel PWM channel
 * @param config Configuration structure
 * @return 0 on success, -1 on error
 */
int pwm_configure(pwm_channel_t channel, const pwm_config_t *config);

/**
 * @brief Set PWM duty cycle
 * @param channel PWM channel
 * @param duty_cycle Duty cycle (0-1000 = 0%-100%)
 */
void pwm_set_duty(pwm_channel_t channel, uint16_t duty_cycle);

/**
 * @brief Set PWM frequency
 * @param channel PWM channel
 * @param frequency Frequency in Hz
 */
void pwm_set_frequency(pwm_channel_t channel, uint16_t frequency);

/**
 * @brief Enable PWM output
 * @param channel PWM channel
 */
void pwm_enable(pwm_channel_t channel);

/**
 * @brief Disable PWM output
 * @param channel PWM channel
 */
void pwm_disable(pwm_channel_t channel);

/*============================================================================
 *                         SERVO CONTROL FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize servo motor
 * @param pin Servo control pin
 */
void servo_init(uint8_t pin);

/**
 * @brief Set servo angle
 * @param angle Angle in degrees (0-180)
 */
void servo_set_angle(uint16_t angle);

/**
 * @brief Get current servo angle
 * @return Current angle in degrees
 */
uint16_t servo_get_angle(void);

/**
 * @brief Set servo pulse width directly
 * @param pulse_us Pulse width in microseconds
 */
void servo_set_pulse(uint16_t pulse_us);

/**
 * @brief Smoothly move servo to target angle
 * @param target_angle Target angle in degrees
 * @param step_delay_ms Delay between steps in ms
 */
void servo_move_smooth(uint16_t target_angle, uint16_t step_delay_ms);

/*============================================================================
 *                         BUZZER CONTROL FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize buzzer
 * @param pin Buzzer control pin
 */
void buzzer_init(uint8_t pin);

/**
 * @brief Play a tone at specified frequency
 * @param frequency Frequency in Hz
 */
void buzzer_tone(uint16_t frequency);

/**
 * @brief Stop buzzer tone
 */
void buzzer_off(void);

/**
 * @brief Play alarm pattern
 * @param pattern Pattern type (0=continuous, 1=beep, 2=siren)
 */
void buzzer_alarm(uint8_t pattern);

/*============================================================================
 *                         MOTOR CONTROL FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize motor driver
 * @param pin Motor control pin
 */
void motor_init(uint8_t pin);

/**
 * @brief Set motor speed
 * @param speed Speed (0-100%)
 */
void motor_set_speed(uint8_t speed);

/**
 * @brief Turn motor on at full speed
 */
void motor_on(void);

/**
 * @brief Turn motor off
 */
void motor_off(void);

#endif /* PWM_H */
