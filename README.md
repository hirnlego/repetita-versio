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
- 3 button mode (loop, one-shot and gated) with input
- looper and delay mode
- variable decay with multi-mode filter and degradation in the feedback path
- loop start control with automatic loop wrapping
- loop length and playback direction in a single, continuous, control
- continuous read speed control, from 0.2x to 1x to 4x, with controllable slew
up to 10 seconds
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
- XYZ > [button]
- FSU > [trigger]

**Blend:** Dry/wet balance control.

- ccw > fully dry;
- cw > fully wet.

**Start:** Sets the loop starting/re-triggering point. The loop wraps around at
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

**Size:** Changes both the loop size and playback direction. It can be channel-
dependant.

- ccw > full loop size but playback is reversed;
- noon > note mode;
- cw > full loop size, forward playback.

**Decay:** Controls the level of decay of the recorded signal. This signal
passes through a degradation unit and a resonant filter. When the looper is
frozen, this knob also controls how much of the filtered fed-back signal is
mixed with the wet frozen signal.

- ccw > instant decay;
- cw > no decay.

**Rate:** The rate of playback of the buffer, from 0.2x to 4x. In note mode,
controls the transposition (from -2 octaves to +2 octave). It can be channel-
dependant.

- ccw > slowest rate (0.2x);
- noon > normal rate (1x);
- cw > fastest rate (4x).

**Freeze:** Progressively mixes the un-frozen and the frozen buffer. It can be
channel-dependant. The signal that is normally written in the main buffer is
also written in another identical buffer. When this parameter is greater than
0%, the second buffer is instantaneously frozen and is not written anymore. By
turning the knob cw the wet signal is progressively mixed with the frozen
signal, and when fully cw only the frozen signal is heard. To un-freeze the
signal, turn the knob fully ccw at 0%.

- ccw > completely un-frozen (recording);
- cw > completely frozen (no recording).

**Channel:** The top switch set the channel currently controlled:

- left > left channel;
- center > both channels are controlled at the same time;
- right > right channel.

**Button mode:** The bottom switch sets different behaviors for the looper and
how the button and the relative input work:

- left > the looper is both recording and reproducing but the dry signal is
recorded only when the button is depressed or a positive voltage is present at
the input. Caveat: when the switch is in this position, the *global mode* is not
accessible;
- center > the looper reproduction is stopped and can be re-triggered for a one-
shot loop by pressing the button or with a positive voltage at the input. The
reproduction always begins at the start position defined by the **Start** knob;
- right > the looper is continuously reproducing its contents. A press of the
button, or a positive voltage at the input, restarts the reproduction at the
position defined by the **Start** knob.

**Trigger input:** Gate or trigger depending on the relative switch.

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
**Button mode:** right

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

When the bottom switch is either in the center or right position, the rapid
press of the button restarts the reproduction at the start position. In one-shot
mode (center position), it stops at the end of the loop. The same thing happens
when a positive voltage is received at the relative input.

## Global mode

Global options can be accessed when the bottom switch is either in the center or
in the right position by keeping the button pressed for more than 0.3 seconds.
When in *global mode* the leds lit-up with either a light yellow, creamy color
when in looper mode or a light blue, icy color when in delay mode. Caveat:
because the Versio's CV inputs are tied to the respective knobs and it's
impossible to establish if a parameter is being changed using either the knob or
the CV input, voltages present at the inputs will affect the global options!
Unfortunately the only workaround is disengaging the modulations before entering
this mode.
In this mode the knobs acts as follow:

**Blend:** Input gain (default: unity):

- ccw > 0 (no input);
- cw > 5x.

**Start:** Stereo width:

- ccw > mono;
- cw > stereo (default).

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

**Rate:** Rate slew:

- ccw > 0 (no slew, default);
- cw > 10 seconds.

**Freeze:** Controls both the density and the amount of the degradation:

- ccw > no degradation;
- cw > maximum degradation.

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
size never changes, resulting in that every change made to the reading head is
written to the buffer.
In this mode, then, the **Size** knob only acts on the reading head loop, and
changing it results in a stuttering effect, where the same fragment is
continuously repeated and written in the buffer until the writing head finally
loops and a new fragment is written.

### Delay mode

In this mode, activated by turning the **Size** knob clock-wise past noon while
in the *global mode*, the reading and writing loop sizes are kept synchronized
and the module acts as a delay, where the time is given by the loop size.

### Channels control

When the top switch is in the center (it should be the default setting), the
**Start**, **Size**, **Rate** and **Freeze** knobs control both the left and
the right channels at the same time. By flipping the switch either to the left
or to the right, the channels may be controlled separately. When switching back
to the center, the knobs now act as an offset, thus maintaining the distance
between the left and the right position. To align the left and right channels
again simply switch to each channel and twist back the knob to the current
position, so the channel's value is updated.

Splitting the channels can produce a variety of interesting and complex effects,
but there are two main caveats to be aware of:

1) it is often impossible to realign the channels because twisting the knobs
produce destructive modifications to both buffers.

2) more important and less obvious: for the same reason saw above when talking
about the *global mode*, the modulation will always acts only on the currently
selected channel, or on both at the same time if the switch is in the center
position. This can be used creatively (for example as a randomization of sorts),
but be advised that things can go out of control pretty fast :)

Note: when the left and right channels go out of phase, the separation can be
very noticeable in the stereo field, even produce phase cancellation. To
minimize the effect reduce the stereo width by turning the **Start** knob ccw
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
trigger input when the button mode is either one-shot or loop.
Pressing and holding the button while armed will cancel the operation.

## Exploration notes

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
