#include "hardware.h"
#include "dsp.h"
#include "menu.h"

/*
___________ __         .__         .__               
\_   _____//  |_  ____ |  |__      |__| ____   ____  
 |    __)_\   __\/ ___\|  |  \     |  |/    \ /  _ \ 
 |        \|  | \  \___|   Y  \    |  |   |  (  <_> )
/_______  /|__|  \___  >___|  / /\ |__|___|  /\____/ 
        \/           \/     \/  \/         \/        

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
This file provides the main primary loop of the etch module, manages
the hardware events, and sends signals to the various libraries. Most
of the good stuff is in the other files :P

*/



Hardware    hw;         // Class that contains all of low level hardware information (buttons, knobs, encoder, etc.)
DSP         dsp(&hw);   // Class that contains signal input and output processing (including the ISR)
Menu        menu(&hw);  // Class that contains all menu information 

bool fullScreen = false;


/*******************************************
* Event Handler Functions                  *
*******************************************/

void handleSampleRate(){ dsp.setSampleRateExp( min(hw.sampleRatePot + hw.sampleRateCV, uint16_t(1023)) ); }
void handleBitCrush(){   dsp.setBitCrush(      min(hw.bitCrushPot   + hw.bitCrushCV,   uint16_t(1023)) ); }
void handleGlide(){      dsp.setGlide(         min(hw.glidePot      + hw.glideCV,      uint16_t(1023)) ); }

void handleLeftPress(){     if(fullScreen){ fullScreen = false; } else { menu.nextSetting(); }  }
void handleRightPress(){    if(fullScreen){ fullScreen = false; } else { menu.prevSetting(); }  }
void handleUpdateSetting(){ menu.updateSetting(); }

void handleModeSelect(){ if(fullScreen){ fullScreen = false; } else { menu.modeSelect(); } }
void handleScreenViewSelect(){ fullScreen = !fullScreen; }

void handleModeChange(){ dsp.setMode( menu.currentMode ); }

void handleLoopPress(){ menu.checkSetting(); }


/*******************************************
* General Setup Function                   *
*******************************************/

void setup() {
  hw.setup();                                                                  // Initialize the hardware library

  // Connect Event Handlers
  hw.onSampleRateChange( handleSampleRate       );
  hw.onBitCrushChange(   handleBitCrush         );
  hw.onGlideChange(      handleGlide            );
  hw.onLeftPress(        handleLeftPress        );
  hw.onRightPress(       handleRightPress       );
  hw.onRotChange(        handleUpdateSetting    );
  hw.onRotRel(           handleModeSelect       );
  hw.onRotPressLong(     handleScreenViewSelect );
  hw.onLoopPress(        handleLoopPress        );

  menu.setup();                                                                // Initialize menu
  menu.onModeChange( handleModeChange );
  dsp.setup();                                                                 // Initialize signal processing library
}


/*******************************************
* Program Loop Function                    *
*******************************************/

void loop() {
  hw.processEvents();
  switch( menu.currentMode ){
    case 0: // Global Menu Mode
      menu.drawMenu();                                                         // Draw the menu at the bottom of the screen
      break;

    case 1: // Audio Mode

      // Handle Menu Callbacks:
      dsp.setLoopLength(     menu.getAudLoopLength()  );
      dsp.setMorphRate(      menu.getAudMorphRate()   );
      dsp.setResonance(      menu.getAudResonance()   );
      dsp.setReverbAmount(   menu.getReverbAmount()   );
      dsp.setReverbDelay(    menu.getReverbDelay()    );
      dsp.setReverbFeedback( menu.getReverbFeedback() );

      if( fullScreen ){
        dsp.drawOscilloscopeFS();                                              // Draw the Oscilloscope at the top of the screen
      } else {
        dsp.drawOscilloscope();                                                // Draw the Oscilloscope at the top of the screen
        menu.drawMenu();                                                       // Draw the menu at the bottom of the screen
      }
      break;

    case 2: // CV Mode

      dsp.setRoot(       menu.getRoot()         );
      dsp.setScale(      menu.getScale()        );
      dsp.setLoopLength( menu.getCVLoopLength() );
      dsp.setMorphRate(  menu.getCVMorphRate()  );

      if( fullScreen ){
        dsp.drawOscilloscopeFS();                                              // Draw the Oscilloscope at the top of the screen
      } else {
        dsp.drawOscilloscope();                                                // Draw the Oscilloscope at the top of the screen
        menu.drawMenu();                                                       // Draw the menu at the bottom of the screen
      }
      break;

    case 3: // Callibration Mode
      dsp.drawCallibration();                                                  // Draw the Oscilloscope at the top of the screen
      break;
  }

  hw.display();                                                                // Transfer screen buffer to the actual display
}

