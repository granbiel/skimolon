/*
 * Animal Trap Notification System.
 * Sends trap activation notification at fixed intervals
 * This module is the "stand alone" version (the other version is "master" and "slave" ver.).
 * This module turns Pocket Wifi at fixed intervals, picks up trap activation signal (magnet switch), sends IFTTT(webhooks)>Line Notification, 
 * then turns off Pocket Wifi and goes to deepsleep.
 */
 
#include "config.h"// Contains WiFi ssid, password, IFTTT webhooks event name, and webhooks key.
#include <WiFi.h>
#include <WiFiClient.h>

// setup LCD to check errors
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4); //(address, columns, rows)

#define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  (60 * 60)    // Time ESP32 will go to sleep for(in seconds) */

// setup onboard LED to check errors
#define checkLed 2

// Counting Deepsleep>Boot
RTC_DATA_ATTR int bootCount = 0;

 // Server URL
const char* server = "maker.ifttt.com";

WiFiClient client;

/////////////////////////////////////////////////////////////////////////////
void errorBlink() {
  for (int i =0; i < 10; i++) {
    digitalWrite(checkLed, HIGH);
    delay(300);
    digitalWrite(checkLed, LOW);
    delay(300);
  }
}

/////////////////////////////////////////////////////////////////////////////
void wifiBegin() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  lcd.init();
  lcd.print("connect Wifi...");
  delay(1000);
  WiFi.begin(ssid, password);
}

/////////////////////////////////////////////////////////////////////////////
bool checkWifiConnected() { 
  // attempt to connect to WiFi network;
  int count =0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);

    count++;
    if (count > 15) { // about 15s
      Serial.println("(wifiConnect) failed!");
      lcd.init();
      lcd.print("Wifi failed");
     delay(1000);
    errorBlink();
      return false;
    }
  }

  Serial.print("connected to ");
  lcd.init();
  lcd.print("wifi connected");
  delay(1000);
  Serial.println(ssid);
  Serial.println(WiFi.localIP());

  return true;
}

/////////////////////////////////////////////////////////////////////////////
void send(String bootCount, String flag, String value3) {
  while (!checkWifiConnected()) {
    wifiBegin();
  }
  delay(10 * 1000);
  
  Serial.println("\nStarting connection to server...");
  lcd.init();
  lcd.print("connecting to server...");
  delay(1000);

  int connectCount = 0;
  while (connectCount < 10 && !client.connect(server, 80)) {
    Serial.println("Connection failed! Retry...");
    lcd.init();
    lcd.print("Connection fail. retry.");
    delay(1000);

    delay(30 * 1000);// retry interval
    connectCount++;
  }

  //connect to server faild.
  if (connectCount = 10 && !client.connect(server, 80)) {
    Serial.println("Server connection failed!");
    lcd.init();
    lcd.print("connect server faild");
    delay(1000);

  }

   //connect to server success.
   if (client.connect(server, 80)) {
    Serial.println("Connected to server!");
    lcd.init();
    lcd.print("connected to server!");
    delay(1000);

    // Make a HTTP request:
    String url = "/trigger/" + makerEvent + "/with/key/" + makerKey;
    url += "?value1=" + bootCount + "&value2=" + flag + "&value3=" + value3;
    client.println("GET " + url + " HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
    Serial.println("GET " + url + " HTTP/1.1");
    Serial.print("Host: ");
    Serial.println(server);
    Serial.println("Connection: close");
    Serial.println();

    Serial.print("Waiting for response "); //WiFiClientSecure uses a non blocking implementation

    int count = 0;
    while (!client.available()) {
      delay(50); //
      Serial.print(".");

      count++;
      if (count > 20 * 20) { // about 20s
        Serial.println("(send) failed!");
        errorBlink();
        return;
      }
    }
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting from server.");
      client.stop();
    }
  }
}

//////////////////////////////////////////////////////////////////////////
void setup() {
  pinMode(checkLed, OUTPUT); //error check LED
  
  Serial.begin(115200);
  delay(1000);

  //LCD setup
  lcd.init();
  lcd.backlight();//backlight on
  
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  lcd.init();
  lcd.print("Boot number: " + String(bootCount));
  delay(1000);
  
  //configure the wake up source
  //set ESP32 to wake up every (TIME_TO_SLEEP) seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  lcd.init();
  lcd.println("I sleep for " + String(TIME_TO_SLEEP) + " Seconds");
  delay(1000);

  
  pinMode(14, OUTPUT); // Controls relay which controls RakutenWifiPocket(RWP).
  pinMode(12, INPUT_PULLUP); // Magnet switch (trap activation sensor).
  digitalWrite(14, LOW);
}

////////////////////////////////////////////////////////////////////////
void loop() {

// Turn on RWP
  digitalWrite(14, HIGH);
  delay(3000);
  digitalWrite(14, LOW);
  Serial.println("RWP should be ON now. Standing by for it to connect 4G network");
  lcd.init();
  lcd.print("RWP is on. STB for 4G net.");
  delay(1000);

  delay(30 *1000); //Wait for RWP to connect to the 4G network

  // after RWP is on, ESP connect to Wifi > check > retry
  wifiBegin();
  while (!checkWifiConnected()) {
    wifiBegin(); 
  }

 String magStatus;
 String bootNo = String(bootCount);

//Check if the trap is activated
  if (digitalRead(12) == HIGH) {
    magStatus = "TrapActivated!!!"; 
  } else {
    magStatus = "N.U";//N.U = nothing unusual
  }

  Serial.println("Sending" + magStatus +","+ bootNo);
  send(bootNo, magStatus, "Test3");
  

// Turn off RWP
  Serial.println("Turning RWP off.");
  lcd.init();
  lcd.print("RWP off now.");
  delay(5000);

 digitalWrite(14, HIGH); // Turn on the screen
 delay(1000);
 digitalWrite(14, LOW);
 delay(3000);
 digitalWrite(14, HIGH); // Power off
 delay(5000);

// Have a goodnight
  Serial.println("Going to sleep now");
  lcd.init();
  lcd.print("I sleep now");
  delay(1000);

  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
  
  }
  
