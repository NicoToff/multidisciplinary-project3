#include <Arduino.h>
#include <UNIT_UHF_RFID.h>
#include <WiFi.h>
#include <PubSubClient.h>
/* The following file contains these variables:
    const char *SSID;
    const char *PASSWORD;
    const char *MICHAUX_MQTT;
    const char *MICHAUX_MQTT_USERNAME;
    const char *MICHAUX_MQTT_PASSWORD;
    const int MICHAUX_MQTT_PORT;
    const char *EPC_DISCOVERED_TOPIC;
    const char *ALIVE_TOPIC;
 */
#include "./secret/ssid_password.h"

#define WIFI_LED 33
#define MQTT_LED 32

#define MAX_BUFFER 30

Unit_UHF_RFID uhf;
WiFiClient wifiClient;
PubSubClient mqttClient(MICHAUX_MQTT, MICHAUX_MQTT_PORT, wifiClient);

unsigned long lastEpcSent = 0;
unsigned long lastContact = 0;

String epc[MAX_BUFFER];
int epcCount = 0;

IPAddress initWiFi()
{
    digitalWrite(WIFI_LED, LOW);
    digitalWrite(MQTT_LED, LOW);
    WiFi.mode(WIFI_STA); // STA = Station = Client
    WiFi.begin(SSID, PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(WIFI_LED, !digitalRead(WIFI_LED)); // LED blinks when (re))connecting
        delay(100);
    }

    digitalWrite(WIFI_LED, HIGH);
    return WiFi.localIP();
}

bool mqttConnect()
{
    mqttClient.setBufferSize(1024);
    if (WiFi.status() != WL_CONNECTED)
    {
        initWiFi();
    }
    bool connected = mqttClient.connected();
    if (!connected)
    {
        digitalWrite(MQTT_LED, LOW);
        mqttClient.connect("esp32_NicoToff", MICHAUX_MQTT_USERNAME, MICHAUX_MQTT_PASSWORD);
        connected = mqttClient.connected();
        if (connected)
        {
            digitalWrite(MQTT_LED, HIGH);
        }
        else
        {
            Serial.println("MQTT connect FAILED!");
        }
    }
    return connected;
}

bool sendToMqtt(String topic, String message, bool retain = false)
{
    bool sent = false;
    if (mqttConnect())
    {
        sent = mqttClient.publish(topic.c_str(), message.c_str(), retain);
    }
    if (!sent)
    {
        Serial.println("Couldn't send to MQTT!");
    }
    return sent;
}

void setup()
{
    Serial.begin(115200);
    pinMode(WIFI_LED, OUTPUT);
    pinMode(MQTT_LED, OUTPUT);
    uhf.begin(&Serial2, 115200, GPIO_NUM_16, GPIO_NUM_17, false);
    IPAddress ip = initWiFi();
    Serial.print("WiFi connected on: ");
    Serial.println(ip);
    sendToMqtt(ALIVE_TOPIC, "Hi");
}

boolean isInArray(String *array, int arraySize, String value)
{
    for (int i = 0; i < arraySize; i++)
    {
        if (array[i] == value)
        {
            return true;
        }
    }
    return false;
}

void loop()
{
    uint8_t tagCount = uhf.pollingMultiple(4);
    Serial.print("\nTag count: ");
    Serial.println(tagCount);

    if (tagCount > 0)
    {
        for (int i = 0; i < tagCount; i++)
        {
            // If the "uhf.cards[i].epc_str" isn't in the "epc" array, add it
            if (!isInArray(epc, MAX_BUFFER, uhf.cards[i].epc_str) && epcCount < MAX_BUFFER)
            {
                epc[epcCount] = uhf.cards[i].epc_str;
                epcCount++;
            }
        }

        if (millis() - lastEpcSent > 5000)
        {
            lastEpcSent = millis();
            // Concatenate all the EPCs in a single string
            String message = "";
            for (int i = 0; i < epcCount; i++)
            {
                message.concat(epc[i]);
                if (i < epcCount - 1) // "if not last element"
                {
                    message.concat(";");
                }
            }
            Serial.print("Sending to MQTT: ");
            Serial.println(message);
            if (message != "")
            {
                sendToMqtt(EPC_DISCOVERED_TOPIC, message);
                // Clear the epc array
                for (int i = 0; i < epcCount; i++)
                {
                    epc[i] = "";
                }
                lastContact = millis();
            }
            lastEpcSent = millis();
            epcCount = 0;
        }
    }
    if (millis() - lastContact > 15000)
    {
        lastContact = millis();
        sendToMqtt(ALIVE_TOPIC, "Hi");
    }
    delay(20);
}