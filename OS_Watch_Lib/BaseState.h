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

#ifndef BASESTATE_H
#define BASESTATE_H
#include <Arduino.h>

#ifndef SSD1306_128_64//Always check Class files with Actual #define strings!
  #include <Adafruit_SSD1306.h>
#endif

#ifndef BLUETOOTHMANAGER_H//Always check Class files with Actual #define strings!
  #include "BluetoothManager.h";
#endif

typedef void (*stateChangeCallback)(char *stateID);

class BaseState{
  public:
    BaseState(Adafruit_SSD1306 *screen);
    ~BaseState();
    
    boolean isDirty;
    //graphical Blocks
    virtual void updateDisplay(long lastUpdateTime);
    virtual void sync();
    virtual void render();
    
    //Input Functions (Required)
    virtual void btnInterruptAction(boolean isDimmed);
    virtual void btnUpAction(boolean isDimmed);
    virtual void btnDownAction(boolean isDimmed);
    virtual void btnBackAction(boolean isDimmed);

    //called when new BLE message is recieved
    virtual void incomingMessageCallback(const struct ble_msg_attributes_value_evt_t *msg);

    //Non Overridden Functions
    boolean getDisplayDimStatus();
    void setDisplayDimStatus(boolean isDimmed);
    //state change functions
    void setStateChangeRequestCallback(stateChangeCallback f);
    void makeChangeRequest(char *stateID);
    void showLoadingWheel();
    void hideLoadingWheel();
    void renderLoadingWheel();
    
    const unsigned char getLoadingWheel();
    void renderWheelGraphic(long lastUpdateTime);
    void renderNoConnectionGraphic();

    //Bluetooth manager access
    BluetoothManager getBluetoothManager();
    void setBluetoothManager(BluetoothManager *bleManager);
    //Screen access
    static Adafruit_SSD1306 getGlobalScreenRef();
    static void setGlobalScreenRef(Adafruit_SSD1306 *screenRef);
    //Vars required
    Adafruit_SSD1306 *_screen;
    char *STATE_ID;
    boolean isLoading;
    //static const unsigned char *wheelAnimated[8];

    
    //Declaired helper functions
    void integerToBytes(long val, byte b[4]);
    long bytesToInteger(byte b[4]);
    long byteToInteger(byte b);
    int freeRam();


    
};
#endif
