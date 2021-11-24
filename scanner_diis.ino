/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_wifi.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoJson.h>
#define SLEEP_TIME 10   // seconds
#define WAIT_WIFI_LOOP 60
#define SCANNER_ID "8e61a75d-12b7-4bda-8bc1-ed5983d33408-003"

const char* ssid = "OPPO F7";
const char* password =  "11111111";
bool data_sent = false;

int scanTime = 60; //In seconds

BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {

    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value
    // put your main code here, to run repeatedly:
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    int count = foundDevices.getCount();
    StaticJsonDocument<200> doc;
     for (int i = 0; i < count; i++){
         BLEAdvertisedDevice d = foundDevices.getDevice(i);
         HTTPClient http;  
         http.begin("https://diis.herokuapp.com/api/time");  //Specify destination for HTTP request
         http.addHeader("Content-Type", "application/json");  
              // Add values in the document
              //
        doc["device_name"] = d.getName().c_str();
        doc["device_address"] = d.getAddress().toString().c_str();
        doc["scanner_id"] = SCANNER_ID;
        doc["rssi"] = d.getRSSI();

        String requestBody;
        serializeJson(doc, requestBody);
        Serial.println(requestBody);
        int httpResponseCode = http.POST(requestBody);
         if(httpResponseCode>0){
  
          String response = http.getString();                       //Get the response to the request
        
          Serial.println(httpResponseCode);   //Print return code
          Serial.println(response);           //Print request answer
      
       }else{
      
          Serial.print("Error on sending POST: ");
          Serial.println(httpResponseCode);
    
      }
      http.end();  //Free resources
      data_sent = true;
     }
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    
  }
}

void loop() {
    esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000); // translate second to micro second

    log_i("Enter deep sleep for %d seconds...\n", SLEEP_TIME);

    esp_wifi_stop();
    esp_deep_sleep_start();
}
