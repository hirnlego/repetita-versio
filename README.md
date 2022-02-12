# Repetita Versio

A versatile looper for the Versio platform.

## Etymology

Repetita: from Latin Repetere for “repeat” (as in Repetita Iuvant, meaning “repeating does good”)

Versio: from Latin for “versatile”

“Versatile repeater”

## Controls

- Knob 1 > [blend]
- Knob 2 > [start]
- Knob 3 > [tone]
- Knob 4 > [size]
- knob 5 > [decay]
- Knob 6 > [rate]
- Knob 7 > [freeze]
- ABC > [channel]
- XYZ > [play]
- FSU > [trigger]

**Blend:** Dry/wet balance control.

**Start:** Sets the loop starting/retriggering point.

**Tone:** Applies a resonant filter to both the dry signal - that is mixed with the feedback - and the wet signals - that goes to the output:
- ccw > no filter;
- cw > cutoff at 1000 Hz.

**Size:** Changes the loop size:
- ccw > full loop length but reversed, goes backwards;
- noon > note mode;
- cw > full loop length, goes forward.

**Decay:** Controls the level of decay of the recorded signal:
- ccw > instant decay;
- cw > no decay.

**Rate:** The rate of reproduction of the buffer. In note mode, controls the 
transposition (from -2 octaves to +2 octave):
- ccw > slowest rate (0.2x);
- noon > normal rate (1x);
- cw > fastest rate (4x).

**Freeze:** Progressively freezes the buffer:
- ccw > completely un-frozen (recording);
- cw > completely frozen (no recording).

**Channel:** The top switch set the channel currently controlled:
- left > left channel;
- center > both channels are controlled;
- right > right channel.

**Play mode:** The bottom switch sets how the play is triggered:
- left > momentary, no restart, slowly accelerates/decelerates while changing the pitch;
- center > one shot, restarts at loop start point;
- right > continuously loops.

**Play input:** Gate or trigger depending on the relative switch.

## Buffering

As soon as the module starts the buffer is filled with the input signal. During
the buffering process the leds gradually light up red to represent how much
buffer is being written, with all 4 leds fully lit representing a full buffer.
The maximum buffer length is 150 seconds, but if a shorter buffer is desired
the process may be interrupted by pressing the button.
Upon completion (or interruption) of the process the reproduction of the
buffer's content begins.
The buffer can be cleared and the process started again by pressing and holding
the button for more than one second in edit mode (see below).

## Triggering the looper

When the looper is in either loop or trigger mode the rapid press of the button
restarts the looper at the **start point**.
In gate mode the looper plays while the button is pressed.
The same behaviour applies when the module receives a positive voltage at the
**play input**.

## Edit mode

Edit mode is entered by pressing and holding the button for more than 0.25
seconds when the module is either in loop or in trigger mode.

## Global mode

Global options are accessed in any mode keeping the button pressed for more than
0.25 seconds. In this scenario the knobs acts as follow:

**Blend:** Input gain:
- ccw > 0 (no input);
- cw > 5x.

**Start:** Offset of the right looper's read head from the left's:
- ccw > 0 (no offset);
- cw > right loop length (maximum offset).

**Tone:** Filter type:
- ccw > low pass;
- noon > band pass;
- cw > high pass.

**Start:** Length of the crossfade (in samples):
- ccw > 0;
- cw > 4800.

**Decay:** Stereo width:
- ccw > mono;
- cw > stereo.

**Rate:** Rate slew (in seconds):
- ccw > 0 (no slew);
- cw > 10.

**Freeze:** Locks the read and write loop lengths:
- ccw > off (looper mode);
- cw > on (delay mode).

## Leds

The 4 leds visualize different information depending of the context.

During buffering, the leds light up gradually with a red color while the buffer
fills. When the 4 leds are all lit with a bright red color the buffer has been
completely wrote. During the normal operation, the leds light up gradually
indicating the read head's position of the longest of the two channels. Their
color depends on the **Size** knob's position:

- light yellow when full cw;
- green when at noon;
- dark purple when full ccw.

When one of the two channels is selected, the 4 leds split in two groups, one
for the left channel and one for the right. The above colors apply also in this
mode.

## Exploration notes

### As a looper (loop sync on - default)

### As a delay (loop sync off)

When the loop sync option is off (**Freeze** knob ccw of noon in global mode), 
with a relatively short buffer (or loop) length the looper acts as a delay, where 
the time is given by the length itself:

- **Size:** when fully cw the delay time is at its longest (the full recorded
    buffer), lowering it allows to reach a slap-back type of delay. Lower values
    send the looper in chorus territory. The delayed sound may be reverted by
    moving the knob ccw past noon;
- **Blend:** blends dry and delayed (wet) signals;
- **Decay:** controls the delay feedback amount, the higher the value the smeared
    is the wet signal. Noon is a good amount to let the delay tail fade slowly;

### Comb filter zone (loop sync off)

Also when loop sync is off, if the loop length is really short (< 10ms) the
looper acts as a comb filter:

- **Size:** between approximately 1 and 2 o'clock the loop length is
    sufficiently short to allow the production of a comb filter-like sound,
    especially with **Decay** dialed high. Going ccw past noon produced a
    different and grittier result, given that the looper is going backwards.
    Interesting results can be achieved when reaching the lower boundary between
    approximately 10 and 11 o'clock, just before the loop gets too long and
    disperse;
- **Decay:** the higher the amount, the more prominent is the effect;
- **Rate:** dialing the right amount can produce a flanged sound, otherwise it
    easier for the wet signal to get destroyed: at higher amounts the sound gets
    progressively more electric and fizzy, while going ccw past noon it gets
    crackling and then disappears.

### As an oscillator (of sorts)

When the **Size** knob is at noon the loop length is at its shortest (a little
less than 1ms). When in this position, the looper act as a wavetable oscillator
of sorts, and it can be played with an acceptably good tracking along 4 octaves:

- **Rate:** at noon it should produce a C4 (?), fully ccw is 2 octaves below,
    fully cw is 2 octaves above. The relative CV input should track decently;
- **Freeze:** when fully frozen, the played sound fragment is fixed and thus the
    produced sound is clear and defined. Let some of the unfrozen sound pass
    through to get a dirtier tone;
- **Start:** the timbre of the produced sound can be altered by changing the
    loop's starting point. If the sound's volume is too low, try moving this
    knob to find a stronger fragment.

### Self oscillation

The looper can be used also when no inputs are connected. Just jump-start it by
placing the **Start** knob at noon and the **Decay** knob fully cw. From here
it's just a matter of feeding the initial sound an play with the looper.