# TODO

- bug; fix gate mode when changing the relation between read and write heads (loop length, ratio, freeze)
- bug: the filter doesn't respect gate mode
- compensate for the low volume when rate is slow
- realign read heads when loopers params have been successfully picked-up in mono mode
- rework the size parameter to account for the longest loop
- flanger mode -> stepped rate?
- clickless activation of the filter
- button + rate = slew?
- what to do with feedback when frozen?
- save status

## DONE

- ~~bug: turn off buffering leds when the buffer has been completely written~~
- ~~bug; fix buffering leds when resetting the looper (does the looper reset correctly at all?)~~
- ~~button + mix knob = input gain~~
- ~~button + tone = filter type~~
- ~~bug: CV inputs don't work with parameters pick-up~~
- ~~bug: when coming from node mode we should update the rate parameter~~
- ~~bug: don't use TimeHeldMs() for handling button hold, use the internal clock~~
- ~~parameters affecting both loopers at the same time should be offsets of each looper's base parameters~~
- ~~nice to have: parameters pick-up, so the parameters don't have an immediate effect when switching the selected looper~~
