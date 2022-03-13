# Repetita Versio

A versatile looper for the Versio platform.

## Etymology

Repetita: from Latin Repetere for “repeat” (as in Repetita Iuvant, meaning
“repeating does good”).

Versio: from Latin for “versatile”.

“Versatile repeater”

## Description

RV is a firmware that will require some time to master but will reward with a
lot of fun even with just a few twists of the kobs.
At its core it's two independent circular buffers for the left and right
channels and its primary function is that of a looper, but its design lets RV go
into other territories, such as delay, comb filter, stutter and distortion. It
also can play nicely as a wavetable oscillator or provide various flavors of
noise and raw percussions, even when nothing is connected to the inputs.

The features:

- 80 seconds buffer @ 48KHz
- independent control for the left and right channels of loop start, loop
length, speed and freeze amount
- 3 play mode (loop, one-shot and gated) with input
- looper and delay mode
- variable decay with multimode filter in the path
- loop start control with automatic loop wrapping
- loop length and playback direction in a single, continuous, control
- continuous read speed control, from 0.2x to 1x to 4x, with slew control
- "note" mode with 4 octaves tracking
- instantaneous buffer freezing
- control of mix between frozen and unfrozen signals
- stereo width control
- filter amount control
- input gain control

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
- ccw > fully dry;
- cw > fully wet.

**Start:** Sets the loop starting/retriggering point. The loop wraps around at
the end of the buffer, so its size doesn't change when the start point moves. It
can be channel-dependant.
- ccw > begin of the buffer;
- cw > end of the buffer.

**Tone:** Applies a resonant filter to the feedback path. The default type is
band-pass, but it can be changed to low-pass or high-pass in *global mode* (see
below). When the looper is frozen some of this filtered signal is mixed with
the wet signal that goes directly to the output. The resonance amount depends on
the decay amount (the higher the decay the lower the resonance):
- ccw > cutoff at 0 Hz (no filter);
- cw > cutoff at 1500 Hz.

**Size:** Changes the loop size. It can be channel-dependant.
- ccw > full loop size but playback is reversed;
- noon > note mode;
- cw > full loop size, forward playback.

**Decay:** Controls the level of decay of the recorded signal:
- ccw > instant decay;
- cw > no decay.

**Rate:** The rate of playback of the buffer. In note mode, controls the
transposition (from -2 octaves to +2 octave). It can be channel-dependant.
- ccw > slowest rate (0.2x);
- noon > normal rate (1x);
- cw > fastest rate (4x).

**Freeze:** Progressively mix the un-frozen and the frozen buffer. It can be
channel-dependant.
- ccw > completely un-frozen (recording);
- cw > completely frozen (no recording).

**Channel:** The top switch set the channel currently controlled:
- left > left channel;
- center > both channels are controlled;
- right > right channel.

**Play mode:** The bottom switch sets how the reproduction is triggered:
- left > gated
- center > one-shot, restarts at loop start point;
- right > continuously loops.

**Play input:** Gate or trigger depending on the relative switch.

## Startup state

The advised startup state of the UI would be:

**Blend:** 50%
**Start:** 0%
**Filter:** 0%
**Size:** 100%
**Decay:** 0%
**Rate:** 50%
**Freeze:** 0%
**Channel:** center
**Play mode:** right

## Buffering

As soon as the module starts, the buffer is filled with the input signal. During
the buffering process the leds gradually light up red to represent how much
buffer is being written, with all 4 leds fully lit representing a full buffer.
The maximum buffer length is 80 seconds, but the process may be interrupted at
any time by pressing the button, resulting in a shorter working buffer.
Upon completion (or interruption) of the process, the reproduction of the
buffer's content begins automatically.
The buffer can be easily reset (see below).

## Triggering the looper

When the looper is in either loop or one-shot mode, the rapid press of the
button restarts the looper at the start position. In one-shot mode, the
reproduction stops at the end of the loop.
In gate mode the looper plays while the button is pressed.
The same behaviour applies when the module receives a positive voltage at the
**Play input**.

## Global mode

Global options are accessed in any mode keeping the button pressed for more than
0.3 seconds. When in *global mode* the leds lit-up with either a light yellow,
creamy color when in looper mode or a light blue, icy color when in delay mode.
In this mode the knobs acts as follow:

**Blend:** Input gain (default: unity):
- ccw > 0 (no input);
- cw > 5x.

**Start:** Offset of the right looper's read head from the left's:
- ccw > 0 (no offset, default);
- cw > right loop length (maximum offset).

**Tone:** Filter type:
- ccw > low pass;
- noon > band pass (default);
- cw > high pass.

**Start:** Looper/delay mode:
- ccw > looper mode (default);
- cw > delay mode.

**Decay:** Filter level, that is the amount of the filtered part of the feedback
that is written back in the buffer:
- ccw > 0;
- cw > unity.

**Rate:** Rate slew (in seconds):
- ccw > 0 (no slew, default);
- cw > 10.

**Freeze:** Stereo width:
- ccw > mono;
- cw > stereo (default).

## Leds

The 4 leds visualize different information depending of the context.

During buffering, the leds light up gradually with a red color while the buffer
fills. When the 4 leds are all lit with a bright red color the buffer has been
completely filled. During the normal operation, the leds light up gradually
indicating the read head's position of the longest of the two channels. Their
color depends on both the current mode (looper/delay) and the **Size** knob's
position.

In looper mode:

- cw > orange;
- *flanger zone* (forward) > yellow;
- *flanger zone* (backwards) > lime;
- ccw > green.

In delay mode:

- cw > blue;
- *flanger zone* (forward) > azure;
- *flanger zone* (backwards) > lavender;
- ccw > purple.

In both modes the leds are lit white when the **Size** knob is at noon.

## Operation

### Looper mode (default)

In this mode the writing head uses all the available working buffer and its loop
size never changes, resulting in every change made to the reading head is written
to the buffer.
In this mode the **Size** knob only acts on the reading head loop, and changing
it results in a stuttering effect, where the same fragment is continuosly
repeated and written in the buffer until the writing head finally loops and a
new fragment is written.

### Delay mode

In this mode, activated by turning the **Size** knob clock-wise past noon while
in the *global mode*, the reading and writing loop sizes are kept synchronized and
the module acts as a delay, where the time is given by the loop size.

- **Size:** when fully cw the delay time is at its longest (the fully recorded
    buffer), lowering it allows to obtain a slap-back type of delay. Lower values
    send the looper in chorus territory. The delayed sound may be reversed by
    moving the knob ccw past noon.

#### Comb filter

When the loop size is really short (< 10ms) the looper acts as a comb filter:

- **Size:** between approximately 1 and 2 o'clock the loop size is
    sufficiently short to allow the generation of a comb filter-like sound,
    especially with **Decay** dialed high. Going ccw past noon produced a
    different and grittier result, given that the looper is going backwards.
    Interesting results can be achieved when reaching the lower boundary between
    approximately 10 and 11 o'clock, just before the loop gets long again;
- **Decay:** the higher the amount, the more prominent is the effect;
- **Rate:** dialing the right amount can produce a flanged sound, otherwise it
    easier for the wet signal to get destroyed: at higher amounts the sound gets
    progressively more electric and fizzy, while going ccw past noon it gets
    crackling and then disappears.

### Splitting the channels

When the **Channel** switch is in the center (it should be the default setting),
the **Start**, **Size**, **Rate** and **Freeze** knobs control both the left and
the right channels at the same time. By flipping the switch either to the left
or to the right, the channels may be controlled separately. When switching back
to the center, the knobs now act as an offset, thus maintaining the distance
between the left and the right position. To align the left and right channels
again simply switch to the each channel and twist the knob to the current
position, so the channel's value is updated.

Splitting the channels can produce a variety of interesting and complex effects,
but there are two main gotchas to be aware of:

1) it is often impossibile to go back to the original state of the channels,
mainly for two reasons. The first one is that the relative start points can
easily diverge, for example when changing the **Rate** knob. With luck this can
be resolved by moving the **Start** knob fully ccw in *global mode* and thus
re-aliging the right channel with the left. The second reason is that the left
and right buffers' contents will also diverge to a point in which even re-
aligning the channels will not be enough to get back to two correlated signals.

2) given that the Versio's CV inputs are tied to the knobs, the modulation will
always acts only on the currently selected channel, or on both at the same time
if the switch is in the center position. Obviously this can be used creatively
(for example as a randomization of sorts), but be advised that things can go out
of control pretty fast.

Note: when the left and right channels go out of phase, the separation can be
very noticeable in the stereo field, even produce phase cancellation. To
minimize the effect reduce the stereo width by turning the **Freeze** knob ccw
while in *global mode*. This can also be useful for bringing closer two totally
uncorrelated sound sources in the left and right channels.

### Resetting the buffer

The size of the working buffer is defined at the moment of the completion (or
interruption) of the initial buffering process. In the case of requiring a
buffer of different size, the buffering process can be repeated at any time.
First, the operation must be armed by pressing and holding the button for more
than 1 second. Once armed, the four leds will be all lit up red. Now pressing
the button will start the buffering and a second press will stop it. Starting
and stopping the buffering can also be done by sending positive voltages to the
trigger input when the play mode is either one-shot or loop.
Pressing and holding the button while armed will cancel the operation.

## Explorative notes

### As an oscillator (of sorts)

When the **Size** knob enters the *flanger zone* the **Rate** knob acts as a
pitch control and the looper can act as a wavetable oscillator of sorts. If the
**Size** knob is at noon the loop size is at its shortest (a little less than
1 ms) and the looper should produce a C4 note:

- **Rate:** fully ccw is 2 octaves below and fully cw is 2 octaves above. The
    relative CV input should track decently;
- **Freeze:** when fully frozen, the played sound fragment is fixed and thus the
    produced sound is clear and defined. Let some of the unfrozen sound pass
    through to get a more organic tone;
- **Start:** the timbre of the produced sound can be altered by changing the
    loop's starting point. If the sound's volume happens to be too low, try
    moving this knob to find a louder fragment.

### Self oscillation

Jump-start the oscillation by placing the module in delay mode, **Decay** fully
cw and **Size** at noon. From here, there's a lot of fun to be had by playing
with the **Size**, **Filter** and **Rate** knobs. Don't forget to crank up the
filter level (**Decay** knob in *global mode*) for a stronger filter effect!