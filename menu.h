#ifndef MENU_H
#define MENU_H

#include "hardware.h"
#include "dsp.h"

/*
___________ __         .__     
\_   _____//  |_  ____ |  |__  
 |    __)_\   __\/ ___\|  |  \ 
 |        \|  | \  \___|   Y  \
/_______  /|__|  \___  >___|  /
        \/           \/     \/ 
   _____                       
  /     \   ____   ____  __ __ 
 /  \ /  \_/ __ \ /    \|  |  \
/    Y    \  ___/|   |  \  |  /
\____|__  /\___  >___|  /____/ 
        \/     \/     \/        
        
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
This library implements a set of menu functions for the Etch Module.
*/


/*******************************************
* CV Mode Operation                        *
*******************************************/

// Potentiometers:
// • Bit Crush     - Adjusts scale probability threshhold
// • Sample Crush  - Adjusts BPM (0-300 BPM). When put zero, then Sample Crush CV as gate
// • Glide         - Determins how quickly the current note rises and falls

// CV Inputs:
// • Input         - Pitch CV input

// Buttons:
// • Loop          - Puts the module into loop mode

// Additional Menu Options:
// • Quant Root    - Root note of the quantized scale
// • Quant Scale   - Selects the scale
// • Morph Rate    - In Loop Mode this determines the percentage of time that new samples are added to loop
// • Loop Length   - Determines the number of notes in the loop
// • Arpeggiation  - Style of Arpeggiation (0 - none... ?)

/*******************************************
* Header Definitions                       *
*******************************************/

#define OPT_INT   0  // Normal integer type, appears with a bar
#define OPT_SCALE 1  // Text option for different scales "Major", "Minor", etc.
#define OPT_NOTE  2  // C, C#, D, D#, E, F, F#, G, G#, A, A#, B

#define OPT_LOOP_NO     0
#define OPT_LOOP_YES    1
#define OPT_LOOP_EITHER 3

// Main Menu Label Strings
const char MENU_AUDIO_MODE[]        PROGMEM = "Audio Mode         ";
const char MENU_QUANTIZER_MODE[]    PROGMEM = "Quantizer Mode     ";
const char MENU_CALLIBRATION_MODE[] PROGMEM = "Callibration Mode  ";

#define NUM_MENU_MODES 4
uint8_t menu_counts[NUM_MENU_MODES] = {0};
const char* MAIN_MENU[] = { MENU_AUDIO_MODE, MENU_QUANTIZER_MODE, MENU_CALLIBRATION_MODE };



// Audio Menu Label Strings
const char MENU_LOOP_LENGTH[] PROGMEM = "Loop Length  ";
const char MENU_RESONANCE[]   PROGMEM = "Resonance    ";
const char MENU_REVERB_AMT[]  PROGMEM = "Reverb Amount";
const char MENU_REVERB_DLY[]  PROGMEM = "Reverb Delay ";
const char MENU_REVERB_FBK[]  PROGMEM = "Reverb Feedbk";
const char MENU_MORPH_RATE[]  PROGMEM = "Morph Rate   ";

// CV Menu Label Strings
const char MENU_QUANT_ROOT[]  PROGMEM = "Quant Root   ";
const char MENU_QUANT_SCALE[] PROGMEM = "Quant Scale  "; // 0 - Major, 1 - Minor

// Menu Option Strings
const char OPTION_SCALE_MAJOR[] PROGMEM = "Major";
const char OPTION_SCALE_MINOR[] PROGMEM = "Minor";


// Menu Setting Class
struct MenuSetting {
  uint8_t     mode;
  const char* label;
  uint8_t     value;
  uint8_t     max;
  uint8_t     inc;
  uint8_t     type;
  uint8_t     loopMode;
};

// Setting Identifiers:
#define MS_AUD_LOOP_LENGTH  0
#define MS_AUD_MORPH_RATE   1
#define MS_AUD_RESONANCE    2
#define MS_AUD_REVERB_AMT   3
#define MS_AUD_REVERB_DLY   4
#define MS_AUD_REVERB_FBK   5
#define MS_CV_QUANT_ROOT    6
#define MS_CV_QUANT_SCALE   7
#define MS_CV_LOOP_LENGTH   8
#define MS_CV_MORPH_RATE    9


#define NUM_MENU_SETTINGS 10
MenuSetting MenuSettings[ NUM_MENU_SETTINGS ]{

//  Mode  Label String      Val   Max   Increment  Type       Loop Mode Required?
  { 1,    MENU_LOOP_LENGTH, 0x10, 0xFF, 0x04,      OPT_INT,   OPT_LOOP_YES    },
  { 1,    MENU_MORPH_RATE,  0x01, 0x0F, 0x01,      OPT_INT,   OPT_LOOP_YES    },
  { 1,    MENU_RESONANCE,   0x00, 0xFF, 0x04,      OPT_INT,   OPT_LOOP_EITHER },

  { 1,    MENU_REVERB_AMT,  0x00, 0xFF, 0x04,      OPT_INT,   OPT_LOOP_EITHER },
  { 1,    MENU_REVERB_DLY,  0x80, 0xFF, 0x04,      OPT_INT,   OPT_LOOP_EITHER },
  { 1,    MENU_REVERB_FBK,  0x80, 0xFF, 0x02,      OPT_INT,   OPT_LOOP_EITHER },


  { 2,    MENU_QUANT_ROOT,  0x00, 0x0C, 0x01,      OPT_NOTE,  OPT_LOOP_EITHER },
  { 2,    MENU_QUANT_SCALE, 0x00, 0x15, 0x01,      OPT_SCALE, OPT_LOOP_EITHER },
  { 2,    MENU_LOOP_LENGTH, 0x10, 0xFF, 0x01,      OPT_INT,   OPT_LOOP_YES },
  { 2,    MENU_MORPH_RATE,  0x00, 0xFF, 0x01,      OPT_INT,   OPT_LOOP_YES }

};




// Default Menu Template Character Map
uint8_t menuTemplate[84] = {
  0xC4, 0xC4, 0xC2, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC2, 0xC4, 0xC4,
  0x20, 0x20, 0xB3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20,
  0xAE, 0x20, 0xB3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0xAF,
  0x20, 0x20, 0xB3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xB3, 0x20, 0x20
};

// Define a gradient of characters for progress bars
uint8_t gradChars[5] = { 0x20, 0xB0, 0xB1, 0xB2, 0xDB }; // characters that progressively go from blank to all white

// Note Scale
const char notes[12]  = {'C','C','D','D','E','F','F','G','G','A','A','B'};
const char sharps[12] = {' ','#',' ','#',' ',' ','#',' ','#',' ','#',' '};

// Scale Names
const char SCALE_00[] PROGMEM = "Chromatic    ";
const char SCALE_01[] PROGMEM = "Major        ";
const char SCALE_02[] PROGMEM = "Major Weight ";
const char SCALE_03[] PROGMEM = "Minor        ";
const char SCALE_04[] PROGMEM = "Minor Weight ";
const char SCALE_05[] PROGMEM = "Diminished   ";
const char SCALE_06[] PROGMEM = "Wholetone    ";
const char SCALE_07[] PROGMEM = "Dorian       ";
const char SCALE_08[] PROGMEM = "Blues        ";
const char SCALE_09[] PROGMEM = "Mixolydian   ";
const char SCALE_10[] PROGMEM = "Hungarian    ";
const char SCALE_11[] PROGMEM = "Pentatonic   ";
const char SCALE_12[] PROGMEM = "Melodic Minor";
const char SCALE_13[] PROGMEM = "Arabian      ";
const char SCALE_14[] PROGMEM = "Balinese     ";
const char SCALE_15[] PROGMEM = "Spanish Gypsy";
const char SCALE_16[] PROGMEM = "Oriental     ";
const char SCALE_17[] PROGMEM = "Prometheus   ";
const char SCALE_18[] PROGMEM = "Japanese     ";
const char SCALE_19[] PROGMEM = "Egyptian     ";
const char SCALE_20[] PROGMEM = "Iberian      ";
const char SCALE_21[] PROGMEM = "Romanian     ";



const char* const scaleNames[] = { SCALE_00, SCALE_01, SCALE_02, SCALE_03, SCALE_04,
                                   SCALE_05, SCALE_06, SCALE_07, SCALE_08, SCALE_09, 
                                   SCALE_10, SCALE_11, SCALE_12, SCALE_13, SCALE_14,
                                   SCALE_15, SCALE_16, SCALE_17, SCALE_18, SCALE_19,
                                   SCALE_20, SCALE_21 };


class Menu {
  private:
    Hardware* hw;

    // Display Character Buffer:
    uint8_t  *display_buffer        = NULL; // Contains a pointer to the display buffer
    uint8_t   display_char_buffer[SCREEN_BUFFER_COLS * SCREEN_BUFFER_ROWS]; // Contains a character buffer for the whole display in a 21x8 grid
    uint8_t   char_buffer_page = 0;         // Determines which of the two char buffer pages is currently turned on
    uint16_t  display_offset   = 0;         // The desired pixel offset of the display
    uint16_t  display_offset_c = 0;         // The current pixel offset of the display

    uint8_t selectedMode  = 0;        // used for the mode selection menu


    // Callback Functions:
    void (*cb_settingChange)() = NULL; // Event triggers when a setting in the menu changes
    void (*cb_modeChange)()    = NULL; // Event triggers when a new menu mode is chosen

    // Menu Generation functions:
    void swapCharPage( uint8_t direction );           // Animate in the hidden page. 0 - right to left, 1 - left to right
    void generateBottomMenu( MenuSetting &setting, bool isVisible );
    void generateBottomMenu( char *label, uint16_t val, uint8_t type, bool isVisible ); // Create populated menu template in the buffer
    void drawBottomMenu();

    void generateModeMenu( bool isVisible );
    void drawModeMenu();


  public:
    Menu( Hardware* _hw ){ hw = _hw; }; //Constructor

    // Menu Settings:
    uint8_t currentSetting = 0;        // The currently selected menu item
    uint8_t currentMode    = 1;        // maps to the current menu mode
    uint8_t setting_type   = 0;        // stores the type of the current setting

    void setup();

    // Button controls:
    void nextSetting();                // Select the next settings (move menu right to left)
    void prevSetting();                // Select the previous setting (move menu left to right)
    void checkSetting();               // Ensure the current setting is valid, and if not, find the next good one.

    void updateSetting();              // Update current setting based on current rotary encoder value
    void setMenuMode( uint8_t mode );
    void modeSelect();                 // Triggered when user presses the rotary encoder

    // Callbacks:
    void onSettingChange( void (*fn)() ){ cb_settingChange = fn; }  // Assign callback for changing a menu setting
    void onModeChange(    void (*fn)() ){ cb_modeChange    = fn; }  // Assign callback for changing switching menus

    // Render:
    void drawMenu();
    void drawNum( uint8_t line, uint16_t val);
    void updateMenu();

    // Menu Setting Fetch Options:
    uint8_t getAudLoopLength(){ return( hw->loop ? MenuSettings[MS_AUD_LOOP_LENGTH].value : 0 ); }
    uint8_t getAudMorphRate(){  return( MenuSettings[ MS_AUD_MORPH_RATE ].value ); }
    uint8_t getAudResonance(){  return( MenuSettings[ MS_AUD_RESONANCE  ].value ); }

    uint8_t getReverbAmount(){   return( MenuSettings[ MS_AUD_REVERB_AMT ].value ); }
    uint8_t getReverbDelay(){    return( MenuSettings[ MS_AUD_REVERB_DLY ].value ); }
    uint8_t getReverbFeedback(){ return( MenuSettings[ MS_AUD_REVERB_FBK ].value ); }

    uint8_t getRoot(){         return( MenuSettings[ MS_CV_QUANT_ROOT  ].value ); }
    uint8_t getScale(){        return( MenuSettings[ MS_CV_QUANT_SCALE ].value ); }
    uint8_t getCVLoopLength(){ return( hw->loop ? MenuSettings[ MS_CV_LOOP_LENGTH ].value : 0 ); }
    uint8_t getCVMorphRate(){  return( MenuSettings[ MS_CV_MORPH_RATE  ].value ); }

};


void Menu::setup(){
  for( uint8_t i=0; i<NUM_MENU_SETTINGS; i++ ){
    menu_counts[MenuSettings[i].mode]++;
  }


  display_buffer = hw->displayBuffer();   // Grab a reference to the display buffer so we can write to it
  setMenuMode( 0 );                       // Initiate the first setting along with the encoder configuration
}

/*******************************************
* Mode Menu Display Functions              *
*******************************************/

void Menu::generateModeMenu( bool isVisible ){
  uint8_t page = isVisible ? char_buffer_page : (char_buffer_page + 1) % 2;
  uint8_t *dPtr = &display_char_buffer[page * SCREEN_VISIBLE_COLS];
  for( int8_t i = selectedMode-3; i<=selectedMode+3; i++ ){
    memset( dPtr, 0x20, SCREEN_VISIBLE_COLS );
    if( (i >= 0) && (i < NUM_MENU_MODES-1) ){
      memcpy_P( dPtr+1, MAIN_MENU[i], 19 );
    }
    dPtr += SCREEN_BUFFER_COLS;
  }
  memset( dPtr, 0x20, SCREEN_VISIBLE_COLS );
}

void Menu::drawModeMenu(){
  display_offset_c = (display_offset_c + display_offset) >> 1;
  uint8_t charSubCol = display_offset_c % CHAR_WIDTH;
  uint8_t charCol    = (display_offset_c / CHAR_WIDTH) % SCREEN_BUFFER_COLS;

  for( uint16_t col = 0; col < SCREEN_WIDTH-2; col++ ){
    display_buffer[col + (SCREEN_WIDTH * 0)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 0) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 1)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 1) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 2)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 2) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 3)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 3) ] * CHAR_WIDTH + charSubCol ] ^ 0xFF;
    display_buffer[col + (SCREEN_WIDTH * 4)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 4) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 5)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 5) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 6)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 6) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 7)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 7) ] * CHAR_WIDTH + charSubCol ];

    if( ++charSubCol == CHAR_WIDTH ){
      charSubCol = 0;
      if( ++charCol == SCREEN_BUFFER_COLS ) charCol = 0;
    }
  }
}




/*******************************************
* Bottom Menu Display Functions            *
*******************************************/

// Render Character Buffer for Menu
void Menu::generateBottomMenu( MenuSetting &setting, bool isVisible ){
  char label[14];
  strcpy_P( label, setting.label );
  generateBottomMenu( label, setting.value, setting.type, isVisible );  
}


void Menu::generateBottomMenu( char *label, uint16_t val, uint8_t type, bool isVisible ){
  uint8_t page = isVisible ? char_buffer_page : (char_buffer_page + 1) % 2;
  uint8_t *dPtr = &display_char_buffer[SCREEN_BUFFER_COLS*4 + page * SCREEN_VISIBLE_COLS];
  uint8_t *sPtr = &menuTemplate[0];

  setting_type = type;

  memcpy( dPtr, sPtr, SCREEN_VISIBLE_COLS);    // Copy over top line of the menu template
  dPtr += SCREEN_BUFFER_COLS;                  // Increment destination pointer to next row in the buffer
  sPtr += SCREEN_VISIBLE_COLS;                 // Increment to the next row in the source template
  memcpy( dPtr, sPtr, SCREEN_VISIBLE_COLS);    // Copy over next line of the menu template
  memcpy( dPtr+4, label,  13);                 // Overwrite the label 4 columns in from the left (up to 13 characters)
  dPtr += SCREEN_BUFFER_COLS;                  // Increment destination pointer to next row in the buffer
  sPtr += SCREEN_VISIBLE_COLS;                 // Increment to the next row in the source template
  memcpy( dPtr, sPtr, SCREEN_VISIBLE_COLS);    // Copy over next line of the menu template
  dPtr += SCREEN_BUFFER_COLS;                  // Increment destination pointer to next row in the buffer
  sPtr += SCREEN_VISIBLE_COLS;                 // Increment to the next row in the source template
  memcpy( dPtr, sPtr, SCREEN_VISIBLE_COLS);    // Copy over next line of the menu template
  dPtr += 4;                                   // Move four columns to the right so we can output the value

  switch( type ){
    case OPT_INT:
      sprintf( (char *)dPtr, "%3d", val );       // Print the value
      dPtr += 3;                                 // Increment 3 columns to the right to draw the bar visualization
      memset( dPtr, gradChars[4], val/25 );      // Set the cells of the bar that are full to pure white
      dPtr += val/25;                            // Move pointer over by the number of cells that we filled
      memset( dPtr, gradChars[ (val/5)%5 ], 1);  // Set the final cell to the correct amonunt of fill
      break;
    case OPT_NOTE:
      memset( dPtr++, notes[val % 12],  1 );     // Print the note letter
      memset( dPtr++, sharps[val % 12], 1 );     // Print whether the note is sharp or not
      break;
    case OPT_SCALE:
      dPtr -= SCREEN_BUFFER_COLS;                // Go up a row so we can draw the keyboard
      dPtr -= SCREEN_BUFFER_COLS;                // Go up a row so we can draw the keyboard
      memcpy(   dPtr, hw->keyboard, 14 );        // Draw the keyboard
      dPtr += SCREEN_BUFFER_COLS;                // Go to the next row 
      dPtr += SCREEN_BUFFER_COLS;                // Go to the next row 
      memcpy_P( dPtr, scaleNames[val], 13 );     // Write the scale name
      break;
    default:
      break;
  }
}

void Menu::updateMenu(){
  if( setting_type == OPT_SCALE ){
    uint8_t page = 1; //isVisible ? char_buffer_page : (char_buffer_page + 1) % 2;
    uint8_t *dPtr = &display_char_buffer[SCREEN_BUFFER_COLS*4 + page * SCREEN_VISIBLE_COLS];
    dPtr += SCREEN_BUFFER_COLS + 4;
    memcpy( dPtr, hw->keyboard, 14 );
  }
}


// Set up menu swap animation
void Menu::swapCharPage( uint8_t direction ){
  if( direction == 0 ){ //Right to Left
    if( (display_offset & 0xC0) == 0x40 ){
      display_offset_c = 126;
      display_offset   = 253;
    } else {
      display_offset_c = 0;
      display_offset   = 127;
    }
  } else {
    if( (display_offset & 0xC0) == 0x40 ){
      display_offset_c = 126;
      display_offset   = 0;
    } else {
      display_offset_c = 252;
      display_offset   = 126;
    }
  }
}

// Render Menu to Screen
void Menu::drawBottomMenu(){
  display_offset_c = (display_offset_c + display_offset) >> 1;
  uint8_t charSubCol = display_offset_c % CHAR_WIDTH;
  uint8_t charCol    = (display_offset_c / CHAR_WIDTH) % SCREEN_BUFFER_COLS;

  updateMenu();

  for( uint16_t col = 0; col < SCREEN_WIDTH-2; col++ ){
    display_buffer[col + (SCREEN_WIDTH * 4)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 4) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 5)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 5) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 6)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 6) ] * CHAR_WIDTH + charSubCol ];
    display_buffer[col + (SCREEN_WIDTH * 7)] = font5x7[ display_char_buffer[ charCol + (SCREEN_BUFFER_COLS * 7) ] * CHAR_WIDTH + charSubCol ];

    if( ++charSubCol == CHAR_WIDTH ){
      charSubCol = 0;
      if( ++charCol == SCREEN_BUFFER_COLS ) charCol = 0;
    }
  }
}





/*******************************************
* Externally Accessible Functions          *
*******************************************/

void Menu::setMenuMode( uint8_t mode ){
  selectedMode = currentMode-1;
  currentMode = mode;
  if( mode == 0 ){
    hw->configEncoder( selectedMode, 0, NUM_MENU_MODES-2, 1 );
    char_buffer_page = (char_buffer_page + 1) & 0b1;
    generateModeMenu( false );
    swapCharPage( 0 );
  } else {
    currentSetting = NUM_MENU_SETTINGS - 1; // Move pointer to the end of the settings list so it finds the first menu setting
    nextSetting();
  }
}


void Menu::nextSetting(){
  if( menu_counts[currentMode]==0 ) return;
  currentSetting = (currentSetting + 1) % NUM_MENU_SETTINGS;
  while( (MenuSettings[currentSetting].mode != currentMode) || ( (MenuSettings[currentSetting].loopMode!=OPT_LOOP_EITHER) && (MenuSettings[currentSetting].loopMode!=hw->loop)) ){
    currentSetting = (currentSetting + 1) % NUM_MENU_SETTINGS;
  }

  hw->configEncoder( MenuSettings[currentSetting].value, 0, MenuSettings[currentSetting].max, MenuSettings[currentSetting].inc );
  char_buffer_page = (char_buffer_page + 1) & 0b1;
  generateBottomMenu( MenuSettings[currentSetting], false );
  swapCharPage( 0 );
}

void Menu::prevSetting(){
  if( menu_counts[currentMode]==0 ) return;
  currentSetting = (currentSetting + NUM_MENU_SETTINGS - 1) % NUM_MENU_SETTINGS;
  while( (MenuSettings[currentSetting].mode != currentMode) || ( (MenuSettings[currentSetting].loopMode!=OPT_LOOP_EITHER) && (MenuSettings[currentSetting].loopMode!=hw->loop)) ){
  //while( MenuSettings[currentSetting].mode != currentMode ){
    currentSetting = (currentSetting + NUM_MENU_SETTINGS - 1) % NUM_MENU_SETTINGS;
  }

  hw->configEncoder( MenuSettings[currentSetting].value, 0, MenuSettings[currentSetting].max, MenuSettings[currentSetting].inc );
  char_buffer_page = (char_buffer_page + 1) & 0b1;
  generateBottomMenu( MenuSettings[currentSetting], false );
  swapCharPage( 1 );
}

void Menu::checkSetting(){
  if( (MenuSettings[currentSetting].loopMode!=OPT_LOOP_EITHER) && (MenuSettings[currentSetting].loopMode!=hw->loop) ){
    currentSetting = 0;
    nextSetting();
  }
}


void Menu::updateSetting(){
  if( currentMode == 0 ){
    selectedMode = hw->rotVal;
    generateModeMenu( true );
  } else {
    MenuSettings[currentSetting].value = hw->rotVal;
    generateBottomMenu( MenuSettings[currentSetting], true );
  }
}

void Menu::modeSelect(){
  if( currentMode == 0 ){
    setMenuMode( selectedMode + 1 );
  } else {
    setMenuMode( 0 );
  }
  if( cb_modeChange ) cb_modeChange();
}


void Menu::drawMenu(){
  if( currentMode == 0 ){
    drawModeMenu();
  } else {
    drawBottomMenu();
  }
}





#endif