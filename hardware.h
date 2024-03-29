#ifndef HARDWARE_H
#define HARDWARE_H

/*
___________ __         .__                                   
\_   _____//  |_  ____ |  |__                                
 |    __)_\   __\/ ___\|  |  \                               
 |        \|  | \  \___|   Y  \                              
/_______  /|__|  \___  >___|  /                              
        \/           \/     \/                               
  ___ ___                  .___                              
 /   |   \_____ _______  __| _/_  _  _______ _______   ____  
/    ~    \__  \\_  __ \/ __ |\ \/ \/ /\__  \\_  __ \_/ __ \ 
\    Y    // __ \|  | \/ /_/ | \     /  / __ \|  | \/\  ___/ 
 \___|_  /(____  /__|  \____ |  \/\_/  (____  /__|    \___  >
       \/      \/           \/              \/            \/ 
        
ETCH Firmware source code designed to run on the AVR128DA28.
Copyright (C) 2024 Tyler Klein (Things Made Simple)
Etch Hardware Design by Juanito Moore (Modular for the Masses)


This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

--- Description: ---
This library abstracts the hardware specifics of the etch module 
into a set of events and values that can be used by the rest of the
firmware code.
*/

#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>
#include "font.h"

/*******************************************
* Screen Defaults & Setup                  *
*******************************************/

// The pins for I2C are defined by the Wire-library. 
// Library Assumes: SDA - PA2, SCL - PA3
#define SCREEN_WIDTH   128            // OLED display width, in pixels
#define SCREEN_HEIGHT  64             // OLED display height, in pixels
#define OLED_RESET     -1             // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D           // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define SCREEN_VISIBLE_COLS 21        // The number of visible columns on the screen (not the entire buffer)
#define SCREEN_BUFFER_COLS  42        // The number of columns in the buffer
#define SCREEN_BUFFER_ROWS   8        // The number of rows in the buffer


/*******************************************
* Hardware Input Tracking                  *
*******************************************/

// INPUT PINS:
#define PIN_POT_SR    PIN_PD2         // Sample Rate Potentiometer
#define PIN_CV_SR     PIN_PD3         // Sample Rate CV Input
#define PIN_POT_BC    PIN_PD4         // Bit Crush Potentiometer
#define PIN_CV_BC     PIN_PD5         // Bit Crush CV Input
#define PIN_POT_GLIDE PIN_PF0         // Glide Potentiometer
#define PIN_CV_GLIDE  PIN_PF1         // Glide CV Input

#define PIN_BTN_LOOP  PIN_PA6         // Loop Button
#define PIN_LED_LOOP  PIN_PA7         // LED Indicator for Loop Button

#define PIN_ROT_DN    PIN_PC0         // Rotary Encoder Turn Right
#define PIN_ROT_UP    PIN_PC1         // Rotary Encoder Turn Left
#define PIN_ROT_BTN   PIN_PC2         // Rotary Encoder Button Press

#define PIN_BTN_LEFT  PIN_PA4         // Left Menu Button
#define PIN_BTN_RIGHT PIN_PA5         // Right Menu Button

#define ANALOG_READ_THRESHOLD  5      // Delta that input has to change by in order to update a setting
#define UPDATE_PERIOD          1      // Number of ms between analog reads

#define LONG_PRESS_TIME  500


/*******************************************
 * Rotary Encoder Interrupts & Functions   *
 *******************************************/

// Global Variables used in the Interrupt Service Routine
volatile int16_t rot_value   = 0;                                              // current value of the rotary encoder
volatile uint8_t rot_min     = 0;                                              // current minimum of the rotary encoder
volatile uint8_t rot_max     = 128;                                            // current maximum of the rotary encoder
volatile uint8_t rot_inc     = 1;                                              // the increment that the value increases by
volatile bool    rot_changed = false;                                          // set to true when rotary encoder changes

//Rotary Encoder Interrupt Service Routine - Handles interrupt events caused by rotating the encoder
ISR(PORTC_PORT_vect) {
  PORTC.INTFLAGS = PORTC.INTFLAGS;                                             // Reset the interrupt flag
  if( PORTC.IN & 0b00000010 ){                                                 // If rotary encoder turned counter clockwise
    rot_value -= rot_inc;                                                      // Decrease rotary encoder value by increment
    if( rot_value < rot_min ) rot_value = rot_min;                             // Make sure it didn't go below the minimum
  } else {                                                                     // If rotary encoder turned clockwise
    rot_value += rot_inc;                                                      // Increase rotary encoder value by increment
    if( rot_value > rot_max ) rot_value = rot_max;                             // Make sure it didn't go above the maximum
  }
  rot_changed = true;
}


/*******************************************
 * Primary Hardware Class Definition       *
 *******************************************/

class Hardware{
  private:
    uint32_t next_update = 0;
    uint32_t rot_press_time = 1;


    uint16_t analogIn[6]  = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};       // Buffer containing all of the analog inputs
    uint8_t  digitalIn[4] = {0xFF, 0xFF, 0xFF, 0xFF};                          // Buffer containing all of the digital inputs

    bool analogReadFiltered( uint16_t &val, uint8_t pin, uint8_t threshold );  // Read Analog settings trigger when things change
    bool digitalReadFiltered( uint8_t &val, uint8_t pin );                     // Read Digital settings and trigger when things change
    bool updateEncoder();                                                      // Read Encoder and trigger when things change

    void (*cb_rightPress)()   = NULL;                                          // Event function pointer for when the ">>" button gets pressed
    void (*cb_leftPress)()    = NULL;                                          // Event function pointer for when the "<<" button gets pressed
    void (*cb_loopPress)()    = NULL;                                          // Event function pointer for when the loop button gets pressed
    void (*cb_rotPress)()     = NULL;                                          // Event function pointer for when the encoder gets pressed
    void (*cb_rotPressLong)() = NULL;                                          // Event function pointer for when the encoder gets pressed
    void (*cb_rotRel)()       = NULL;                                          // Event function pointer for when the encoder gets Released
    void (*cb_rotRelLong)()   = NULL;                                          // Event function pointer for when the encoder gets Released after long press

    void (*cb_rotChange)()          = NULL;                                    // Event triggers when encoder rotates, and passes value based on configuration
    void (*cb_sampleRateChange)()   = NULL;                                    // Event function pointer for when the loop button gets pressed
    void (*cb_bitCrushChange)()     = NULL;                                    // Event function pointer for when the loop button gets pressed
    void (*cb_glideChange)()        = NULL;                                    // Event function pointer for when the loop button gets pressed

    Adafruit_SSD1306 screen;


  public:
    uint16_t  sampleRatePot = 0;                                               // Individual reading of the Sample Rate Potentiometer
    uint16_t  sampleRateCV  = 0;                                               // Individual reading of the Sample Rate CV Input
    uint16_t  sampleRate    = 0;                                               // Combined value of the Potentiometer and CV Input

    uint16_t  bitCrushPot   = 0;                                               // Individual reading of the Bit Crush Potentiometer
    uint16_t  bitCrushCV    = 0;                                               // Individual reading of the Bit Crush CV Input
    uint16_t  bitCrush      = 0;                                               // Combined value of the Potentiometer and CV Input

    uint16_t  glidePot      = 0;                                               // Individual reading of the Glide Potentiometer
    uint16_t  glideCV       = 0;                                               // Individual reading of the Glide CV Input
    uint16_t  glide         = 0;                                               // Combined value of the Potentiometer and CV Input

    bool      loop          = 0;                                               // Value of the loop button
    uint16_t  rotVal        = 0;                                               // The current value of the rotary encoder

    char      keyboard[15] = {0};                                              // Contains characters for the current active keyboard


    Hardware();                                                                // Constructor
    void setup();                                                              // Setup function
    void processEvents();                                                      // Process Events Function

    // Event Handler Setup Functions
    void onRightPress(   void (*fn)() ){ cb_rightPress   = fn; }               // Assign callback function for pressing right button
    void onLeftPress(    void (*fn)() ){ cb_leftPress    = fn; }               // Assign callback function for pressing left button
    void onLoopPress(    void (*fn)() ){ cb_loopPress    = fn; }               // Assign callback function for pressing loop button
    void onRotPress(     void (*fn)() ){ cb_rotPress     = fn; }               // Assign callback function for pressing rotary encoder button
    void onRotPressLong( void (*fn)() ){ cb_rotPressLong = fn; }               // Assign callback function for pressing rotary encoder button
    void onRotRel(       void (*fn)() ){ cb_rotRel       = fn; }               // Assign callback function for quick releasing rotary encoder button
    void onRotRelLong(   void (*fn)() ){ cb_rotRelLong   = fn; }               // Assign callback function for releasing rotary encoder button after long press

    void onRotChange(        void (*fn)() ){ cb_rotChange         = fn; }      // Assign callback for rotating the encoder
    void onSampleRateChange( void (*fn)() ){ cb_sampleRateChange  = fn; }      // Assign callback for adjusting the Sample Rate Potentiometer or CV
    void onBitCrushChange(   void (*fn)() ){ cb_bitCrushChange    = fn; }      // Assign callback for adjusting the Bit Crush Potentiometer or CV
    void onGlideChange(      void (*fn)() ){ cb_glideChange       = fn; }      // Assign callback for adjusting the Glide Potentiometer or CV

    // Rotary Encoder Functions:
    void configEncoder( uint8_t val, uint8_t min, uint8_t max, uint8_t increment );

    // Display Functions
    uint8_t *displayBuffer(){ return screen.getBuffer(); }
    void display(){ 
      //TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;                               // Turn off the ISR so it doesn't create a conflict
      screen.display(); 
      //TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;                                 // Turn the ISR back on
    }

    void drawCStr( const char *buffer, uint8_t length, uint8_t line, uint8_t column);
    void drawCStr( const char *buffer, uint8_t length, uint8_t line){ drawCStr(buffer, length, line, 0); }
    void drawNum( uint16_t val, uint8_t line);
};

Hardware::Hardware() : screen(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET){}



/*******************************************
* SETUP                                    *
*******************************************/

void Hardware::setup(){
  // Screen Setup
  delay(10);
  if(!screen.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;); // Don't proceed, loop forever
  }
  screen.clearDisplay();
  screen.display();


  // Setup Pin Mode for Buttons
  pinMode(PIN_BTN_LOOP,  INPUT_PULLUP);                                        // Setup the Loop Button as a pull-up input
  pinMode(PIN_BTN_LEFT,  INPUT_PULLUP);                                        // Setup the Left Button as a pull-up input
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);                                        // Setup the Right Button as a pull-up input

  // Setup Pin Mode for Outputs
  pinMode(PIN_LED_LOOP,  OUTPUT);                                              // Setup the record input as a pull-up input

  //Rotary Encoder Setup
  pinMode(PIN_ROT_BTN,   INPUT_PULLUP);                                        // Setup the rotary encoder button pin as a pull-up input
  pinMode(PIN_ROT_UP,    INPUT_PULLUP);                                        // Setup the rotary encoder up pin as a pull-up input
  pinMode(PIN_ROT_DN,    INPUT_PULLUP);                                        // Setup the record encoder down pin as a pull-up input
  PORTC.PIN0CTRL = 0b00001011;                                                 // Setup the interrupt, PULLUPEN = 1, ISC = 5 trigger low level

}



/*******************************************
* PIN READING                              *
*******************************************/

// Note for the analog reads, we do things in a couple of steps. First we check the value of the analog pot
// Then we compare it to the prior value to see if it has changed by a threshold. If it has, then analogReadFiltered
// will return true and we will update the setting_XXX value. If it didn't, then analogReadFiltered will still update
// the value in the analogIn array (even though that value won't carry to the setting variable). This allows us to
// accommodate drift in the analog inputs.

bool Hardware::analogReadFiltered( uint16_t &val, uint8_t pin, uint8_t threshold ){
  if( val==0xFFFF ){                               // If system just initialized, val will be set to 0xFFFF
    val = analogRead(pin);                         // Overwrite val with the current analog value and
    return( true );                                // Tell the system that we need to update
  }                                                // Otherwise, if everything is already initialized...
  uint16_t vNew = (analogRead(pin) + val) >> 1;    // Average the current value with the new one
  if( abs( val - vNew ) > threshold ){             // See if the change is larger than threshold
    val = vNew;                                    // Update the value
    return( true );                                // Return true since we updated the value
  }                                                // If not larger than threshold, still update the value
  //val = vNew;                                    // to allow for some level of drift over time
  return( false );                                 // but return false to indicate that the value didn't change enough
}

bool Hardware::digitalReadFiltered( uint8_t &val, uint8_t pin ){
  if( val==0xFF ){                                 // If system just initialized, val will be set to 0xFF
    val = digitalRead(pin);                        // Overwrite val with the current digital value and
    return( true );                                // Tell the system that we need to update
  }                                                // Otherwise, if everything is already initialized...
  uint8_t vNew = digitalRead(pin);                 // Grab the current value of the pin
  if( vNew != val ){                               // See if the value changed
    val = vNew;                                    // Update the value
    return( true );                                // Return true since we updated the value
  }                                                // If not
  return( false );                                 // but return false to indicate that the value didn't change enough
}



/*******************************************
* Rotary Encoder Management                *
*******************************************/

//Sets up the parameters used to process the rotary encoder events
void Hardware::configEncoder( uint8_t val, uint8_t min, uint8_t max, uint8_t increment ){
  rot_min = min;                                                               // The minimum value that the rotary encoder can select
  rot_max = max;                                                               // The maximum value that the rotary encoder can select
  rot_value = val;                                                             // The current value of the rotary encoder
  rot_inc = increment;                                                         // The amount to increment / decrement with each change
  rot_changed = true;                                                          // A flag indicating that the encorder changed
}

//Check to see if the encoder changed, and manage the changed flag
bool Hardware::updateEncoder(){
  if( !rot_changed ) return( false );                                          // If didn't change, return false
  rot_changed = false;                                                         // Unset changed flag
  rotVal = rot_value;                                                          // Assign the hardware's value to the global value
  return( true );                                                              // Return true
}



/*******************************************
* Event Management                         *
*******************************************/

// This is the main event loop for checking the value of different inputs
// and triggering the associated events using callback functions

void Hardware::processEvents(){
  bool updateSR = false;
  bool updateBC = false;
  bool updateGl = false;
  bool updateLP = false;
  bool updateRP = false;
  bool updateRot = false;
  //bool updateRotShort = false;
  //bool updateRotLong  = false;


  // Manage timer so events only get updated every UPDATE_PERIOD milliseconds
  unsigned long t = millis();                                                  // Capture the current time
  if(t < next_update) return;                                                  // See if it is time to check for midi updates again
  next_update = t + UPDATE_PERIOD;                                             // Set the next time we need to check for midi updates

  // Read the analog & digital inputs
  TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;                                  // Turn off the ISR so it doesn't create a conflict
  if( analogReadFiltered( analogIn[0], PIN_POT_SR,    ANALOG_READ_THRESHOLD ) ){ sampleRatePot = analogIn[0]; updateSR = true; }
  if( analogReadFiltered( analogIn[1], PIN_CV_SR,     ANALOG_READ_THRESHOLD ) ){ sampleRateCV  = analogIn[1]; updateSR = true; }
  if( analogReadFiltered( analogIn[2], PIN_POT_BC,    ANALOG_READ_THRESHOLD ) ){ bitCrushPot   = analogIn[2]; updateBC = true; }
  if( analogReadFiltered( analogIn[3], PIN_CV_BC,     ANALOG_READ_THRESHOLD ) ){ bitCrushCV    = analogIn[3]; updateBC = true; }
  if( analogReadFiltered( analogIn[4], PIN_POT_GLIDE, ANALOG_READ_THRESHOLD ) ){ glidePot      = analogIn[4]; updateGl = true; }
  if( analogReadFiltered( analogIn[5], PIN_CV_GLIDE,  ANALOG_READ_THRESHOLD ) ){ glideCV       = analogIn[5]; updateGl = true; }

  if( cb_leftPress  && digitalReadFiltered(digitalIn[1], PIN_BTN_LEFT)  && (digitalIn[1]==0) ){ updateLP  = true; }
  if( cb_rightPress && digitalReadFiltered(digitalIn[2], PIN_BTN_RIGHT) && (digitalIn[2]==0) ){ updateRP  = true; }

  if( digitalReadFiltered(digitalIn[3], PIN_ROT_BTN) ){ updateRot = true; }

  // Read the loop button
  if( digitalReadFiltered(digitalIn[0], PIN_BTN_LOOP) ){                       // Check the current state of the loop button
    if( digitalIn[0]==0 ) loop = !loop;                                        // If the loop button changed to "pressed" invert state of "loop"
    cb_loopPress();
  }

  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;                                    // Turn the ISR back on

  digitalWrite( PIN_LED_LOOP, loop );                                          // Set loop LED based on status of loop

  // Run callback functions if the analog values changed
  if( updateSR && cb_sampleRateChange ){ cb_sampleRateChange(); }
  if( updateBC && cb_bitCrushChange   ){ cb_bitCrushChange();   }
  if( updateGl && cb_glideChange      ){ cb_glideChange();      }


  // Run callback functions if button state changes
  if( updateLP )  cb_leftPress();
  if( updateRP )  cb_rightPress();
  if( updateRot ){                                                             // Rotary encoder button state has changed
    if( digitalIn[3] == 0 ){                                                   // The button was pressed
      rot_press_time = millis() + LONG_PRESS_TIME;                             // Capture the pressed time
      if( cb_rotPress ) cb_rotPress();                                         // Trigger the on press event
    } else {                                                                   // The button was released
      if( millis() < rot_press_time ){                                         // See if the button has been held down for less than the long-press threshold
        if( cb_rotRel ) cb_rotRel();                                           // Trigger a short-release event
      } else {                                                                 // If it is more than the long-press threshold
        if( cb_rotRelLong ) cb_rotRelLong();                                   // Trigger a long-release event 
      }
    }
  } else {                                                                     // Rotary encoder button state hasn't changed state
    if( digitalIn[3] == 0 ){                                                   // See if the button is currently held down
      if( (millis() > rot_press_time) && (rot_press_time > 0) ){               // See if the button has been held down longer than long-press threshold and the event hasn't fired already
        rot_press_time = 0;                                                    // Set rot_press_time to zero so the event doesn't re-trigger again
        if( cb_rotPressLong ) cb_rotPressLong();                               // Trigger a long-press event
      }
    }

  }
  if( cb_rotChange && updateEncoder() ){ cb_rotChange(); }
}


void Hardware::drawCStr( const char *buffer, uint8_t length, uint8_t line, uint8_t column){
  uint8_t charSubCol = 0; //display_offset_c % CHAR_WIDTH;
  uint8_t charCol = 0;
  uint8_t colWidth = min(length * CHAR_WIDTH, SCREEN_WIDTH);
  uint8_t *display_buffer = displayBuffer();
  uint16_t offset = (SCREEN_WIDTH * line) + (column * CHAR_WIDTH);

  for( uint16_t col = 0; col < colWidth; col++ ){
    display_buffer[col + offset] = font5x7[ uint8_t(buffer[charCol]) * CHAR_WIDTH + charSubCol ];

    if( ++charSubCol == CHAR_WIDTH ){
      charSubCol = 0;
      charCol++;
    }
  }
}



void Hardware::drawNum( uint16_t val, uint8_t line){
  char buffer[6] = "     ";
  sprintf(buffer, "%d", val);
  uint8_t charSubCol = 0; //display_offset_c % CHAR_WIDTH;
  uint8_t charCol = 0;
  uint8_t *display_buffer = displayBuffer();

  for( uint16_t col = 0; col < 5*CHAR_WIDTH; col++ ){
    display_buffer[col + (SCREEN_WIDTH * line)] = font5x7[ buffer[charCol] * CHAR_WIDTH + charSubCol ];

    if( ++charSubCol == CHAR_WIDTH ){
      charSubCol = 0;
      charCol++;
    }
  }
}

#endif