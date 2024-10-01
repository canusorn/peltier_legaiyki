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

const int ThermoCouplesNum = 1;
MAX6675 ThermoCouples[ThermoCouplesNum] =
{
  MAX6675(5, &SPI),   //  HW SPI
};

unsigned long previousMillis = 0;        // will store last time LED was updated
uint16_t gsheetInterval;
float temp[4];
float volt = 2.1;

void setup()
{
  Serial.begin(115200);

  SPI.begin();

  for (int i = 0; i < ThermoCouplesNum; i++)
  {
    ThermoCouples[i].begin();
    ThermoCouples[i].setSPIspeed(4000000);
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}


void loop()
{
  Blynk.run();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    gsheetInterval++;

    for (int THCnumber = 0; THCnumber < ThermoCouplesNum; THCnumber++)
    {
      int status = ThermoCouples[THCnumber].read();
      float t = ThermoCouples[THCnumber].getTemperature();
      temp[THCnumber] = t;
      Serial.print("temp" + String(THCnumber) + ": ");
      Serial.println(t);

      //      delay(100);  //  time to flush all Serial stuff
    }

    Blynk.virtualWrite(V0, temp[0]);
    Blynk.virtualWrite(V1, temp[1]);
    Blynk.virtualWrite(V2, temp[2]);
    Blynk.virtualWrite(V3, temp[3]);

    if (gsheetInterval >= GSHEETUPDATE) {
      gsheetInterval = 0;
gsheet();
      Serial.println("gsheet update");
    }
  }
}

void gsheet() {
  WiFiClientSecure client;
  client.setInsecure(); // This disables SSL certificate validation

  HTTPClient https;

  String serverURL = "https://script.google.com/macros/s/AKfycby8GBnsweci9Yd0INXaivmouKPz0BsxRJXUKucj1Oq8feIyDGniExQO8AU8QVirU82a/exec";
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
