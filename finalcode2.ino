#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <NewPing.h>

#define AIR_TEMP 8
#define BATH_TEMP 9
#define TAP_TEMP 10
#define TRIGGER_PIN 2   // to 5
#define ECHO_PIN 3      // to 6
#define MAX_DISTANCE 200

OneWire air(AIR_TEMP);
OneWire bath(BATH_TEMP);
OneWire tap(TAP_TEMP);

DallasTemperature sensor1(&air);
DallasTemperature sensor2(&bath);
DallasTemperature sensor3(&tap);

SoftwareSerial BT(11, 13); //RX, TX
// creates a "virtual" serial port/UART
// connect BT module TX to D11
// connect BT module RX to D13
// connect BT Vcc to 5V, GND to GND 

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

LiquidCrystal lcd(19,14,18,17,16,15);

double current_height;
float maxheight = 11.0; //in cm
double current_temperature;
double current_heat;
double current_prediction;
boolean initprediction = false;
boolean done = false;
float literspersecond = 0.041; //L per second


void setup() {
  current_height = 0;
  maxheight = 0.0;      // change
  current_temperature = 0;
  current_heat = 0.0;
  current_prediction = 0.0;
  
  pinMode(0,OUTPUT);
  analogWrite(15,0);
  lcd.begin(16, 2);
  //lcd.print("hello,world!");
  Serial.begin(9600);
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();
  
  BT.begin(9600);
  BT.println("Print Values");
  
  
  
  
  
}

void printlcd() {
  //lcd.clear();
  //lcd.setCursor(0,1);
  //lcd.print("correct");
  //delay(150);
  }
  
float height() {
  delay(50);
  unsigned int uS = sonar.ping();
  pinMode(ECHO_PIN,OUTPUT);
  digitalWrite(ECHO_PIN,LOW);
  pinMode(ECHO_PIN,INPUT);
  //Serial.print("Ping: ");
  delay(250);
  return (uS / US_ROUNDTRIP_CM);
}

float getAirTemp() {
  delay(500);
  sensor1.requestTemperatures(); 
  return sensor1.getTempCByIndex(0);  
}

float getBathTemp() {
  delay(500);
  sensor2.requestTemperatures(); 
  return sensor2.getTempCByIndex(0);  
}

float getTapTemp() {
  delay(500);
  sensor3.requestTemperatures(); 
  return sensor3.getTempCByIndex(0);  
}


void loop() {
  //lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  //lcd.print(millis()/1000);
  if(initprediction == false) {
    delay(13000);
    initprediction = true;
    if(getTapTemp() >= getAirTemp()) {
      init_hotprediction();
    }
    else {
      init_coldprediction();
    }
    //current_temperature = getTapTemp();
    current_height = 0.0;
    //current_prediction = 0.0;
    current_heat = 0.0;
    }
   else {
    /*
     * if(done == false) {
     * 
     */
    float ht = height();
    float currtap = getTapTemp();
    if(abs(currtap - current_temperature) >=2) {
      double air_temp = getAirTemp();
      double thermal_conductivity = 50;
      Serial.println("Currtap 1");
      Serial.println(currtap);
      double area = 27.5*40.5*0.01*0.01;
      
      float newheight = 30 - height();
      double volume = area*(newheight - current_height) * 0.01;
      
      Serial.println("Height change");
      Serial.println(newheight);
      
      current_heat = current_heat + (volume*1000*4182*current_temperature);
      double heat_loss = 0.0;

      int count = (int)(ceil((volume*1000)/literspersecond));

      for(int i=0;i<count;i++){
         heat_loss = heat_loss + thermal_conductivity*area*(current_temperature - air_temp);
        }
      current_heat = current_heat - heat_loss;
      heat_loss = 0.0;
      
      Serial.println("Current heat");
      Serial.println(current_heat);
      delay(50000);
      currtap = getTapTemp();
      Serial.println("Currtap 1");
      Serial.println(currtap);
      //Serial.println(newheight);

      double factor = 0.0;
      if(currtap<air_temp){
        factor = -2.5;
        }
       if(currtap>air_temp){
        factor = 2.5;
        } 
      
      double vol2 = area*(11.0 - newheight) * 0.01;
      Serial.println("is this zero 1 ");
      Serial.println(4182*11.0*0.01*area*1000);

      double prediction = 4182*currtap*vol2*1000;
      double predict_loss = 0.0;

      int count2 = (int)(ceil((vol2*1000)/literspersecond));
      
      for(int k=0;k<count2;k++){
        predict_loss = predict_loss + thermal_conductivity*area*(currtap - air_temp);
        }

      prediction = prediction - predict_loss;
      predict_loss = 0.0;
      
      current_prediction = (current_heat + prediction)/(4182*11.0*0.01*area*1000);
      current_prediction = current_prediction + factor;
      current_temperature = currtap;
      current_height = newheight;
      
      


      
    }
    /* }
     * float ht1 = height();
     * float ht2 = height();
     * float ht3 = height();
     * if(ht1 == ht2 && ht2==ht3) {
     *  done = true;
     *  print final temperature
     *  newton's law of cooling;
     *  
     * }
     */
      
      
      
    }
    Serial.println("Current Prediction");
    Serial.println(current_prediction);
    //delay(500);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(current_prediction,2);
    Serial.println("Bath Temperature");
    Serial.println(getBathTemp());
    Serial.println("Height");
    Serial.println(30-height()); 
    
}

void init_hotprediction() { 
   Serial.println("Hot Prediction ");
   double area = 27.5*40.5*0.01*0.01;
   double tap_temp = getTapTemp();
   current_temperature = tap_temp;
   Serial.println(tap_temp);
   double air_temp = getAirTemp();
   double thermal_conductivity = 60;
   double specificheat = 4182;
   int density = 1000;   //in kg/m^3
   double flowrate = literspersecond;
   double volume = 11.2; //in L
   double heat = 0.0;
   double heat_loss = 0.0;
   double mass = density * flowrate * 0.001;
   int count = (int)ceil(volume/flowrate);
   for(int i=0;i<count;i++){
    heat = heat + (mass*specificheat*tap_temp);
    heat_loss = thermal_conductivity*area*(tap_temp - air_temp);
    heat = heat - heat_loss;
    }
  float predicttemp = heat/(mass*count*specificheat);
  current_prediction = (double)predicttemp;
  Serial.println(predicttemp);
  //printlcd(predicttemp);  
}

void init_coldprediction() { 
   Serial.println("Cold Prediction ");
   double area = 27.5*39.2*0.01*0.01;
   double tap_temp = getTapTemp();
   current_temperature = tap_temp;
   Serial.println(tap_temp);
   double air_temp = getAirTemp();
   double thermal_conductivity = 60;
   double specificheat = 4182;
   int density = 1000;   //in kg/m^3
   double flowrate = literspersecond;
   double volume = 11.2; //in L
   double heat = 0.0;
   double heat_loss = 0.0;
   double mass = density * flowrate *0.001;
   int count = (int)ceil(volume/flowrate);
   for(int i=0;i<count;i++){
    heat = heat + (mass*specificheat*tap_temp);
    heat_loss = thermal_conductivity*area*(air_temp - tap_temp);
    heat = heat + heat_loss;
    }
  float predicttemp = heat/(mass*count*specificheat);
  current_prediction = (double)predicttemp;
  Serial.println(predicttemp);
  //printlcd(predicttemp);  
}

void newtoncooling() {
  double area = 27.5*39.2*0.01*0.01;
  double thermal_conductivity = 60;
  double airtemp = getAirTemp();
  
  double currentpredict = current_prediction;
  
  for(int i=60;i<=900;i=i+60){
    double system_heat = currentpredict*4182*11.0*0.01*area*1000;
    double losspersecond = thermal_conductivity*area*(currentpredict - airtemp);
    double currheat = system_heat - (losspersecond*60);
    currentpredict = currheat/(4182*11.0*0.01*area*1000);
    Serial.print("At time t= ");
    Serial.print(i/60);
    Serial.print(" minutes temperature is ");
    Serial.print(currentpredict);
    Serial.println("  degrees C");
    }
  
  
  }



