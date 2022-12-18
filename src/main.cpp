#include <Arduino.h>
#include <UNIT_UHF_RFID.h>
#include <WiFi.h>
#include <PubSubClient.h>

/* Contains the SSID and PASSWORD of the WiFi network, and the MQTT that are used topics :
   const char *ssid = "SSID_NAME";
   const char *password = "PASSWORD";
   const char *TOPIC = "TOPIC";
   const char *ALIVE_TOPIC = "ALIVE_TOPIC";
 */
#include "./secret/ssid_password.h"

#define MAX_BUFFER 20

Unit_UHF_RFID uhf;
WiFiClient wifiClient;
PubSubClient mqttClient("test.mosquitto.org", 1883, wifiClient);

IPAddress initWiFi()
{
    WiFi.mode(WIFI_STA); // STA = Station = Client
    WiFi.begin(SSID, PASSWORD);

    while (WiFi.status() != WL_CONNECTED) // TODO: Put a LED to show the status
    {
        Serial.print('.');
        delay(100);
    }

    return WiFi.localIP();
}

bool mqttConnect()
{
    mqttClient.setBufferSize(1024);
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Reconnecting to WiFi...");
        initWiFi();
    }
    bool connected = mqttClient.connected();
    if (!connected)
    {
        Serial.print("Connecting to MQTT...");
        mqttClient.connect("esp32_NicoToff");
        connected = mqttClient.connected();
        connected ? Serial.println(" Connected!") : Serial.println(" FAILED!!!");
    }
    return connected;
}

bool sendToMqtt(String topic, String message)
{
    bool sent = false;
    if (mqttConnect())
    {
        sent = mqttClient.publish(topic.c_str(), message.c_str());
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
    uhf.begin(&Serial2, 115200, GPIO_NUM_16, GPIO_NUM_17, false);
    Serial.println("UHF RFID Reader ready!\n");
    Serial.print("Connecting to WiFi..");
    IPAddress ip = initWiFi();
    Serial.print("WiFi connected: ");
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

unsigned long lastSendingVerif = 0;
unsigned long lastSent = 0;

String epc[20];
int epcCount = 0;

void loop()
{
    uint8_t card_count = uhf.pollingMultiple(4);
    Serial.print("\nCount: ");
    Serial.println(card_count);

    if (card_count > 0)
    {

        for (int i = 0; i < card_count; i++)
        {
            // If the "uhf.cards[i].epc_str" isn't in the "epc" array, add it
            if (!isInArray(epc, MAX_BUFFER, uhf.cards[i].epc_str))
            {
                epc[epcCount] = uhf.cards[i].epc_str;
                epcCount++;
            }
        }

        if (millis() - lastSendingVerif > 5000)
        {
            lastSendingVerif = millis();
            // Concatenate all the EPCs in a single string
            String message = "";
            for (int i = 0; i < epcCount; i++)
            {
                message.concat(epc[i]);
                if (i < epcCount - 1)
                {
                    message.concat(";");
                }
            }
            Serial.print("Sending to MQTT: ");
            Serial.println(message);
            if (message != "")
            {
                sendToMqtt(TOPIC, message);
                // Clear the epc array
                for (int i = 0; i < epcCount; i++)
                {
                    epc[i] = "";
                }
                lastSent = millis();
            }
            lastSendingVerif = millis();
            epcCount = 0;
        }
    }
    if (millis() - lastSent > 15000)
    {
        lastSent = millis();
        Serial.println("Saying Hi! (Alive)");
        sendToMqtt(ALIVE_TOPIC, "Hi");
    }
    delay(20);
}