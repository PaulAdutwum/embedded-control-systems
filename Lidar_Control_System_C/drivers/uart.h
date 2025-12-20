/**
 * @file uart.h
 * @brief UART Driver for Serial Communication
 * @author Paul Adutwum
 * @date 2025
 *
 * Provides UART communication interface for sensor data transmission.
 */

#ifndef UART_H
#define UART_H

#include "../include/config.h"

/*============================================================================
 *                         UART CONFIGURATION
 *============================================================================*/

typedef struct
{
    uint32_t baud_rate;
    uint8_t data_bits; /* 5, 6, 7, or 8 */
    uint8_t stop_bits; /* 1 or 2 */
    uint8_t parity;    /* 0=none, 1=even, 2=odd */
} uart_config_t;

/*============================================================================
 *                         UART FUNCTIONS
 *============================================================================*/

/**
 * @brief Initialize UART with default configuration
 * @param baud_rate Baud rate (e.g., 9600, 115200)
 * @return 0 on success, -1 on error
 */
int uart_init(uint32_t baud_rate);

/**
 * @brief Initialize UART with custom configuration
 * @param config UART configuration structure
 * @return 0 on success, -1 on error
 */
int uart_init_config(const uart_config_t *config);

/**
 * @brief Transmit a single byte
 * @param data Byte to transmit
 */
void uart_tx_byte(uint8_t data);

/**
 * @brief Receive a single byte (blocking)
 * @return Received byte
 */
uint8_t uart_rx_byte(void);

/**
 * @brief Receive a byte with timeout
 * @param data Pointer to store received byte
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, -1 on timeout
 */
int uart_rx_byte_timeout(uint8_t *data, uint32_t timeout_ms);

/**
 * @brief Transmit a string
 * @param str Null-terminated string
 */
void uart_tx_string(const char *str);

/**
 * @brief Transmit a buffer
 * @param buffer Data buffer
 * @param length Buffer length
 */
void uart_tx_buffer(const uint8_t *buffer, uint16_t length);

/**
 * @brief Print formatted string (printf-like)
 * @param format Format string
 * @param ... Variable arguments
 */
void uart_printf(const char *format, ...);

/**
 * @brief Check if data is available to read
 * @return Number of bytes available
 */
uint16_t uart_available(void);

/**
 * @brief Flush transmit buffer
 */
void uart_flush_tx(void);

/**
 * @brief Flush receive buffer
 */
void uart_flush_rx(void);

/**
 * @brief Send sensor data in protocol format
 * @param angle Current angle in degrees
 * @param distance Distance in centimeters
 */
void uart_send_sensor_data(uint16_t angle, uint16_t distance);

#endif /* UART_H */
