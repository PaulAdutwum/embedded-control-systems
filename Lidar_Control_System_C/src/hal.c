/**
 * @file hal.c
 * @brief Hardware Abstraction Layer Implementation
 * @author Paul Adutwum
 * @date 2025
 * 
 * Platform-specific hardware implementations for AVR ATmega328P.
 */

#include "../include/hal.h"

/*============================================================================
 *                         PLATFORM INCLUDES
 *============================================================================*/

#ifdef __AVR__
    #include <avr/io.h>
    #include <avr/interrupt.h>
    #include <util/delay.h>
#else
    /* Simulation/testing platform */
    #include <stdio.h>
    #include <time.h>
    #include <unistd.h>
#endif

/*============================================================================
 *                         PRIVATE VARIABLES
 *============================================================================*/

static volatile uint32_t system_time_ms = 0;
static volatile uint32_t system_time_us = 0;
static timer_callback_t periodic_callback = NULL;
static uint32_t callback_period_ms = 0;
static uint32_t last_callback_time = 0;

/*============================================================================
 *                         GPIO IMPLEMENTATION
 *============================================================================*/

void hal_gpio_init(uint8_t pin, gpio_mode_t mode) {
#ifdef __AVR__
    /* Determine port and bit from Arduino-style pin number */
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    uint8_t bit;
    
    if (pin < 8) {
        /* Port D (pins 0-7) */
        ddr = &DDRD;
        port = &PORTD;
        bit = pin;
    } else if (pin < 14) {
        /* Port B (pins 8-13) */
        ddr = &DDRB;
        port = &PORTB;
        bit = pin - 8;
    } else {
        /* Port C (analog pins A0-A5 = 14-19) */
        ddr = &DDRC;
        port = &PORTC;
        bit = pin - 14;
    }
    
    switch (mode) {
        case GPIO_MODE_OUTPUT:
            *ddr |= (1 << bit);
            break;
        case GPIO_MODE_INPUT:
            *ddr &= ~(1 << bit);
            *port &= ~(1 << bit);  /* Disable pull-up */
            break;
        case GPIO_MODE_INPUT_PULLUP:
            *ddr &= ~(1 << bit);
            *port |= (1 << bit);   /* Enable pull-up */
            break;
    }
#else
    (void)pin;
    (void)mode;
#endif
}

void hal_gpio_write(uint8_t pin, gpio_state_t state) {
#ifdef __AVR__
    volatile uint8_t *port;
    uint8_t bit;
    
    if (pin < 8) {
        port = &PORTD;
        bit = pin;
    } else if (pin < 14) {
        port = &PORTB;
        bit = pin - 8;
    } else {
        port = &PORTC;
        bit = pin - 14;
    }
    
    if (state == GPIO_STATE_HIGH) {
        *port |= (1 << bit);
    } else {
        *port &= ~(1 << bit);
    }
#else
    (void)pin;
    (void)state;
#endif
}

gpio_state_t hal_gpio_read(uint8_t pin) {
#ifdef __AVR__
    volatile uint8_t *pinr;
    uint8_t bit;
    
    if (pin < 8) {
        pinr = &PIND;
        bit = pin;
    } else if (pin < 14) {
        pinr = &PINB;
        bit = pin - 8;
    } else {
        pinr = &PINC;
        bit = pin - 14;
    }
    
    return (*pinr & (1 << bit)) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
#else
    (void)pin;
    return GPIO_STATE_LOW;
#endif
}

void hal_gpio_toggle(uint8_t pin) {
#ifdef __AVR__
    volatile uint8_t *port;
    uint8_t bit;
    
    if (pin < 8) {
        port = &PORTD;
        bit = pin;
    } else if (pin < 14) {
        port = &PORTB;
        bit = pin - 8;
    } else {
        port = &PORTC;
        bit = pin - 14;
    }
    
    *port ^= (1 << bit);
#else
    (void)pin;
#endif
}

/*============================================================================
 *                         TIMER IMPLEMENTATION
 *============================================================================*/

void hal_timer_init(void) {
#ifdef __AVR__
    /* Configure Timer0 for 1ms interrupt */
    /* Using CTC mode with prescaler 64 */
    TCCR0A = (1 << WGM01);  /* CTC mode */
    TCCR0B = (1 << CS01) | (1 << CS00);  /* Prescaler 64 */
    
    /* Set compare value for 1ms @ 16MHz: 16000000 / 64 / 1000 = 250 */
    OCR0A = 249;
    
    /* Enable compare match interrupt */
    TIMSK0 |= (1 << OCIE0A);
    
    /* Enable global interrupts */
    sei();
#endif
    
    system_time_ms = 0;
    system_time_us = 0;
}

uint32_t hal_timer_get_ms(void) {
#ifdef __AVR__
    uint32_t ms;
    uint8_t sreg = SREG;
    cli();
    ms = system_time_ms;
    SREG = sreg;
    return ms;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

uint32_t hal_timer_get_us(void) {
#ifdef __AVR__
    uint32_t us;
    uint8_t sreg = SREG;
    cli();
    /* Get timer count and combine with milliseconds */
    uint8_t timer_count = TCNT0;
    us = system_time_ms * 1000 + ((uint32_t)timer_count * 4);  /* 4us per tick at 64 prescaler */
    SREG = sreg;
    return us;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#endif
}

void hal_delay_ms(uint32_t ms) {
#ifdef __AVR__
    while (ms--) {
        _delay_ms(1);
    }
#else
    usleep(ms * 1000);
#endif
}

void hal_delay_us(uint32_t us) {
#ifdef __AVR__
    while (us > 0) {
        if (us >= 10) {
            _delay_us(10);
            us -= 10;
        } else {
            _delay_us(1);
            us--;
        }
    }
#else
    usleep(us);
#endif
}

void hal_timer_register_callback(timer_callback_t callback, uint32_t period_ms) {
    periodic_callback = callback;
    callback_period_ms = period_ms;
    last_callback_time = hal_timer_get_ms();
}

/*============================================================================
 *                         ADC IMPLEMENTATION
 *============================================================================*/

void hal_adc_init(void) {
#ifdef __AVR__
    /* Set reference to AVCC with external capacitor at AREF pin */
    ADMUX = (1 << REFS0);
    
    /* Enable ADC with prescaler 128: 16MHz/128 = 125kHz */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
#endif
}

uint16_t hal_adc_read(uint8_t channel) {
#ifdef __AVR__
    /* Select ADC channel */
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    
    /* Start conversion */
    ADCSRA |= (1 << ADSC);
    
    /* Wait for conversion to complete */
    while (ADCSRA & (1 << ADSC));
    
    /* Return 10-bit result */
    return ADC;
#else
    (void)channel;
    return 512;  /* Return mid-range for simulation */
#endif
}

uint16_t hal_adc_read_avg(uint8_t channel, uint8_t samples) {
    if (samples == 0) samples = 1;
    
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += hal_adc_read(channel);
    }
    
    return (uint16_t)(sum / samples);
}

/*============================================================================
 *                         INTERRUPT IMPLEMENTATION
 *============================================================================*/

void hal_interrupts_enable(void) {
#ifdef __AVR__
    sei();
#endif
}

void hal_interrupts_disable(void) {
#ifdef __AVR__
    cli();
#endif
}

uint8_t hal_critical_enter(void) {
#ifdef __AVR__
    uint8_t sreg = SREG;
    cli();
    return sreg;
#else
    return 0;
#endif
}

void hal_critical_exit(uint8_t state) {
#ifdef __AVR__
    SREG = state;
#else
    (void)state;
#endif
}

/*============================================================================
 *                         SYSTEM IMPLEMENTATION
 *============================================================================*/

void hal_system_init(void) {
    /* Initialize timer first */
    hal_timer_init();
    
    /* Initialize ADC */
    hal_adc_init();
    
    /* Enable interrupts */
    hal_interrupts_enable();
}

void hal_system_reset(void) {
#ifdef __AVR__
    /* Use watchdog timer for reset */
    #include <avr/wdt.h>
    wdt_enable(WDTO_15MS);
    while (1);  /* Wait for watchdog reset */
#else
    /* Simulation: just exit */
    exit(0);
#endif
}

void hal_system_sleep(void) {
#ifdef __AVR__
    #include <avr/sleep.h>
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
#else
    usleep(1000);
#endif
}

/*============================================================================
 *                         INTERRUPT HANDLERS
 *============================================================================*/

#ifdef __AVR__
/**
 * @brief Timer0 Compare Match Interrupt (1ms tick)
 */
ISR(TIMER0_COMPA_vect) {
    system_time_ms++;
    
    /* Check for periodic callback */
    if (periodic_callback != NULL && callback_period_ms > 0) {
        if ((system_time_ms - last_callback_time) >= callback_period_ms) {
            periodic_callback();
            last_callback_time = system_time_ms;
        }
    }
}
#endif

