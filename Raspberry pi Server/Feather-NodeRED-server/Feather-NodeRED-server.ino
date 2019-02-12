// rf69 demo tx rx.pde
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, reliable messaging client
// with the RH_RF69 class. RH_RF69 class does not provide for addressing or
// reliability, so you should only use RH_RF69  if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example rf69_server.
// Demonstrates the use of AES encryption, setting the frequency and modem 

#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

/************ Radio Setup ***************/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF69_FREQ 433.0
// who am i? (server address)
#define MY_ADDRESS     1
#define RETRIES        5





#if defined (__AVR_ATmega328P__)  // Feather 328P w/wing
  #define RFM69_INT     3  // 
  #define RFM69_CS      4  //
  #define RFM69_RST     2  // "A"
#endif





// Singleton instance of the radio driver
RH_RF69 rf69(RFM69_CS, RFM69_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram rf69_manager(rf69, MY_ADDRESS);

typedef enum { waitForDownlink, transferToServer,waitForUplink, txUplink, waitAck} SEN_NODE;
SEN_NODE activeState = waitForDownlink; // Create a variable and initialize it to first State

int16_t packetnum = 0;  // packet counter, we increment per xmission
String incomingString;

// Dont put this on the stack:
uint8_t data[] = "OK";
// Dont put this on the stack:
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
uint8_t from;
char *charArray;
byte  a = 0;
byte temp = 0;
char payLoad[50];


void setup() 
{
  Serial.begin(115200);
  //while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer

    
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  Serial.println("Node-RED server");
  Serial.println();

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
  // Defaults after init are 433.0MHz, modulation GFSK_Rb250Fd250, +13dbM (for low power module)
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



void loop() {
  

    switch (activeState)
    {
      case waitForDownlink:
      {
        Serial.println("State: Wait for Downlink");
        if (rf69_manager.available())
  {
    // Wait for a message addressed to us from the client
       Serial.println("State: Check SPI Receive");
    if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = 0; // zero out remaining string
       if (!rf69_manager.sendtoWait(data, sizeof(data), from)) // Sending ACK to the client
        Serial.println("Sending failed (no ack)");
        
      

        charArray = (char*)buf;
        

         activeState = transferToServer;
    }
     
  }

  else 
  activeState = waitForUplink;
  
        delay(250);
        break;
      }

      case transferToServer:
      {
        Serial.println("State: Transfer to Server");
        Serial.print("RawData: ");Serial.println(charArray);
        if (from%2 == 0)
        {
          // Even Number
          sprintf(charArray+strlen(charArray), "TS%d", from); 
          
          }
        else
        
        {
          
          Serial.println(charArray);
          if (strncmp(charArray, "ON",2)== 0)
          {

            sprintf(charArray+strlen(charArray), ",%d", from);
            
                     
            }
            
          else if (strncmp(charArray, "OFF", 3) == 0)
          {

            sprintf(charArray+strlen(charArray), ",%d", from);
                                 
          }
          


          
        }
        
        Serial.println(charArray);

        activeState = waitForDownlink;
        delay(250);
        break;
      }

      case waitForUplink:
      {

        Serial.println("State: Wait for Uplink");
        
         if (Serial.available()) {
                // read the incoming byte:
                incomingString = Serial.readString();
                Serial.setTimeout(100);

                // say what you got:
                Serial.println(incomingString);
                
                int commaIndex = incomingString.indexOf(',');
                int secondCommaIndex = incomingString.indexOf(',', commaIndex+1);
                String firstValue = incomingString.substring(0, commaIndex);
                String secondValue = incomingString.substring(commaIndex+1, secondCommaIndex);
                
                Serial.println(firstValue);
                Serial.println(secondValue);
                secondValue = secondValue+'\n';
                a = secondValue.toInt();
                Serial.println(a);

                
                firstValue.toCharArray(payLoad,50);

                
               
      
      
      
      
      activeState = txUplink;
      
} 

      else
      activeState = waitForDownlink;

       delay(250);    
        break;
      }

      case txUplink:
      {
        Serial.println("State: Tx Uplink ");

        Serial.print("Payload: "); Serial.print(payLoad); Serial.print("to the client "); 
        Serial.println(a);
  
      // Send a message to the DESTINATION!
         rf69_manager.sendtoWait((uint8_t *)payLoad, strlen(payLoad), a);
      // Now wait for a reply from the server
      // go to next state checkACK
       
      



        

        activeState = waitAck;
        delay(250);
        break;
      }
      
       case waitAck:
      {

        Serial.println("State: Wait for ACK ");

        if (temp<RETRIES)
        {
          
          uint8_t len = sizeof(buf);
          uint8_t from; 
       
          if (rf69_manager.recvfromAckTimeout(buf, &len, 3000, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got reply from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf); 
      temp = 0;
      activeState = waitForDownlink;
      
      }
      else
      { 
      temp++;
      activeState = txUplink;
      }
          
          
        }


        else
        {
          temp = 0;
          activeState = waitForDownlink;
        }
        
        

        delay(250);
        break;
      }

      
    }


        
}
