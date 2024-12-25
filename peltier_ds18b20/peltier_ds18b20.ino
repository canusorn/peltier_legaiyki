/*
  https://github.com/canusorn/peltier_legaiyki
*/

#define GSHEETUPDATE 60   // เวลา update ขึ้น gsheet [sec]

#define VOLT_PIN 32        // pin ต่อ volt ของ peltier   0-4095
#define SAMPLE 50.0
#define RLOAD 22

//  pin สำหรับ temp sensor 1-4
#define TEMP_PIN_1 19
#define TEMP_PIN_2 21
#define TEMP_PIN_3 22
#define TEMP_PIN_4 23

// ใส่ชื่อ และรหัสผ่าน wifi ที่ต้องการเชื่อมต่อ
char *ssid = "WARAMET.J_2.4G";
char *pass = "0967289141t";

// ลิ้งของ google sheet
String gsheet_url = "https://script.google.com/macros/s/AKfycby8wfkzhUpx3kPQieQmcQxf3LB3oUCKd6tFjBQEvapaVKToHtKyi6J5aLbg94263iF0zw/exec";


#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWiretemp1(TEMP_PIN_1);
OneWire oneWiretemp2(TEMP_PIN_2);
OneWire oneWiretemp3(TEMP_PIN_3);
OneWire oneWiretemp4(TEMP_PIN_4);

DallasTemperature temp1(&oneWiretemp1);
DallasTemperature temp2(&oneWiretemp2);
DallasTemperature temp3(&oneWiretemp3);
DallasTemperature temp4(&oneWiretemp4);

unsigned long previousMillis = 0, previousGsheet;       // will store last time LED was updated
float temp[4], volt, curr;

void setup()
{
  Serial.begin(115200);

  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  temp1.begin();
  temp2.begin();
  temp3.begin();
  temp4.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void loop()
{

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    volt = 0;
    float Read[4];
    for (int i = 0; i < SAMPLE ; i++)
    {
      // อ่านค่าแรงดันจาก adc นำมาบวกหาค่าเฉลี่ย
      volt += analogRead(VOLT_PIN);
      delay(10);  //  time to flush all Serial stuff
    }

    // หาค่าเฉลี่ย peltier volt
    volt = volt / SAMPLE;
    //    Serial.print("analog:" + String(volt));
    volt = volt / 4095.0 * 3.3 * 1.18;
    curr = volt / RLOAD;

    temp1.requestTemperatures();
    temp[0] = temp1.getTempCByIndex(0);
    Serial.println("temp1 : " + String(temp[0]));

    temp2.requestTemperatures();
    temp[1] = temp2.getTempCByIndex(0);
    Serial.println("temp2 : " + String(temp[1]));

    temp3.requestTemperatures();
    temp[2] = temp3.getTempCByIndex(0);
    Serial.println("temp3 : " + String(temp[2]));

    temp4.requestTemperatures();
    temp[3] = temp4.getTempCByIndex(0);
    Serial.println("temp4 : " + String(temp[3]));

    Serial.println("Volt:" + String(volt) + "V\tCurr:" + String(curr, 3) + "A");

    Serial.println("------------------");

    // อัพเดทค่าขึ้น google sheet
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

  String serverURL = gsheet_url;
  serverURL += "?temp1=" + String(temp[0], 0);
  serverURL += "&temp2=" + String(temp[1], 0);
  serverURL += "&temp3=" + String(temp[2], 0);
  serverURL += "&temp4=" + String(temp[3], 0);
  serverURL += "&volt=" + String(volt, 2);
  serverURL += "&curr=" + String(curr, 3);
  serverURL += "&resis=" + String(RLOAD);
  //  Serial.println(serverURL);

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