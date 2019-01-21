#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#include <Wire.h>
#include "Adafruit_MCP9808.h"

#include "LowPower.h"

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0
#define vccRFM69HCW 8
// Where to send packets to!
#define DEST_ADDRESS   1
// change addresses for each client board, any number :)
#define MY_ADDRESS     2

#if defined (__AVR_ATmega328P__)  // Feather 328P w/wing
  #define RFM69_INT     3  // RFM69 ---> G0
  #define RFM69_CS      4  // RFM69 ---> CS
  #define RFM69_RST     2  // RFM69 ---> RST
  
#endif

// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

int16_t packetnum = 0;  // packet counter, we increment per xmission
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t data[] = "  OK";

// Finite State Machine 
typedef enum { accquireData, transmitData, prepareSleep, executeSleep } SEN_NODE;
SEN_NODE activeState = accquireData; // Create a variable and initialize it to first State



// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#define vccMCP9808 A3


void setup() {
  
  Serial.begin(115200);
  digitalWrite(vccMCP9808,HIGH);
  pinMode(vccMCP9808,OUTPUT);
  tempsensor.begin();
  Serial.println("Hi");
  Serial.flush();
  pinMode(vccMCP9808,INPUT);
  digitalWrite(vccMCP9808,LOW);
  
}

void loop() {
  
  switch(activeState)
  {
    case accquireData:
    {
      Serial.println("Data is being accquired");
      digitalWrite(vccMCP9808,HIGH);
      pinMode(vccMCP9808,OUTPUT);
      delay(250);
      Wire.begin();
      float c = tempsensor.readTempC();
      float f = c * 9.0 / 5.0 + 32;
      Serial.print("Temp: "); Serial.print(c); Serial.print("*C\t"); 
      Serial.print(f); Serial.println("*F");
      pinMode(vccMCP9808,INPUT);
      digitalWrite(vccMCP9808,LOW);
      
        
      delay(1000);
      activeState = transmitData;
      break;
    }
    
    case transmitData:
    {
      Serial.println("Data has been transmitted");
      delay(1000);
      activeState = prepareSleep;
      break;
    }
    
    case prepareSleep:
    {
      
      Serial.println("Prepare to sleep");
      TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));

      // turn off I2C pull-ups
      digitalWrite (A4, LOW);
      digitalWrite (A5, LOW);
      delay(1000);
      activeState = executeSleep;
      break;
   
    }
    
    case executeSleep:
    {
      
      Serial.println("Sleep mode execution");
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      activeState = accquireData;
      break;
      
    }

  }
  
}


void radioInit()
{
  digitalWrite(vccRFM69HCW,LOW);
  delay(250);
   // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);
  
  if (!rf69_manager.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
  // No encryption
  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
  
  digitalWrite(vccRFM69HCW,HIGH);

  }


