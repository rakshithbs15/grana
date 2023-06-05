#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <iostream>
#include <string>
#include "time.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool bluetooth_connected=false;

int R=21;
int G=22;
int B=23;
int relay=16;
int CurVal = 0;
int value=0;
int oldvalue=1;
unsigned long val, startTime,endTime;
float getVPP();
char data_to_send [4];
char temperatureString [2];
char value_rcvd [5];
const int sensorIn = 4;      
int mVperAmp = 66;           
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
int i=0;
void Cooloff();
float CheckCurrentValue();

     

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define DHTDATA_CHAR_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device Connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      ESP.restart();
      //pServer->getAdvertising()->start();
      //deviceConnected = false;
      //Serial.println("Device Disconnected!");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value_str = pCharacteristic->getValue();
       Serial.print("*********");
       Serial.print("New value: ");
       for (int i = 0; i < value_str.length(); i++)
       {
          value_rcvd[i]=value_str[i];
          Serial.print(value_str[i]);
       }
       Serial.println("*********");
      if (value_str.length() > 0) {
        value=atoi(value_rcvd);

       
      }
    }
};



void setup() {
//WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector  

// turn on the power
//digitalWrite(powerControlSettings.powerControlOutputPin, HIGH);

// let things power up
//delay(500);

//WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); //enable brownout detector

 
  Serial.begin(115200);
 
  pinMode(relay,OUTPUT);
  digitalWrite(relay, LOW);
 
  // Create the BLE Device
  BLEDevice::init("Robomill");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());


  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
 

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("The device started, now you can pair it with BLE!");
  Serial.println("Waiting for a client connection to be established...");
}

void loop() {


     
    // CheckCurrentValue();
    if(value==0 && oldvalue!=0)
    {
      oldvalue=0;
      Serial.println("Ready to Start");
      digitalWrite(relay, LOW);  
      SendNotification(1);
    }
   

    else if (value==1 && oldvalue!=1)
    {
      oldvalue=1;
      Serial.println("Cotinuous On Mode");
      digitalWrite(relay, HIGH);
    }


    else if (value>1 && oldvalue!=value)
    {
      oldvalue=value;
      Serial.print("Timed Mode has begun for ");
      Serial.print(value);
      Serial.print(" seconds");
      Serial.println();
      startTime = millis();
      digitalWrite(relay, HIGH);
      while(millis() < startTime + value*1000UL)
      {
        //Waiting for the time to tick.
        //CheckCurrentValue();
         
      }

      Serial.println("Timed Mode has ended, waiting for cooloff");      
      digitalWrite(relay, LOW);
      Cooloff();
      SendNotification(1);
    }
    
 
  Serial.println ("");
  
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
 // Serial.println(VRMS);
  AmpsRMS = (VRMS * 1000)/mVperAmp;
  if (AmpsRMS-7.2>1.5)
  {
    i++;
    if(i==3)
    {
      digitalWrite(relay, LOW); 
      i=0;
      value=0;
    }
  }
  Serial.println(i);
  Serial.print(AmpsRMS-7.2);
  Serial.print(" Amps RMS  ---  ");
  


}


void SendNotification(int value_to_send)
{
      sprintf (data_to_send, "% d", value_to_send);

      pCharacteristic-> setValue (data_to_send);
      pCharacteristic-> notify ();
      Serial.print ("*** Sent Data:");
      Serial.print (data_to_send);
      Serial.println ("***");    
}





 float getVPP()
  { 
   float result;
   int readValue;                // value read from the sensor
   int maxValue = 0;             // store max value here
   int minValue = 4096;          // store min value here
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the minimum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 5.0)/4096.0;  
   return result; 
 }




void Cooloff()
{
      startTime = millis();
      while(millis() < startTime + 5000UL)
      {
        //Waiting for the time to tick for cooloff.
      }    
      value=0;
      oldvalue=0;
      Serial.println("Cooloff done, ready for next command");
}

