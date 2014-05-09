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

#include "MenuState.h"
#ifndef SSD1306_128_64//Always check Class files with Actual #define strings!
  #include <Adafruit_SSD1306.h>
#endif

#ifndef BASESTATE_H//Always check Class files with Actual #define strings!
  #include "BaseState.h"
#endif

static char* main_menu[9] = {
  "Time","E-Mail", "Twitter", "Facebook", "Reddit", "Option 6", "Option 7", "Option 8", "Option 9"};
static int main_menu_action[9] = {
  1,2, 3, 4, 5, 6, 7, 8, 9};
static int top_menu_id = 0;
static int hilight_menu_id = 0;

static byte MENU_STATE_APP_ID = 0x01;
static byte MENU_STATE_APP_ACTION_1 = 0x00;
static byte MENU_STATE_APP_ACTION_2 = 0x01;

MenuState::MenuState(Adafruit_SSD1306 *screen) : BaseState(_screen){
  STATE_ID = "MENUSTATE";
  _screen =  screen;
  setupMenu();
}

MenuState::~MenuState(){
  
}

void MenuState::setupMenu(){
  top_menu_id = 0;
  hilight_menu_id = 0;
}

void MenuState::render(){
  _screen->setTextSize(1);
  switch (hilight_menu_id) {
  case 0:
    _screen->fillRect(0, 0, _screen->width(), _screen->height()/3, WHITE);
    _screen->drawRect(0, _screen->height()/3, _screen->width(), _screen->height()/3, WHITE);
    _screen->drawRect(0, (_screen->height()/3)*2, _screen->width(), _screen->height()/3, WHITE);
    break;
  case 1:
    _screen->drawRect(0, 0, _screen->width(), _screen->height()/3, WHITE);
    _screen->fillRect(0, _screen->height()/3, _screen->width(), _screen->height()/3, WHITE);
    _screen->drawRect(0, (_screen->height()/3)*2, _screen->width(), _screen->height()/3, WHITE);
    break;
  case 2:
    _screen->drawRect(0, 0, _screen->width(), _screen->height()/3, WHITE);
    _screen->drawRect(0, _screen->height()/3, _screen->width(), _screen->height()/3, WHITE);
    _screen->fillRect(0, (_screen->height()/3)*2, _screen->width(), _screen->height()/3, WHITE);
    break;
  } 

  if(hilight_menu_id == 0)
    _screen->setTextColor(BLACK);
  else
    _screen->setTextColor(WHITE);

  if(top_menu_id<=8)
  {
    _screen->setCursor(6,6);
    _screen->println(main_menu[top_menu_id]);
  }

  if(hilight_menu_id == 1)
    _screen->setTextColor(BLACK);
  else
    _screen->setTextColor(WHITE);

  if(top_menu_id<=7)
  {
    _screen->setCursor(6, (_screen->height()/3)+6);
    _screen->println(main_menu[top_menu_id+1]);
  }

  if(hilight_menu_id == 2)
    _screen->setTextColor(BLACK);
  else
    _screen->setTextColor(WHITE);

  if(top_menu_id<=6)
  {
    _screen->setCursor(6, ((_screen->height()/3)*2)+6);
    _screen->println(main_menu[top_menu_id+2]);
  }
  //Popup test
  /*
  _screen->fillRect(8, 8, _screen->width()-16, _screen->height()-16, BLACK);
  _screen->fillRect(10, 10, _screen->width()-20, _screen->height()-20, WHITE);
   _screen->setTextColor(BLACK);
  _screen->setCursor(12, 12);
  _screen->println("Error: Please");
  _screen->setCursor(12, 22);
  _screen->println("Connect BLE");*/
  
  _screen->display();
}

void MenuState::btnInterruptAction(boolean isDimmed){
      Serial.println("Btn intterupt requested");
      if(getDisplayDimStatus() == true){
        _screen->dim(false);
        setDisplayDimStatus(false);
      }
      else
      {
        Serial.println("Not dimmed, so change states");

        int currentMenuID = getSelectedMenuID();
        if(getSelectedMenuAction(currentMenuID) == 1)
        {
          
          char *stateID = "TIMESTATE";
          makeChangeRequest(stateID);
        } 
      }
}

void MenuState::btnDownAction(boolean isDimmed){
    if(getDisplayDimStatus() == true){
      _screen->dim(false);
      setDisplayDimStatus(false);
    }
    else
    {
      if(hilight_menu_id>=2)
          top_menu_id++;
      else
          hilight_menu_id++;
      _screen->clearDisplay();
      render();
    }
}

void MenuState::btnUpAction(boolean isDimmed){
      if(getDisplayDimStatus() == true){
        _screen->dim(false);
        setDisplayDimStatus(false);
      }
      else
      {
        if(hilight_menu_id<=0)
          top_menu_id--;
        else
          hilight_menu_id--;
        if(top_menu_id<=0)
          top_menu_id = 0;
        if(hilight_menu_id<=0)
          hilight_menu_id = 0;   
        _screen->clearDisplay();
        render();
      }
}

int MenuState::getSelectedMenuID(){
  return top_menu_id+hilight_menu_id;
}

int MenuState::getSelectedMenuAction(int id){
  return main_menu_action[id];
}

void MenuState::incomingMessageCallback(const struct ble_msg_attributes_value_evt_t *msg) {
  
}

void MenuState::updateDisplay(long lastUpdateTime){
  
}


void MenuState::sync(){
  
}




