#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <Adafruit_Sensor.h>
#include <DHT.h>

#include <SPI.h>
#include <MFRC522.h>

// DHT

#define DHTPIN D8
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// RFID

constexpr uint8_t RST_PIN = D3;  // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;   // Configurable, see typical pin layout above

MFRC522 rfid(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;

//  API

int lampu1 = D0;
int lampu2 = D1;
// int lampu3 = ;
int pintu = D2;
// int pagar = ;

float t = 0.0;
float h = 0.0;

// WIFI

String internal_id_ktp = "51372254";

const char* ssid = "A22";
const char* pass = "00000001";
const char* host = "192.168.181.85";
const uint16_t port = 80;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522

  dht.begin();

  pinMode(D8, INPUT);

  pinMode(lampu1, OUTPUT);
  pinMode(lampu2, OUTPUT);
  // pinMode(lampu3, OUTPUT);
  pinMode(pintu, OUTPUT);
  // pinMode(pagar, OUTPUT);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  delay(1000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.print("Your IP Address : ");
  Serial.println(WiFi.localIP());
}

void loop() {

  WiFiClient client;
  HTTPClient http;
  String Link;
  String LinkDHT;
  String LinkRFID;

  String tag;

  if (!client.connect(host, 80)) {
    Serial.println("Connection Failed");
    delay(5000);
  }

  if (!rfid.PICC_IsNewCardPresent())
    Serial.println("No Tag Detected");
  if (rfid.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    Serial.println(tag);
    if (tag == internal_id_ktp) {
      Serial.println("Access Granted!");
      LinkRFID = "http://" + String(host) + "/moduliot/bukapintu.php";
      http.begin(client, LinkRFID);
      http.GET();
      http.end();
    }
    tag = "";
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  float newT = dht.readTemperature();
  float newH = dht.readHumidity();
  t = newT * 10;
  h = newH;

  LinkDHT = "http://" + String(host) + "/moduliot/sensordht.php?temp=" + String(t) + "&humidity=" + String(h);
  http.begin(client, LinkDHT);
  http.GET();
  http.end();

  Link = "http://" + String(host) + "/moduliot/getindicator.php";
  http.begin(client, Link);

  int httpCode = http.GET();

  if (httpCode > 0) {

    String payload = http.getString();
    char json[500];
    payload.toCharArray(json, 500);

    StaticJsonDocument<200> doc;
    deserializeJson(doc, json);

    int lampu1State = doc["lampu1"];
    int lampu2State = doc["lampu2"];
    // int lampu3State = doc["lampu3"];
    int pintuState = doc["pintu"];
    // int pagarState = doc["pagar"];

    if (lampu1State == 0) {
      digitalWrite(lampu1, LOW);
    } else {
      digitalWrite(lampu1, HIGH);
    }
    if (lampu2State == 0) {
      digitalWrite(lampu2, LOW);
    } else {
      digitalWrite(lampu2, HIGH);
    }
    // if (lampu3State == 0) {
    //   digitalWrite(lampu3, LOW);
    // } else {
    //   digitalWrite(lampu3, HIGH);
    // }
    if (pintuState == 0) {
      digitalWrite(pintu, LOW);
    } else {
      digitalWrite(pintu, HIGH);
    }
    // if (pagarState == 0) {
    //   digitalWrite(pagar, LOW);
    // } else {
    //   digitalWrite(pagar, HIGH);
    // }

    // Serial Monitor

    Serial.println(payload);

    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print("° C   | ");

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.println("%");

    Serial.println("");

  }
  delay(500);
}