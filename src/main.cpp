
#include <sstream>
#include <ArduinoJson.h>
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <TimeLib.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <WiFiUdp.h>

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

#define WIFI_SSID "Apto 1208"
#define WIFI_PASSWORD "lafayette13"

#define API_KEY "AIzaSyDqLj5ncPkpaS3L4IoI9ATQmgRCa3uXOVo"

#define DATABASE_URL "https://fir-c0626-default-rtdb.firebaseio.com/"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

MFRC522 mfrc522(SS_PIN, RST_PIN);
int statuss = 0;
int out = 0;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
String username = "null";
bool unlockDoor = false;
String password = "null";
String cardId = "null";
int startTime = 0;
int endTime = 0;
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
FirebaseJsonArray event;
std::stringstream sstm3;
std::string query3;
std::stringstream sstm4;
std::string query4;
IPAddress timeServer(132, 163, 97, 6);
const int timeZone = -3;
// EthernetUDP Udp;
WiFiUDP Udp;
unsigned int localPort = 8888;
int loopInt = 0;

ICACHE_RAM_ATTR void doorBell()
{
  Serial.println("doorbel pressed");
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

const int NTP_PACKET_SIZE = 48;     // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
time_t getNtpTime()
{
  while (Udp.parsePacket() > 0)
    ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 5000)
  {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
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

  Udp.begin(localPort);
  setSyncProvider(getNtpTime);

  while (timeStatus() == timeNotSet)
  {
    Serial.println("waitinggg");
  }

  Serial.println(hour());

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
    Firebase.RTDB.setBool(&fbdo, "profile/flags/0/unlockDoor", unlockDoor);
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
      // Firebase.RTDB.pushArray(&fbdo, "/profile/listOfEvents", &event);
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
    Serial.println("");
    Serial.println(scanValue.length());
    scanValue = scanValue.substring(1, 12);
    cardScan = false;
    Firebase.RTDB.getInt(&fbdo, "profile/numCards");
    sValue = "profile/cards/";

    /* Firebase.RTDB.getArray(&fbdo, "profile/listOfEvents");
    event = fbdo.jsonArray();
    Firebase.RTDB.pushArray(&fbdo, "profile/listOfEvents", &event);
    */
    loopInt = fbdo.intData();
    for (int z = 0; z < loopInt; z++)
    {
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
        sstm3.str("");
        sstm3 << "profile/flags/0/" << scanValue.c_str();
        query3 = sstm3.str();
        Firebase.RTDB.setBool(&fbdo, query3, true);
        Firebase.RTDB.getBool(&fbdo, query2);
        Serial.println(fbdo.boolData());
        if (fbdo.boolData())
        {
          sstm4.str("");
          sstm4 << sValue << z << "/accessTimes/0/startTime";
          query4 = sstm4.str();
          Firebase.RTDB.getInt(&fbdo, query4);
          startTime = fbdo.intData();
          sstm4.str("");
          sstm4 << sValue << z << "/accessTimes/0/endTime";
          query4 = sstm4.str();
          Firebase.RTDB.getInt(&fbdo, query4);
          endTime = fbdo.intData();
          if (hour() <= endTime && hour() >= startTime)
          {
            unlockDoor = true;
            return;
          }
          else {
            Serial.println("not in access time");
          }
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

// send an NTP request to the time server at the given address
