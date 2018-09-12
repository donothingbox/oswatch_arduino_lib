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
  
/*  PINS FOR BLE112
 1 (Top) Yellow = 3v3 power
 2 (orange) = P2_2
 3 (brown) = P2_1
 4 (purple) = reset
 5 (green) = Ground
 */
 
#include "BluetoothManager.h"
#include <SoftwareSerial.h>
#include "BGLib.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Time.h>

#include "MenuState.h"
#include "TimeState.h"
#include "RSSState.h"


// uncomment the following lines for debug serial output of different classes
//#define DEBUG
#define BLUETOOTHMANAGER_DEBUG

#define TRACE_RAM_USAGE

// ================================================================
// Configure Arduino I/O Pins
// ================================================================

#define INTERRUPT_BTN 6   //Left Button Pin
#define BACK_BTN      16   //Back Button Pin
#define TOP_BTN      15   //Top Button Pin
#define BOTTOM_BTN   14   //Bottom Button Pin
#define MOTOR_PIN    17   //For Vibration Motor
#define LED_1_PIN    23   // Left LED Indicator
#define LED_2_PIN    22   // Right LED Indicator

// ================================================================
// Configure Display USING hardware SPI
// ================================================================

#define OLED_MOSI  11 // MOSI
#define OLED_CLK   13 //SCK
#define OLED_CS    10 //SS
#define OLED_DC    9
#define OLED_RESET  12
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);

// ================================================================
// Configure BUTTONS
// ================================================================

boolean button_matrix_status[4] = {
  false, false,false};
#define INTERRUPT_BTN_ID 0
#define TOP_BTN_ID 1
#define BOTTOM_BTN_ID 2
#define BACK_BTN_ID 3

uint8_t button_matrix_ids[4] = {
  INTERRUPT_BTN, TOP_BTN,BOTTOM_BTN, BACK_BTN};

// ================================================================
// Configure Libraries & Apps
// ================================================================

#define BLE_RESET_PIN  4 // BLE reset pin (active-low) default 4
// - BLE P1_5 -> Arduino Digital Pin 5 (BLE host wake-up -> Arduino I/O 5)
#define BLE_HOST_PIN  3 //Not applicable in this design, would need to be connected to an Interrupt pin. Future designs may share Physical button interrupts with This
// If using the *_hwake15 project firmware:
#define BLE_WAKEUP_PIN  5  // BLE wake-up pin


BluetoothManager bleManager(BLE_RESET_PIN, BLE_HOST_PIN, BLE_WAKEUP_PIN, LED_1_PIN, LED_2_PIN);

static BaseState *activeState;
static MenuState menuState(&display);
static TimeState timeState(&display);
static RSSState rssState(&display);

#define TIME_TO_INDICATE_BLE_STATE_CHANGE  10000


static uint8_t lastRecordedBLEState = 0;
static int timeDisplayingStatusChange = 0;
static boolean bleStateCountdownActive = false;

// ================================================================
// Clock and timer configs
// ================================================================

#define DIM_COUNTDOWN 10
boolean isDisplayDimmed = false;
int countdownTillSleep = DIM_COUNTDOWN;
int heartbeat = 0;

static unsigned long old_time = 0;
static unsigned long new_time = 0;

// ================================================================
// ARDUINO APPLICATION SETUP AND LOOP FUNCTIONS
// ================================================================

// initialization sequence
void setup() {
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display(); // show splashscreen
  delay(1000);
  display.clearDisplay();   // clears the screen and buffer
  // use 38400 since it works at 8MHz
  Serial.begin(38400);
  while (!Serial);
  //Configure BLE
  bleManager.setupBluetooth();
  bleManager.setBLEEventHandle(incomingMessageCallback);
  //Configure Alerts, and turn them off!
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  pinMode(LED_1_PIN, OUTPUT);
  digitalWrite(LED_1_PIN, HIGH);
  pinMode(LED_2_PIN, OUTPUT);
  digitalWrite(LED_2_PIN, LOW);
  //Init Buttons
  pinMode(INTERRUPT_BTN, INPUT);
  pinMode(TOP_BTN, INPUT_PULLUP);
  pinMode(BOTTOM_BTN, INPUT_PULLUP);
  pinMode(BACK_BTN, INPUT_PULLUP);

  digitalWrite(MOTOR_PIN, HIGH);
  delay(300);
  digitalWrite(MOTOR_PIN, LOW);
  
  BaseState::setGlobalScreenRef(&display);

  //Display First Loaded State
  activeState = &menuState;
  activeState->setStateChangeRequestCallback(stateChangeRequested);
  activeState->setBluetoothManager(&bleManager);
  activeState->render();
  

}

// main application loop
void loop() {
  //First, update Clock
  new_time = millis();
  long timeDifference = new_time - old_time;
  old_time = new_time;
  // listens for changes in Buttons, and renders the screen as needed
  buttonInterfaceStateCheck(); 
  //render any screen specific non-input based graphics
  activeState->updateDisplay(timeDifference);
  // keep polling for new data from BLE
  bleManager.checkActivity();
  //Really useful for debugging BLE, but not required
  bleManager.bleStateIndication();

  uint8_t ble_state = bleManager.getState();
  if(ble_state != lastRecordedBLEState)
  {
    //begin new countdown
    bleStateCountdownActive = true;
    lastRecordedBLEState = ble_state;
    timeDisplayingStatusChange = 0;
    bleManager.setBLEIndicatorFlag(true);
  }
  
  if(bleStateCountdownActive)
  {
    timeDisplayingStatusChange+=timeDifference;
    if(timeDisplayingStatusChange>=TIME_TO_INDICATE_BLE_STATE_CHANGE)
    {
      bleManager.setBLEIndicatorFlag(false);
    }
  }
  
  
  
  //TODO This is really hacky, and NEEDS to be changed
  heartbeat++;
  if(heartbeat>=10000){
    heartbeat = 0;
    countdownTillSleep --;
    if(countdownTillSleep<=0)
    {
      display.dim(true);
      isDisplayDimmed = true;
      display.clearDisplay();
      display.display(); 
    }
  }
}

// ================================================================
// BLE message fwd
//
// we do this to allow "global" events parsed here
// ================================================================

void incomingMessageCallback(const struct ble_msg_attributes_value_evt_t *msg) {
   digitalWrite(LED_2_PIN, LOW);
   Serial.println(F("INCOMING - "));
   Serial.print(F("{"));
   
   
    byte test = msg -> value.data[0];
    int incrementor = 0;
    while(test != NULL && incrementor<20){
        test = msg -> value.data[incrementor];
        //Serial.print(char(test));
        incrementor++;
    }
   
   Serial.println(F("}"));
   //Serial.print("Message length is: " );
   //Serial.println(incrementor);
 



   
   Serial.print(msg -> value.data[0]);
   Serial.print(F(" : "));
   Serial.print(msg -> value.data[1]);
   Serial.print(F(" : "));
   Serial.println(msg -> value.data[2]);
   activeState->incomingMessageCallback(msg);
}

// ================================================================
// STATE CHANGER
//
// this acts as a basic state manager
// ================================================================

void stateChangeRequested(char *stateID){
  
  //Use this for mem
  //free(p);
  //malloc(
  
  Serial.print(F("State change requested: ")); 
  Serial.println(stateID); 
  if(strcmp(stateID, "") == 0){
     Serial.println(F("ERROR: UNDEFINED STATE")); 
  }
  else if(strcmp(stateID, "MENUSTATE") == 0){
    Serial.println(F("Changing to MenuState")); 
    //activeState = 0;
    activeState = &menuState;
    activeState->setBluetoothManager(&bleManager);
    activeState->setStateChangeRequestCallback(stateChangeRequested);
    activeState->render();
  }
  else if(strcmp(stateID, "TIMESTATE") == 0){
    Serial.println(F("Changing to TimeState")); 
    //activeState = 0;
    activeState = &timeState;
    activeState->setBluetoothManager(&bleManager);
    activeState->setStateChangeRequestCallback(stateChangeRequested);
    activeState->sync();
  }  
  else if(strcmp(stateID, "RSSSTATE") == 0){
    Serial.println(F("Changing to RSSState")); 
    //activeState = 0;
    activeState = &rssState;
    activeState->setBluetoothManager(&bleManager);
    activeState->setStateChangeRequestCallback(stateChangeRequested);
    activeState->sync();
  }  
}

// ================================================================
// MASTER BUTTON INTERFACE SWITCH STATEMENTS
//
// this acts as a basic input manager
// ================================================================


void buttonInterfaceStateCheck(){
  if(isButtonDown(INTERRUPT_BTN_ID))
    activeState->btnInterruptAction(false);
  else if(isButtonDown(TOP_BTN_ID))
    activeState->btnUpAction(false);
  else if(isButtonDown(BOTTOM_BTN_ID))
    activeState->btnDownAction(false);
  else if(isButtonDown(BACK_BTN_ID))
    activeState->btnBackAction(false);
}

boolean isButtonDown(uint8_t id){
  boolean btn_status = button_matrix_status[id];
  boolean btn_pin = button_matrix_ids[id];
  boolean btn_status_now =  digitalRead(btn_pin);
  if(btn_status != btn_status_now)
  {
    button_matrix_status[id] = btn_status_now;
    if(btn_status == HIGH)
    {
      Serial.println(F("btn down"));
      displayMemory();
      countdownTillSleep = DIM_COUNTDOWN;
      display.dim(false);
      isDisplayDimmed = false;
      return true;
    }
    else
    {
      Serial.println(F("btn up"));
      displayMemory();
      countdownTillSleep = DIM_COUNTDOWN;
      display.dim(false);
      isDisplayDimmed = false;
      return false;
    }
  }
  return false;
}

void displayMemory(){
  #ifdef TRACE_RAM_USAGE
     Serial.print(F(":: :: :: :: :: FREE MEMORY: "));
     Serial.print(freeRam());
     Serial.println(F(" :: :: :: :: ::"));
  #endif
}


int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

