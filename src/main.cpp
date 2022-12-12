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

#include <ArduinoJson.h>
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Apto 1208"
#define WIFI_PASSWORD "lafayette13"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDqLj5ncPkpaS3L4IoI9ATQmgRCa3uXOVo"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://fir-c0626-default-rtdb.firebaseio.com/" 

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
String username = "null";
bool unlockDoor = true;
String password = "null";
String cardId = "null";
String atStartTime = "null";
String atEndTime = "null";
bool sendNotificacion = false;
bool cardScan = false;
bool doorbellPressed = false;  


ICACHE_RAM_ATTR void doorBell() {
  Serial.println("is alive!!");
  doorbellPressed = true;

    return;
}

void cardAccess() {
  cardScan = true;
  if (Firebase.RTDB.setBool(&fbdo, "profile/flags/0/cardScan", cardScan))
    {
      cardScan = false;
    } 
}

void setup(){
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
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
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  pinMode(15, INPUT);
  // pinMode(2, INPUT);
  attachInterrupt(digitalPinToInterrupt(15), doorBell, FALLING);
  // attachInterrupt(digitalPinToInterrupt(2), cardAccess, RISING);
}

void loop(){
  if (doorbellPressed) {
      if (Firebase.RTDB.setBool(&fbdo, "profile/flags/0/doorBell", doorbellPressed))
    {
      doorbellPressed = false;
    } 
  }
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
   /* if (Firebase.RTDB.setInt(&fbdo, "test/int", count)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    count++;
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "test/float", 0.01 + random(0,100))){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }*/
    
    if (Firebase.RTDB.getBool(&fbdo, "profile/flags/0/unlockDoor"))
    {
      unlockDoor=fbdo.boolData();
      Serial.println(unlockDoor);
    } 
    // if (Firebase.RTDB.getArray(&fbdo, "profile/listOfEvents"))
   // {
      //deserializeJson(document,fbdo.jsonData().stringValue);
      //document=fbdo.jsonData();
      //const char* holder=document[username];
     // Serial.println(fbdo.dataPath());
     // FirebaseJsonArray &data = fbdo.jsonArray();
      //const char index='0'; 
     // Serial.println(data.getStr(index));
    //} 
  }
}

