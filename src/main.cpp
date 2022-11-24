#include <Arduino.h>
#include <UNIT_UHF_RFID.h>
#include <WiFi.h>
#include <PubSubClient.h>

/* Contient :
   const char *ssid = "SSID_NAME";
   const char *password = "PASSWORD";
 */
#include "./secret/ssid_password.h"

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
    Serial.println("UHF RFID Reader info:");
    Serial.println("Version: " + uhf.getVersion());
    Serial.println("Select: " + uhf.selectInfo());
    Serial.println("UHF RFID Reader ready!\n");
    IPAddress ip = initWiFi();
    Serial.print("WiFi connected: ");
    Serial.println(ip);
}

void loop()
{
    uint8_t card_count = uhf.pollingMultiple(4);
    Serial.print("\nCount: ");
    Serial.println(card_count);
    if (card_count > 0)
    {
        for (int i = 0; i < card_count; i++)
        {
            Serial.print("RSSI: ");
            Serial.print(uhf.cards[i].rssi_str);
            Serial.print(" -- PC: ");
            Serial.print(uhf.cards[i].pc_str);
            Serial.print(" -- EPC: ");
            Serial.println(uhf.cards[i].epc_str);
            sendToMqtt(TOPIC, uhf.cards[i].epc_str);
        }
    }
    delay(100);
}