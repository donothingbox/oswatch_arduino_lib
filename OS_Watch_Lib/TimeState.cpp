/*
Copyright 2014 DoNothingBox LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include "TimeState.h"
#ifndef SSD1306_128_64//Always check Class files with Actual #define strings!
  #include <Adafruit_SSD1306.h>
#endif

#ifndef BASESTATE_H//Always check Class files with Actual #define strings!
  #include "BaseState.h"
#endif

#include <Time.h>

#define TIME_ZONE_OFFSET -7

static byte TIME_STATE_APP_ID = 0x02;
static byte TIME_STATE_APP_ACTION_1 = 0x00;
static byte TIME_STATE_APP_ACTION_2 = 0x01;

static boolean isWaitingForResponse = false;
static int millisSinceLastUpdate = 0;

static byte cachedAction;
static uint8_t cashedDataPacket[6];

static int frameCounter = 0;
static int frameClock = 0;

TimeState::TimeState(Adafruit_SSD1306 *screen) : BaseState(_screen){
  STATE_ID = "TIMESTATE";
  _screen =  screen;
}

TimeState::~TimeState(){
}



void TimeState::render(){

}


void TimeState::updateDisplay(long lastUpdateTime){
  frameClock+=lastUpdateTime;
  if(frameClock>=(1000/8)){
    frameCounter++;
    frameClock = 0;
    if(frameCounter>7)
      frameCounter = 0;
  }
  if(false)
  {
    
   // _screen->clearDisplay();
   // _screen->drawBitmap(56, 24,  wheelAnimated[frameCounter], 16, 16, 1);
   // _screen->display();
  }
  else
  {
    if(isWaitingForResponse)
      millisSinceLastUpdate+=lastUpdateTime;
      
    /*
    if(millisSinceLastUpdate>=1000)
    {
        Serial.println("Re sending lost data packet");
        millisSinceLastUpdate = 0;
        getBluetoothManager().transmitMessage(TIME_STATE_APP_ID, cachedAction, cashedDataPacket);
    }*/
  }
}


void TimeState::sync(){
  if(getBluetoothManager().getState() == 4 || getBluetoothManager().getState() == 5)
  {
      millisSinceLastUpdate = 0;
      uint8_t length = (byte) 6;
      uint8_t data_packet[6] = {length,100,100,100,100,254};
      //cashedDataPacket[0] = length;
      //cashedDataPacket[1] = 100;
      //cashedDataPacket[2] = 100;
      //cashedDataPacket[3] = 100;
      //cashedDataPacket[4] = 100;
      //cashedDataPacket[5] = 254;
      //cachedAction = TIME_STATE_APP_ACTION_1;
      getBluetoothManager().transmitMessage(TIME_STATE_APP_ID, TIME_STATE_APP_ACTION_1, data_packet);
  }
  else
  {
    _screen->clearDisplay();
    _screen->setTextSize(1);
    _screen->fillRect(8, 8, _screen->width()-16, _screen->height()-16, BLACK);
    _screen->fillRect(10, 10, _screen->width()-20, _screen->height()-20, WHITE);
    _screen->setTextColor(BLACK);
    _screen->setCursor(12, 12);
    _screen->println(F("Error: Please"));
    _screen->setCursor(12, 22);
    _screen->println(F("Connect Phone"));
    _screen->display();
  }
}


void TimeState::renderClockType1(){
   Serial.println(F("render time type 1"));
  _screen->clearDisplay();
  _screen->setTextSize(5);
  _screen->setCursor(0, 0);
  _screen->setTextWrap(false);
  int8_t hour_time = hour();
  if(hour_time==0)
    hour_time = (uint8_t) 12;
  else if(hour_time>=13)
    hour_time -= (uint8_t) 12;
    
  if(hour_time<=9)
  {
    _screen->setCursor(30, 0);
    _screen->print(hour_time);
  }
  else
    _screen->print(hour_time);
    
  _screen->setCursor(50, 0);
  _screen->print(":");
  _screen->setCursor(70, 0);
  int8_t minute_time = minute();
  
  if(minute_time<=9){ 
    _screen->print("0");
    _screen->setCursor(99, 0);
    _screen->print(minute_time);
  }
  else
    _screen->print(minute_time);
    
    
  _screen->setTextSize(2);

  char *wkdy[7] = {"SUN", "MON", "TUE", "WED", "THR", "FRI", "SAT"};
  _screen->setCursor(5, 43);
  _screen->print(wkdy[weekday()-1]);
  _screen->setCursor(60, 43);
  _screen->print(month());
  _screen->setCursor(78, 43);
  _screen->print("/");
  _screen->setCursor(95, 43);
  _screen->print(day());
  _screen->display();
}


void TimeState::btnInterruptAction(boolean isDimmed){
      if(getDisplayDimStatus() == true){
        _screen->dim(false);
        setDisplayDimStatus(false);
      }
      else
      {
        
        isWaitingForResponse = true;
        uint8_t length = (byte) 6;
        uint8_t data_packet[6] = {length,100,99,98,97,254};
        
        cashedDataPacket[0] = length;
        cashedDataPacket[1] = 100;
        cashedDataPacket[2] = 99;
        cashedDataPacket[3] = 98;
        cashedDataPacket[4] = 97;
        cashedDataPacket[5] = 254;
        cachedAction = TIME_STATE_APP_ACTION_1;
        getBluetoothManager().transmitMessage(TIME_STATE_APP_ID, TIME_STATE_APP_ACTION_1, data_packet);
      }
}

void TimeState::btnUpAction(boolean isDimmed){
    //_screen->clearDisplay();

}

void TimeState::btnDownAction(boolean isDimmed){
      _screen->clearDisplay();
     char *stateID = "MENUSTATE";
     makeChangeRequest(stateID);
}

void TimeState::displayPopup(){

}


void TimeState::incomingMessageCallback(const struct ble_msg_attributes_value_evt_t *msg) {
    isWaitingForResponse = false;
    if(msg -> value.data[0] == 0x02)//Time return
    {
      byte b[4] = {
        msg -> value.data[4], msg -> value.data[3], msg -> value.data[2], msg -> value.data[1] };
      long timeStamp = bytesToInteger(b);
      Serial.print(F("TimeStamp: "));
      Serial.println(timeStamp);
      setTime(timeStamp);
      long timeZoneOffset = 60*60*TIME_ZONE_OFFSET;
      adjustTime(timeZoneOffset);//Adjusting time zone . . . 
      renderClockType1();
    }
}



