/*

   SPI      SO     CLK
   SPI    GPIO19  GPIO18
*/



#define VOLT_PIN 34   /// 0-4095
#define HEATTER_PIN 13
#define HEATTER_CONTROL 4
#define HEATTER_TEMP 35
#define GSHEETUPDATE 60   // update in second

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID "TMPL6q23KVn0a"
#define BLYNK_TEMPLATE_NAME "petier"
#define BLYNK_AUTH_TOKEN "qkzrSj-Iuyv-7eEddbtGUn6ZZMFGVZWV"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include "MAX6675.h"

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "G6PD";
char pass[] = "570610193";

const int ThermoCouplesNum = 4;
MAX6675 ThermoCouples[ThermoCouplesNum] =
{
  MAX6675(33, &SPI),   //  HW SPI
  MAX6675(25, &SPI),   //  HW SPI
  MAX6675(26, &SPI),   //  HW SPI
  MAX6675(27, &SPI),   //  HW SPI
};

unsigned long previousMillis = 0, previousGsheet;       // will store last time LED was updated
float temp[4], volt;

void setup()
{
  Serial.begin(115200);

  digitalWrite(HEATTER_PIN, HIGH);
  pinMode(HEATTER_PIN, OUTPUT);

  SPI.begin();

  for (int i = 0; i < ThermoCouplesNum; i++)
  {
    ThermoCouples[i].begin();
    ThermoCouples[i].setSPIspeed(1000000);
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}


void loop()
{
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    uint8_t error = 0;
    volt = 0;
    for (int THCnumber = 0; THCnumber < ThermoCouplesNum; THCnumber++)
    {
      int status = ThermoCouples[THCnumber].read();
      error += status;

      float t = ThermoCouples[THCnumber].getTemperature();
      if (!status) {
        temp[THCnumber] = t;
        Serial.print("Temperature" + String(THCnumber + 1) + ": ");
        Serial.println(t);
      } else {
        Serial.println("Temp" + String(THCnumber + 1) + "error:" + String(status));
      }

      volt += analogRead(VOLT_PIN);

      delay(100);  //  time to flush all Serial stuff
    }

    volt = volt / ThermoCouplesNum;
    Serial.println("Volt:" + String(volt));

    if (!error) {
      if (temp[HEATTER_CONTROL - 1] > HEATTER_TEMP) {
        if (digitalRead(HEATTER_PIN) == LOW) Serial.println("heater on");
        digitalWrite(HEATTER_PIN, HIGH );
      } else if (temp[HEATTER_CONTROL - 1] < HEATTER_TEMP - 1) {
        if (digitalRead(HEATTER_PIN) == HIGH) Serial.println("heater off");
        digitalWrite(HEATTER_PIN, LOW);
      }
    }


    Serial.println("------------------");
    if (!error) {
      Blynk.virtualWrite(V0, temp[0]);
      Blynk.virtualWrite(V1, temp[1]);
      Blynk.virtualWrite(V2, temp[2]);
      Blynk.virtualWrite(V3, temp[3]);
    }
    Blynk.virtualWrite(V4, volt);

    if (currentMillis - previousGsheet >= GSHEETUPDATE * 1000) {
      previousGsheet = currentMillis;
      Serial.println("gsheet update");
      gsheet();
    }
  }
}

void gsheet() {
  WiFiClientSecure client;
  client.setInsecure(); // This disables SSL certificate validation

  HTTPClient https;

  String serverURL = "https://script.google.com/macros/s/AKfycbx_aDuA3ACO2HCGtyKPUSAldk_69eArUkAhw-jow6Yj0g41fBfZPJivrP4fbf0rDOjHrA/exec";
  serverURL += "?temp1=" + String(temp[0]);
  serverURL += "&temp2=" + String(temp[1]);
  serverURL += "&temp3=" + String(temp[2]);
  serverURL += "&temp4=" + String(temp[3]);
  serverURL += "&volt=" + String(volt);

  if (https.begin(client, serverURL)) { // Start the connection
    int httpCode = https.GET(); // Make a GET request

    // Check the returning code
    if (httpCode > 0) {
      Serial.printf("HTTPS GET... code: %d\n", httpCode);

      // Get the response payload
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end(); // Close the connection
  } else {
    Serial.println("Unable to connect");
  }
}
