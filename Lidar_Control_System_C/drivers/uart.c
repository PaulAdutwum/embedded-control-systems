/**
 * @file uart.c
 * @brief UART Driver Implementation
 * @author Paul Adutwum
 * @date 2025
 */

#include "uart.h"
#include "../include/hal.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/*============================================================================
 *                         PRIVATE VARIABLES
 *============================================================================*/

/* Circular buffer for received data */
static volatile uint8_t rx_buffer[UART_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

/* Circular buffer for transmit data */
static volatile uint8_t tx_buffer[UART_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

static bool uart_initialized = false;

/*============================================================================
 *                         REGISTER DEFINITIONS (AVR ATmega328P)
 *============================================================================*/

/* UART Register Addresses - Platform specific */
#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>

#define UART_STATUS_REG UCSR0A
#define UART_CONTROL_REG_B UCSR0B
#define UART_CONTROL_REG_C UCSR0C
#define UART_BAUD_REG_H UBRR0H
#define UART_BAUD_REG_L UBRR0L
#define UART_DATA_REG UDR0

#define UART_RX_COMPLETE RXC0
#define UART_TX_COMPLETE TXC0
#define UART_DATA_EMPTY UDRE0
#define UART_RX_ENABLE RXEN0
#define UART_TX_ENABLE TXEN0
#define UART_RX_INT_ENABLE RXCIE0
#define UART_TX_INT_ENABLE TXCIE0
#else
/* Generic placeholder for non-AVR platforms */
static volatile uint8_t UART_STATUS_REG;
static volatile uint8_t UART_CONTROL_REG_B;
static volatile uint8_t UART_CONTROL_REG_C;
static volatile uint8_t UART_BAUD_REG_H;
static volatile uint8_t UART_BAUD_REG_L;
static volatile uint8_t UART_DATA_REG;

#define UART_RX_COMPLETE 7
#define UART_TX_COMPLETE 6
#define UART_DATA_EMPTY 5
#define UART_RX_ENABLE 4
#define UART_TX_ENABLE 3
#define UART_RX_INT_ENABLE 7
#define UART_TX_INT_ENABLE 6
#endif

/*============================================================================
 *                         PRIVATE FUNCTIONS
 *============================================================================*/

/**
 * @brief Calculate UBRR value for given baud rate
 */
static uint16_t calculate_ubrr(uint32_t baud_rate)
{
    return (uint16_t)((F_CPU / (16UL * baud_rate)) - 1);
}

/**
 * @brief Check if TX buffer is empty
 */
static inline bool uart_tx_buffer_empty(void)
{
    return (tx_head == tx_tail);
}

/**
 * @brief Check if RX buffer is empty
 */
static inline bool uart_rx_buffer_empty(void)
{
    return (rx_head == rx_tail);
}

/**
 * @brief Get number of bytes in RX buffer
 */
static inline uint16_t uart_rx_buffer_count(void)
{
    return (rx_head - rx_tail + UART_BUFFER_SIZE) % UART_BUFFER_SIZE;
}

/*============================================================================
 *                         PUBLIC FUNCTIONS
 *============================================================================*/

int uart_init(uint32_t baud_rate)
{
    uart_config_t config = {
        .baud_rate = baud_rate,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = 0 /* No parity */
    };
    return uart_init_config(&config);
}

int uart_init_config(const uart_config_t *config)
{
    if (config == NULL)
    {
        return -1;
    }

    uint16_t ubrr = calculate_ubrr(config->baud_rate);

#ifdef __AVR__
    /* Set baud rate */
    UART_BAUD_REG_H = (uint8_t)(ubrr >> 8);
    UART_BAUD_REG_L = (uint8_t)(ubrr & 0xFF);

    /* Enable receiver and transmitter */
    UART_CONTROL_REG_B = (1 << UART_RX_ENABLE) | (1 << UART_TX_ENABLE);

    /* Set frame format: 8 data bits, 1 stop bit, no parity */
    UART_CONTROL_REG_C = (1 << UCSZ01) | (1 << UCSZ00);

    /* Enable RX interrupt */
    UART_CONTROL_REG_B |= (1 << UART_RX_INT_ENABLE);
#endif

    /* Initialize buffers */
    rx_head = 0;
    rx_tail = 0;
    tx_head = 0;
    tx_tail = 0;

    uart_initialized = true;

    return 0;
}

void uart_tx_byte(uint8_t data)
{
    if (!uart_initialized)
        return;

#ifdef __AVR__
    /* Wait for empty transmit buffer */
    while (!(UART_STATUS_REG & (1 << UART_DATA_EMPTY)))
        ;

    /* Put data into buffer, sends the data */
    UART_DATA_REG = data;
#endif
}

uint8_t uart_rx_byte(void)
{
#ifdef __AVR__
    /* Wait for data to be received */
    while (!(UART_STATUS_REG & (1 << UART_RX_COMPLETE)))
        ;

    /* Get and return received data from buffer */
    return UART_DATA_REG;
#else
    return 0;
#endif
}

int uart_rx_byte_timeout(uint8_t *data, uint32_t timeout_ms)
{
    uint32_t start_time = hal_timer_get_ms();

    while ((hal_timer_get_ms() - start_time) < timeout_ms)
    {
#ifdef __AVR__
        if (UART_STATUS_REG & (1 << UART_RX_COMPLETE))
        {
            *data = UART_DATA_REG;
            return 0;
        }
#endif
    }

    return -1; /* Timeout */
}

void uart_tx_string(const char *str)
{
    if (str == NULL)
        return;

    while (*str)
    {
        uart_tx_byte((uint8_t)*str++);
    }
}

void uart_tx_buffer(const uint8_t *buffer, uint16_t length)
{
    if (buffer == NULL)
        return;

    for (uint16_t i = 0; i < length; i++)
    {
        uart_tx_byte(buffer[i]);
    }
}

void uart_printf(const char *format, ...)
{
    char buffer[128];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    uart_tx_string(buffer);
}

uint16_t uart_available(void)
{
    return uart_rx_buffer_count();
}

void uart_flush_tx(void)
{
#ifdef __AVR__
    /* Wait for all data to be transmitted */
    while (!(UART_STATUS_REG & (1 << UART_TX_COMPLETE)))
        ;
#endif
}

void uart_flush_rx(void)
{
#ifdef __AVR__
    uint8_t dummy;
    while (UART_STATUS_REG & (1 << UART_RX_COMPLETE))
    {
        dummy = UART_DATA_REG;
        (void)dummy; /* Suppress unused variable warning */
    }
#endif
    rx_head = 0;
    rx_tail = 0;
}

void uart_send_sensor_data(uint16_t angle, uint16_t distance)
{
    /* Protocol format: "angle,distance." */
    uart_printf("%u,%u.", angle, distance);
}

/*============================================================================
 *                         INTERRUPT HANDLERS
 *============================================================================*/

#ifdef __AVR__
/**
 * @brief UART Receive Complete Interrupt
 */
ISR(USART_RX_vect)
{
    uint8_t data = UART_DATA_REG;
    uint16_t next_head = (rx_head + 1) % UART_BUFFER_SIZE;

    /* Store data if buffer is not full */
    if (next_head != rx_tail)
    {
        rx_buffer[rx_head] = data;
        rx_head = next_head;
    }
    /* Otherwise, data is lost (buffer overflow) */
}
#endif
