/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
*/

#define WIFI_SSID "AndroidAP6AB0"
#define WIFI_PASSWORD "12345678"
#define POST_URL "http://diis.herokuapp.com/api/time"
#define SCAN_TIME 10     // seconds
#define WAIT_WIFI_LOOP 10 // around 4 seconds for 1 loop
#define SLEEP_TIME 10   // seconds

// Change ID for each scanner
#define SCANNER_ID "8e61a75d-12b7-4bda-8bc1-ed5983d33408-004"

#include <Arduino.h>
#include <sstream>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_wifi.h>

#include <HTTPClient.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

WiFiMulti wifiMulti;
std::stringstream ss;
bool data_sent = false;
int wait_wifi_counter = 0;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      log_i("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

void setup()
{

  log_i("ESP32 BLE Scanner");

  // disable brownout detector to maximize battery life
  log_i("disable brownout detector");
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  log_i("BLEDevice::init()");
  BLEDevice::init("");

  // put your main code here, to run repeatedly:
  BLEScan *pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);

  log_i("Start BLE scan for %d seconds...\n", SCAN_TIME);

  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();
  ss << "[";
  for (int i = 0; i < count; i++)
  {
    if (i > 0)
    {
      ss << ",";
    }
    BLEAdvertisedDevice d = foundDevices.getDevice(i);
    ss << "{\"device_address\":\"" << d.getAddress().toString() << "\",\"device_rssi\":" << d.getRSSI();

    if (d.haveName())
    {
      ss << ",\"device_name\":\"" << d.getName() << "\"";
    }

    if (d.haveAppearance())
    {
      ss << ",\"device_appearance\":" << d.getAppearance();
    }

    if (d.haveManufacturerData())
    {
      std::string md = d.getManufacturerData();
      uint8_t *mdp = (uint8_t *)d.getManufacturerData().data();
      char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
      ss << ",\"device_manufacturerdata\":\"" << pHex << "\"";
      free(pHex);
    }

    if (d.haveServiceUUID())
    {
      ss << ",\"device_serviceuuid\":\"" << d.getServiceUUID().toString() << "\"";
    }

    if (d.haveTXPower())
    {
      ss << ",\"device_txpower\":" << (int)d.getTXPower();
    }

    if (SCANNER_ID)
    {
      ss << ",\"scanner_id\":\"" << SCANNER_ID << "\"";
    }

    ss << "}";
  }
  ss << "]";

  log_i("Scan done!");

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
}

void loop()
{
  // wait for WiFi connection
  if ((wifiMulti.run() == WL_CONNECTED))
  {
    log_i("WiFi Connected");

    // HTTP POST BLE list
    HTTPClient http;

    log_i("[HTTP] Payload:\n%s", ss.str().c_str());
    log_i("[HTTP] Begin...");

    // configure traged server and url
    http.begin(POST_URL);
    http.addHeader("Content-Type", "application/json");

    // start connection and send HTTP header
    int httpCode = http.POST(ss.str().c_str());

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      log_i("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        log_i("[HTTP] Response:\n%s", http.getString());
      }
    }
    else
    {
      log_i("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    data_sent = true;
  }

  // wait WiFi connected
  if (data_sent || (wait_wifi_counter > WAIT_WIFI_LOOP))
  {
    esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000); // translate second to micro second

    log_i("Enter deep sleep for %d seconds...\n", SLEEP_TIME);

    esp_wifi_stop();
    esp_deep_sleep_start();
  }
  else
  {
    wait_wifi_counter++;

    log_i("Waiting count: %d\n", wait_wifi_counter);
  }
}
