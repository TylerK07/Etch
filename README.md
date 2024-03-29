# Etch
 Etch is a digital signal processing Eurorack module that selectively removes signal quality vertically (through bit crushing) and horizontally (through sample-crushing)


## Modes:
There are two modes, Audio and CV Quantizer mode. Conceptually both modes are based on the idea of “etching” away the accuracy of a signal either horizontally or vertically. 
This of course ends up creating horizontal and vertical lines——like an etch-a-sketch. These etchings occur using two controls: "Rate" and "Crush"
A third setting, "Filter" provides additional options for manipulating the output signal. 

How these settings work in practice depends on the context of the input signal——whether it is a control voltage or an audio input. So, let’s break those two modes down.


## Audio Mode:
In audio mode, the module treats the input as an audio input signal. It passes the signal through a capacitor to remove DC offset, and supports a range of -5v to +5v

### Hardware Controls (Also CV Controllable):
* **Rate:** This pot controls the sample rate between ~20 Hz up to ~12.5 KHz (this is about as fast as I’ve gotten the micro to go and still stay responsive). The rate changes exponentially to allow for 1v/oct control.
* **Crush:** This pot controls the bit crush, but with more granularity. The value is rounded to the nearest 1...255, providing 255 levels of bit crush.
* **Filter:** This pot controls a low-pass filter with resonant feedback

### Audio Mode Menu Options (Use Rotary Encoder):
* **Resonance:** Controls the amount of feedback that gets sent back into the filter stage
* **Reverb Amount:** Controls the wet/dry mix of the input signal and the reverb signal
* **Reverb Delay:** Controls the delay of the signal, note that this also will be affected by Rate, and the delay will increase as the sample rate decreases
* **Reverb Feedback:** Controls the decay of the reverb signal allowing it to repeat for a longer period of time
  

### Audio Loop Mode:
At any time you can tap the “loop” button and the last 256 samples in the output buffer will lock and repeat over. This adds some interesting new properties:
* **Rate:** Since this control already operates at 1v/oct, rate becomes a pitch control for the waveform locked in the output buffer.
* **Loop Length:** This new menu setting controls how many samples from the buffer to include in the repeat loop. The smaller the buffer, the higher the pitch too.
* **Morph Rate:** While the buffer loop repeats, Etch will continue capturing the input into a separate buffer. Then, based on the speed indicated by the value of morph, etch will blend from one buffer to the next. Once it reaches the new buffer, it will repeat the process with a new sample. The blending of one sample to the next uses a tweening function so everything feels seamless and creates either a fast or slowly evolving sound. This also makes for some really crazy effects when the morph value is low and the sample rate is high. Kind of like robot speech or something. 


## Control Voltage Mode:
In CV Mode, the input gets treated as 0-10v control voltages and the DC offset is not removed. While the controls are conceptually similar, in practice they apply in different ways.

### Hardware Controls (Also CV Controllable):
* **Rate:** Still fundamentally controls the sample rate, but since we are dealing with CVs, this actually acts as a fixed-rate sample and hold. If you turn sample rate all of the way down, then the Rate CV input will be treated as a sample & hold gate.
* **Crush:** Still affects the voltage accuracy (like bit-crush), but if we treat the input CV as a pitch CV the purpose of bit-crushing changes to quantizing accuracy. More on this in a moment... 
* **Filter:** Adds a drag to the input value——just like a normal filter. Because the filtering occurs before the notes are quantized, it ends up controlling the interval range of consecutive notes.
* **Quant Root:** This basically transposes the quantized note by some number of notes allowing you to choose which scale you want to output. This is especially useful for changing “chords” on the fly. 
* **Quant Scale:** This allows you to flip between different scales including the weighted major and weighted minor scales, but also a variety of other “fixed” scales that simply quantize the notes the same way every time. 

### A note about scale quantization:
Many of the scales are "fixed quantized" scales, meaning that any incoming note will be translated into the closes note on the scale. This is how most quantizers work.

The Etch module also features "weighted" Major and Minor scales. These scales associate weights to each note in the scale based on how often the note is usually played in Major and Minor music. The **Crush** control then adjusts which of those notes to include each time it quantizes a note, ranging from just the root note, to the full major/minor scale to a fully chromatic scale with all possible "accidentals." You can think of this as **Scale Crushing** An element of randomization ensures that the quantization feels more musical and less mechanical. This also fixes the issue of major and minor scales being fundamentally the same (because there are relative major/minor scales). By adjusting the statistical likelihood of notes, you can make the same set of notes sound more major or minor.


### CV Loop Mode:
Just like in audio mode, you can also tap that loop button, and it brings a whole new flavor to CV mode. All of the pots (Rate / Crush / Filter) still work the exact same way as before, but there are a few new options:
* **Loop Length:** This works just like the audio rate mode, but in CV mode it turns things into a looping CV pattern of adjustable duration. Increasing the sample rate is going to increase the accuracy of the loop, but also reduce the loop’s actual duration, so there is a lot of flexibility in adjusting how you want this to work.
* **Morph:** Yep, this one works here too, and it allows for very complex melody generation because you can effectively shift a repeating pattern of notes from one pattern to another one. So you get this continually repeating and yet adjusting pattern.


## Calibration Mode:
This mode is used to calibrate the trim pot on the back of the module. Just connect the input to the output with a patch cable and then twist the trimpot per the instructions on the screen.


## Other Feature Notes:
* Push and hold the rotary encoder in either CV or Audio more to make the oscilloscope visualization full-screen