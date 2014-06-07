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

#ifndef RSSSTATE_H
#define RSSSTATE_H
#include <Arduino.h>

#ifndef SSD1306_128_64//Always check Class files with Actual #define strings!
  #include <Adafruit_SSD1306.h>
#endif

#ifndef BASESTATE_H//Always check Class files with Actual #define strings!
  #include "BaseState.h"
#endif

class RSSState : public BaseState {
  public:
    RSSState(Adafruit_SSD1306 *screen);
    ~RSSState();
    void setupMenu();
    void updateDisplay(long lastUpdateTime);
    void sync();
    void render();
    void btnInterruptAction(boolean isDimmed);
    void btnUpAction(boolean isDimmed);
    void btnDownAction(boolean isDimmed);
    void btnBackAction(boolean isDimmed);

    void displayPopup();
    int getSelectedMenuID();
    int getSelectedMenuAction(int id);
    void incomingMessageCallback(const struct ble_msg_attributes_value_evt_t *msg);
    Adafruit_SSD1306 *_screen;
};
#endif
