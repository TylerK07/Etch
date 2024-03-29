#ifndef DSP_H
#define DSP_H

/*
___________ __         .__      
\_   _____//  |_  ____ |  |__   
 |    __)_\   __\/ ___\|  |  \  
 |        \|  | \  \___|   Y  \ 
/_______  /|__|  \___  >___|  / 
        \/           \/     \/  
________    ___________________ 
\______ \  /   _____/\______   \
 |    |  \ \_____  \  |     ___/
 |    `   \/        \ |    |    
/_______  /_______  / |____|    
        \/        \/            
        
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
This library implements a set of limited DSP capabilities for the Etch Module,
like bit-crush, sample crush, low-pass filtering, reverb, scale quantizing, etc.

*/


/*******************************************
* Header Definitions                       *
*******************************************/

// Hardware pin map:
#define PIN_IN_AUD    PIN_PD0                                    // Accepts -5 to 5v input and removes DC offset
#define PIN_IN_CV     PIN_PD1                                    // Accepts 0 to 10v and includes DC offset
#define PIN_OUTPUT    PIN_PD6                                    // Audio / CV Output
#define PIN_OFFSET    PIN_PA1                                    // Output Offset for CV vs Audio

// Clock & period settings:
#define M_CLOCK_FRQ   25000000                                   // Master Clock Frequency of the AVR Microcontroller
#define LOW_SAMP_FRQ  32                                         // The lowest frequency for audio sample rate (it goes up from here with the input)

#define OCT_RANGE     9                                          // Number of octaves in the sample rate range
#define ISR_OCT_RANGE 5                                          // Number of octaves that can be adjusted by the ISR
#define UNITS_PER_OCT (1024/OCT_RANGE)                           // Number of units per octave

#define BUFFER_SIZE    256                                       // Using 256 instead of 240 so the 8-bit pointer can just roll over on its own
uint16_t input_buffer[BUFFER_SIZE]={0x200};                      // Stores the input from the audio in
uint16_t output_buffer[BUFFER_SIZE]={0x200};                     // Stores the information to display on the screen
uint16_t morph_buffer[BUFFER_SIZE]={0x200};                      // Stores the morph state

volatile uint16_t bitcrush_conversion[1024] = {0x200};           // Array that holds the bitcrush conversion table for the current bitcrush setting


#define REVERB_BUFFER_SIZE 2048                                  // Size of the reverb buffer. Any bigger and we are going to run out of memory!
uint16_t reverb_buffer[ REVERB_BUFFER_SIZE ]={0x200};            // Reverb buffer that keeps track of the sample history

#define SAMPLE_RATE_CONVERSION_SIZE 1024                         // Pre-calculated sample rate conversion table that converts the linear pot input
uint16_t sample_rate_conversion[SAMPLE_RATE_CONVERSION_SIZE]={0};// into the sample period that determines the ISR period

#define CALLIBRATION_OFFSET 12                                   // Offset variable for callibrating the output of the module


/*******************************************
* DSP Properties / Settings                *
*******************************************/

volatile uint16_t input_index  = 0;                             // Points to the next byte to overwrite in the input buffer
volatile uint16_t output_index = 0;                             // Points to the next byte to overwrite in the output buffer

volatile uint16_t rolling_avg  = 0x200;                         // Rolling average of bitCrush(sampleCrush(input)) - set it to the middle value to start
volatile uint16_t rolling_avg2 = 0x200;                         // Rolling average of bitCrush(sampleCrush(input)) - set it to the middle value to start
volatile uint16_t rolling_avg3 = 0x200;                         // Rolling average of bitCrush(sampleCrush(input)) - set it to the middle value to start
volatile uint16_t rolling_avg4 = 0x200;                         // Rolling average of bitCrush(sampleCrush(input)) - set it to the middle value to start
volatile uint16_t rolling_avg5 = 0x200;                         // Rolling average of bitCrush(sampleCrush(input)) - set it to the middle value to start
volatile uint8_t  resonance = 0;

volatile uint16_t glide         = 0;                            // Filter setting in CV mode that adjusts how quickly a note can change to match the input voltage
volatile uint16_t alpha         = 128;                          // Pre-calculated filter weight: cutoff / ( (sampleRate/(2*Pi)) + cutOff) * 255
volatile uint16_t sample_rate   = 1023;                         // Tracks the current sample rate setting
volatile int16_t  scale_crush   = 0;                            // Holds the current value of the scale_crush setting used in CV mode

volatile uint16_t loop_length   = 0;                            // Length of the loop
volatile uint16_t loop_pointer  = 0;                            // Current sample in the loop to play 
volatile uint8_t  morph_rate    = 4;                            // Rate that new samples get captured and morphed into
volatile uint32_t morph_counter = 16;                           // Percentage of the way through the current morph cycle

volatile uint16_t reverb_read_index    = 0;                     // Current read position within the reverb buffer
volatile uint16_t reverb_write_index   = REVERB_BUFFER_SIZE-1;  // Current write position within the reverb buffer
volatile uint16_t reverb_delay         = REVERB_BUFFER_SIZE-1;  // The number of buffer elements between the read and write pointers
volatile uint8_t  reverb_feedback      = 16;                    // Percentage mix of feedback (out of 256)
volatile uint8_t  reverb_wet_mix       = 128;                   // Percentage mix of original signal (out of 256)

volatile uint8_t  note_offset = 0;                              // The amount to offset the notes by (transposition)
volatile uint8_t  scale_index = 0;                              // This is the current scale that notes are being quantized to

volatile bool     skip_ISR = false;                             // Flag that is turned on while in the ISR to prevent the ISR from running again
volatile uint8_t  ISR_period = 1;                               // Counter that drops the ISR down by a number of octaves - 1: 0 Oct, 2: 1 Oct, 4: 2 Oct, 8: 4 Oct, 16: 5 Oct ... 
volatile uint8_t  ISR_counter = ISR_period;                     // The ISR counts down the counter from period and resets

volatile uint8_t  dsp_mode = 0;                                 // 0 - Off, 1 - Audio mode; 2 - CV mode;

#define SCALE_PROB_RANGE 250                                    // Determines the randomization of the scale thresholds for weighted scales
volatile uint8_t prob_map[12] = {0};                            // Holds the current weighted thresholds for the keys for quantizing 
volatile uint8_t note_scale   = 0;                              // The output note
volatile uint8_t note_oct     = 0;                              // The output octave

bool trigger_mode = false;                                      // Trigger_mode sets the mode of the ISR so that it only advances when trigger_gate is true
bool trigger_gate = false;                                      // When trigger gate is true, the ISR will execute once and then stop (if trigger_mode is true)
bool pause_ISR    = false;                                      // Tracks when the ISR is paused

#define CV_CLOCK_DIVIDER 128;                                   // This is the clock divider count for the CV mode. 
volatile uint16_t clock_divider = 1;                            // clock_divider counts down from CV_CLOCK_DIVIDER to decide when to execute the ISR in CV mode

volatile uint32_t frame_period = 0;                             // Keeps track of the time to render a sample in the ISR. Currently it takes about 60 uS. At 70, things get real laggy


//Mode Definitions:
#define MODE_IDLE  0                                            // In idle mode, the output just sits at 0x200
#define MODE_AUDIO 1                                            // DSP works in Audio mode 
#define MODE_CV    2                                            // DSP works in CV Quantizer mode
#define MODE_CAL   3                                            // DSP goes into calibration mode so you can center the output voltage


// TWEEN FUNCTION NOTES
// • The tween array is a pre-calculated function that provides a smooth "S-curve" transition that eases in and eases out
// • This conversion is used to transition smoothly from one input "grain" to the next in the morph function

uint8_t TWEEN_FN[257] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x02,0x02,
0x02,0x03,0x03,0x04,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07,0x08,0x09,0x09,0x0A,
0x0B,0x0B,0x0C,0x0D,0x0D,0x0E,0x0F,0x10,0x10,0x11,0x12,0x13,0x14,0x15,0x15,0x16,
0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x27,
0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2F,0x30,0x31,0x32,0x33,0x35,0x36,0x37,0x38,0x3A,
0x3B,0x3C,0x3E,0x3F,0x40,0x42,0x43,0x44,0x46,0x47,0x48,0x4A,0x4B,0x4D,0x4E,0x4F,
0x51,0x52,0x54,0x55,0x56,0x58,0x59,0x5B,0x5C,0x5E,0x5F,0x61,0x62,0x63,0x65,0x66,
0x68,0x69,0x6B,0x6C,0x6E,0x6F,0x71,0x72,0x74,0x75,0x77,0x78,0x7A,0x7B,0x7D,0x7E,
0x80,0x81,0x83,0x84,0x86,0x87,0x89,0x8A,0x8C,0x8D,0x8F,0x90,0x92,0x93,0x95,0x96,
0x98,0x99,0x9B,0x9C,0x9D,0x9F,0xA0,0xA2,0xA3,0xA5,0xA6,0xA8,0xA9,0xAA,0xAC,0xAD,
0xAF,0xB0,0xB1,0xB3,0xB4,0xB6,0xB7,0xB8,0xBA,0xBB,0xBC,0xBE,0xBF,0xC0,0xC2,0xC3,
0xC4,0xC6,0xC7,0xC8,0xC9,0xCB,0xCC,0xCD,0xCE,0xCF,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
0xD7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
0xE8,0xE9,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEE,0xEF,0xF0,0xF1,0xF1,0xF2,0xF3,0xF3,
0xF4,0xF5,0xF5,0xF6,0xF7,0xF7,0xF8,0xF8,0xF9,0xF9,0xFA,0xFA,0xFA,0xFB,0xFB,0xFC,
0xFC,0xFC,0xFD,0xFD,0xFD,0xFD,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF, 0xFF 
};


/* various other tween functions where dist goes from 0 to 63, or 0 to 127. In the end I used 0 to 255
#define TWEEN( v1, v2, dist, max ){ int16_t((((int32_t(v2)-int32_t(v1)) * (dist)) / (max)) + (v1)) }
#define TWEEN64(  v1, v2, dist ){ uint16_t( ( uint32_t(v2) * uint8_t(dist) + uint32_t(v1) * (  63-uint8_t(dist) ) ) >> 6 ) }
#define TWEEN128( v1, v2, dist ){ uint16_t( ( uint32_t(v2) * uint8_t(dist) + uint32_t(v1) * ( 127-uint8_t(dist) ) ) >> 7 ) }
*/
// TWEEN256_POS takes a weighted average between v1 and v2 when v2 > v1
#define TWEEN256_POS( v1, v2, dist ) ( uint16_t(v1) + uint16_t((uint32_t(uint16_t(v2)-uint16_t(v1)) * uint32_t(dist)) >> 8) )

// TWEEN256_NEG takes a weighted average between v1 and v2 when v1 > v2
#define TWEEN256_NEG( v1, v2, dist ) ( uint16_t(v1) - uint16_t((uint32_t(uint16_t(v1)-uint16_t(v2)) * uint32_t(dist)) >> 8) )

// TWEEN256 figures out which POS/NEG function to use to take a weighted average between v1 and v2
#define TWEEN256( v1, v2, dist )     ( ( uint16_t(v2)>uint16_t(v1) ) ? TWEEN256_POS(v1,v2,dist) : TWEEN256_NEG(v1,v2,dist) )


/*******************************************
* QUANTIZING DEFINITIONS                   *
*******************************************/
// Scale Probabilities
#define UNITS_PER_NOTE (1024/120)

int16_t SCALE_PROB[22][12] = {
/*  C    C#   D    D#   E    F    F#   G    G#   A    A#   B */
  { 635, 635, 635, 635, 635, 635, 635, 635, 635, 635, 635, 635 }, // Chromatic
  { 635,   0, 635,   0, 635, 635,   0, 635,   0, 635,   0, 635 }, // Major Quantized
  { 635, 223, 348, 233, 438, 409, 252, 519, 239, 366, 229, 288 }, // Major Weighted
  { 635,   0, 635, 635,   0, 635,   0, 635, 635,   0, 635,   0 }, // Minor Quantized
  { 635, 268, 352, 538, 260, 353, 254, 475, 398, 259, 334, 317 }, // Minor Weighted
  { 635,   0,   0, 635,   0,   0, 635,   0,   0, 635,   0,   0 }, // Diminished
  { 635,   0, 635,   0, 635,   0, 635,   0, 635,   0, 635,   0 }, // Whole Tone
  { 635,   0, 635, 635,   0, 635,   0, 635,   0, 635, 635,   0 }, // Dorian
  { 635,   0,   0, 635,   0, 635, 635, 635,   0,   0, 635,   0 }, // Blues
  { 635,   0, 635,   0, 635, 635,   0, 635,   0, 635, 635,   0 }, // Mixolydian
  { 635,   0, 635, 635,   0,   0, 635, 635, 635,   0,   0, 635 }, // Hungarian
  { 635,   0,   0, 635,   0, 635,   0, 635,   0,   0, 635,   0 }, // Pentatonic
//{ 635,   0, 635, 635,   0, 635,   0, 635,   0, 635,   0, 635 }, // Melodic Minor
  { 635,   0, 352, 538,   0, 353,   0, 475,   0, 500,   0, 475 }, // Melodic Minor (Weighted)
  { 635,   0, 635,   0, 635, 635, 635,   0, 635,   0, 635,   0 }, // Arabian
  { 635, 635,   0, 635,   0,   0,   0, 635, 635,   0,   0,   0 }, // Balinese
  { 635, 635,   0,   0, 635, 635,   0, 635, 635,   0, 635,   0 }, // Spanish Gypsy
  { 635, 635,   0,   0, 635, 635, 635,   0,   0, 635, 635,   0 }, // Oriental
  { 635,   0, 635,   0, 635,   0, 635,   0,   0, 635, 635,   0 }, // Prometheus
  { 635, 635,   0,   0,   0, 635,   0, 635,   0,   0, 635,   0 }, // Japanese
  { 635,   0, 635,   0,   0, 635,   0, 635,   0,   0, 635,   0 }, // Egyptian
  { 635, 635,   0,   0, 635, 635,   0, 635,   0,   0, 635,   0 }, // Iberian
  { 635,   0, 635, 635,   0,   0, 635, 635,   0, 635, 635,   0 }  // Romanian
};



/*******************************************
* MAIN ISR PROCESSING FUNCTION             *
*******************************************/

ISR(TCA0_OVF_vect) {

  // --- TRIGGER DETECTION --- //
  if( (dsp_mode == MODE_CV) && trigger_mode ){                                 // If we are in dsp_mode and trigger_mode is set
    if( analogRead( PIN_CV_SR ) > 50 ){                                        // Then check if the sample rate CV pin is not zero
      if( trigger_gate == false ){                                             // And if trigger_gate was currently false
        trigger_gate = true;                                                   // then turn trigger_gate on (so we don't retrigger the gate again)
        pause_ISR = false;                                                     // and un-pause the ISR
      }
    } else {                                                                   // Otherwise if analog read is zero, then set trigger_gate to false
      trigger_gate = false;                                                    // then set trigger_gate to false
    }
    if( pause_ISR ){                                                           // If we need to pause the ISR then
      TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                                // reset the interrupt vector
      return;                                                                  // and return out of the ISR function
    }
    clock_divider = 1;                                                         // Otherwise, set the clock_divider to 1 so the CV function doesn't sub-divide 
    pause_ISR = true;                                                          // Re-pause the ISR for the next run through the loop
  }

  // --- ISR SPEED LIMIT --- //
  if( skip_ISR ){                                                              // If the ISR tries to run again while the current ISR is running, well, that's bad.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                                  // If skip_ISR is true, then reset the ISR vector
    return;                                                                    // and return out of the ISR function
  }
  skip_ISR = true;                                                             // Turn skip_ISR on until we get through the enormous amonunt of stuff we need to do...

  uint32_t frame_start = micros();                                             // Capture the current value of micros so we can see how long it takes to render the sample


  switch( dsp_mode ){                                                          // What we want to do depends entirely on the current mode
    case MODE_IDLE:                                                            // If we are in Idle mode then
      // ----------------------- //
      //        IDLE MODE
      // ----------------------- //

      DAC0.DATA = 0x8000;                                                      // just set the output value to the middle of the output range
      break;                                                                   // and then bump out of the case statement

    case MODE_AUDIO: // Audio Mode
      if( --ISR_counter <= 0 ){                                                // See if we have gotten to zero on the counter
        ISR_counter = ISR_period;
        if( loop_length == 0 ){


          // ----------------------- //
          //    NORMAL AUDIO MODE
          // ----------------------- //
          uint16_t val = analogRead( PIN_IN_AUD ); // Capture the initial value


          // NORMAL BIT CRUSH NOTES:
          input_buffer[input_index] = bitcrush_conversion[val];

          // LOW FREQUENCY FILTER NOTES:
          // • The filter first takes a weighted average of the input and the inverted output (x2) from the last sample with the input to create feedback loop
          // • The next four filter stages act like independent filter poles by taking a weighted average between the prior value and the new value.
          // • The value of alpha is derived from the "cutoff / ( sampleRate/(2*PI) + cutoff )" formula. Since I don't really care about the exact cutoff frequency
          //   I just use: 255 * cutoff / (128 + cutoff ) which seems to give a decent--if imprecise--filter range. 

          uint16_t output;                                                         // Create a temporary variable to keep track of the output
/*
          val = input_buffer[input_index] + uint16_t(0x5FF) - (rolling_avg5 << 1); // Add together the input with the inverted feedback
          val = constrain( val, uint16_t(0x200), uint16_t(0x5FF)) - 0x200;         // Constrain the added signals and re-center
          val = constrain( val << 2, uint16_t(0x600), uint16_t(0x9FF) ) - 0x600;   // Scale up the resonance signal by a factor of 4x and re-center
          rolling_avg  = TWEEN256( input_buffer[input_index], val, resonance );    // Take a weighted average between the current input and the full feedback level
*/


          // FEEDBACK / RESONANCE STAGE
          val = (input_buffer[input_index] + uint16_t(0x7FF) - (rolling_avg5 << 1)) << 2; // Add together the input with the inverted feedback
          val = constrain( val, uint16_t(0x1600), uint16_t(0x19FF) ) - 0x1600;   // Scale up the resonance signal by a factor of 4x and re-center
          rolling_avg  = TWEEN256( input_buffer[input_index], val, resonance );    // Take a weighted average between the current input and the full feedback level

          // LOW PASS FILTER STAGES
          rolling_avg2 = TWEEN256( rolling_avg2, rolling_avg,  alpha );        // FILTER STAGE 1
          rolling_avg3 = TWEEN256( rolling_avg3, rolling_avg2, alpha );        // FILTER STAGE 2
          rolling_avg4 = TWEEN256( rolling_avg4, rolling_avg3, alpha );        // Filter Stage 3
          rolling_avg5 = TWEEN256( rolling_avg5, rolling_avg4, alpha );        // Filter Stage 4
          output = rolling_avg5;                                               // Store this into the output variable for the reverb stage

          
          // REVERB NOTES:
          // • reverb_buffer contains a set of 2048 output values that make up the tape loop
          // • reverb_write_index tracks the location of the most recent value written to the buffer -- e.g. "current time"
          //   this is effectively like the write-head on a tape loop
          // • reverb_read_index tracks the location of the of the next value to read out of the buffer this works like
          //   like the read head on the tape loop
          // • First, we write a weighted average of the current sample and the sample stored at the reverb_read_index
          //   we use the value of reverb_feedback to determine how much of the reverb sample to include vs. the rolling_avg sample
          // • Next, we calulate an output value using a weighted average of the current sample (rolling_avg) and the sample stored 
          //   at the reverb_read_index. The weighting of these two values is determined by reverb_wet_mix
          // • Finally, this value is written into the output_buffer for the visualization and loop mode, and then written to DAC

          val = constrain( reverb_buffer[reverb_read_index] << 1, uint16_t(0x200), uint16_t(0x5FF) ) - uint16_t(0x200);
          reverb_buffer[reverb_write_index] = TWEEN256(output, val, reverb_feedback);
          val = constrain( reverb_buffer[reverb_read_index] + output, uint16_t(0x200), uint16_t(0x5FF)) - uint16_t(0x200);
          output = TWEEN256(output, val, reverb_wet_mix );

          morph_buffer[output_index] = output;                                 // Store output into the morph_buffer for future use if the user flips into morph mode
          output_buffer[output_index] = output;                                // Store output into the output buffer for the oscilloscope visualization

          DAC0.DATA = output << 6;                                             // Send the output value to the DAC
          
          input_index = (input_index + 1) & 0xFF;                              // Increment the input pointer
          output_index = (output_index + 1) & 0xFF;                            // Increment the output pointer
          if( ++reverb_read_index  >= REVERB_BUFFER_SIZE ) reverb_read_index  = 0; // Increment the read pointer of the reverb loop
          if( ++reverb_write_index >= REVERB_BUFFER_SIZE ) reverb_write_index = 0; // Increment the write pointer of the reverb loop



        } else {


          // ----------------------- //
          //   AUDIO LOOPING MODE
          // ----------------------- //

          // SOUND MORPHING NOTES:
          // • morph_rate is between 0 and 15 (4 bit number)
          // • morph_counter is determined by left shifting a 1 by morph_rate and then counting down from there to zero
          // • In order to convert morph_counter to a number consistently between 0 and 255 (to pull the correct TWEEN value)
          //   we neeed to shift the count to the appropriate bit depth for the morph_rate. If the rate is >= 8 then the morph_counter will be at least an 
          //   8-bit values and we right shift by (morph_rate - 8) bits so it is exactly an 8-bit value. Otherwise we left shift by (8 - morph_rate) bits.
          // • Once we have an 8-bit value, then we can pull the corresponding value from the TWEEN_FN array and use that to choose how much to weight the
          //   input_buffer vs. the output_buffer. 

          uint16_t output;
          if( morph_rate >= 8 ){
            output = TWEEN256( input_buffer[loop_pointer], morph_buffer[loop_pointer], TWEEN_FN[(morph_counter >> (morph_rate - 8))] );
          } else {
            output = TWEEN256( input_buffer[loop_pointer], morph_buffer[loop_pointer], TWEEN_FN[(morph_counter << (8 - morph_rate))] );
          }


          // BIT CRSUH
          output = bitcrush_conversion[output]; // bitcush the output


          // LOW FREQUENCY FILTER (See Notes Above):
          uint16_t val = (output + uint16_t(0x7FF) - (rolling_avg5 << 1)) << 2; // Add together the input with the inverted feedback
          val = constrain( val, uint16_t(0x1600), uint16_t(0x19FF) ) - 0x1600;   // Scale up the resonance signal by a factor of 4x and re-center
          rolling_avg  = TWEEN256( output, val, resonance );    // Take a weighted average between the current input and the full feedback level

          rolling_avg2 = TWEEN256( rolling_avg2, rolling_avg,  alpha );        // FILTER STAGE 2
          rolling_avg3 = TWEEN256( rolling_avg3, rolling_avg2, alpha );        // FILTER STAGE 2
          rolling_avg4 = TWEEN256( rolling_avg4, rolling_avg3, alpha );        // Filter Stage 3
          rolling_avg5 = TWEEN256( rolling_avg5, rolling_avg4, alpha );        // Filter Stage 4
          output = rolling_avg5;                                               // Store this into the output variable for the reverb stage

          // REVERB (See Notes Above):
          val = constrain( reverb_buffer[reverb_read_index] << 1, uint16_t(0x200), uint16_t(0x5FF) ) - uint16_t(0x200);
          reverb_buffer[reverb_write_index] = TWEEN256(output, val, reverb_feedback);
          val = constrain( reverb_buffer[reverb_read_index] + output, uint16_t(0x200), uint16_t(0x5FF)) - uint16_t(0x200);
          output = TWEEN256(output, val, reverb_wet_mix );

          output_buffer[loop_pointer] = output;
          DAC0.DATA = output << 6;                                             // Send the output value to the DAC


          // MORPH COUNTER NOTES:
          // • Once the morph_counter reaches zero, the output_buffer gets overwritten by the input_buffer for one cycle and the input_buffer gets written into
          // • When the morph_coutner is still counting, we still exectue analogRead to maintain the same timing. Would be great if we didn't need this, but you get clicking...
          // • The loop pointer ticks once with every ISR. Once the loop fully cycles, it ticks the morph_counter. 
          // • When the morph_counter reaches zero, it resets based on morph_rate

          if( morph_counter == 0 ){                                            // See if the morph_counter has reached zero yet
            morph_buffer[loop_pointer] = input_buffer[loop_pointer];           // If it did, then start repopulating the morph_buffer with the current input_buffer
            input_buffer[loop_pointer] = analogRead( PIN_IN_AUD );             // And simultaneously, start overwriting the input_buffer with some new values
          } else {                                                             // If not, then just...
            analogRead( PIN_IN_AUD );                                           // Read the current value of the CV input to keep the timing the same
          }

          if( ++loop_pointer >= loop_length ){                                 // Track progress through the loop, and once we hit the end of the loop
            loop_pointer = 0;                                                  // Reset the loop pointer to zero and
            if( morph_counter--==0 ) morph_counter = uint16_t(1)<<morph_rate;  // if morph_counter also hit zero, reset it to count down from 2^morph_rate 
          }

          if( ++reverb_read_index >= REVERB_BUFFER_SIZE ) reverb_read_index = 0;   // Increment the read pointer of the reverb loop
          if( ++reverb_write_index >= REVERB_BUFFER_SIZE ) reverb_write_index = 0; // Increment the write pointer of the reverb loop
        }
      }
      break;

    case MODE_CV: // CV Mode

      // Clock Divider: This cuts down the frequency of the ISR to something more reasonable for CV tracking
      // CV_CLOCK_DIVIDER determines the number of ISR ticks to wait before processing the CV

      if( --clock_divider == 0 ){                                              // Check the clock divider to see if we should skip this ISR cycle
        clock_divider = CV_CLOCK_DIVIDER;                                      // Reset the count-down timer

        if( loop_length == 0 ){


          // ----------------------- //
          //   CV MODE
          // ----------------------- //

          // ------ INPUT ------ //
          // Capture the current analog value from the CV input pin (not the audio input pin). Remember
          // that the CV input pin does not have a DC-blocking capacitor, while the audio input does.
          uint16_t val = analogRead( PIN_IN_CV );                              // Capture the initial value
          input_buffer[input_index] = val;                                   // Capture value in the input array

          // ------ TRANSFORMATION: Glide ------ //
          rolling_avg = (uint32_t(rolling_avg) * glide + (uint32_t(val) * 16)) / (glide + 16); // Calculate the glide average for filtering
          val = rolling_avg;                                                   // Set val to the filtered value

          // ------ TRANSFORMATION: Scale Crush ------ //
          uint8_t note = ((uint32_t(val) * 120) >> 10 );                       // Quantize the note to a chromatic scale, assuming 1v/oct
          note_scale = note % 12;                                              // Identify the note within the 12 note chromatic scale
          note_oct   = note / 12;                                              // Figure out the octave of the note

          
          for( uint8_t i = 0; i<12; i++ ){                                     // Generate a probability map for each of the 12 notes in the scale
            prob_map[i] = ( SCALE_PROB[scale_index][i] + random(SCALE_PROB_RANGE) - (SCALE_PROB_RANGE>>1) ) > (1023 - scale_crush) ? 1 : 0; 
          }                                                                    // Probability map will contain a 1 or 0 for each key in scale if it is valid or not
          while( (note_scale>0) && (prob_map[note_scale] == 0) ) note_scale--; // Take the current note and constrain it to the probability mapped scale

          // ------ TRANSFORMATION: Transposition ------ //
          note = note_scale + note_oct * 12 + note_offset;                        // Calculate the new note and add the transposition
          if( note > 120 ) note = 120;                                         // Constrain the note to be less than 120 notes (10v output 12 notes per octave)

          uint16_t output = (uint32_t(note) << 10) / 120;                      // Convert from a note number to an output voltage

          // ------ OUTPUT ------ //
          output_buffer[output_index] = output;                              // Store the output value into the output buffer so it can be shown on the screen
          morph_buffer[output_index] = output;                               // Store the output value into the morph buffer
          DAC0.DATA = output << 6;                                             // Set the DAC output

          // Increment the input and output pointers so they can be tracked in their respective buffers
          input_index  = (input_index  + 1) & 0xFF;                        // Increment the input_index (rotate around 255)
          output_index = (output_index + 1) & 0xFF;                        // Increment the output_index (rotate around 255)

        } else {


          // ----------------------- //
          //   CV LOOPING MODE
          // ----------------------- //

          uint16_t val;                                                        // will contain the output value

          // ------ INPUT ------ //
          // Capture the current analog value from the CV input pin (not the audio input pin). Remember
          // that the CV input pin does not have a DC-blocking capacitor, while the audio input does.
          if( morph_rate >= 8 ){
            val = TWEEN256( input_buffer[loop_pointer], morph_buffer[loop_pointer], TWEEN_FN[(morph_counter >> (morph_rate - 8))] );
          } else {
            val = TWEEN256( input_buffer[loop_pointer], morph_buffer[loop_pointer], TWEEN_FN[(morph_counter << (8 - morph_rate))] );
          }


          // ------ TRANSFORMATION: Glide ------ //
          rolling_avg = (uint32_t(rolling_avg) * glide + (uint32_t(val) * 16)) / (glide + 16); // Calculate the glide average for filtering
          val = rolling_avg;                                                   // Set val to the filtered value


          // ------ TRANSFORMATION: Scale Crush ------ //
          uint8_t note = ((uint32_t(val) * 120) >> 10 );                       // Quantize the note to a chromatic scale, assuming 1v/oct
          note_scale = note % 12;                                              // Identify the note within the 12 note chromatic scale
          note_oct   = note / 12;                                              // Figure out the octave of the note


          for( uint8_t i = 0; i<12; i++ ){                                     // Generate a probability map for each of the 12 notes in the scale
            prob_map[i] = ( SCALE_PROB[scale_index][i] + random(SCALE_PROB_RANGE) - (SCALE_PROB_RANGE>>1) ) > (1023 - scale_crush) ? 1 : 0; 
          }                                                                    // Probability map will contain a 1 or 0 for each key in scale if it is valid or not
          while( (note_scale>0) && (prob_map[note_scale] == 0) ) note_scale--; // Take the current note and constrain it to the probability mapped scale


          // ------ TRANSFORMATION: Transposition ------ //
          note = note_scale + note_oct * 12 + note_offset;                     // Calculate the new note and add the transposition
          if( note > 120 ) note = 120;                                         // Constrain the note to be less than 120 notes (10v output 12 notes per octave)
          uint16_t output = (uint32_t(note) << 10) / 120;                      // Convert from a note number to an output voltage

 
          // ------ OUTPUT ------ //
          output_buffer[output_index] = output;                                // Store the output value into the output buffer so it can be shown on the screen
          DAC0.DATA = output << 6;                                             // Set the DAC output


          // MORPH COUNTER NOTES:
          // • Once the morph_counter reaches zero, the morph_buffer gets overwritten by the input_buffer for one cycle and the input_buffer gets written into
          // • When the morph_coutner is still counting, we still exectue analogRead to maintain the same timing. Would be great if we didn't need this, but you get clicking...
          // • The loop pointer ticks once with every ISR. Once the loop fully cycles, it ticks the morph_counter. 
          // • When the morph_counter reaches zero, it resets based on morph_rate

          if( morph_counter == 0 ){                                            // See if the morph_counter has reached zero yet
            morph_buffer[loop_pointer] = input_buffer[loop_pointer];           // If it did, then start repopulating the morph_buffer with the current input_buffer
            input_buffer[loop_pointer] = analogRead( PIN_IN_CV );              // And simultaneously, start overwriting the input_buffer with some new values
          } else {                                                             // If not, then just...
            analogRead( PIN_IN_CV );                                           // Read the current value of the CV input to keep the timing the same
          }

          if( ++loop_pointer >= loop_length ){                                 // Track progress through the loop, and once we hit the end of the loop
            loop_pointer = 0;                                                  // Reset the loop pointer to zero and
            if( morph_counter--==0 ) morph_counter = uint16_t(1)<<morph_rate;  // if morph_counter also hit zero, reset it to count down from 2^morph_rate 
          }
        }        
      }
      break;


    case MODE_CAL:


      // ----------------------- //
      //   CALLIBRATION MODE
      // ----------------------- //

      if( --clock_divider == 0 ){                                              // Subdivide the ISR by counting down the clock_divider
        clock_divider = CV_CLOCK_DIVIDER;                                      // Once the clock_divider hits zero, reset it back to CV_CLOCK_DIVIDER
        uint16_t val = analogRead( PIN_IN_CV );                                // Capture the initial value
        input_buffer[input_index] = val;                                       // Just et the input_buffer to the current analog input value
        output_buffer[output_index] = val;                                     // Just et the output_buffer to the current analog input value
        rolling_avg = (rolling_avg + val) >> 1;                                // Calculate the rolling_avg value of the input for the visualization

        DAC0.DATA = 0xFFC0;                                                    // Set the DAC output to its highest value
        input_index  = (input_index  + 1) & 0xFF;                              // Increment the input_index around the buffer (anding wiht 0xFF will flip it around at 256)
        output_index = (output_index + 1) & 0xFF;                              // Increment the output_index around the buffer (anding wiht 0xFF will flip it around at 256)
      }
      break;
  }

  frame_period = micros() - frame_start;                                       // Calculate the frame_period in microseconds
  
  skip_ISR = false;                                                            // We are now done with the ISR, so we can turn off skip_ISR
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;                                    // Don't forget to reset the interrupt flag!!
}


/*******************************************
* DSP CLASS                                *
*******************************************/

class DSP {
  private:
    uint8_t  *display_buffer        = NULL; // Contains a pointer to the display buffer
    Hardware* hw;

  public:
    DSP( Hardware* _hw ){ hw = _hw; };                                         // Constructor
    void setup();                                                              // Setup the hardware for the DAC
    void setMode( uint8_t mode );                                              // Set the mode of the DSP MODE_IDLE, MODE_AUD, MODE_CV, MODE_CAL
    uint16_t getFramePeriod(){ return frame_period; }                          // Exposes the frame period externally to the class

    // --- External Hardware Control Functions ---
    void setSampleRateExp(uint16_t sr);                                        // Set the sample rate exponentially
    void setBitCrush(uint16_t bc){                                             // Set the bitcrush and scale crush values
      scale_crush = bc; 
      uint16_t bc_counter = bc>>1;                        
      uint16_t val = 0;
      for( uint16_t i = 0; i<1024; i++ ){
        bitcrush_conversion[i] = min(val + (bc>>2), uint16_t(0x3FF));
        if( bc_counter-- == 0 ){ bc_counter = bc>>1; val = i; }
      }
    }
    void setGlide(uint16_t gl){                                                // Set the value of the "filter" from 0...1024
      glide = 255-(gl >> 2);                                                   // Glide goes from 0...255, but gets inverted
      alpha = min( ((gl >> 2) * uint8_t(255) ) / ( uint8_t(64) + (gl >> 2) ), uint8_t(255)); // Sets the value of the filter
    }

    // --- Audio Menu Setting Functions ---
    void setMorphRate( uint8_t _morph_rate ){                                  // Set the speed of the morph rate (only used in loop mode)
      morph_rate = _morph_rate;                                                // Assign the value. But then ensure that the current morph_counter
      if( morph_counter > (uint16_t(1) << morph_rate) ) morph_counter = uint16_t(1) << morph_rate; // Update the morph counter to be 2^morph_rate 
    }
    void setLoopLength(     uint16_t _loop_length ){    loop_length     = _loop_length; }          // Set value of loop_length     0...255
    void setResonance(      uint8_t _resonance ){       resonance       = _resonance; }            // Set value of resonance       0...255
    void setReverbFeedback( uint8_t _reverb_feedback ){ reverb_feedback = _reverb_feedback >> 1; } // Set value of reverb_feedback 0...255
    void setReverbAmount(   uint8_t _reverb_wet_mix ){  reverb_wet_mix  = _reverb_wet_mix; }       // Set value of reverb_wet_mix  0...255
    void setReverbDelay(    uint8_t _reverb_delay ){                                               // Set value of reverb_delay    0...255
      reverb_delay = uint16_t(_reverb_delay) << 3;                             // Scale reverb delay to 0...2048 to match the buffer size
      reverb_read_index = (reverb_write_index + REVERB_BUFFER_SIZE - reverb_delay) % REVERB_BUFFER_SIZE; // Set the reverb_read_index based on the current dealy
    }

    // CV Menu Setting Functions
    void setRoot(  uint8_t _note_offset ){ note_offset = _note_offset; }       // Set the root note for transposition. Note: Transposition occurs after quantization
    void setScale( uint8_t _scale_index  ){ scale_index  = _scale_index;  }    // Set the current scale ID

    // Visualization Functions
    void drawOscilloscope();                                                   // Draws oscilloscope in the top 32 rows of the screen
    void drawOscilloscopeFS();                                                 // Draws oscilloscope in the full 64 rows of the screen
    void drawCallibration();                                                   // Draws the callibration visualization and text instructions

    // External Buffer Access
    uint16_t *outputBuffer(){ return output_buffer; }                          // Return pointer to the output buffer
    uint16_t *inputBuffer(){  return input_buffer;  }                          // Return pointer to the input buffer
};


/*******************************************
* Setup Function                           *
*******************************************/

void DSP::setup(){                                                             // Core setup function for the DSP class
  // pre-calculate sample_rate_conversion array                                
  for( uint16_t i=0; i<SAMPLE_RATE_CONVERSION_SIZE; i++ ){                     // With only 1024 possible values for sr, we can pre-calculate the period associated
    sample_rate_conversion[i] = (M_CLOCK_FRQ/LOW_SAMP_FRQ) / (pow(2, float(i)*OCT_RANGE/1024)); // with a 1v/oct exponential sample rate input.
    if( sample_rate_conversion[i] < 2000 ) sample_rate_conversion[i] = 2000;   // This caps the sample period at a minimum of 80 uS to ensure ISR doesn't trigger too fast
  }                                                                            // 80 uS represents 12.5 kHz, which is the highest sample rate this module supports

  // Set up the DAC
  PORTD.PIN6CTRL &= ~PORT_ISC_gm;                                              // This sets up the interrupt service routine, but don't ask me how
  PORTD.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;                                 // It's assigning magical constants to unknowns parts locked away
  PORTD.PIN6CTRL &= ~PORT_PULLUPEN_bm;                                         // in the deep recesses of the micro's memory. But it works!
  DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm | DAC_RUNSTDBY_bm;                 // This just turns on the DAC. 

  pinMode(PIN_OFFSET,    OUTPUT);                                              // Setup the output offset pin 

  // Set Up the Timer Interrupt
  takeOverTCA0();                                                              // Override the timer
  TCA0.SINGLE.PER = 10000;                                                     // This is the sample period (20 MHz / this value)  0x0271
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;                                     // Enable overflow interrupt
  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm;                                    // Enable the timer with no prescaler  

  setSampleRateExp(1000);                                                      // Set an initial sample rate value (will be overwritten by the actual knob's value)
  setBitCrush(1000);                                                           // Set an initial bit crush value (will be overwritten by the actual knob's value)
  setGlide(1000);                                                              // Set an initial glide value (will be overwritten by the actual knob's value)

  setMode( MODE_IDLE );                                                        // Set the mode to idle so that the ISR just outputs the mid-point value

  display_buffer = hw->displayBuffer();                                        // Link up the local display_buffer value to the hw->displayBuffer();
}


/*******************************************
* Helper Functions                         *
*******************************************/

void DSP::setMode( uint8_t mode ){
  switch( mode ){
    case MODE_IDLE:  digitalWrite( PIN_OFFSET, false ); break;                 // Audio Mode - output -5v to +5v
    case MODE_AUDIO: digitalWrite( PIN_OFFSET, false ); break;                 // Audio Mode - output -5v to +5v
    case MODE_CV:    digitalWrite( PIN_OFFSET, true  ); break;                 // CV Mode - output 0v to +10v
    case MODE_CAL:   digitalWrite( PIN_OFFSET, false ); break;                 // CV Mode - output 0v to +10v
  }
  dsp_mode = mode;
}

// Hardware Handler Functions
void DSP::setSampleRateExp(uint16_t sr){                                       // Set the sample rate
  if( dsp_mode == MODE_CV ){                                                   // If we are in CV mode, then we need to check to see if we should switch to
    trigger_mode = (hw->sampleRatePot < 16 );                                  // trigger mode (as indicated by used turning SR knob all the way down)
  }
  if( trigger_mode == true ){                                                  // If trigger_mode mode is true then we still set a sampel rate (of 5)
    sample_rate = 5;                                                           // because this is used to determine the zoom level in the visualization
    TCA0.SINGLE.PER = sample_rate_conversion[800];                             // We also ensure the ISR rate is fast enough to catch even short triggers
  } else {                                                                     // If we are not in trigger mode, then we can just set the sample_rate to
    sample_rate = sr;                                                          // the value of sr and then...
    uint8_t  srOct = sr / UNITS_PER_OCT;                                       // Calculate current octave of SR by dividing by units_per_oct
    int8_t   octShift = OCT_RANGE - ISR_OCT_RANGE - srOct;                     // Figure out how many octaves this needs to shift to get into the correct range

    if( octShift > 0 ){                                                        // If that number ends up being more than zero, we need to shift it.
      ISR_period = 0b1 << octShift;                                            // Set the period of the ISR counter to be 2^octShift 
      TCA0.SINGLE.PER = sample_rate_conversion[sr + octShift * UNITS_PER_OCT]; // Calc the period based on SR, but shift the pitch up by octShift octaves
    } else {                                                                   // If we octShift is 0 or less, we don't have to change the octave at all
      ISR_period = 0b1;                                                        // Set the ISR_period to 1 so the ISR runs every time
      TCA0.SINGLE.PER = sample_rate_conversion[sr];                            // Set the timer count based on the original value of sr
    }
  }
}





/*******************************************
* Visualization Functions                  *
*******************************************/

// Draw a half-screen version of the oscilloscope (the menu appears at the bottom, though it is drawn be a separate function)
void DSP::drawOscilloscope(){
  uint32_t bit_img_col  = 0;                                                   // Contains a 32-pixel column (1 bit per pixel) of the image
  uint32_t new_bit_val  = 0;                                                   // Puts 1's in all of the bits between the last value and the current value
  uint32_t last_bit_val = 0;                                                   // Stores prior value of new_bit_val for comparison
  uint8_t  highlight = 0x00;                                                   // Stores a highlight mask to use for showing a the selected columns
  uint8_t  pixels_per_pos = 0;                                                 // The number of pixels to consume per element in buffer (divided by 2)
  uint8_t  buffer_pos = 0;                                                     // Current position in the buffer
  uint8_t  buffer_val = 0;                                                     // Tracks the value of the buffer at buffer_pos
  uint8_t  lp = loop_pointer;                                                  // Capture the current position in the loop cuz... its gonna change in the ISR
  
  if( loop_length > 0 ){                                                       // See if we are in loop mode
    if(      loop_length & 0b10000000 ){ pixels_per_pos =  1; }                // based on the highest bit in loop_length, determine the zoom depth for
    else if( loop_length & 0b01000000 ){ pixels_per_pos =  2; }                // rendering the horizontal scale. pixels_per_pos determines how many
    else if( loop_length & 0b00100000 ){ pixels_per_pos =  4; }                // horizontal pixels to render for each element in the loop. In this way
    else if( loop_length & 0b00010000 ){ pixels_per_pos =  8; }                // a short loop will fill the screen and as you increase the loop length
    else if( loop_length & 0b00001000 ){ pixels_per_pos = 16; }                // the visualization will zoom out to accommodate more values in the loop.
    else if( loop_length & 0b00000100 ){ pixels_per_pos = 32; }
    else if( loop_length & 0b00000010 ){ pixels_per_pos = 32; }
    else if( loop_length & 0b00000001 ){ pixels_per_pos = 32; }
  } else {                                                                     // If we are in normal mode, then we use the octave range to determine the zoom
    pixels_per_pos = OCT_RANGE - (sample_rate / UNITS_PER_OCT);                // As sample rate goes up, pixels per position goes down
    pixels_per_pos = pixels_per_pos + (pixels_per_pos >> 1) + 1;               // Adds ~50% 
    buffer_pos = 255 + output_index - (255 / pixels_per_pos) - 1;            // Set the buffer position to the end of the cicular buffer
  }
  buffer_val   = ( output_buffer[buffer_pos] ) >> 5;                           // Grab the current 10-bit buffer val and right shift 5 bits so it goes 0...31
  last_bit_val = ( 0x80000000 >> buffer_val  ) - 1;                            // Set up last_bit_val by putting a 1 in the correct column and then subtract 1
                                                                               // this flips all of the bits under that value from 0 into a 1

  uint8_t screen_col = 0;                                                      // Track the current column on the screen that we are rendering
  for( uint8_t i = 0; i<255; i++ ){                                            // The outer loop goes through each element in the buffer
    if( (i % pixels_per_pos) == 0 ) buffer_pos++;                              // Buffer position updates on every cycle
    if( i & 0b1 ){                                                             // Screen position updates every other cycle
      screen_col = i >> 1;                                                     // The screen position is going to be the index divided by two
      buffer_val = (output_buffer[buffer_pos] )>>5;                            // Grab the current value of the buffer

      if( pixels_per_pos == 1 ){
        highlight = (buffer_pos < loop_length ) && (uint8_t(buffer_pos>>1) != (lp>>1)) ? 0xFF : 0x00; // Highlight value if the buffer position is within the loop length
      } else {
        highlight = (buffer_pos < loop_length ) && (uint8_t(buffer_pos) != lp) ? 0xFF : 0x00; // Highlight value if the buffer position is within the loop length
      }
      
      new_bit_val = (0x80000000 >> buffer_val) - 1;                            // Fill in 1's between the old value and the new one (to vertically connect points)
      bit_img_col = (new_bit_val ^ last_bit_val) | (0x80000000 >> buffer_val); // Put a 1 in the correct row
      last_bit_val = new_bit_val;                                              // Store the value for the next cycle

      display_buffer[screen_col ] = (bit_img_col & 0xFF) ^ highlight;          // Update first 8 bits of the column
      bit_img_col = bit_img_col >> 8;
      display_buffer[screen_col + 128] = (bit_img_col & 0xFF) ^ highlight;     // Update second 8 bits of the column
      bit_img_col = bit_img_col >> 8;
      display_buffer[screen_col + 256] = (bit_img_col & 0xFF) ^ highlight;     // Update third 8 bits of the column
      bit_img_col = bit_img_col >> 8;
      display_buffer[screen_col + 384] = (bit_img_col & 0xFF) ^ highlight;     // Update fourth 8 bits of the column
    }
  }

  //                  CHAR    WHITE KEY STATE       BLACK KEY STATE            Updates the keyboard visualization character array
  hw->keyboard[0x0] = 0xF0 + (prob_map[0x0] << 1) + prob_map[0x1]; // C, C#    The keyboard visualization "string" is used to represent a keyboard using a series
  hw->keyboard[0x1] = 0xF4 + (prob_map[0x0] << 1) + prob_map[0x1]; // C, C#    of 5x7 pixel characters. Each character represents half of a white-key. There are
  hw->keyboard[0x2] = 0xF8 + (prob_map[0x2] << 1) + prob_map[0x1]; // D, C#    4 different combinations, the left half of an all-white key (like C), the right half
  hw->keyboard[0x3] = 0xF4 + (prob_map[0x2] << 1) + prob_map[0x3]; // D, D#    of a key that includes the left-half of a black key (like between C and C#), the left
  hw->keyboard[0x4] = 0xF8 + (prob_map[0x4] << 1) + prob_map[0x3]; // E, D#    half of a key that also includes a black key (like between C# and D) and the right half
  hw->keyboard[0x5] = 0xFC + (prob_map[0x4] << 1) + prob_map[0x3]; // E, D#    of an all-white key (like E). In addition, there are four versions of these keys as well.
  hw->keyboard[0x6] = 0xF0 + (prob_map[0x5] << 1) + prob_map[0x6]; // F, F#    (0) both the white and black key of the character are off
  hw->keyboard[0x7] = 0xF4 + (prob_map[0x5] << 1) + prob_map[0x6]; // F, F#    (1) the white key is off, but the black key is on
  hw->keyboard[0x8] = 0xF8 + (prob_map[0x7] << 1) + prob_map[0x6]; // G, F#    (2) the white key is on, but the black key is off
  hw->keyboard[0x9] = 0xF4 + (prob_map[0x7] << 1) + prob_map[0x8]; // G, G#    (3) both the white and black keys are on.
  hw->keyboard[0xA] = 0xF8 + (prob_map[0x9] << 1) + prob_map[0x8]; // A, G#    In this way, the approprite character can be found by taking the base character value and
  hw->keyboard[0xB] = 0xF4 + (prob_map[0x9] << 1) + prob_map[0xA]; // A, A#    adding to it the binary value of the state of each key.
  hw->keyboard[0xC] = 0xF8 + (prob_map[0xB] << 1) + prob_map[0xA]; // B, A#    prob_map gets updated over and over in the ISR, but the keyboard map only needs to be drawn
  hw->keyboard[0xD] = 0xFC + (prob_map[0xB] << 1) + prob_map[0xA]; // B, A#    once per visualization cycle... that's why it's updated here instead.
  // In half-screen mode, the keyboard strong will be drawn by the menu system if it is the currently selected menu option
}

// Draw a full screen version of the oscilloscope
void DSP::drawOscilloscopeFS(){
  uint32_t bit_img_col_H  = 0;                                                 // Contains a 32-pixel column (1 bit per pixel) of the image
  uint32_t bit_img_col_L  = 0;                                                 // Contains a 32-pixel column (1 bit per pixel) of the image
  uint32_t new_bit_val_H  = 0;                                                 // Puts 1's in all of the bits between the last value and the current value
  uint32_t new_bit_val_L  = 0;                                                 // Puts 1's in all of the bits between the last value and the current value
  uint32_t last_bit_val_H = 0;                                                 // Stores prior value of new_bit_val for comparison
  uint32_t last_bit_val_L = 0;                                                 // Stores prior value of new_bit_val for comparison

  uint8_t  lp = loop_pointer;                                                  // Capture the current position in the loop cuz... its gonna change in the ISR

  uint8_t  highlight = 0x00;                                                   // Stores a highlight mask to use for showing a the selected columns
  uint8_t  pixels_per_pos = 0;                                                 // The number of pixels to consume per element in buffer (divided by 2)
  uint8_t  buffer_pos = 0;                                                     // Current position in the buffer
  uint8_t  buffer_val = 0;                                                     // Tracks the value of the buffer at buffer_pos
  
  if( loop_length > 0 ){                                                       // See if we are in loop mode
    if(      loop_length & 0b10000000 ){ pixels_per_pos =  1; }                // based on the highest bit in loop_length, determine the zoom depth for
    else if( loop_length & 0b01000000 ){ pixels_per_pos =  2; }                // rendering the horizontal scale. pixels_per_pos determines how many
    else if( loop_length & 0b00100000 ){ pixels_per_pos =  4; }                // horizontal pixels to render for each element in the loop. In this way
    else if( loop_length & 0b00010000 ){ pixels_per_pos =  8; }                // a short loop will fill the screen and as you increase the loop length
    else if( loop_length & 0b00001000 ){ pixels_per_pos = 16; }                // the visualization will zoom out to accommodate more values in the loop.
    else if( loop_length & 0b00000100 ){ pixels_per_pos = 32; }
    else if( loop_length & 0b00000010 ){ pixels_per_pos = 32; }
    else if( loop_length & 0b00000001 ){ pixels_per_pos = 32; }
  } else {
    pixels_per_pos = OCT_RANGE - (sample_rate / UNITS_PER_OCT);                // As sample rate goes up, pixels per position goes down
    pixels_per_pos = pixels_per_pos + (pixels_per_pos >> 1) + 1;               // Adds ~50% 
    buffer_pos = 255 + output_index - (255/pixels_per_pos) - 1;              // Set the buffer position to the end of the cicular buffer
  }
  buffer_val   = ( output_buffer[buffer_pos] ) >> 4;                           // Grab the current 10-bit buffer val and right shift 5 bits so it goes 0...31

  last_bit_val_H = (buffer_val > 31) ? (0x80000000 >> (buffer_val & 0x1F))-1 : 0-1;   // Set up last_bit_val by putting a 1 in the correct column and then subtract 1
  last_bit_val_L = (buffer_val > 31) ? 0x00000000 : (0x80000000 >> buffer_val) - 1;   // this flips all of the bits under that value from 0 into a 1

  uint8_t screen_col = 0;                                                      // Track the current column on the screen that we are rendering
  for( uint8_t i = 0; i<255; i++ ){                                            // The outer loop goes through each element in the buffer
    if( (i % pixels_per_pos) == 0 ) buffer_pos++;                              // Buffer position updates on every cycle
    if( i & 0b1 ){                                                             // Screen position updates every other cycle
      screen_col = i >> 1;                                                     // The screen position is going to be the index divided by two
      buffer_val = (output_buffer[buffer_pos] )>>4;                            // Grab the current value of the buffer

      if( pixels_per_pos == 1 ){
        highlight = (buffer_pos < loop_length ) && (uint8_t(buffer_pos>>1) != (lp>>1)) ? 0xFF : 0x00; // Highlight value if the buffer position is within the loop length
      } else {
        highlight = (buffer_pos < loop_length ) && (uint8_t(buffer_pos) != lp) ? 0xFF : 0x00; // Highlight value if the buffer position is within the loop length
      }
      
      new_bit_val_H = (buffer_val > 31) ? (0x80000000 >> (buffer_val & 0x1F)) - 1 : 0-1;       // Calculate the pixels to draw in the top half of the screen
      new_bit_val_L = (buffer_val > 31) ? 0x00000000 : (0x80000000 >> buffer_val) - 1;         // Calculate the pixels to draw in the bottom half of the screen

      bit_img_col_H = (buffer_val > 31) ? (new_bit_val_H ^ last_bit_val_H) | (0x80000000 >> (buffer_val & 0x1F)) : (new_bit_val_H ^ last_bit_val_H); // Put a 1 in the correct row
      bit_img_col_L = (buffer_val > 31) ? (new_bit_val_L ^ last_bit_val_L) : (new_bit_val_L ^ last_bit_val_L) | (0x80000000 >> buffer_val);          // Put a 1 in the correct row

      last_bit_val_H = new_bit_val_H;
      display_buffer[screen_col ] = (bit_img_col_H & 0xFF) ^ highlight;          // Update first 8 bits of the column
      bit_img_col_H = bit_img_col_H >> 8;
      display_buffer[screen_col + 128] = (bit_img_col_H & 0xFF) ^ highlight;     // Update second 8 bits of the column
      bit_img_col_H = bit_img_col_H >> 8;
      display_buffer[screen_col + 256] = (bit_img_col_H & 0xFF) ^ highlight;     // Update third 8 bits of the column
      bit_img_col_H = bit_img_col_H >> 8;
      display_buffer[screen_col + 384] = (bit_img_col_H & 0xFF) ^ highlight;     // Update fourth 8 bits of the column

      last_bit_val_L = new_bit_val_L;
      display_buffer[screen_col + 512] = (bit_img_col_L & 0xFF) ^ highlight;     // Update fifth 8 bits of the column
      bit_img_col_L = bit_img_col_L >> 8;
      display_buffer[screen_col + 640] = (bit_img_col_L & 0xFF) ^ highlight;     // Update sixth 8 bits of the column
      bit_img_col_L = bit_img_col_L >> 8;
      display_buffer[screen_col + 768] = (bit_img_col_L & 0xFF) ^ highlight;     // Update seventh 8 bits of the column
      bit_img_col_L = bit_img_col_L >> 8;
      display_buffer[screen_col + 896] = (bit_img_col_L & 0xFF) ^ highlight;     // Update eighth 8 bits of the column

    }
  }

  //                  CHAR    WHITE KEY STATE       BLACK KEY STATE            Updates the keyboard visualization character array
  hw->keyboard[0x0] = 0xF0 + (prob_map[0x0] << 1) + prob_map[0x1]; // C, C#    The keyboard visualization "string" is used to represent a keyboard using a series
  hw->keyboard[0x1] = 0xF4 + (prob_map[0x0] << 1) + prob_map[0x1]; // C, C#    of 5x7 pixel characters. Each character represents half of a white-key. There are
  hw->keyboard[0x2] = 0xF8 + (prob_map[0x2] << 1) + prob_map[0x1]; // D, C#    4 different combinations, the left half of an all-white key (like C), the right half
  hw->keyboard[0x3] = 0xF4 + (prob_map[0x2] << 1) + prob_map[0x3]; // D, D#    of a key that includes the left-half of a black key (like between C and C#), the left
  hw->keyboard[0x4] = 0xF8 + (prob_map[0x4] << 1) + prob_map[0x3]; // E, D#    half of a key that also includes a black key (like between C# and D) and the right half
  hw->keyboard[0x5] = 0xFC + (prob_map[0x4] << 1) + prob_map[0x3]; // E, D#    of an all-white key (like E). In addition, there are four versions of these keys as well.
  hw->keyboard[0x6] = 0xF0 + (prob_map[0x5] << 1) + prob_map[0x6]; // F, F#    (0) both the white and black key of the character are off
  hw->keyboard[0x7] = 0xF4 + (prob_map[0x5] << 1) + prob_map[0x6]; // F, F#    (1) the white key is off, but the black key is on
  hw->keyboard[0x8] = 0xF8 + (prob_map[0x7] << 1) + prob_map[0x6]; // G, F#    (2) the white key is on, but the black key is off
  hw->keyboard[0x9] = 0xF4 + (prob_map[0x7] << 1) + prob_map[0x8]; // G, G#    (3) both the white and black keys are on.
  hw->keyboard[0xA] = 0xF8 + (prob_map[0x9] << 1) + prob_map[0x8]; // A, G#    In this way, the approprite character can be found by taking the base character value and
  hw->keyboard[0xB] = 0xF4 + (prob_map[0x9] << 1) + prob_map[0xA]; // A, A#    adding to it the binary value of the state of each key.
  hw->keyboard[0xC] = 0xF8 + (prob_map[0xB] << 1) + prob_map[0xA]; // B, A#    prob_map gets updated over and over in the ISR, but the keyboard map only needs to be drawn
  hw->keyboard[0xD] = 0xFC + (prob_map[0xB] << 1) + prob_map[0xA]; // B, A#    once per visualization cycle... that's why it's updated here instead.

  //hw->drawNum(frame_period, 0);
  if( dsp_mode == MODE_CV ) hw->drawCStr(hw->keyboard, 14, 0, 3);     //          In full screen mode, draw the keyboard string onto the top of the screen

}


// Draw the callibration menu that helps you adjust the potentiometer on the back of the module
void DSP::drawCallibration(){
  uint32_t bit_img_col  = 0;                                                   // Contains a 32-pixel column (1 bit per pixel) of the image
  uint32_t new_bit_val  = 0;                                                   // Puts 1's in all of the bits between the last value and the current value
  uint32_t last_bit_val = 0;                                                   // Stores prior value of new_bit_val for comparison
  uint8_t  buffer_pos = (255 + output_index - 254) & 0xFF;                   // Current position in the buffer

  last_bit_val = (uint32_t(0b10) << ((output_buffer[buffer_pos] )>>5)) - 1;    // Set up last_bit_val by putting a 1 in the correct column and then subtract 1

  int16_t currentVal = rolling_avg - (0x200-0x40) - CALLIBRATION_OFFSET;       // Grab the latest weighted average input value

  // Plot out the oscilloscope visualization on the top of the screen
  uint8_t screen_col = 0;                                                      // Track the current column on the screen that we are rendering
  for( screen_col = 0; screen_col<128; screen_col++ ){                         // Loop through the columns in the display
    buffer_pos = (255 + output_index - ((127-screen_col)<<1)) & 0xFF;        // Calculate the buffer position so it appears like the new values flow from the right
 
    new_bit_val = (uint32_t(0b10) << ((output_buffer[buffer_pos])>>5)) - 1;    // Put a 1 in the bit (row) that corresponds with the buffer value of the column
    bit_img_col = (new_bit_val ^ last_bit_val) | (uint32_t(1) << ((output_buffer[buffer_pos])>>5)); // Compose the bit image to display to the screen
    last_bit_val = new_bit_val;                                                // Save the new_bit_value for the next trip around the loop

    display_buffer[ screen_col       ] = (bit_img_col & 0xFF);                 // Update the first set of 8 rows of the visualization 
    bit_img_col = bit_img_col >> 8;
    display_buffer[ screen_col + 128 ] = (bit_img_col & 0xFF) | ((screen_col & 0b10)<<6);  // Update second set of 8 rows and draw a dotted line in the center every other column;
    bit_img_col = bit_img_col >> 8;
    display_buffer[ screen_col + 256 ] = (bit_img_col & 0xFF);                 // Update the third set of 8 columns of the visualization 
    bit_img_col = bit_img_col >> 8;
    display_buffer[ screen_col + 384 ] = (bit_img_col & 0xFF);                 // Update the fourth set of 8 columns of the visualization 
  }

  // Draw the bottom half of the screen visualization depending on how closely
  // aligned the potentiometer is to the correct value
  if( currentVal < -128 ){                                                     // If the value of input is extremely low, then it's likely that the user didn't connect
      hw->drawCStr("CALLIBRATION MODE:   ", 21, 4);                            // output to the input. So prompt them to do that first. We also need them to turn the input
      hw->drawCStr("Connect IN -> OUT    ", 21, 5);                            // attentuation all of the way up
      hw->drawCStr("then turn attenuation", 21, 6);
      hw->drawCStr("fully clockwise      ", 21, 7);

  } else if( currentVal < 0 ){                                                 // If the value is close, then we can use the oscilloscope visualization to have them turn until
      hw->drawCStr("CALLIBRATION MODE:   ", 21, 4);                            // the input line overlaps the dotted line in the scope
      hw->drawCStr("Now turn trimpot CCW ", 21, 5);
      hw->drawCStr("until the horizontal ", 21, 6);
      hw->drawCStr("lines overlap        ", 21, 7);

  } else if( currentVal < 64 ){                                                // Once we start getting really close, then we add a new visualization that moves a line from
    for( screen_col = 0; screen_col<128; screen_col++ ){                       // left to right. The user needs to center that line in the middle of the visualization.
      display_buffer[screen_col + 512] = 0x00;                                 // Make the first set of 8 rows blank
      if( screen_col == 0x40 ){                                                // For the second and fourth sets of 8 rows, if we are in the middle of the screen
        display_buffer[screen_col + 640] = 0xFF;                               // Draw a full 8-row vertical line
        display_buffer[screen_col + 896] = 0xFF;
      } else if( screen_col % 16 == 0){                                        // If we are in one of hte 16 pixel increments (but not in the middle column)
        display_buffer[screen_col + 640] = 0xF0;                               // Draw a 4 row line (tick mark) on the top
        display_buffer[screen_col + 896] = 0x0F;                               // and another 4 row line (tick mark) on the bottom
      } else {
        display_buffer[screen_col + 640] = 0x00;                               // Otherwise, just make the pixels in these two sets of 8 rows black
        display_buffer[screen_col + 896] = 0x00;
      }
      if( (screen_col>>1) == currentVal ){                                     // For the third row, if the current column matches the value of currentVal
        display_buffer[screen_col + 768] = 0xFF;                               // Then draw the verical indicator
      } else {
        display_buffer[screen_col + 768] = 0x00;                               // Otherwise, just make the column black
      }
    }

    if( currentVal < 30 ){                                                     // Provide additional instruction to the user to keep turning Counter Clockwise
      hw->drawCStr("A bit more CCW      ", 20, 4);
    } else if( currentVal < 34 ){                                              // When the indicator is in range, tell them to stop turning the dial
      hw->drawCStr("That's Perfect!     ", 20, 4);
    } else {
      hw->drawCStr("A bit more Clockwise", 20, 4);                             // If they go to far, tell them to turn it back a little
    }

  } else {
      hw->drawCStr("CALLIBRATION MODE:   ", 21, 4);                            // IF they go WAY too far, then turn off the second visualization and tell them to turn
      hw->drawCStr("Now turn trimpot CW  ", 21, 5);                            // clockwise until they get back into the right range
      hw->drawCStr("until the horizontal ", 21, 6);
      hw->drawCStr("lines overlap        ", 21, 7);
  }

}

#endif