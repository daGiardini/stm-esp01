// MQTT Function Implementations with UART Interrupt Support
// https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/AT_Command_Set/MQTT_AT_Commands.html

#include "mqtt.h"
#include <string.h>
#include <stdio.h>
#include "esp8266.h"

mqtt_result_t MQTT_CleanSession(void)
{
    esp8266_response_t response = ESP8266_SendAndWaitResponse("AT+MQTTCLEAN=0\r\n", 5000);

    if (response.status == ESP8266_OK)
    {
        mqtt_result_t mqtt_response = {
            .status = response.status,
            .message = "MQTT - session cleaned successfully\r\n"};
        return mqtt_response;
    }

    mqtt_result_t mqtt_response = {
        .status = response.status,
        .message = "MQTT - session clean failed\r\n"};
    return mqtt_response;
}

mqtt_result_t MQTT_Init(char *clientId)
{
    char command[200];
    snprintf(command, sizeof(command), "AT+MQTTUSERCFG=0,1,\"%s\",\"\",\"\",0,0,\"\"\r\n", clientId);
    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 5000);

    if (response.status != ESP8266_OK)
    {
        mqtt_result_t mqtt_response = {
            .status = response.status,
            .message = "MQTT - initialization failed\r\n"};
        return mqtt_response;
    }

    mqtt_result_t mqtt_response = {
        .status = response.status,
        .message = "MQTT - initialized successfully\r\n"};
    return mqtt_response;
}

mqtt_result_t MQTT_Connect(char *broker, uint16_t port)
{
    char command[200];
    snprintf(command, sizeof(command), "AT+MQTTCONN=0,\"%s\",%d,1\r\n", broker, port);
    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 5000);

    if (response.status != ESP8266_OK)
    {
        mqtt_result_t mqtt_response = {
            .status = response.status,
            .message = "MQTT - connection failed\r\n"};
        return mqtt_response;
    }

    mqtt_result_t mqtt_response = {
        .status = response.status,
        .message = "MQTT - connected successfully\r\n"};

    return mqtt_response;
}

mqtt_result_t MQTT_Publish(char *topic, char *message, uint8_t qos, uint8_t retain)
{
    char command[300];
    snprintf(command, sizeof(command), "AT+MQTTPUB=0,\"%s\",\"%s\",%d,%d\r\n", topic, message, qos, retain);
    esp8266_response_t response = ESP8266_Send(command);

    if (response.status != ESP8266_OK)
    {
        mqtt_result_t mqtt_response = {
            .status = response.status,
            .message = "MQTT - publish failed\r\n"};
        return mqtt_response;
    }

    mqtt_result_t mqtt_response = {
        .status = response.status,
        .message = "MQTT - published successfully\r\n"};
    return mqtt_response;
}

mqtt_result_t MQTT_Subscribe(char *topic, uint8_t qos)
{
    char command[200];
    snprintf(command, sizeof(command), "AT+MQTTSUB=0,\"%s\",%d\r\n", topic, qos);
    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 5000);

    if (response.status != ESP8266_OK)
    {
        mqtt_result_t mqtt_response = {
            .status = response.status,
            .message = "MQTT - subscribe failed\r\n"};
        return mqtt_response;
    }

    mqtt_result_t mqtt_response = {
        .status = response.status,
        .message = "MQTT - subscribed successfully\r\n"};
    return mqtt_response;
}

mqtt_result_t MQTT_Unsubscribe(char *topic)
{
    char command[200];
    snprintf(command, sizeof(command), "AT+MQTTUNSUB=0,\"%s\"\r\n", topic);
    esp8266_response_t response = ESP8266_SendAndWaitResponse(command, 5000);

    if (response.status != ESP8266_OK)
    {
        mqtt_result_t mqtt_response = {
            .status = response.status,
            .message = "MQTT - unsubscribe failed\r\n"};
        return mqtt_response;
    }

    mqtt_result_t mqtt_response = {
        .status = response.status,
        .message = "MQTT - unsubscribed successfully\r\n"};
    return mqtt_response;
}

mqtt_received_message_t MQTT_ParseReceivedMessage(char *raw_message)
{
    mqtt_received_message_t parsed = {0}; // Initialize all fields to 0

    // Try direct format first: "+MQTTSUBRECV:"
    char *mqtt_start = NULL;

    if (strncmp(raw_message, "+MQTTSUBRECV:", 13) == 0)
    {
        // Direct format: "+MQTTSUBRECV:..."
        mqtt_start = raw_message;
    }
    else
    {
        // Search for the pattern anywhere in the message (handles "A+MQTTSUBRECV:" format)
        mqtt_start = strstr(raw_message, "+MQTTSUBRECV:");
    }

    if (mqtt_start == NULL)
    {
        parsed.valid = 0;
        return parsed;
    }

    const char *msg_start = raw_message + 13; // Skip "+MQTTSUBRECV:"
    const char *comma1 = strchr(msg_start, ',');
    if (!comma1)
    {
        parsed.valid = 0;
        return parsed;
    }

    const char *comma2 = strchr(comma1 + 1, ',');
    if (!comma2)
    {
        parsed.valid = 0;
        return parsed;
    }

    const char *comma3 = strchr(comma2 + 1, ',');
    if (!comma3)
    {
        parsed.valid = 0;
        return parsed;
    }

    // Parse Link ID
    parsed.link_id = (uint8_t)atoi(msg_start);

    // Parse Topic
    const char *topic_start = comma1 + 1;
    const char *topic_end = comma2;

    // Skip opening quote
    if (*topic_start == '"')
        topic_start++;

    // Find closing quote
    const char *quote_end = strchr(topic_start, '"');
    if (quote_end && quote_end < topic_end)
        topic_end = quote_end;

    int topic_len = topic_end - topic_start;
    if (topic_len > 0 && topic_len < sizeof(parsed.topic))
    {
        strncpy(parsed.topic, topic_start, topic_len);
        parsed.topic[topic_len] = '\0';
    }

    // Parse Message Length
    parsed.message_length = (uint16_t)atoi(comma2 + 1);

    // Parse Message
    const char *message_start = comma3 + 1;
    strncpy(parsed.message, message_start, sizeof(parsed.message) - 1);
    parsed.message[sizeof(parsed.message) - 1] = '\0';

    // Remove trailing \r\n if present
    int msg_len = strlen(parsed.message);
    if (msg_len > 0 && parsed.message[msg_len - 1] == '\n')
        parsed.message[msg_len - 1] = '\0';
    if (msg_len > 1 && parsed.message[msg_len - 2] == '\r')
        parsed.message[msg_len - 2] = '\0';

    parsed.valid = 1;
    return parsed;
}