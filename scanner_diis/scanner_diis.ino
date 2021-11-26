/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <esp_wifi.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoJson.h>
#define SLEEP_TIME 10   // seconds
#define WAIT_WIFI_LOOP 30


int wait_wifi_counter = 0;
#define SCANNER_ID "8e61a75d-12b7-4bda-8bc1-ed5983d33408-003"

const char* ssid = "OPPO F7";
const char* password =  "11111111";
bool data_sent = false;
StaticJsonDocument<200> doc;
String requestBody;

int scanTime = 60; //In seconds
WiFiMulti wifiMulti;

BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Scanning...");
 
  

    BLEDevice::init("");
    BLEScan *pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);
    // put your main code here, to run repeatedly:
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
    int count = foundDevices.getCount();

    requestBody="[";
      for (int i = 0; i < count; i++){
        if(i>0){
           requestBody=requestBody+",";
        }
         BLEAdvertisedDevice d = foundDevices.getDevice(i);
              // Add values in the document
              //
        doc["device_name"] = d.getName();
        doc["device_address"] = d.getAddress().toString();
        doc["scanner_id"] = SCANNER_ID;
        doc["rssi"] = d.getRSSI();
        serializeJson(doc, requestBody); 
    }
    requestBody=requestBody+"]";
//    Serial.println(requestBody);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
     wifiMulti.addAP(ssid, password);

}

void loop() {
    while (WiFi.status() != WL_CONNECTED) {
       HTTPClient http;  
       http.begin("http://mean.psu.ac.th:3000/api/eventsarr");  //Specify destination for HTTP request
       http.addHeader("Content-Type", "application/json"); 
       Serial.println(requestBody);
       int httpResponseCode = http.POST(String(requestBody));
         if(httpResponseCode>0){
  
          String response = http.getString();                       //Get the response to the request
        
          Serial.println(httpResponseCode);   //Print return code
          Serial.println(response);           //Print request answer
      
       }else{
      
          Serial.print("Error on sending POST: ");
          Serial.println(httpResponseCode);
          Serial.println(http.errorToString(httpResponseCode));
    
      }
      http.end();  //Free resources
      data_sent = true; 
      }
      if(data_sent || (wait_wifi_counter > WAIT_WIFI_LOOP)){
          Serial.println("data sent");
          esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000); // translate second to micro second
      
          log_i("Enter deep sleep for %d seconds...\n", SLEEP_TIME);
      
          esp_wifi_stop();
          esp_deep_sleep_start();
      }else{
        
        wait_wifi_counter++;
    
        log_i("Waiting count: %d\n", wait_wifi_counter);
        
      }

}
