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

/*
  This class is an extended wrapper around BGLib located here:
  
  https://github.com/jrowberg/bglib
  
  It is, first of all, and awesome Lib. Secondly, it's under an MIT license.
  You'll see the notes, etc when you download the Lib.
*/

#ifndef BLUETOOTHMANAGER_H
#define BLUETOOTHMANAGER_H
#include <Arduino.h>

#ifndef __BGLIB_H__
  #include "BGLib.h"
#endif

typedef void (*bleMessageCallback)(const struct ble_msg_attributes_value_evt_t *msg);


class BluetoothManager{
  public:
    BluetoothManager(uint8_t reset_pin, uint8_t host_pin, uint8_t wakeup_pin, uint8_t led_1_pin, uint8_t led_2_pin);
    ~BluetoothManager();
    
    void bleStateIndication();
    void setupBluetooth();
    
    
    void setBLEEventHandle(bleMessageCallback f);
    void setBLEDataCallback();
    void checkActivity();
    static void my_ble_evt_system_boot(const ble_msg_system_boot_evt_t *msg);
    static void onTXCommandComplete();
    static void onBeforeTXCommand();
    static void onTimeout();
    static void onIdle();
    static void onBusy();
    static void setBLEIndicatorFlag(boolean shouldDisplay);
    static uint8_t getState();
    void transmitMessage(byte appId, byte appAction, const uint8_t *data_packet);
    void transmitMessage(byte appId, byte appAction, byte appByte1, byte appByte2, byte appByte3, byte appByte4, byte appByte5);

    static void my_ble_evt_attributes_value(const struct ble_msg_attributes_value_evt_t *msg);
    static void my_ble_evt_connection_disconnect(const struct ble_msg_connection_disconnected_evt_t *msg);
    static void my_ble_evt_connection_status(const ble_msg_connection_status_evt_t *msg);
   private:
    
};



#endif
