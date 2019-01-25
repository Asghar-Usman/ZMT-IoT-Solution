#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>


#include <Wire.h>
#include "Adafruit_MCP9808.h"


#include "LowPower.h"
#define intervalMinutes 1 // Could be changed to any integer value in minutes



#define RF69_FREQ       433.0
#define vccRFM69HCW     7 // Control signal to PNP TIP32

// Where to send packets to!
#define DEST_ADDRESS    1
// change addresses for each client board, any number :)
#define MY_ADDRESS      2

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
char payLoad[4];
// Finite State Machine 
typedef enum { accquireData, transmitData, prepareSleep, executeSleep } SEN_NODE;
SEN_NODE activeState = accquireData; // Create a variable and initialize it to first State



// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#define vccMCP9808    A3

int wakeupCount = intervalMinutes*8;
byte temp = 0;

void setup() {
  
  Serial.begin(115200);
  Serial.println("void setup"); 
  pinMode(vccMCP9808, OUTPUT);
  digitalWrite (vccMCP9808, HIGH);
  delay(500);
  if (!tempsensor.begin()) {
  Serial.println("Couldn't find MCP9808!");
  while (1);
  digitalWrite (vccMCP9808, LOW);
  }
  
}

void loop() {
  
  switch(activeState)
  {
    case accquireData:
    {
      Serial.println("Data is being accquired");
      digitalWrite (vccMCP9808, HIGH);
      delay(250);
      // activate I2C
      Wire.begin();
      float c = tempsensor.readTempC();
      dtostrf(c, 4, 2, payLoad);
      Serial.print("Temp: "); Serial.print(payLoad);Serial.println("*C"); 
      digitalWrite (vccMCP9808, LOW);
      delay(100);
      activeState = transmitData;
      break;
    }
    
    case transmitData:
    {
      radioInit();
      
      Serial.println("Data has been transmitted");     
      delay(1000);  // Wait 1 second between transmits, could also 'sleep' here!  
      
      
      Serial.print("Sending "); Serial.println(payLoad);
  
      // Send a message to the DESTINATION!
      if (rf69_manager.sendtoWait((uint8_t *)payLoad, strlen(payLoad), DEST_ADDRESS)) {
      // Now wait for a reply from the server
      uint8_t len = sizeof(buf);
      uint8_t from;   
      if (rf69_manager.recvfromAckTimeout(buf, &len, 2000, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got reply from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf);     
      
    } else {
      Serial.println("No reply, is anyone listening?");
    }
    }
    
    else {
    Serial.println("Sending failed (no ack)");
    }
    
    digitalWrite(vccRFM69HCW,HIGH);



      
      delay(1000);
      activeState = prepareSleep;
      break;
    }
    
    case prepareSleep:
    {
      Serial.println("Prepare to sleep");
      
      SPI.end(); // shutdown the SPI interface
      pinMode(SCK,INPUT);
      digitalWrite(SCK,LOW); // shut off pullup resistor
      pinMode(MOSI,INPUT);
      digitalWrite(MOSI,LOW); // shut off pullup resistor
      pinMode(MISO,INPUT);
      digitalWrite(MISO,LOW); // shut off pullup resistor
      pinMode(RFM69_CS,INPUT);
      digitalWrite(RFM69_CS,LOW);
      digitalWrite(vccRFM69HCW,HIGH);
      
      TWCR &= ~(bit(TWEN) | bit(TWIE) | bit(TWEA));

      // turn off I2C pull-ups
      digitalWrite (A4, LOW);
      digitalWrite (A5, LOW);
      Serial.flush();
      delay(100);
      activeState = executeSleep;
      break;
    }
    
    case executeSleep:
    {
      
      Serial.println("Sleep mode execution #" + String(temp));
      Serial.flush();
      if (temp >= wakeupCount)
        {
         activeState = accquireData;
         temp = 0;
        }
      else
        {
         LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
         temp ++;
         activeState = executeSleep;
        }
      
      
      break;
      
    }

  }
  
}


void radioInit()
{

  Serial.println("Initialzing....");
  digitalWrite(vccRFM69HCW,LOW);
  digitalWrite(RFM69_RST,LOW);
  pinMode(vccRFM69HCW,OUTPUT);
  pinMode(RFM69_RST,OUTPUT);
  pinMode(SCK,OUTPUT);
  pinMode(MOSI,OUTPUT);
  pinMode(MISO,OUTPUT);
  pinMode(RFM69_CS,OUTPUT);
  

  
  SPI.begin();


  
  //digitalWrite(RFM69_CS,HIGH);
  delay(500);
  
  
  
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

  // If you are using a high power RF69 eg RFM69HW, you *must* set a Tx power with the
  // ishighpowermodule flag set like this:
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW

  // The encryption key has to be the same as the one in the server
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
  
 
  Serial.print("RFM69 radio @");  Serial.print((int)RF69_FREQ);  Serial.println(" MHz");
  
  
  }



