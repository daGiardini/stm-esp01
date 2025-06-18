// ESP8266 Driver Header - Enhanced with Proper Return Types
#ifndef ESP8266_H
#define ESP8266_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Status codes based on actual AT command responses
typedef enum
{
    ESP8266_OK = 0,
    ESP8266_ERROR_COMMAND,
    ESP8266_ERROR_TIMEOUT,
    ESP8266_ERROR_NO_RESPONSE,
    ESP8266_ERROR_UART,
    ESP8266_ERROR_INVALID_PARAM,
    ESP8266_ERROR_BUFFER_OVERFLOW,
    ESP8266_ERROR_WIFI_WRONG_PASSWORD,
    ESP8266_ERROR_WIFI_NOT_FOUND,
    ESP8266_ERROR_WIFI_CONN_FAIL,
    ESP8266_ERROR_WIFI_ALREADY_CONNECTED
} esp8266_status_t;

// WiFi modes
typedef enum
{
    ESP8266_MODE_NULL = 0,
    ESP8266_MODE_STATION = 1,
    ESP8266_MODE_SOFTAP = 2,
    ESP8266_MODE_SOFTAP_STATION = 3
} esp8266_mode_t;

// Response structure
typedef struct
{
    esp8266_status_t status;
    char data[256];
    uint16_t data_length;
} esp8266_response_t;

// Basic result structure
typedef struct
{
    esp8266_status_t status;
    char message[128];
} esp8266_result_t;

// Function declarations with enhanced return types
esp8266_result_t ESP8266_Init(UART_HandleTypeDef *uartHandle);
esp8266_result_t ESP8266_Restart(void);
esp8266_result_t ESP8266_SetMode(esp8266_mode_t mode);
esp8266_result_t ESP8266_ConnectToWiFi(const char *ssid, const char *password);
esp8266_result_t ESP8266_DisconnectWiFi(void);
esp8266_result_t ESP8266_SetAutoConnect(void);
esp8266_response_t ESP8266_SendAndWaitResponse(const char *command, uint32_t timeout_ms);
esp8266_response_t ESP8266_Send(const char *command);

#endif // ESP8266_H