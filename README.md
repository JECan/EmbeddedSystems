# Embedded Systems Projects
## Security System
I implemented a security system for a casino/bank. Microcontroller 1 acts as the actual casino/bank with sensors do notify Microcontroller 2 via USART of the current status of the casino/bank as well as the number of people inside of the building.
There were 4 sensors, 2 photoresistors to detect the people coming in and going out of the building. 1 sensor was a microphone which would simulate a silent alarm/gunshot in the building. The final sensor was a temperature sensor which would act as simulated fire. If any of these sensors were activated, the appropriate flag is sent to the second microcontroller.
There is also a keypad which is used to enter the safe/vault of the system. There is a set of rotating passwords every 5 seconds and if the wrong password is entered three times the alert is sent to microcontroller 2. 

Components used:
* AVR Studio 6
* 2 ATmega1284 microcontrollers
* 1 Keypad
* 2 photoresistors
* 1 microphone
* 1 tmp36 temperature sensor
* 2 16x2 lcd screens
* 2 potentiometers 


All relevant files can be found under `SecuritySystem` 

Link to demo: https://www.youtube.com/watch?v=6Kw8LiY1nk4

## Simon Says
Another project I have done was recreating the Simon Says memory game using an 8x8 led matrix and an LCD screen. EEPROM was also used to store the high score. Source code located in `SimonSays` directory.

Link to the demo: https://www.youtube.com/watch?v=i_mLvQNuinI&feature=youtu.be
