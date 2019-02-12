
#include <SPI.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>








#define RF69_FREQ       433.0


// Where to send packets to!
#define DEST_ADDRESS    1
// change addresses for each client board, any number :)
#define MY_ADDRESS      3
#define RETRIES         5
#define CONTROL_PIN     6

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
char payLoad[5];
// Finite State Machine 
typedef enum { waitForCommand, parsePerform,txData, waitAck} SEN_NODE;
SEN_NODE activeState = waitForCommand; // Create a variable and initialize it to first State

unsigned long currentMillis, previousMillis;

#define UPDATE_INTERVAL_MINUTES 0.2

byte state = HIGH;  // Turn ON the device at bootup
uint8_t len = sizeof(buf);
uint8_t from;
char *charArray;


byte temp = 0;

void setup() {
  
  Serial.begin(115200);
  Serial.println("void setup"); 
  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, state);



    
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

  

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

////char action[4] = "ON";

void loop() {

  currentMillis = millis();

  // Wait for a message addressed to us from the client
    
  //// charArray = action;
    
  switch(activeState)
  {
    
    case waitForCommand:
    {
      Serial.println("State: Wait for Command from Server");
    
    if (rf69_manager.available())
    {
      if (rf69_manager.recvfromAck(buf, &len, &from)) {
      buf[len] = 0; // zero out remaining string

        charArray = (char*)buf;
        Serial.println(charArray);

     
      if (!rf69_manager.sendtoWait(data, sizeof(data), from))
        Serial.println("Sending failed (no ack)");
    }
    activeState = parsePerform;  
    }
    
    else
    activeState = waitForCommand;
    
    delay (250);
   
    
    break;
    }

    case parsePerform:
    {
      
      Serial.println("State:Parse and Perform action");
      Serial.println(charArray);
      if (strncmp (charArray, "ON", 2) == 0)
      {
        Serial.println("You have to TURN ON the device");
        state = HIGH;
      
        
      }

      else if (strncmp (charArray, "OF", 2) == 0)
      {
        
        Serial.println("You have to TURN OFF the device");
        state = LOW;
        
      }
      digitalWrite(CONTROL_PIN, state);
      delay (500);
      strcpy(payLoad, "Done");
      activeState = txData;
      
      break;
      }
      
    case txData:
    {
      
      Serial.println("State: Transmit Data ");
      
      Serial.println(payLoad);
      if (rf69_manager.sendtoWait((uint8_t *)payLoad, strlen(payLoad), DEST_ADDRESS)) {
      // Now wait for a reply from the server
      // go to next state checkACK
      
    activeState = waitAck; 
    }

    else
    {
      /// Write the case for non response Radio 
      
      }
    



      
      activeState = waitAck; 
      delay (250);
      break;
      }

      case waitAck:
    {
      
      Serial.println("State: Wait for ACK from Server");

      

      if (temp < RETRIES) // retry 
      {
        Serial.print("temp :"); Serial.println(temp);
      uint8_t len = sizeof(buf);
      uint8_t from;   
      if (rf69_manager.recvfromAckTimeout(buf, &len, 3000, &from)) {
      buf[len] = 0; // zero out remaining string
      
      Serial.print("Got reply from #"); Serial.print(from);
      Serial.print(" [RSSI :");
      Serial.print(rf69.lastRssi());
      Serial.print("] : ");
      Serial.println((char*)buf); 
      activeState = waitForCommand;
      delay (5000);
      temp = 0;
      

      
      }
      else {
      Serial.println("No reply, is anyone listening?");
      temp++;
      activeState = txData;
      }
    
  }
    else  // skip to next step
      
      {
        Serial.println("Could not get any response");
        temp = 0;
        
        activeState = waitForCommand;
        }
    }

}
 if ((currentMillis-previousMillis)/60000 >= UPDATE_INTERVAL_MINUTES)
  {

    
    /// the update time 

    Serial.println("Please send the status of your device");
    strcpy(payLoad, "Status");
    previousMillis = currentMillis;
    
     if (state == HIGH)
    {
      strcpy(payLoad+6,",ON");
    }
    else
    {
    strcpy(payLoad+6,",OFF");
    }
    Serial.println(payLoad);
    activeState = txData;
    }
    
  else
  {

      /// Do nothing
   
    
    }
}

