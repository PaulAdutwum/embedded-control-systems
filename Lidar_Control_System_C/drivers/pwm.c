/**
 * @file pwm.c
 * @brief PWM Driver Implementation
 * @author Paul Adutwum
 * @date 2025
 */

#include "pwm.h"
#include "../include/hal.h"
#include <stddef.h> /* For NULL */

/*============================================================================
 *                         PRIVATE VARIABLES
 *============================================================================*/

static pwm_config_t pwm_channels[PWM_CHANNEL_COUNT];
static uint16_t current_servo_angle = 90;
static uint8_t servo_pin = SERVO_PWM_PIN;
static uint8_t buzzer_pin = BUZZER_PIN;
static uint8_t motor_pin = MOTOR_PIN;

/*============================================================================
 *                         REGISTER DEFINITIONS (AVR ATmega328P)
 *============================================================================*/

#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

/*============================================================================
 *                    PRIVATE FUNCTIONS
 *============================================================================*/

/**
 * @brief Convert angle to pulse width in microseconds
 * @param angle Angle in degrees (0-180)
 * @return Pulse width in microseconds
 */
static uint16_t angle_to_pulse(uint16_t angle)
{
    /* Clamp angle to valid range */
    if (angle > 180)
        angle = 180;

    /* Map angle (0-180) to pulse width (1000-2000 us) */
    return PWM_MIN_PULSE_US +
           ((uint32_t)angle * (PWM_MAX_PULSE_US - PWM_MIN_PULSE_US)) / 180;
}

/**
 * @brief Generate software PWM pulse for servo
 * @param pulse_us Pulse width in microseconds
 */
static void generate_servo_pulse(uint16_t pulse_us)
{
    /* Critical section for accurate timing */
    uint8_t sreg = hal_critical_enter();

    hal_gpio_write(servo_pin, GPIO_STATE_HIGH);
    hal_delay_us(pulse_us);
    hal_gpio_write(servo_pin, GPIO_STATE_LOW);

    hal_critical_exit(sreg);
}

/*============================================================================
 *                         PWM CORE FUNCTIONS
 *============================================================================*/

void pwm_init(void)
{
    /* Initialize all PWM channels to default state */
    for (int i = 0; i < PWM_CHANNEL_COUNT; i++)
    {
        pwm_channels[i].enabled = false;
        pwm_channels[i].duty_cycle = 0;
        pwm_channels[i].frequency = 0;
    }

#ifdef __AVR__
    /* Configure Timer1 for servo control (16-bit timer) */
    /* Fast PWM mode, prescaler 8 */
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

    /* Set TOP value for 50Hz (20ms period) */
    ICR1 = (F_CPU / (8 * PWM_FREQUENCY)) - 1;

    /* Configure Timer2 for buzzer (8-bit timer) */
    TCCR2A = (1 << WGM21); /* CTC mode */
    TCCR2B = (1 << CS22);  /* Prescaler 64 */
#endif
}

int pwm_configure(pwm_channel_t channel, const pwm_config_t *config)
{
    if (channel >= PWM_CHANNEL_COUNT || config == NULL)
    {
        return -1;
    }

    pwm_channels[channel] = *config;
    hal_gpio_init(config->pin, GPIO_MODE_OUTPUT);

    return 0;
}

void pwm_set_duty(pwm_channel_t channel, uint16_t duty_cycle)
{
    if (channel >= PWM_CHANNEL_COUNT)
        return;

    /* Clamp duty cycle to 0-1000 */
    if (duty_cycle > 1000)
        duty_cycle = 1000;

    pwm_channels[channel].duty_cycle = duty_cycle;

#ifdef __AVR__
    if (channel == PWM_CHANNEL_SERVO)
    {
        /* Convert duty cycle to pulse width */
        uint16_t pulse = (uint32_t)duty_cycle * 20000 / 1000;
        OCR1A = pulse * (ICR1 + 1) / 20000;
    }
#endif
}

void pwm_set_frequency(pwm_channel_t channel, uint16_t frequency)
{
    if (channel >= PWM_CHANNEL_COUNT)
        return;

    pwm_channels[channel].frequency = frequency;
}

void pwm_enable(pwm_channel_t channel)
{
    if (channel >= PWM_CHANNEL_COUNT)
        return;

    pwm_channels[channel].enabled = true;
}

void pwm_disable(pwm_channel_t channel)
{
    if (channel >= PWM_CHANNEL_COUNT)
        return;

    pwm_channels[channel].enabled = false;
    hal_gpio_write(pwm_channels[channel].pin, GPIO_STATE_LOW);
}

/*============================================================================
 *                         SERVO CONTROL FUNCTIONS
 *============================================================================*/

void servo_init(uint8_t pin)
{
    servo_pin = pin;
    hal_gpio_init(pin, GPIO_MODE_OUTPUT);
    current_servo_angle = 90; /* Start at center position */

    /* Configure PWM channel for servo */
    pwm_config_t config = {
        .pin = pin,
        .frequency = PWM_FREQUENCY,
        .duty_cycle = 75, /* ~1.5ms pulse for 90 degrees */
        .enabled = true};
    pwm_configure(PWM_CHANNEL_SERVO, &config);
}

void servo_set_angle(uint16_t angle)
{
    /* Clamp angle to valid range */
    if (angle > 180)
        angle = 180;

    current_servo_angle = angle;
    uint16_t pulse_us = angle_to_pulse(angle);
    servo_set_pulse(pulse_us);
}

uint16_t servo_get_angle(void)
{
    return current_servo_angle;
}

void servo_set_pulse(uint16_t pulse_us)
{
    /* Clamp pulse width to safe range */
    if (pulse_us < PWM_MIN_PULSE_US)
        pulse_us = PWM_MIN_PULSE_US;
    if (pulse_us > PWM_MAX_PULSE_US)
        pulse_us = PWM_MAX_PULSE_US;

#ifdef __AVR__
    /* Using Timer1 hardware PWM */
    OCR1A = (uint32_t)pulse_us * (ICR1 + 1) / 20000;
#else
    /* Software PWM fallback */
    generate_servo_pulse(pulse_us);
#endif
}

void servo_move_smooth(uint16_t target_angle, uint16_t step_delay_ms)
{
    if (target_angle > 180)
        target_angle = 180;

    int16_t current = (int16_t)current_servo_angle;
    int16_t target = (int16_t)target_angle;
    int8_t step = (target > current) ? 1 : -1;

    while (current != target)
    {
        current += step;
        servo_set_angle((uint16_t)current);
        hal_delay_ms(step_delay_ms);
    }
}

/*============================================================================
 *                         BUZZER CONTROL FUNCTIONS
 *============================================================================*/

void buzzer_init(uint8_t pin)
{
    buzzer_pin = pin;
    hal_gpio_init(pin, GPIO_MODE_OUTPUT);
    hal_gpio_write(pin, GPIO_STATE_LOW);
}

void buzzer_tone(uint16_t frequency)
{
    if (frequency == 0)
    {
        buzzer_off();
        return;
    }

#ifdef __AVR__
    /* Configure Timer2 for tone generation */
    uint16_t ocr_value = (F_CPU / (128UL * frequency)) - 1;
    if (ocr_value > 255)
        ocr_value = 255;

    OCR2A = (uint8_t)ocr_value;
    TCCR2A |= (1 << COM2A0); /* Toggle on compare match */

    /* Set buzzer pin as Timer2 output (Pin 11 on ATmega328P) */
    DDRB |= (1 << PB3);
#else
    /* Software tone generation fallback */
    uint16_t half_period_us = 500000 / frequency;

    for (uint16_t i = 0; i < frequency / 10; i++)
    {
        hal_gpio_write(buzzer_pin, GPIO_STATE_HIGH);
        hal_delay_us(half_period_us);
        hal_gpio_write(buzzer_pin, GPIO_STATE_LOW);
        hal_delay_us(half_period_us);
    }
#endif
}

void buzzer_off(void)
{
#ifdef __AVR__
    TCCR2A &= ~(1 << COM2A0); /* Disconnect timer from pin */
#endif
    hal_gpio_write(buzzer_pin, GPIO_STATE_LOW);
}

void buzzer_alarm(uint8_t pattern)
{
    switch (pattern)
    {
    case 0: /* Continuous */
        buzzer_tone(1000);
        break;

    case 1: /* Beep pattern */
        buzzer_tone(1000);
        hal_delay_ms(100);
        buzzer_off();
        hal_delay_ms(100);
        break;

    case 2: /* Siren pattern */
        for (uint16_t freq = 800; freq <= 1200; freq += 50)
        {
            buzzer_tone(freq);
            hal_delay_ms(20);
        }
        for (uint16_t freq = 1200; freq >= 800; freq -= 50)
        {
            buzzer_tone(freq);
            hal_delay_ms(20);
        }
        break;

    default:
        buzzer_off();
        break;
    }
}

/*============================================================================
 *                         MOTOR CONTROL FUNCTIONS
 *============================================================================*/

void motor_init(uint8_t pin)
{
    motor_pin = pin;
    hal_gpio_init(pin, GPIO_MODE_OUTPUT);
    hal_gpio_write(pin, GPIO_STATE_LOW);
}

void motor_set_speed(uint8_t speed)
{
    if (speed > 100)
        speed = 100;

    if (speed == 0)
    {
        motor_off();
        return;
    }

    if (speed == 100)
    {
        motor_on();
        return;
    }

    /* PWM speed control using software PWM */
    uint16_t duty = (uint16_t)speed * 10; /* Convert to 0-1000 range */

#ifdef __AVR__
    /* Using Timer0 for motor PWM (Pin 6) */
    OCR0A = (uint8_t)((speed * 255) / 100);
    TCCR0A |= (1 << COM0A1) | (1 << WGM00) | (1 << WGM01);
    TCCR0B |= (1 << CS01);
#else
    (void)duty; /* Suppress unused warning */
#endif
}

void motor_on(void)
{
    hal_gpio_write(motor_pin, GPIO_STATE_HIGH);
}

void motor_off(void)
{
    hal_gpio_write(motor_pin, GPIO_STATE_LOW);
}
