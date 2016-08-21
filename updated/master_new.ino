/*
 * Pin Configuration
 * 
 * 0 - not used
 * 1 - not used
 * 2 - not used
 * 3 - flowrate pin
 * 4 - enable
 * 5 - rs
 * 6 - connect to TX 
 * 7 - connect to RX 
 * 8 - lcd contrast pin
 * 9 - CE pin of NRF24L01
 * 10 - CS pin of NRF24L01
 * 11 - MOSI pin of NRF24L01
 * 12 - MISO pin of NRF24L01
 * 13 - SCK pin of NRF24L01
 * 14 - d4
 * 15 - d5
 * 16 - d6
 * 17 - d7
 */

/*
 * Things left: Throw data to mobile app.
 * 
 */
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#define FLOWSENSORPIN 3

LiquidCrystal lcd(5, 4, 14, 15, 16, 17);
SoftwareSerial BT(6, 7); 
RF24 radio(9,10);
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
typedef enum { role_ping_out = 1, role_pong_back } role_e;
role_e role;

int count = 0;
int flag = 0;
String data="";
String stat = "";

// count how many pulses!
volatile uint16_t pulses = 0;
// track the state of the pulse pin
volatile uint8_t lastflowpinstate;
// you can try to keep time of how long it is between pulses
volatile uint32_t lastflowratetimer = 0;
volatile float flowrate;

float previous_flow=0.00;
float this_flow = 0.00;
boolean stopped = false;
boolean tmp = false; 

struct payload_t{
  int message;
  float value;
}; 

SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSORPIN);
  
  if (x == lastflowpinstate) {
    lastflowratetimer++;
    return; // nothing changed!
  }
  
  if (x == HIGH) {
    //low to high transition!
    pulses++;  
  }
  
  lastflowpinstate = x;
  flowrate = 1000.0;
  flowrate /= lastflowratetimer;  // in hertz
  lastflowratetimer = 0;
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
  }
}

void setup(void)
{
  
  Serial.begin(57600);
  BT.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  printf_begin();

  pinMode(FLOWSENSORPIN, INPUT);
  digitalWrite(FLOWSENSORPIN, HIGH);
  lastflowpinstate = digitalRead(FLOWSENSORPIN);
  useInterrupt(true);

  radio.begin();
  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);
  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(sizeof(payload_t));
  radio.setPALevel(RF24_PA_MIN);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();
  //radio.printDetails();
}

void loop(void)
{
    if(stat.equals("START")) {
    float liters = pulses;
    liters /= 7.5;
    liters /= 60.0;
  
    if(liters == previous_flow && liters == 0.00) {
      Serial.println("Not Started");
      }

    else if(liters == previous_flow && stopped == false){
      this_flow = liters - this_flow;
      Serial.print("Water in this session ");
      Serial.println(this_flow);
      Serial.println("Not flowing");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Curr Vol:");
      lcd.print(this_flow);
      lcd.setCursor(0,1);
      lcd.print("Tot Vol:");
      lcd.print(liters);
      stopped = true;
      Serial.print("Sending...");
      radio_send(4,this_flow);
      flag = 0;
      
      }
    else if(liters!= previous_flow) {
      previous_flow = liters;
      Serial.println("Flowing");
      if(flag == 0){
        flag = 1;
        Serial.print("Started");
        radio_send(0,0.0);
        }
      stopped = false;
      }
    else {
    //empty
      }    
  
  delay(2000);
  }

  if(stat.equals("STOP")) {
    radio_receive();
    Serial.println("Triggered");
    }
  
  if(!stat.equals("STOP")){
    if(BT.available()) {
     data = "";
     while(BT.available()) {
     data += (char)BT.read();
     delay (10);
     }
     data.toUpperCase();
     
     if(data.equalsIgnoreCase("START")){
      Serial.println("START");
      stat = "START";
      radio_send(1,0.0);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("START");
      }
     else if(data.equalsIgnoreCase("STOP")) {
        Serial.println("STOP");
        stat = "STOP";
        radio_send(2,0.0);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("STOP");  
          }
     else if(data.equalsIgnoreCase("RESET")) {
        Serial.println("RESET");
        stat = "RESET";
        radio_send(3,0.0);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("RESET");
        pulses = 0;
        lastflowpinstate = 0;
        lastflowratetimer = 0;
        flowrate = 0;
        previous_flow=0.00;
        this_flow = 0.00;
        stopped = false;
        flag = 0;
        tmp = false;   
      }
      else if(data.equalsIgnoreCase("PREDICT")){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("PREDICT");
        radio_send(7,0);
      }
      else if(data.equalsIgnoreCase("TMP")){
        radio_send(11,0.0);
        tmp = true;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Set TMP");
      }
      else {
      //error
        //radio_send()
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Received Volume");
        radio_send(15,data.toFloat());
        
      }  
    }
  }
    
}

void radio_send(int message,float value) {
  
    radio.stopListening();
    // Take the time, and send it.  This will block until complete
    //unsigned long time = millis();
    payload_t payload = {message,value};
    //printf("Now sending %d\n ",payload.message);
    Serial.print("Value: ");
    Serial.println(payload.value);
    bool ok = radio.write( &payload, sizeof(payload_t));
    
    if (ok)
      Serial.print("ok...");
    else
      Serial.print("failed.\n\r");

    // Now, continue listening
    radio.startListening();
    delay(1000);
    }

void radio_receive() {
    if ( radio.available() )
     {
      payload_t payload;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &payload, sizeof(payload_t) );

        // Spew it
        //printf("Got payload %d...,data:%f\n",payload.message,payload.value);
        Serial.println(payload.value);
        delay(1000);
  
      }
      payload_check(payload.message,payload.value);
    }
  }

void payload_check(int m,float val) {
  if(m == 5){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("**In Progress**");
    BT.print("nt");
    
    }
  else if(m == 7) {
    stat = "";
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("***** Done *****");
    stat = "";
    }
  else {
    //BT.print(val);
    String str = String(m);
    String str2 = String(val,2);
    String str3 = str + " "+str2+ " C";
    //lcd.print("Sent");
    BT.print(str3);
    }
  }    
