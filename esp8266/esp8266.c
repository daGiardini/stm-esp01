// ESP8266 Function Implementations - Enhanced with Proper Response Handling
// https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/Wi-Fi_AT_Commands.html
#include "esp8266.h"

static UART_HandleTypeDef esp_uart;

// Buffer for receiving responses
#define ESP8266_RX_BUFFER_SIZE 512
static uint8_t rx_buffer[ESP8266_RX_BUFFER_SIZE];

// Private function declarations
static esp8266_status_t ESP8266_ParseResponse(const char *response, const char *command_type);
static void ESP8266_ClearBuffer(void);

esp8266_result_t ESP8266_Init(UART_HandleTypeDef *uartHandle)
{
    esp8266_result_t result = {0};

    if (uartHandle == NULL)
    {
        result.status = ESP8266_ERROR_INVALID_PARAM;
        strcpy(result.message, "ESP8266 - NULL UART handle provided\r\n");
        return result;
    }

    esp_uart = *uartHandle;

    // Initialize the UART peripheral
    if (HAL_UART_Init(&esp_uart) != HAL_OK)
    {
        result.status = ESP8266_ERROR_UART;
        strcpy(result.message, "ESP8266 - UART initialization failed\r\n");
        return result;
    }

    // Wait for module to boot up
    HAL_Delay(2000);

    // Send initial AT command to check if the module is responsive
    for (int i = 0; i < 3; i++)
    {
        esp8266_response_t response = ESP8266_SendAndWaitResponse("AT\r\n", 1000);

        if (response.status == ESP8266_OK)
        {
            result.status = ESP8266_OK;
            strcpy(result.message, "ESP8266 - initialized successfully\r\n");
            return result;
        }

        HAL_Delay(500);
    }

    result.status = ESP8266_ERROR_NO_RESPONSE;
    strcpy(result.message, "ESP8266 - No response from module\r\n");
    return result;
}

esp8266_result_t ESP8266_Restart(void)
{
    esp8266_result_t result = {0};

    esp8266_response_t response = ESP8266_SendAndWaitResponse("AT+RST\r\n", 1000);

    if (response.status != ESP8266_OK)
    {
        result.status = response.status;
        strcpy(result.message, "ESP8266 - Failed to send restart command\r\n");
        return result;
    }

    // Wait for restart to complete and look for "ready"
    HAL_Delay(3000);

    // Check if module is ready after restart
    response = ESP8266_SendAndWaitResponse("AT\r\n", 2000);

    result.status = response.status;
    if (response.status == ESP8266_OK)
    {
        strcpy(result.message, "ESP8266 - restarted successfully\r\n");
    }
    else
    {
        strcpy(result.message, "ESP8266 - restart failed or module not responding\r\n");
    }

    return result;
}

esp8266_result_t ESP8266_SetMode(esp8266_mode_t mode)
{
    esp8266_result_t result = {0};

    if (mode < ESP8266_MODE_STATION || mode > ESP8266_MODE_SOFTAP_STATION)
    {
        result.status = ESP8266_ERROR_INVALID_PARAM;
        strcpy(result.message, "WiFi - Invalid mode specified\r\n");
        return result;
    }

    char command[32];
    snprintf(command, sizeof(command), "AT+CWMODE=%d\r\n", mode);

    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 3000);

    result.status = response.status;

    if (response.status == ESP8266_OK)
    {
        const char *mode_names[] = {"Null", "Station", "SoftAP", "Station+SoftAP"};
        snprintf(result.message, sizeof(result.message) + 2,
                 "WiFi - mode set to %s\r\n", mode_names[mode]);
    }
    else
    {
        strcpy(result.message, "WiFi - Failed to set mode\r\n");
    }

    return result;
}

esp8266_result_t ESP8266_ConnectToWiFi(const char *ssid, const char *password)
{
    esp8266_result_t result = {0};

    if (ssid == NULL || password == NULL)
    {
        result.status = ESP8266_ERROR_INVALID_PARAM;
        strcpy(result.message, "WiFi - NULL SSID or password provided\r\n");
        return result;
    }

    if (strlen(ssid) == 0 || strlen(ssid) > 32)
    {
        result.status = ESP8266_ERROR_INVALID_PARAM;
        strcpy(result.message, "WiFi - Invalid SSID length (1-32 characters)\r\n");
        return result;
    }

    // First disconnect from any existing connection
    ESP8266_DisconnectWiFi();
    HAL_Delay(1000);

    char command[128];
    int len = snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

    if (len >= sizeof(command))
    {
        result.status = ESP8266_ERROR_BUFFER_OVERFLOW;
        strcpy(result.message, "Command too long for buffer\r\n");
        return result;
    }

    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 15000);

    result.status = response.status;

    switch (response.status)
    {
    case ESP8266_OK:
        strcpy(result.message, "WiFi - Connected successfully\r\n");
        break;

    case ESP8266_ERROR_WIFI_WRONG_PASSWORD:
        strcpy(result.message, "WiFi - Wrong password\r\n");
        break;

    case ESP8266_ERROR_WIFI_NOT_FOUND:
        strcpy(result.message, "WiFi - Network not found\r\n");
        break;

    case ESP8266_ERROR_WIFI_CONN_FAIL:
        strcpy(result.message, "WiFi - Connection failed\r\n");
        break;

    case ESP8266_ERROR_TIMEOUT:
        strcpy(result.message, "WiFi - Connection timeout\r\n");
        break;

    default:
        strcpy(result.message, "WiFi - Unknown connection error\r\n");
        break;
    }

    return result;
}

esp8266_result_t ESP8266_DisconnectWiFi(void)
{
    esp8266_result_t result = {0};

    esp8266_response_t response = ESP8266_SendAndWaitResponse("AT+CWQAP\r\n", 5000);

    result.status = response.status;

    if (response.status == ESP8266_OK)
    {
        strcpy(result.message, "WiFi - Disconnected\r\n");
    }
    else
    {
        strcpy(result.message, "WiFi - Failed to disconnect\r\n");
    }

    return result;
}

esp8266_result_t ESP8266_SetAutoConnect(void)
{
    esp8266_result_t result = {0};

    char command[32];
    snprintf(command, sizeof(command), "AT+CWAUTOCONN=1\r\n");

    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 2000);

    if (response.status == ESP8266_OK)
    {
        strcpy(result.message, "WiFi - Auto-connect mode enabled\r\n");
    }
    else
    {
        strcpy(result.message, "WiFi - Failed to enable auto-connect mode\r\n");
    }

    return result;
}

// Private helper functions
esp8266_response_t ESP8266_SendAndWaitResponse(const char *command, uint32_t timeout_ms)
{
    esp8266_response_t response = {0};

    if (command == NULL)
    {
        response.status = ESP8266_ERROR_INVALID_PARAM;
        return response;
    }

    ESP8266_ClearBuffer();

    // Send command
    HAL_StatusTypeDef uart_status = HAL_UART_Transmit(&esp_uart, (uint8_t *)command,
                                                      strlen(command), 1000);
    if (uart_status != HAL_OK)
    {
        response.status = ESP8266_ERROR_UART;
        return response;
    }

    // Wait for response
    uint32_t start_time = HAL_GetTick();
    uint16_t rx_index = 0;

    while ((HAL_GetTick() - start_time) < timeout_ms)
    {
        uint8_t received_char;

        if (HAL_UART_Receive(&esp_uart, &received_char, 1, 10) == HAL_OK)
        {
            if (rx_index < (ESP8266_RX_BUFFER_SIZE - 1))
            {
                rx_buffer[rx_index++] = received_char;
                rx_buffer[rx_index] = '\0';

                // Check for complete response (ends with OK, ERROR, or FAIL)
                if (strstr((char *)rx_buffer, "\r\nOK\r\n") ||
                    strstr((char *)rx_buffer, "\r\nERROR\r\n") ||
                    strstr((char *)rx_buffer, "\r\nFAIL\r\n"))
                {
                    break;
                }
            }
            else
            {
                // Buffer overflow
                response.status = ESP8266_ERROR_BUFFER_OVERFLOW;
                return response;
            }
        }
    }

    // Copy response data
    response.data_length = rx_index;
    if (rx_index < sizeof(response.data))
    {
        strcpy(response.data, (char *)rx_buffer);
    }

    // Parse the response to determine status
    response.status = ESP8266_ParseResponse((char *)rx_buffer, command);

    return response;
}

esp8266_response_t ESP8266_Send(const char *command)
{
    esp8266_response_t response = {0};

    if (command == NULL)
    {
        response.status = ESP8266_ERROR_INVALID_PARAM;
        return response;
    }

    ESP8266_ClearBuffer();

    // Send command
    HAL_StatusTypeDef uart_status = HAL_UART_Transmit(&esp_uart, (uint8_t *)command,
                                                      strlen(command), 1000);
    if (uart_status != HAL_OK)
    {
        response.status = ESP8266_ERROR_UART;
        return response;
    }

    // No waiting for response, just return success
    response.status = ESP8266_OK;

    return response;
}

static esp8266_status_t ESP8266_ParseResponse(const char *response, const char *command_type)
{
    if (response == NULL)
    {
        return ESP8266_ERROR_NO_RESPONSE;
    }

    // Check for standard responses
    if (strstr(response, "OK"))
    {
        return ESP8266_OK;
    }

    if (strstr(response, "ERROR"))
    {
        return ESP8266_ERROR_COMMAND;
    }

    if (strstr(response, "FAIL"))
    {
        return ESP8266_ERROR_COMMAND;
    }

    // Check for WiFi-specific errors
    if (strstr(response, "+CWJAP:1"))
        return ESP8266_ERROR_TIMEOUT;
    if (strstr(response, "+CWJAP:2"))
        return ESP8266_ERROR_WIFI_WRONG_PASSWORD;
    if (strstr(response, "+CWJAP:3"))
        return ESP8266_ERROR_WIFI_NOT_FOUND;
    if (strstr(response, "+CWJAP:4"))
        return ESP8266_ERROR_WIFI_CONN_FAIL;

    // If we get here, we might have timed out
    return ESP8266_ERROR_TIMEOUT;
}

static void ESP8266_ClearBuffer(void)
{
    memset(rx_buffer, 0, sizeof(rx_buffer));
}
