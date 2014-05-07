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

#include "BluetoothManager.h"

#ifndef __BGLIB_H__
  #include "BGLib.h"
#endif

#ifndef SOFTWARESERIAL_H
  #include <SoftwareSerial.h>
#endif

//Best to define these in the main file, easier to turn off and on. This is for reference only
//#define BLUETOOTHMANAGER_DEBUG


// ================================================================
// BLE STATE TRACKING
// ================================================================
// BLE state machine definitions
#define BLE_STATE_STANDBY           0
#define BLE_STATE_SCANNING          1
#define BLE_STATE_ADVERTISING       2
#define BLE_STATE_CONNECTING        3
#define BLE_STATE_CONNECTED_MASTER  4
#define BLE_STATE_CONNECTED_SLAVE   5

// BLE state/link status tracker
static uint8_t ble_state = BLE_STATE_STANDBY;
static uint8_t ble_encrypted = 0;  // 0 = not encrypted, otherwise = encrypted
static uint8_t ble_bonding = 0xFF; // 0xFF = no bonding, otherwise = bonding handle

// ================================================================
// HARDWARE CONNECTIONS AND GATT STRUCTURE SETUP
// ================================================================

// NOTE: this assumes you are using one of the following firmwares:
//  - BGLib_U1A1P_38400_noflow
//  - BGLib_U1A1P_38400_noflow_wake16
//  - BGLib_U1A1P_38400_noflow_wake16_hwake15
// If not, then you may need to change the pin assignments and/or
// GATT handles to match your firmware.
// use SoftwareSerial on pins D2/D3 for RX/TX (Arduino side)

static bleMessageCallback messageCallback;


static uint8_t BLE_RESET_PIN = 4; // BLE reset pin (active-low) default 4
// - BLE P1_5 -> Arduino Digital Pin 5 (BLE host wake-up -> Arduino I/O 5)
static uint8_t BLE_HOST_PIN = 5; //Not applicable in this design, would need to be connected to an Interrupt pin. Future designs may share Physical button interrupts with This
// If using the *_hwake15 project firmware:
static uint8_t BLE_WAKEUP_PIN = 7;  // BLE wake-up pin
static uint8_t LED_1_PIN = 23;
static uint8_t LED_2_PIN = 22;


static SoftwareSerial bleSerialPort(2, 3);
// - BLE P0_4 -> Arduino Digital Pin 2 (BLE TX -> Arduino soft RX)
// - BLE P0_5 -> Arduino Digital Pin 3 (BLE RX -> Arduino soft TX)

// create BGLib object:
//  - use SoftwareSerial por for module comms
//  - use nothing for passthrough comms (0 = null pointer)
//  - enable packet mode on API protocol since flow control is unavailable
static BGLib ble112((HardwareSerial *)&bleSerialPort, 0, 1);

#define BGAPI_GET_RESPONSE(v, dType) dType *v = (dType *)ble112.getLastRXPayload()
#define GATT_HANDLE_C_RX_DATA   17  // 0x11, supports "write" operation
#define GATT_HANDLE_C_TX_DATA   20  // 0x14, supports "read" and "indicate" operations


BluetoothManager::BluetoothManager(uint8_t reset_pin, uint8_t host_pin, uint8_t wakeup_pin, uint8_t led_1_pin, uint8_t led_2_pin){
  BLE_RESET_PIN = reset_pin;
  BLE_HOST_PIN = host_pin;
  BLE_WAKEUP_PIN = wakeup_pin;  
  LED_1_PIN = led_1_pin;
  LED_2_PIN = led_2_pin;
}

BluetoothManager::~BluetoothManager(){
  
}

void BluetoothManager::setupBluetooth(){
  
  // initialize BLE reset pin (active-low)
  pinMode(BLE_RESET_PIN, OUTPUT);
  digitalWrite(BLE_RESET_PIN, HIGH);
  // initialize BLE wake-up pin to allow (not force) sleep mode (assumes active-high)
  pinMode(BLE_WAKEUP_PIN, OUTPUT);
  digitalWrite(BLE_WAKEUP_PIN, LOW);
  // reset module (maybe not necessary for your application)
  digitalWrite(BLE_RESET_PIN, LOW);
  delay(5); // wait 5ms
  digitalWrite(BLE_RESET_PIN, HIGH);
  
   // set up internal status handlers (these are technically optional)
  ble112.onBusy = &onBusy;
  ble112.onIdle = &onIdle;
  ble112.onTimeout = &onTimeout;

  // ONLY enable these if you are using the <wakeup_pin> parameter in your firmware's hardware.xml file
  // BLE module must be woken up before sending any UART data
  ble112.onBeforeTXCommand = &onBeforeTXCommand;
  ble112.onTXCommandComplete = &onTXCommandComplete;

  // set up BGLib event handlers
  ble112.ble_evt_system_boot = &my_ble_evt_system_boot;
  ble112.ble_evt_connection_status = &my_ble_evt_connection_status;
  ble112.ble_evt_connection_disconnected = &my_ble_evt_connection_disconnect;
  ble112.ble_evt_attributes_value = my_ble_evt_attributes_value;
  bleSerialPort.begin(38400);
}

void BluetoothManager::setBLEEventHandle(bleMessageCallback f){
  messageCallback = f;
}


void BluetoothManager::bleStateIndication(){
   // blink Arduino LED based on state:
  //  - solid = STANDBY
  //  - 1 pulse per second = ADVERTISING
  //  - 2 pulses per second = CONNECTED_SLAVE
  //  - 3 pulses per second = CONNECTED_SLAVE with encryption
  uint16_t slice = millis() % 1000;
  if (ble_state == BLE_STATE_STANDBY) {
    digitalWrite(LED_1_PIN, HIGH);
  } 
  else if (ble_state == BLE_STATE_ADVERTISING) {
    digitalWrite(LED_1_PIN, slice < 100);
  } 
  else if (ble_state == BLE_STATE_CONNECTED_SLAVE) {
    if (!ble_encrypted) {
      digitalWrite(LED_1_PIN, slice < 100 || (slice > 200 && slice < 300));
    } 
    else {
      digitalWrite(LED_1_PIN, slice < 100 || (slice > 200 && slice < 300) || (slice > 400 && slice < 500));
    }
  }
}

int BluetoothManager::getState(){
   return ble_state;
}

void BluetoothManager::my_ble_evt_system_boot(const ble_msg_system_boot_evt_t *msg){
#ifdef BLUETOOTHMANAGER_DEBUG
  Serial.print("###\tsystem_boot: { ");
  Serial.print("major: "); 
  Serial.print(msg -> major, HEX);
  Serial.print(", minor: "); 
  Serial.print(msg -> minor, HEX);
  Serial.print(", patch: "); 
  Serial.print(msg -> patch, HEX);
  Serial.print(", build: "); 
  Serial.print(msg -> build, HEX);
  Serial.print(", ll_version: "); 
  Serial.print(msg -> ll_version, HEX);
  Serial.print(", protocol_version: "); 
  Serial.print(msg -> protocol_version, HEX);
  Serial.print(", hw: "); 
  Serial.print(msg -> hw, HEX);
  Serial.println(" }");
#endif

  // system boot means module is in standby state
  //ble_state = BLE_STATE_STANDBY;
  // ^^^ skip above since we're going right back into advertising below

  // set advertisement interval to 200-300ms, use all advertisement channels
  // (note min/max parameters are in units of 625 uSec)
  ble112.ble_cmd_gap_set_adv_parameters(320, 480, 7);
  while (ble112.checkActivity(1000));

  // USE THE FOLLOWING TO LET THE BLE STACK HANDLE YOUR ADVERTISEMENT PACKETS
  // ========================================================================
  // start advertising general discoverable / undirected connectable
  //ble112.ble_cmd_gap_set_mode(BGLIB_GAP_GENERAL_DISCOVERABLE, BGLIB_GAP_UNDIRECTED_CONNECTABLE);
  //while (ble112.checkActivity(1000));

  // USE THE FOLLOWING TO HANDLE YOUR OWN CUSTOM ADVERTISEMENT PACKETS
  // =================================================================

  // build custom advertisement data
  // default BLE stack value: 0201061107e4ba94c3c9b7cdb09b487a438ae55a19
  uint8 adv_data[] = {
    0x02, // field length
    BGLIB_GAP_AD_TYPE_FLAGS, // field type (0x01)
    0x06, // data (0x02 | 0x04 = 0x06, general discoverable + BLE only, no BR+EDR)
    0x11, // field length
    BGLIB_GAP_AD_TYPE_SERVICES_128BIT_ALL, // field type (0x07)
    0xe4, 0xba, 0x94, 0xc3, 0xc9, 0xb7, 0xcd, 0xb0, 0x9b, 0x48, 0x7a, 0x43, 0x8a, 0xe5, 0x5a, 0x19
  };

  // set custom advertisement data
  ble112.ble_cmd_gap_set_adv_data(0, 0x15, adv_data);
  while (ble112.checkActivity(1000));

  // build custom scan response data (i.e. the Device Name value)
  // default BLE stack value: 140942474c69622055314131502033382e344e4657
  uint8 sr_data[] = {
    0x14, // field length
    BGLIB_GAP_AD_TYPE_LOCALNAME_COMPLETE, // field type
    'M', 'y', ' ', 'A', 'r', 'd', 'u', 'i', 'n', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0'
  };
  // get BLE MAC address
  ble112.ble_cmd_system_address_get();
  while (ble112.checkActivity(1000));
  BGAPI_GET_RESPONSE(r0, ble_msg_system_address_get_rsp_t);
  // assign last three bytes of MAC address to ad packet friendly name (instead of 00:00:00 above)
  sr_data[13] = (r0 -> address.addr[2] / 0x10) + 48 + ((r0 -> address.addr[2] / 0x10) / 10 * 7); // MAC byte 4 10's digit
  sr_data[14] = (r0 -> address.addr[2] & 0xF)  + 48 + ((r0 -> address.addr[2] & 0xF ) / 10 * 7); // MAC byte 4 1's digit
  sr_data[16] = (r0 -> address.addr[1] / 0x10) + 48 + ((r0 -> address.addr[1] / 0x10) / 10 * 7); // MAC byte 5 10's digit
  sr_data[17] = (r0 -> address.addr[1] & 0xF)  + 48 + ((r0 -> address.addr[1] & 0xF ) / 10 * 7); // MAC byte 5 1's digit
  sr_data[19] = (r0 -> address.addr[0] / 0x10) + 48 + ((r0 -> address.addr[0] / 0x10) / 10 * 7); // MAC byte 6 10's digit
  sr_data[20] = (r0 -> address.addr[0] & 0xF)  + 48 + ((r0 -> address.addr[0] & 0xF ) / 10 * 7); // MAC byte 6 1's digit
  // set custom scan response data (i.e. the Device Name value)
  ble112.ble_cmd_gap_set_adv_data(1, 0x15, sr_data);
  while (ble112.checkActivity(1000));
  // put module into discoverable/connectable mode (with user-defined advertisement data)
  ble112.ble_cmd_gap_set_mode(BGLIB_GAP_USER_DATA, BGLIB_GAP_UNDIRECTED_CONNECTABLE);
  while (ble112.checkActivity(1000));
  // set state to ADVERTISING
  ble_state = BLE_STATE_ADVERTISING;
  Serial.println("adv");
}

void BluetoothManager::onTXCommandComplete(){
  digitalWrite(BLE_WAKEUP_PIN, LOW);
}

void BluetoothManager::onBeforeTXCommand(){
    // wake module up (assuming here that digital pin 5 is connected to the BLE wake-up pin)
  digitalWrite(BLE_WAKEUP_PIN, HIGH);

  // wait for "hardware_io_port_status" event to come through, and parse it (and otherwise ignore it)
  uint8_t *last;
  while (1) {
    ble112.checkActivity();
    last = ble112.getLastEvent();
    if (last[0] == 0x07 && last[1] == 0x00) break;
  }

  // give a bit of a gap between parsing the wake-up event and allowing the command to go out
  delayMicroseconds(1000);
}

void BluetoothManager::onTimeout(){
  // reset module (might be a bit drastic for a timeout condition though)
  digitalWrite(BLE_RESET_PIN, LOW);
  delay(5); // wait 5ms
  digitalWrite(BLE_RESET_PIN, HIGH);
}

void BluetoothManager::onIdle(){
  // turn LED off when we're no longer busy
  //digitalWrite(LED_PIN, LOW);
}

void BluetoothManager::onBusy(){
  // turn LED on when we're busy
  //digitalWrite(LED_PIN, HIGH);
}

void BluetoothManager::transmitMessage(byte appId, byte appAction, const uint8_t *data_packet){
  uint8_t output_data[data_packet[0]+2];
  output_data[0] = data_packet[0]+2;
  output_data[1] = appId;
  output_data[2] = appAction;
  for(int i=1;i<data_packet[0];i++)
  {
    output_data[i+2] = data_packet[i];
  }
  
  Serial.print("Outgoing Message: ");
  Serial.println(output_data[0]);
  Serial.print("{ ");

  for (uint8_t i = 0; i < output_data[0]; i++) {
  Serial.print(output_data[i]);
  Serial.print(" ");
  }
  Serial.print("}");
  
  
  ble112.ble_cmd_attributes_write(GATT_HANDLE_C_TX_DATA, 0, output_data[0], output_data);
}

void BluetoothManager::checkActivity(){
  ble112.checkActivity();
}

void BluetoothManager::my_ble_evt_attributes_value(const struct ble_msg_attributes_value_evt_t *msg){
#ifdef BLUETOOTHMANAGER_DEBUG
  Serial.print("###\tattributes_value: { ");
  Serial.print("connection: "); 
  Serial.print(msg -> connection, HEX);
  Serial.print(", reason: "); 
  Serial.print(msg -> reason, HEX);
  Serial.print(", handle: "); 
  Serial.print(msg -> handle, HEX);
  Serial.print(", offset: "); 
  Serial.print(msg -> offset, HEX);
  Serial.print(", value_len: "); 
  Serial.print(msg -> value.len, HEX);
  Serial.print(", value_data: ");
  // this is a "uint8array" data type, which is a length byte and a uint8_t* pointer
  for (uint8_t i = 0; i < msg -> value.len; i++) {
    if (msg -> value.data[i] < 16) Serial.write('0');
    Serial.print(msg -> value.data[i], HEX);
  }
  Serial.println(" }");
#endif

  // check for data written to "c_rx_data" handle
  if (msg -> handle == GATT_HANDLE_C_RX_DATA && msg -> value.len > 0) {
    Serial.print("Standard data RX: ");
    Serial.println(msg -> value.data[0]);//ID Byte

   messageCallback(msg);

  }
}

void BluetoothManager::my_ble_evt_connection_disconnect(const struct ble_msg_connection_disconnected_evt_t *msg){
#ifdef BLUETOOTHMANAGER_DEBUG
  Serial.print("###\tconnection_disconnect: { ");
  Serial.print("connection: "); 
  Serial.print(msg -> connection, HEX);
  Serial.print(", reason: "); 
  Serial.print(msg -> reason, HEX);
  Serial.println(" }");
#endif

  // set state to DISCONNECTED
  //ble_state = BLE_STATE_DISCONNECTED;
  // ^^^ skip above since we're going right back into advertising below

  // after disconnection, resume advertising as discoverable/connectable
  //ble112.ble_cmd_gap_set_mode(BGLIB_GAP_GENERAL_DISCOVERABLE, BGLIB_GAP_UNDIRECTED_CONNECTABLE);
  //while (ble112.checkActivity(1000));

  // after disconnection, resume advertising as discoverable/connectable (with user-defined advertisement data)
  ble112.ble_cmd_gap_set_mode(BGLIB_GAP_USER_DATA, BGLIB_GAP_UNDIRECTED_CONNECTABLE);
  while (ble112.checkActivity(1000));

  // set state to ADVERTISING
  ble_state = BLE_STATE_ADVERTISING;

  // clear "encrypted" and "bonding" info
  ble_encrypted = 0;
  ble_bonding = 0xFF;
}

void BluetoothManager::my_ble_evt_connection_status(const ble_msg_connection_status_evt_t *msg){
#ifdef BLUETOOTHMANAGER_DEBUG
  Serial.print("###\tconnection_status: { ");
  Serial.print("connection: "); 
  Serial.print(msg -> connection, HEX);
  Serial.print(", flags: "); 
  Serial.print(msg -> flags, HEX);
  Serial.print(", address: ");
  // this is a "bd_addr" data type, which is a 6-byte uint8_t array
  for (uint8_t i = 0; i < 6; i++) {
    if (msg -> address.addr[i] < 16) Serial.write('0');
    Serial.print(msg -> address.addr[i], HEX);
  }
  Serial.print(", address_type: "); 
  Serial.print(msg -> address_type, HEX);
  Serial.print(", conn_interval: "); 
  Serial.print(msg -> conn_interval, HEX);
  Serial.print(", timeout: "); 
  Serial.print(msg -> timeout, HEX);
  Serial.print(", latency: "); 
  Serial.print(msg -> latency, HEX);
  Serial.print(", bonding: "); 
  Serial.print(msg -> bonding, HEX);
  Serial.println(" }");
#endif

  // "flags" bit description:
  //  - bit 0: connection_connected
  //           Indicates the connection exists to a remote device.
  //  - bit 1: connection_encrypted
  //           Indicates the connection is encrypted.
  //  - bit 2: connection_completed
  //           Indicates that a new connection has been created.
  //  - bit 3; connection_parameters_change
  //           Indicates that connection parameters have changed, and is set
  //           when parameters change due to a link layer operation.

  // check for new connection established
  if ((msg -> flags & 0x05) == 0x05) {
    // track state change based on last known state, since we can connect two ways
    if (ble_state == BLE_STATE_ADVERTISING) {
      ble_state = BLE_STATE_CONNECTED_SLAVE;
    } 
    else {
      ble_state = BLE_STATE_CONNECTED_MASTER;
    }
  }
  // update "encrypted" status
  ble_encrypted = msg -> flags & 0x02;
  // update "bonded" status
  ble_bonding = msg -> bonding;
}
