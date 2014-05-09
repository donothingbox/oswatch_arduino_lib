oswatch_arduino_lib
===================

On the Arduino side, these lib files represent the foundation of running the OSWatch. You will need to add these libraries to the Arduino IDE to run the watch code.


NOTES:

1.) NEAR FUTURE PLANS - Currently both Library files and the project itself live in the same GitHub project, and the same Arduino folder. This will change in the near future one the Base Arduino OSWatch Lib is stable. 

The Current existing files that will move to a Abstract Library directory are the following:

BaseState.h
BaseState.cpp
BluetoothManager.h
BluetoothManager.cpp

These 2 classes represent the foundation for the OSWatch Project on the Arduino side. 

2.) LIBRARY NOTES - 

BluetoothManager:

A glorified wrapper for for BGLib. It handles the vast majority of BLE related “legwork”. Some callbacks for events are setup for messages coming from the BLE112/113 and a helper function “transmitMessage” sends messages to BLE. 

It also handles BLE states and connection / disconnection functions. 

BaseState:

All states (from a menu, to a watch face, to a RSS reader, etc need to extend this class. 

In the extended class you define the following functions:

	updateDisplay(timeDifference) —
 
	whatever state is active should has this function called 	in the main loop of the Arduino application. The 		timeDifference var is the millis since last update. Useful 	for graphics, etc. There is also a “dirty” boolean to ease 	re-rendering. 

	btnInterruptAction(boolean isDimmed) — 

	Called when the interruptBtn is pressed. For State Events.
	isDimmed let you know if the screen is dimmed.

	btnUpAction(boolean isDimmed) — 

	Called when the upBtn is pressed. For State Events.	
	

	btnDownAction(boolean isDimmed) — 

	Called when the downBtn is pressed. For State Events.


	incomingMessageCallback(const struct ble_msg *msg);

	This function gets called when the BLE sends a message. 	Whatever state is active will have this be called. Add 		your custom BLE event parsers here. 


Currently, a MenuState class and TimeState Class are included, for reference examples. 

	
