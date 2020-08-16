/* Uporabi z aplikacijo za android, dostopno na:
 *  
 *  https://play.google.com/store/apps/details?id=appinventor.ai_samo_penic.Geiger_counter_2020
 *  
 *  Poletni tabor inovativnih tehnologij
 *  
 *  Samo Penic, 2020
 */
#include "driver/adc.h"
#include "WiFi.h" 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BT_NAME "Geiger counter No. 1"
#define LED 2
#define GEIGER_INPUT 22

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint32_t three_s_counter_latch=0;
uint32_t counter_cyclic_buffer[30]={0};
int cyclic_buffer_index=0;
uint32_t running_counter=0;

uint32_t counter=0;
uint8_t txString[8]; //20 bytes is maximum packet size for BTLE!!!
int i;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void IRAM_ATTR count_plus_one() {
//    three_s_counter++;
    running_counter++;
}

void setup() {
//  setCpuFrequencyMhz(80);
  adc_power_off();
  WiFi.mode(WIFI_OFF);
  
  Serial.begin(115200); /* Serijski vmesnik je vedno uporaben za razhroscevanje in raziskovanje med razvojem */
  pinMode(GEIGER_INPUT, INPUT); /* Nastavimo vhodni signal -- napetostni impulzi, kot odraz tokovnih pulzov skozi Geiger-Mullerjevo elektronko */
  attachInterrupt(GEIGER_INPUT, &count_plus_one, RISING); /* Ob impulzu zazenemo prekinitveno rutino. Tako optimalno prestejemo impulze */

  pinMode(LED, OUTPUT); /* Na ploscici imamo se eno modro LED. Morda jo kdaj uporabimo? */


  // Create the BLE Device
  BLEDevice::init(BT_NAME); /* Poimenujmo nas stevec. Ime se bo pokazalo na seznamu BT naprav na telefonu */

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  

}

void loop() {
  if (deviceConnected) {
    txString[0]=(counter>>16)&0x000000FF;
    txString[1]=(counter>>24)&0x000000FF;
    txString[2]=counter&0x000000FF;
    txString[3]=(counter>>8)&0x000000FF;

    //data for 100 ms counter
    txString[4]=running_counter&0x000000FF;
    txString[5]=(running_counter>>8)&0x000000FF;

    cyclic_buffer_index++;
    if(cyclic_buffer_index==30) cyclic_buffer_index=0;
    counter_cyclic_buffer[cyclic_buffer_index]=running_counter;
    running_counter=0;    
    three_s_counter_latch=0;
      for(i=0;i<30;i++){
        three_s_counter_latch+=counter_cyclic_buffer[i];
      }
    three_s_counter_latch=three_s_counter_latch*20;
    counter++;
    txString[6]=three_s_counter_latch&0x000000FF;
    txString[7]=(three_s_counter_latch>>8)&0x000000FF;

    pCharacteristic->setValue(txString,8);
    
    pCharacteristic->notify(); // Send the value to the app!

  }
  delay(99);
}
