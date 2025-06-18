#ifndef MQTT_H
#define MQTT_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp8266.h"

// MQTT client configuration structure
typedef struct
{
    char broker[100];
    uint16_t port;
    char clientId[50];
} MQTT_InitTypeDef_t;

typedef struct
{
    esp8266_status_t status;
    char message[128];
} mqtt_result_t;

typedef struct
{
    uint8_t link_id;
    char topic[64];
    uint16_t message_length;
    char message[256];
    uint8_t valid;
} mqtt_received_message_t;

// Function prototypes
mqtt_result_t MQTT_CleanSession(void);
mqtt_result_t MQTT_Init(char *clientId);
mqtt_result_t MQTT_Connect(char *broker, uint16_t port);
mqtt_result_t MQTT_Subscribe(char *topic, uint8_t qos);
mqtt_result_t MQTT_Publish(char *topic, char *message, uint8_t qos, uint8_t retain);
mqtt_result_t MQTT_Unsubscribe(char *topic);
mqtt_received_message_t MQTT_ParseReceivedMessage(char *received_data);

#endif // MQTT_H