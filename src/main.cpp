#include <Arduino.h>
#include <UNIT_UHF_RFID.h>

Unit_UHF_RFID uhf;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    };
    uhf.begin(&Serial2, 115200, GPIO_NUM_16, GPIO_NUM_17, false);
    Serial.println("UHF RFID Reader info:");
    Serial.println("Version: " + uhf.getVersion());
    Serial.println("Select: " + uhf.selectInfo());
    Serial.println("UHF RFID Reader ready!\n");
}

void loop()
{
    uint8_t card_count = uhf.pollingMultiple(5);
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
        }
    }
    delay(1500);
}