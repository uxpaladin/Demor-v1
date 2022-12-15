/*
  Rui Santos
  Complete project details at our blog.
    - ESP32: https://RandomNerdTutorials.com/esp32-firebase-realtime-database/
    - ESP8266: https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based in the RTDB Basic Example by Firebase-ESP-Client library by mobizt
  https://github.com/mobizt/Firebase-ESP-Client/blob/main/examples/RTDB/Basic/Basic.ino
*/

#include <sstream>
#include <ArduinoJson.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 2   // D4
#define RST_PIN 0  // D3
#define LED_PIN 4  // D2
#define FAIL_PIN 5 // D1

// Insert your network credentials
#define WIFI_SSID "Apto 1208"
#define WIFI_PASSWORD "lafayette13"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDqLj5ncPkpaS3L4IoI9ATQmgRCa3uXOVo"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://fir-c0626-default-rtdb.firebaseio.com/"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

MFRC522 mfrc522(SS_PIN, RST_PIN);
// MFRC522::MIFARE_Key key;
int statuss = 0;
int out = 0;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
String username = "null";
bool unlockDoor = false;
String password = "null";
String cardId = "null";
String atStartTime = "null";
String atEndTime = "null";
bool sendNotificacion = false;
bool cardScan = false;
bool doorbellPressed = false;
unsigned long unlockTime = 0;
bool relockDoorFlag = false;
unsigned long updateLockState = 0;
bool grantAccess = false;
unsigned long scanTimer = 0;
std::stringstream sstm;
std::stringstream sstm2;
std::string query;
std::string query2;
std::string sValue;
String scanValue;

ICACHE_RAM_ATTR void doorBell()
{
  Serial.println("is alive!!");
  doorbellPressed = true;

  return;
}

void cardAccess()
{
  cardScan = true;
  if (Firebase.RTDB.setBool(&fbdo, "profile/flags/0/cardScan", cardScan))
  {
    cardScan = false;
  }
}

void setup()
{
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  pinMode(15, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAIL_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(15), doorBell, FALLING);

  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  /* for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  } */
}

void loop()
{
  if (unlockTime == 0 && (millis() - updateLockState) > 5000)
  {
    Firebase.RTDB.getBool(&fbdo, "profile/flags/0/unlockDoor");
    if (fbdo.boolData())
    {
      unlockDoor = fbdo.boolData();
    }
  }

  if (unlockDoor)
  {
    digitalWrite(LED_PIN, HIGH);
    unlockTime = millis();
    Serial.println("door unlocked");
    unlockDoor = false;
    relockDoorFlag = true;
  }
  if (relockDoorFlag && ((millis() - unlockTime) > 5000))
  {
    digitalWrite(LED_PIN, LOW);
    Firebase.RTDB.setBool(&fbdo, "profile/flags/0/unlockDoor", unlockDoor);
    Serial.println("door locked");
    unlockTime = 0;
    relockDoorFlag = false;
  }

  if (doorbellPressed)
  {
    if (Firebase.RTDB.setBool(&fbdo, "profile/flags/0/doorBell", doorbellPressed))
    {
      doorbellPressed = false;
    }
  }

  if (mfrc522.PICC_IsNewCardPresent() && (millis() - scanTimer) > 6000) // previene que puedas escanear la tarjeta multiples veces seguidas
  {
    scanTimer = millis();
    if (mfrc522.PICC_ReadCardSerial())
    {
      Serial.println("card true");
      Firebase.RTDB.setBool(&fbdo, "profile/flags/0/cardScan", true);
      cardScan = true;
      // mfrc522.PICC_HaltA();
    }
  }

  if (cardScan)
  {
    scanValue = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      scanValue.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      scanValue.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    scanValue.toUpperCase();
    //  Firebase.RTDB.getString(&fbdo, "profile/cards/0/id");
    //  String idValue = fbdo.stringData();
    Serial.println("");
    Serial.println(scanValue.length());
    scanValue = scanValue.substring(1, 12);
    // Serial.println(scanValue);
    // Serial.println("after 1st scan print");
    cardScan = false;
    Firebase.RTDB.getInt(&fbdo, "profile/numCards");
    sValue = "profile/cards/";
    for (int z = 0; z < fbdo.intData(); z++)
    {
      // String query = "";
      sstm.str("");
      sstm << sValue << z << "/id";
      query = sstm.str();
      sstm2.str("");
      sstm2 << sValue << z << "/enabled";
      query2 = sstm2.str();
      Firebase.RTDB.getString(&fbdo, query);
      Serial.println(fbdo.stringData());
      if (scanValue.compareTo(fbdo.stringData()) == 0)
      {
        Serial.println("success");
        Serial.println(query2.c_str());
        Firebase.RTDB.getBool(&fbdo, query2);
        Serial.println(fbdo.boolData());
        if (fbdo.boolData())
        {
          unlockDoor = true;
          return;
        }
        else
        {
          Serial.println("not enabled");
        }
      }
      Serial.println("card not approved");
    }
    Serial.println("fail");
    digitalWrite(FAIL_PIN, HIGH);
    delay(500);
    digitalWrite(FAIL_PIN, LOW);
    delay(500);
    digitalWrite(FAIL_PIN, HIGH);
    delay(500);
    digitalWrite(FAIL_PIN, LOW);
    delay(500);
    digitalWrite(FAIL_PIN, HIGH);
    delay(500);
    digitalWrite(FAIL_PIN, LOW);
    delay(500);
  }

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.getBool(&fbdo, "profile/flags/0/unlockDoor"))
    {
      unlockDoor = fbdo.boolData();
    }
  }
}
