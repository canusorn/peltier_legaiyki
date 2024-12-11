/*
  https://github.com/canusorn/peltier_legaiyki
*/

#define GSHEETUPDATE 60   // เวลา update ขึ้น gsheet [sec]

#define VOLT_PIN 32        // pin ต่อ volt ของ peltier   0-4095
#define SAMPLE 50.0
#define RLOAD 22

//  pin สำหรับ temp sensor 1-4
#define TEMP_PIN_1 36
#define TEMP_PIN_2 39
#define TEMP_PIN_3 34
#define TEMP_PIN_4 35

// ใส่ชื่อ และรหัสผ่าน wifi ที่ต้องการเชื่อมต่อ
char *ssid = "WARAMET.J_2.4G";
char *pass = "0967289141t";

// ลิ้งของ google sheet
String gsheet_url = "https://script.google.com/macros/s/AKfycbwSdXorMo-fpIbu1VsOgLwU2U8OUW-1ZdnTy_8nkuPBOagK0U-L2KdvKjv66qFaBWT0vQ/exec";


#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>


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
      Read[0] += analogRead(TEMP_PIN_1);
      Read[1] += analogRead(TEMP_PIN_2);
      Read[2] += analogRead(TEMP_PIN_3);
      Read[3] += analogRead(TEMP_PIN_4);

      delay(10);  //  time to flush all Serial stuff
    }

    for (int i = 0; i < 4 ; i++)
    {
      float Vr, Rt, ln, T0 = 25 + 273.15;
      Read[i] /= SAMPLE;
      Read[i] = (3.3 / 4095.0) * Read[i];
      Vr = 3.3 - Read[i];
      Rt = Read[i] / (Vr / 6300);
      ln = log(Rt / 10000);
      temp[i] = (1 / ((ln / 3950) + (1 / T0)));
      temp[i] = temp[i] - 273.15;
      temp[i] = temp[i] * 1.3 + 32; // calibrate
      Serial.println("Temp" + String(i) + ": " + String(temp[i], 1));
    }

    // หาค่าเฉลี่ย peltier volt
    volt = volt / SAMPLE;
    //    Serial.print("analog:" + String(volt));
    volt = volt / 4095.0 * 3.3 * 1.18;
    curr = volt / RLOAD;

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
  serverURL += "&resis=" + String(RLOAD, 0);

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
