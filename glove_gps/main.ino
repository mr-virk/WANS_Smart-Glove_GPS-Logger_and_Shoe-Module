#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

//Pin Configuration
const int buttonPin = D6;
const int  buzzer = D5;

// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

// Insert your network credentials
#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "pass"

// Insert Firebase project API Key
#define API_KEY "key"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "email"
#define USER_PASSWORD "pass"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "firebase_url"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String latitudePath = "/latitude";
String longitudePath = "/longitude";
String gpsDatePath = "/gpsDate";
String gpsTimePath = "/gpsTime";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 19800, 60000); //19800 is the UTC+5 in seconds and 60000 is the inteval of update in milliseconds

// Variable to save current epoch time
int timestamp;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000; //10 Minutes

//GPS
TinyGPSPlus gps;
SoftwareSerial SerialGPS(4, 5);

float Latitude , Longitude;
int year , month , date, hour , minute , second;
String DateString , TimeString , LatitudeString , LongitudeString;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP()); 
  Serial.println();
  for (int i = 0; i < 2; i++) {
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    delay(100);
  }
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(9600);

  //WiFiServer server(8888);  // Use port 8888

  pinMode(buzzer, OUTPUT); //declare buzzer as output
  pinMode(buttonPin, INPUT_PULLUP);

  initWiFi();
  timeClient.begin();

  timeClient.setTimeOffset(0);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/gps_logs";
}

void loop() {
  checkGPS();

  dataLogFirebase();

  if (digitalRead(buttonPin) == LOW) {
    checkTime();
  }
}

//GPS
void checkGPS() {
  while (SerialGPS.available() > 0)
    if (gps.encode(SerialGPS.read()))
    {
      if (gps.location.isValid())
      {
        Latitude = gps.location.lat();
        LatitudeString = String(Latitude , 6);
        Longitude = gps.location.lng();
        LongitudeString = String(Longitude , 6);
      }

      if (gps.date.isValid())
      {
        DateString = "";
        date = gps.date.day();
        month = gps.date.month();
        year = gps.date.year();

        if (date < 10)
          DateString = '0';
        DateString += String(date);

        DateString += " / ";

        if (month < 10)
          DateString += '0';
        DateString += String(month);
        DateString += " / ";

        if (year < 10)
          DateString += '0';
        DateString += String(year);
      }

      if (gps.time.isValid())
      {
        TimeString = "";
        hour = gps.time.hour() + 5; //adjust UTC
        minute = gps.time.minute();
        second = gps.time.second();

        if (hour < 10)
          TimeString = '0';
        TimeString += String(hour);
        TimeString += " : ";

        if (minute < 10)
          TimeString += '0';
        TimeString += String(minute);
        TimeString += " : ";

        if (second < 10)
          TimeString += '0';
        TimeString += String(second);
      }

    }
  delay(100);
}

//GPS Logging
void dataLogFirebase() {
  //Data Logging to Firebase
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    json.set(latitudePath.c_str(), String(LatitudeString));
    json.set(longitudePath.c_str(), String(LongitudeString));
    json.set(gpsDatePath.c_str(), String(DateString));
    json.set(gpsTimePath.c_str(), String(TimeString));
    json.set(timePath, String(timestamp));
    Serial.printf("Logging GPS... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
      delay(100);
    }
  }
  digitalWrite(buzzer, LOW);
}

void checkTime() {
  int localTime = timeClient.getHours() + 5;  // 5 is the UTC+5
  Serial.println(localTime);
  if (localTime < 16 && localTime > 4) { //Day
    for (int i = 0; i < 2; i++) {
      digitalWrite(buzzer, HIGH);
      delay(300);
      digitalWrite(buzzer, LOW);
      delay(300);
    }
  }
  else if (localTime > 16 && localTime < 4) { //Night
    for (int i = 0; i < 3; i++) {
      digitalWrite(buzzer, HIGH);
      delay(300);
      digitalWrite(buzzer, LOW);
      delay(300);
    }
  }
  digitalWrite(buzzer, LOW);
}
