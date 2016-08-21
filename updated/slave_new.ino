/*
 * Pin Configuration:
 * 
 * 0
 * 1
 * 2 - data pin of ds18b20
 * 3 - data pin of ds18b20
 * 4 - enable
 * 5 - rs
 * 6 - data pin of ds18b20
 * 7 - echo pin
 * 8 - trig pin
 * 9 - CE pin of NRF24L01
 * 10 - CS pin of NRF24L01
 * 11 - MOSI pin of NRF24L01
 * 12 - MISO pin of NRF24L01
 * 13 - SCK pin of NRF24L01
 * 14 - d4
 * 15 - d5
 * 16 - d6
 * 17 - d7
 * 18 - SDA
 * 19 - SCL
 */
#include <Statistic.h> 
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <NewPing.h>
#include "Adafruit_TMP007.h"

#define AIR 2
#define TAP 3
#define BATH 6
#define TRIGGER_PIN  8  
#define ECHO_PIN     7  
#define MAX_DISTANCE 200

//Temperature Sensors
OneWire oneWire1(AIR);
OneWire oneWire2(TAP);
OneWire oneWire3(BATH);
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
Adafruit_TMP007 tmp007;

Statistic tap;
Statistic start;
Statistic stopped;

//LCD Display
LiquidCrystal lcd(5, 4, 14, 15, 16, 17);

//Wireless transceiver
RF24 radio(9,10);

//Ultrasonic Sensor
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
typedef enum { role_ping_out = 1, role_pong_back } role_e;
role_e role;

struct payload_t{
  int message;
  float value;
}; 

double heat = 0.0;
double tot_vol = 0.0;
double curr_vol = 0.0;
double sys_temp = 0.0;
double FIN_VOL = 0.0;
double prediction = 0.0;

//Mean
double mean;
//variance
double variance;
double k;
double epsilon = 0.01;




boolean tmp = false;

String stat = "";
String temp="";

void setup(void)
{
  role = role_pong_back;
  
  Serial.begin(57600);
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  //lcd.print("Hello");

  tap.clear();
  start.clear();
  stopped.clear();
  
  //printf_begin();
  Serial.println("RF24/examples/pingpair/");
  radio.begin();
  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);
  radio.setPayloadSize(sizeof(payload_t));
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  radio.printDetails();
  if (! tmp007.begin()) {
    Serial.println("No sensor found");
    while (1);
  }
}

float getAirTemp() {
  delay(500);
  sensor1.requestTemperatures(); 
  return sensor1.getTempCByIndex(0);  
}

float getTapTemp() {
  delay(500);
  sensor2.requestTemperatures(); 
  return sensor2.getTempCByIndex(0);  
}

float getBathTemp() {
  delay(500);
  sensor3.requestTemperatures(); 
  return sensor3.getTempCByIndex(0);  
}


float getTMP(){
  delay(4000);
  float objt = tmp007.readObjTempC();
  Serial.println("Object Temperature: "); 
  return objt;
  }

int height() {
  delay(50);                     
  Serial.print("Ping: ");
  int height = sonar.ping_cm();
  return height;
}

void loop(void)
{
  getBathTemp();
  getTapTemp();
  getAirTemp();
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
      radio_receive(payload.message,payload.value);
    }
   if(temp.equals("READ")) {
    Serial.print("from sensor: ");
    //Serial.println(getBathTemp());
    tap.add(getTap());
    
    } 
  
}

void radio_send(int message,float value) {
  
    radio.stopListening();
    // Take the time, and send it.  This will block until complete
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

void radio_receive(int m,float val) {
  if(m == 0){
    temp = "READ";
    
    }
  if(m == 1){
      //start reading values
      tot_vol = 0.0;
      curr_vol = 0.0;
      heat = 0.0;
      prediction = 0.0;
      mean = 0.0;
      variance = 0.0;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Reading values");
      stat = "START";
      measure_start();
      }
      
    if(m == 2){
      //stop reading values, display final temperature
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Bath Temp: ");
      lcd.print(getBathTemp());
      lcd.setCursor(0,1);
      lcd.print("Determining cooling");
      measure_stop();

      //do newton's law here and send to database
      newton_cool();
      }
    if(m == 3){
      curr_vol = 0.0;
      tot_vol = 0.0;
      heat = 0.0;
      sys_temp = 0.0;
      FIN_VOL = 0.0;
      prediction = 0.0;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("All values reset");
      tap.clear();
      start.clear();
      stopped.clear();
      mean = 0.0;
      variance = 0.0;
      tmp = false;
      
      //empty database
      
      }
    if(m == 4){
      //add to heat equation
      curr_vol = val;
      tot_vol = tot_vol + curr_vol;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Wait for 1 min:");
      delay(1000L*60L);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Bath Temp:");
      //lcd.setCursor(0,1);
      lcd.print(getBathTemp());
      
      tap.clear();
      temp = "no";
      }
    
    if(m==7) {
      if(tot_vol<FIN_VOL){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Predicting..");
        delay(1000L*60L);
        //might want to use average. let's see after testing.
        prediction = (4182*tot_vol*getBathTemp()) + (4182*(FIN_VOL-tot_vol)*getTap());
        prediction = prediction/(FIN_VOL*4182);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Pred Temp:");
        lcd.print(prediction);
        lcd.setCursor(0,1);
        lcd.print("Tap:");
        lcd.print(getTap());
        }
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Bath full!");
        }
      
      
      }  
    if(m == 11) {
      tmp = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Using TMP007");
      }
    if(m == 15) {
      FIN_VOL = val;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Volume Set");
      
      }    
  }

float getTap() {
  if(tmp == false) {
    return getTapTemp();
    }
  if(tmp == true) {
    return getTMP();
    }  
  }

void recurse_moments(double datapoint,int dataindex){
  if(dataindex == 1){
    mean = datapoint;
    variance = 0.0;
    }
  else {
    double b = 1/((double)dataindex);
    double a = 1.0 - b;
    variance = a * (variance + (b*(mean - datapoint)*(mean - datapoint)));
    mean = (a*mean)+(b*k);
    }  
  }
  
void newton_cool() {
  delay(1000);
  radio_send(5,0);
  
  float airtemp = getAirTemp();
  int steps = 25;
  float old_bathtemp = getBathTemp();
  float new_bathtemp; 
  int j = 0;
  float first = getBathTemp();
  double k_estimation;
  delay(5L*1000L);
  for(int i=1;i<=steps;i++) {
    delay(1000L*55L); 
    new_bathtemp = getBathTemp();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Bath Temp:");
    lcd.print(getBathTemp());
    //lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("Air Temp:");
    lcd.print(getAirTemp());
    
    if((new_bathtemp - airtemp > 0.01) && (old_bathtemp - airtemp>0.01)) {
      j = j+1;
      k_estimation = log((old_bathtemp - airtemp)/(new_bathtemp - airtemp))/1; //here, we are estimating k in terms of minutes. if you want seconds, diviide by 60
      Serial.print("At t = ");
      Serial.print(i);
      Serial.println(k_estimation);
      recurse_moments(k_estimation,j);
      }
    
    if(i<10){
      double a = (new_bathtemp - airtemp)*(exp(-k_estimation*(10-i))) + airtemp;
      delay(1000);
      radio_send(10,a);
    }
    if(i<20) {
      double b = (new_bathtemp - airtemp)*(exp(-k_estimation*(20-i))) + airtemp;
      delay(1000);
      radio_send(20,b);
      }
    if(i<30){
      double c = (new_bathtemp - airtemp)*(exp(-k_estimation*(30-i))) + airtemp;
      delay(1000);
      radio_send(30,c);
      }
    if(i<40){
      double d = (new_bathtemp - airtemp)*(exp(-k_estimation*(40-i))) + airtemp;
      delay(1000);
      radio_send(40,d);
    }
    if(i<50){
      double e = (new_bathtemp - airtemp)*(exp(-k_estimation*(50-i))) + airtemp;
      delay(1000);
      radio_send(50,e);
      }
     
    double f = (new_bathtemp - airtemp)*(exp(-k_estimation*(60-i))) + airtemp;
    delay(1000);
    radio_send(60,f);
    
    delay(1000);  
    //Determine time at which it cools to 37   
    old_bathtemp = new_bathtemp;
    }
  Serial.println("Mean: ");
  Serial.print(mean);
  //Serial.print()
  delay(2000);
  radio_send(7,0);
  
  }

void measure_start() {
  for(int i=0;i<5;i++){
    start.add(height());
    }
  }
  
void measure_stop() {
  for(int i=0;i<5;i++){
    stopped.add(height());
    }
  }  
