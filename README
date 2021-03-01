
Bristol Emulations
------------------

This is a write-up of each of the emulated synthesisers. The algorithms
employed were 'gleaned' from a variety of sources including the original
owners manuals, so they may be a better source of information. The author
has owned and used a selection but far from all of the originals. Some of them
were built just from descriptions of their operation, or from understanding
how synths work - most of them were based on the Mini Moog anyway. Many of
the synths share components: the filter covers most of them, the Prophets and
Oberheims share a common oscillator and the same LFO is used in many of them.
Having said that each one differs considerably in the resulting sound that is
generated, more so than initially expected. Each release refines each of the
components and the result is that all emulations benefit from the improvements.
All the emulations have distinctive sounds, not least due to that the original
instruments used different modulations and mod routing.
The filter, which is a large defining factor in the tonal qualities of any
synth, is common to all the emulations. The filter implements a few different 
algorithms and these do separate each of the synths: the Explorer layering
two low pass filters on top of each other: the OB-Xa using different types
depending on 'Pole' selection. Since release 0.20.8 the emulator has had a
Houvillainen non-linear ladder filter integrated which massively improves 
the quality at considerable expense to the CPU.
There is one further filter algorithm used solely for the Leslie rotary 
emulator crossover, this is a butterworth type filter.

Bristol is in no way related to any of the original manufacturers whose 
products are emulated by the engine and represented by the user interface,
bristol does not suggest that the emulation is a like representation of the
original instrument, and the author maintains that if you want the original
sound then you are advised to seek out the original product. Alternatively a
number of the original manufacturers now provide their own vintage collections
which are anticipated to be more authentic. All names and trademarks used by
Bristol are ownership of the respective companies and it is not inteded to 
misappropriate their use here. If you have concerns you are kindly requested
to contact the author.

The write-up includes the parameter operations, modulations, a description of
the original instrument and a brief list of the kind of sounds you can expect
by describing a few of the well known users of the synth.

Several emulations have not been written up. Since the APR 2600 was implemented
it became a rather large job to actually describe what was going on. If you 
really want to know about the synths that are not in this document then you
might want to search for their owners manuals.

All emulations are available from the same engine, just launch multiple GUIs
and adjust the midi channels for multi timbrality and layering.

It is noted here that the engine is relatively 'dumb'. Ok, it generates a very
broad range of sounds, currently about 25 different synthesisers and organs,
but it is not really intelligent. Memories are a part of the GUI specification
- it tells the engine which algorithm to use on which MIDI channel, then it
calls a memory routine that configures all the GUI controllers and a side effect
of setting the controllers is that their values are sent to the engine. This is
arguably the correct model but it can affect the use of MIDI master keyboards.
The reason is that the GUI is really just a master keyboard for the engine and
drives it with MIDI SYSEX messages over TCP sessions. If you were to alter the
keyboard transpose, for example, this would result in the GUI sending different
'key' numbers to the engine when you press a note. If you were already driving
the synth from a master keyboard then the transpose button in the Brighton GUI
would have no effect - the master keyboard would have to be transposed instead.
This apparent anomaly is exacerbated by the fact that some parameters still are
in the engine, for example master tuning is in the engine for the pure fact that
MIDI does not have a very good concept of master tuning (only autotuning).
Irrespective of this, bristol is a synthesiser so it needs to be played, 
tweaked, driven. If you think that any of the behaviour is anomalous then let
me know. One known issue in this area is that if you press a key, transpose
the GUI, then release the key - it will not go off in the engine since the GUI
sends a different key code for the note off event - the transposed key. This
cannot be related to the original keypress. This could be fixed with a MIDI all
notes off event on 'transpose', but I don't like them. Also, since the 0.20
stream the problem only affects a few of the emulations, the rest now sending
a transpose message to the engine and letting it do the work.

Since release 0.30.6 the engine correctly implements monophonic note logic.
Prior to this the whole engine was polyphonic and playing with one voice only
gave last note preference which dramatically affects playing styles - none of
the cool legato effects of the early monophonics. The quoted release fix this
limitation where the engine will keep a keymap of all played keys (one per
emulation) when started with a single voice and uses this map to provide
consistent note precedence, high note logic, low note logic or just using the
previously implemented last note logic. In this release the keymap was only
maintained with monophonic emulations, this is a potential extension as even
in polyphonic mode it would be useful for arpeggiation (which is currently
implemented using a FIFO rather than an ordered keymap).




    Moog Mini
    ---------

It is perhaps not possible to write up who used this synth, the list is endless.
Popular as it was about the first non-modular synthesiser, built as a fixed
configuration of the racked or modular predecessors.

Best known at the time on Pink Floyd 'Dark Side of the Moon' and other albums.
Rick Wakeman used it as did Jean Michel Jarre. Wakefield could actually
predict the sound it would make by just looking at the settings, nice to be
able to do if a little unproductive but it went to show how this was treated
as an instrument in its own right. It takes a bit of work to get the same sweet,
rich sounds out of the emulation, but it can be done with suitable tweaking.

The original was monophonic, although a polyphonic version was eventually made
after Moog sold the company - the MultiMoog. This emulation is more comparable
to that model as the sound is a bit thinner and can be polyphonic. The design
of this synth became the pole bearer for the following generations: it had 
three oscillators, one of which could become a low frequency modulator. They
were fed into a mixer with a noise source, and were then fed into a filter
with 2 envelope generators to contour the wave. Modulation capabilities were
not extensive, but interestingly enough it did have a frequency modulation (FM)
capability, eventually used by Yamaha to revolutionise the synthesiser market
starting the downfall of analogue synthesis twenty years later.

All the analogue synths were temperature sensitive. It was not unusual for the
synths to 'detune' between sound test and performance as the evening set in.
To overcome this they could optionally produce a stable A-440Hz signal for 
tuning the oscillators manually - eventually being an automated option in the
newer synths. Whilst this digital version has stable frequency generation the
A-440 is still employed here for the sake of it.

Modifiers and mod routing are relatively simple, Osc-3 and noise can be mixed,
and this signal routed to the oscillator 1 and 2 frequency or filter cutoff.

The synth had 5 main stages as follows:

Control:

    Master tuning: up/down one note.

    Glide: (glissando, portamento). The rate at which one key will change its
    frequency to the next played key, 0 to 30 seconds.

    Mod: source changes between Osc-3 and noise.

    Release: The envelope generators had only 3 parameters. This governed whether
    a key would release immediately or would use Decay to die out.

    Multi: Controls whether the envelope will retrigger for each new keypress.

Oscillators:

    There are three oscillators. One and two are keyboard tracking, the third
    can be decoupled and used as an LFO modulation source.

    Oscillator 1:
        Octave step from 32' to 1'.
        Waveform selection: sine/square/pulse/ramp/tri/splitramp
        Mod: controls whether Osc-3/noise modulates frequency

    Oscillator 2:
        Octave step from 32' to 1'.
        Fine tune up/down 7 half notes.
        Waveform selection: sine/square/pulse/ramp/tri/splitramp
        Mod: controls whether Osc-3/noise modulates frequency

    Oscillator 3:
        Octave step from 32' to 1'.
        Fine tune up/down 7 half notes.
        Waveform selection: sine/square/pulse/ramp/tri/splitramp
        LFO switch to decouple from keytracking.

Mixer:

    Gain levels for Oscillator 1/2/3
    Mixing of the external input source into filter
    Noise source with white/pink switch.

    Note: The level at which Osc-3 and noise modulates sound depends on its
    gain here, similarly the noise. The modulator mix also affects this, but
    allows Osc-3 to mod as well as sound. The modwheel also affect depth.

Filter:

    Cutoff frequency

    Emphasis (affects Q and resonance of filter).

    Contour: defines how much the filter envelope affects cutoff.

    Mod - Keyboard tracking of cutoff frequency.

    Mod - Osc-3/noise modulation of cutoff frequency.

Contour:

    The synth had two envelope generators, one for the filter and one for the
    amplifier. Release is affected by the release switch. If off the the sound
    will release at the rate of the decay control.

    Attack: initial ramp up of gain.

    Decay: fall off of maximum signal down to:

    Sustain: gain level for constant key-on level.

    Key: Touch sensitivity of amplifier envelope.

Improvements to the Mini would be some better oscillator waveforms, plus an
alternative filter as this is a relatively simple synthesiser and could do
with a warmer filter (this was fixed with integration of the houvillanen filters
although the do consume a lot of CPU to do it).

The Output selection has a Midi channel up/down selector and memory selector.
To read a memory either use the up/down arrows to go to the next available
memory, or type in a 3 digit number on the telephone keypad and press 'L' for
load or 'S' for save.

As of release 0.20.5 Vasiliy Basic contributed his Mini memory banks and they
are now a part of the distribution:

Programs for Bristol's "Mini" (from 50 to 86 PRG)

List of programs:

    -Melodic-
    50 - Trumpet
    51 - Cello
    52 - Guitar 1
    53 - Guitar 2
    54 - Fingered Bass
    55 - Picked Bass
    56 - Harmonica
    57 - Accordion
    58 - Tango Accordion
    59 - Super Accordion
    60 - Piano
    61 - Dark Organ
    62 - Flute
    63 - Music Box
    64 - Glass Xylo
    65 - Glass Pad
    66 - Acid Bass
    
    -Drums-
    67 - Bass Drum 1 
    68 - Bass Drum 2
    69 - Bass Drum 3
    70 - Bass Drum 4
    71 - Tom
    72 - Snare 1
    73 - Snare 2
    74 - Snare 3
    75 - Snare 4
    76 - Cl HH 1
    77 - Op HH 1
    78 - Crash Cym 1
    79 - Crash Cym 2
    80 - Cl HH 2
    81 - Op HH 2
    
    -FX-
    82 - Sea Shore
    83 - Helicopter 1
    84 - Helicopter 2
    85 - Bird Tweet
    86 - Birds Tweet




    Sequential Circuits Prophet-5
    Sequential Circuits Prophet-52 (the '5' with chorus)
    ----------------------------------------------------

Sequential circuits released amongst the first truly polyphonic synthesisers
where a group of voice circuits (5 in this case) were linked to an onboard
computer that gave the same parameters to each voice and drove the notes to
each voice from the keyboard. The device had some limited memories to allow 
for real live stage work. The synth was amazingly flexible regaring the
oscillator options and modulation routing, producing some of the fattest 
sounds around. They also had some of the fattest pricing as well, putting it
out of reach of all but the select few, something that maintained its mythical
status. David Sylvian of Duran Duran used the synth to wide acclaim in the
early 80's as did many of the new wave of bands.

The -52 is the same as the -5 with the addition of a chorus as it was easy, it
turns the synth stereo for more width to the sound, and others have done it on
the Win platform.

The design of the Prophet synthesisers follows that of the Mini Moog. It has
three oscillators one of them as a dedicated LFO. The second audio oscillator
can also function as a second LFO, and can cross modulate oscillator A for FM 
type effects. The audible oscillators have fixed waveforms with pulse width
modulation of the square wave. These are then mixed and sent to the filter with
two envelopes, for the filter and amplifier.

Modulation bussing is quite rich. There is the wheel modulation which is global,
taking the LFO and Noise as a mixed source, and send it under wheel control to
any of the oscillator frequency and pulse width, plus the filter cutoff. Poly
mods take two sources, the filter envelope and Osc-B output (which are fully
polyphonic, or rather, independent per voice), and can route them through to
Osc-A frequency and Pulse Width, or through to the filter. To get the filter
envelope to actually affect the filter it needs to go through the PolyMod
section. Directing the filter envelope to the PW of Osc-A can make wide, breathy
scanning effects, and when applied to the frequency can give portamento effects.

LFO:

    Frequency: 0.1 to 50 Hz
    Shape: Ramp/Triangle/Square. All can be selected, none selected should
    give a sine wave (*)

    (*) Not yet implemented.

Wheel Mod:

    Mix: LFO/Noise
    Dest: Osc-A Freq/Osc-B Freq/Osc-A PW/Osc-B PW/Filter Cutoff

Poly Mod: These are affected by key velocity.

    Filter Env: Amount of filter envelope applied
    Osc-B: Amount of Osc-B applied:
    Dest: Osc-A Freq/Osc-A PW/Filter Cutoff

Osc-A:

    Freq: 32' to 1' in octave steps
    Shape: Ramp or Square
    Pulse Width: only when Square is active.
    Sync: synchronise to Osc-B

Osc-B:

    Freq: 32' to 1' in octave steps
    Fine: +/- 7 semitones
    Shape: Ramp/Triangle/Square
    Pulse Width: only when Square is active.
    LFO: Lowers frequency by 'several' octaves.
    KBD: enable/disable keyboard tracking.

Mixer:

    Gain for Osc-A, Osc-B, Noise

Filter:

    Cutoff: cuttof frequency
    Res: Resonance/Q/Emphasis
    Env: amount of PolyMod affecting to cutoff.

Envelopes: One each for PolyMod (filter) and amplifier.

    Attack
    Decay
    Sustain
    Release

Global:

    Master Volume
    A440 - stable sine wave at A440 Hz for tuning.
    Midi: channel up/down
    Release: release all notes
    Tune: autotune oscillators.
    Glide: amount of portamento

    Unison: gang all voices to a single 'fat' monophonic synthesiser.

This is one of the fatter of the Bristol synths and the design of the mods
is impressive (not my design, this is as per sequential circuits spec). Some
of the cross modulations are noisy, notably 'Osc-B->Freq Osc-A' for square
waves as dest and worse as source.

The chorus used by the Prophet-52 is a stereo 'Dimension-D' type effect. The
signal is panned from left to right at one rate, and the phasing and depth at
a separate rate to generate subtle chorus through to helicopter flanging.

Memories are loaded by selecting the 'Bank' button and typing in a two digit
bank number followed by load. Once the bank has been selected then 8 memories
from the bank can be loaded by pressing another memory select and pressing
load. The display will show free memories (FRE) or programmed (PRG).




    Yamaha DX-7
    -----------

Released in the '80s this synth quickly became the most popular of all time.
It was the first fully digital synth, employed a revolutionary frequency 
modulated algorithm and was priced much lower than the analogue monsters
that preceded it. Philip Glass used it to wide effect for Miami Vice, Prince
had it on many of his albums, Howard Jones produced albums filled with its
library sounds. The whole of the 80's were loaded with this synth, almost to
the point of saturation. There was generally wide use of its library sounds
due to the fact that it was nigh on impossible to programme, only having entry
buttons and the algorithm itself was not exactly intuitive, but also because
the library was exceptional and the voices very playable. The emulation is a
6 operator per voice, and all the parameters are directly accessible to ease
programming.

The original DX had six operators although cheaper models were release with
just 4 operators and a consequently thinner sound. Each operator is a sine
wave oscillator with its own envelope generator for amplification and a few 
parameters that adjusted its modulators. It used a number of different 
algorithms where operators were mixed together and then used to adjust the
frequency of the next set of operators. The sequence of the operators affected
the net harmonics of the overall sound. Each operator has a seven stage 
envelope - 'ramp' to 'level 1', 'ramp' to 'level 2', 'decay' to 'sustain',
and finally 'release' when a key is released. The input gain to the frequency
modulation is controllable, the output gain is also adjustable, and the final
stage operators can be panned left and right.

Each operator has:

    Envelope:

        Attack: Ramp rate to L1
        L1: First target gain level
        Attack: Ramp rate from L2 to L2
        L2: Second target gain level
        Decay: Ramp rate to sustain level
        Sustain: Continuous gain level
        Release: Key release ramp rate

    Tuning:

        Tune: +/- 7 semitones
        Transpose: 32' to 1' in octave increments

        LFO: Low frequency oscillation with no keyboard control

    Gain controls:

        Touch: Velocity sensitivity of operator.

        In gain: Amount of frequency modulation from input
        Out gain: Output signal level

        IGC: Input gain under Mod control
        OGC: Output gain under Mod control

        Pan: L/R pan of final stage operators.

Global and Algorithms:

    24 different operator staging algorithms
    Pitchwheel: Depth of pitch modifier
    Glide: Polyphonic portamento
    Volume
    Tune: Autotune all operators

Memories can be selected with either submitting a 3 digit number on the keypad,
or selecting the orange up/down buttons.

An improvement could be more preset memories with different sounds that can
then be modified, ie, more library sounds. There are some improvements that
could be made to polyphonic mods from key velocity and channel/poly pressure
that would not be difficult to implement.

The addition of triangle of other complex waveforms could be a fun development
effort (if anyone were to want to do it).

The DX still has a prependancy to seg fault, especially when large gains are
applied to input signals. This is due to loose bounds checking that will be
extended in a present release.




    Roland Juno-60
    --------------

Roland was one of the main pacemakers in analogue synthesis, also competing
with the Sequential and Oberheim products. They did anticipate the moving
market and produced the Juno-6 relatively early. This was one of the first
accessible synths, having a reasonably fat analogue sound without the price
card of the monster predecessors. It brought synthesis to the mass market that
marked the decline of Sequential Circuits and Oberheim who continued to make
their products bigger and fatter. The reduced price tag meant it had a slightly
thinner sound, and a chorus was added to extend this, to be a little more
comparable.

The synth again follows the Mini Moog design of oscillators into filter into
amp. The single oscillator is fattened out with harmonics and pulse width
modulation. There is only one envelope generator that can apply to both the
filter and amplifier.

Control:

    DCO: Amount of pitch wheel that is applied to the oscillators frequency.
    VCF: Amount of pitch wheel that is applied to the filter frequency.

    Tune: Master tuning of instrument

    Glide: length of portamento

    LFO: Manual control for start of LFO operation.

Hold: (*)

    Transpose: Up/Down one octave
    Hold: prevent key off events

LFO:

    Rate: Frequency of LFO
    Delay: Period before LFO is activated
    Man/Auto: Manual or Automatic cut in of LFO

DCO:

    LFO: Amount of LFO affecting frequency. Affected by mod wheel.
    PWM: Amount of LFO affecting PWM. Affected by mod wheel.

    ENV/LFO/MANUAL: Modulator for PWM

    Waveform:
        Pulse or Ramp wave. Pulse has PWM capabily.
    
    Sub oscillator:
        On/Off first fundamental square wave.
    
    Sub:
        Mixer for fundamental

    Noise:
        Mixer of white noise source.

HPF: High Pass Filter

    Freq:
        Frequency of cutoff.

VCF:

    Freq:
        Cutoff frequency

    Res:
        Resonance/emphasis.
    
    Envelope:
        +ve/-ve application
    
    Env:
        Amount of contour applied to cutoff
    
    LFO:
        Depth of LFO modulation applied.
    
    KBD:
        Amount of key tracking applied.

VCA:

    Env/Gate:
        Contour is either gated or modulated by ADSR
    
    Level:
        Overall volume

ADSR:

    Attack
    Decay
    Sustain
    Release

Chorus:

    8 Selectable levels of Dimension-D type helicopter flanger.

* The original instrument had a basic sequencer on board for arpeggio effects
on each key. In fact, so did the Prophet-10 and Oberheims. This was only 
implemented in 0.10.11.

The LFO cut in and gain is adjusted by a timer and envelope that it triggers.

The Juno would improve from the use of the prophet DCO rather than its own one.
It would require a second oscillator for the sub frequency, but the prophet DCO
can do all the Juno does with better resampling and PWM generation.




    Moog Voyager (Bristol "Explorer")
    ---------------------------------

This was Robert Moog's last synth, similar in build to the Mini but created
over a quarter of a century later and having far, far more flexibility. It 
was still monophonic, a flashback to a legendary synth but also a bit like
Bjorn Borg taking his wooden tennis racket back to Wimbledon long after having
retired and carbon fibre having come to pass. I have no idea who uses it and
Bjorn also crashed out in the first round. The modulation routing is exceptional
if not exactly clear.

The Voyager, or Bristol Explorer, is definitely a child of the Mini. It has
the same fold up control panel, three and half octave keyboard and very much
that same look and feel. It follows the same rough design of three oscillators
mixed with noise into a filter with envelopes for the filter and amplifier.
In contrast there is an extra 4th oscillator, a dedicated LFO bus also Osc-3
can still function as a second LFO here. The waveforms are continuously 
selected, changing gradually to each form: bristol uses a form of morphing
get get similar results. The envelopes are 4 stage rather than the 3 stage
Mini, and the effects routing bears no comparison at all, being far more
flexible here.

Just because its funny to know, Robert Moog once stated that the most difficult
part of building and releasing the Voyager was giving it the title 'Moog'. He
had sold his company in the seventies and had to buy back the right to use his
own name to release this synthesiser as a Moog, knowing that without that title
it probably would not sell quite as well as it didn't.

Control:

    LFO:
        Frequency
        Sync: LFO restarted with each keypress.

    Fine tune +/- one note
    Glide 0 to 30 seconds.

Modulation Busses:

    Two busses are implemented. Both have similar capabilities but one is
    controlled by the mod wheel and the other is constantly on. Each bus has
    a selection of sources, shaping, destination selection and amount.

    Wheel Modulation: Depth is controller by mod wheel.

        Source: Triwave/Ramp/Sample&Hold/Osc-3/External
        Shape: Off/Key control/Envelope/On
        Dest: All Osc Frequency/Osc-2/Osc-3/Filter/FilterSpace/Waveform (*)
        Amount: 0 to 1.

    Constant Modulation: Can use Osc-3 as second LFO to fatten sound.

        Source: Triwave/Ramp/Sample&Hold/Osc-3/External
        Shape: Off/Key control/Envelope/On
        Dest: All Osc Frequency/Osc-2/Osc-3/Filter/FilterSpace/Waveform (*)
        Amount: 0 to 1.

        * Destination of filter is the cutoff frequency. Filter space is the 
        difference in cutoff of the two layered filters. Waveform destination 
        affects the continuously variable oscillator waveforms and allows for 
        Pulse Width Modulation type effects with considerably more power since
        it can affect ramp to triangle for example, not just pulse width.

Oscillators:

    Oscillator 1:
        Octave: 32' to 1' in octave steps
        Waveform: Continuous between Triangle/Ramp/Square/Pulse

    Oscillator 2:
        Tune: Continuous up/down 7 semitones.
        Octave: 32' to 1' in octave steps
        Waveform: Continuous between Triangle/Ramp/Square/Pulse

    Oscillator 3:
        Tune: Continuous up/down 7 semitones.
        Octave: 32' to 1' in octave steps
        Waveform: Continuous between Triangle/Ramp/Square/Pulse

    Sync: Synchronise Osc-2 to Osc-1
    FM: Osc-3 frequency modulates Osc-1
    KBD: Keyboard tracking Osc-3
    Freq: Osc-3 as second LFO

Mixer:

    Gain levels for each source: Osc-1/2/3, noise and external input.

Filters:

    There are two filters with different configuration modes:

    1. Two parallel resonant lowpass filters.
    2. Serialised HPF and resonant LPF

    Cutoff: Frequency of cutoff
    Space: Distance between the cutoff of the two filters.
    Resonance: emphasis/Q.
    KBD tracking amount
    Mode: Select between the two operating modes.

Envelopes:

    Attack
    Decay
    Sustain
    Release

    Amount to filter (positive and negative control)

    Velocity sensitivity of amplifier envelope.

Master:

    Volume
    LFO: Single LFO or one per voice (polyphonic operation).
    Glide: On/Off portamento
    Release: On/Off envelope release.

The Explorer has a control wheel and a control pad. The central section has
the memory section plus a panel that can modify any of the synth parameters as
a real time control. Press the first mouse key here and move the mouse around
to adjust the controls. Default values are LFO frequency and filter cutoff 
but values can be changed with the 'panel' button. This is done by selecting
'panel' rather than 'midi', and then using the up/down keys to select parameter
that will be affected by the x and y motion of the mouse. At the moment the
mod routing from the pad controller is not saved to the memories, and it will
remain so since the pad controller is not exactly omnipresent on MIDI master
keyboards - the capabilities was put into the GIU to be 'exact' to the design.

This synth is amazingly flexible and difficult to advise on its best use. Try
starting by mixing just oscillator 1 through to the filter, working on mod 
and filter options to enrich the sound, playing with the oscillator switches
for different effects and then slowly mix in oscillator 2 and 3 as desired.

Memories are available via two grey up/down selector buttons, or a three digit
number can be entered. There are two rows of black buttons where the top row
is 0 to 4 and the second is 5 to 9. When a memory is selected the LCD display
will show whether it is is free (FRE) or programmed already (PRG).




   Hammond B3 (dual manual)
    ------------------------

The author first implemented the Hammond module, then extended it to the B3
emulation. Users of this are too numerous to mention and the organ is still
popular. Jimmy Smith, Screaming Jay Hawkins, Kieth Emerson, Doors and 
almost all american gospel blues. Smith was profuse, using the instrument for
a jazz audience, even using its defects (key noise) to great effect. Emerson
had two on stage, one to play and another to kick around, even including
stabbing the keyboard with a knife to force keylock during performances
(Emerson was also a Moog fan with some of the first live performances). He
also used the defects of the system to great effect, giving life to the over-
driven Hammond sound.

The Hammond was historically a mechanical instrument although later cheaper
models used electronics. The unit had a master motor that rotated at
the speed of the mains supply. It drove a spindle of cog wheels and next to 
each cog was a pickup. The pickup output went into the matrix of the harmonic
drawbars. It was originally devised to replace the massive pipe organs in
churches - Hammond marketed their instruments with claims that they could not be
differentiated from the mechanical pipe equivalent. He was taken to court by
the US government for misrepresentation, finally winning his case using a double
blind competitive test against a pipe organ, in a cathedral, with speakers
mounted behind the organ pipes and an array of music scholars, students and 
professionals listening. The results spoke for themselves - students would
have scored better by simply guessing which was which, the professionals
fared only a little better than that. The age of the Hammond organ had arrived.

The company had a love/hate relationship with the Leslie speaker company - the
latter making money by selling their rotary speakers along with the organ to
wide acceptance. The fat hammond 'chorus' was a failed attempt to distance
themselves from Leslie. That was never achieved due to the acceptance of the
Leslie, but the chorus did add another unique sound to the already awesome
instrument. The rotary speaker itself still added an extra something to the
unique sound that is difficult imagine one without the other. It has a wide
range of operating modes most of which are included in this emulator.

The chorus emulation is an 8 stage phase shifting filter algorithm with a 
linear rotor between the taps.

Parameterisation of the first B3 window follows the original design:

    Leslie: Rotary speaker on/off
    Reverb: Reverb on/off
    VibraChorus: 3 levels of vibrato, 3 of chorus.
    Bright: Added upper harmonics to waveforms.

Lower and Upper Manual Drawbars: The drawbars are colour coded into white for
even harmonics and black for odd harmonics. There are two subfrequencies in 
brown. The number given here are the length of organ pipe that would 
correspond to the given desired frequency.

    16    - Lower fundamental
    5 1/3 - Lower 3rd fundamental
    8     - Fundamental
    4     - First even harmonic
    2 2/3 - First odd harmonic
    2     - Second even harmonic
    1 3/5 - Second odd harmonic
    1 1/3 - Third odd harmonic
    1     - Third even harmonic

The drawbars are effectively mixed for each note played. The method by which
the mixing is done is controlled in the options section below. There were 
numerous anomalies shown by the instrument and most of them are emulated.

The Hammond could provide percussives effect the first even and odd harmonics.
This gave a piano like effect and is emulated with Attack/Decay envelope.

    Perc 4'     - Apply percussive to the first even harmonic
    Perc 2 2/3' - Apply percussive to the first odd harmonic
    Slow        - Adjust rate of decay from about 1/2 second to 4 seconds.

    Soft        - Provide a soft attack to each note.

The soft attack is an attempt to reduce the level of undesired key noise. The
keyboard consisted of a metal bar under each key that made physical contact 
with 9 sprung teeth to tap off the harmonics. The initial contact would generate
noise that did not really accord to the pipe organ comparison. This was 
reduced by adding a slow start to each key, but the jazz musicians had used
this defect to great effect, terming it 'key click' and it became a part of
the Hammond characteristics. Some musicians would even brag about how noisy
there organ was.

On the left had side are three more controls:

    Volume potentiometer

    Options switch discussed below.

    Rotary Speed: low/high speed Leslie rotation. Shifts between the speeds
    are suppressed to emulate the spin up and down periods of the leslie motors.

The options section, under control of the options button, has the parameters
used to control the emulation. These are broken into sections and discussed
individually.

Leslie:

The Leslie rotary speaker consisted of a large cabinet with a bass speaker and
a pair of high frequency air horns. Each were mounted on its own rotating table
and driven around inside the cabinet by motors. A crossover filter was used to
separate the frequencies driven to either speaker. Each pair was typically 
isolated physically from the other. As the speaker rotated it would generate
chorus type effects, but far richer in quality. Depending on where the speaker
was with respect to the listener the sound would also appear to rotate. There
would be different phasing effects based on signal reflections, different
filtering effects depending on where the speaker was in respect to the cabinet
producing differences resonances with respect to the internal baffling.

    Separate:
    Sync:
    No Bass:
        The Leslie had two motors, one for the horns and one for the voice coil
        speaker. These rotated at different speeds. Some players preferred to 
        have both rotate at the same speed, would remove the second motor and
        bind the spindles of each speaker table, this had the added effect
        that both would also spin up at the same rate, not true of the 
        separated motors since each table had a very different rotary moment.
        The 'No Bass' option does not rotate the voice coil speaker. This was
        typically done since it would respond only slowly to speed changes,
        this left just the horns rotating but able to spin up and down faster.

    Brake:
        Some cabinets had a brake applied to the tables such that when the
        motor stopped the speakers slowed down faster.

    X-Over:
        This is the cross over frequency between the voice coil and air horns.
        Uses a butterworth filter design.

    Inertia:
        Rate at which speaker rotational speed will respond to changes.

    Overdrive:
        Amount by which the amplifier is overdriven into distortion.

    H-Depth/Frequency/Phase
    L-Depth/Frequency/Phase
        These parameters control the rotary phasing effect. The algorithm used
        has three differently phased rotations used for filtering, phasing and
        reverberation of the sound. These parameters are used to control the
        depth and general phasing of each of them, giving different parameters
        for the high and low speed rotations. There are no separate parameters
        for the voice coil or air horns, these parameters are for the two
        different speeds only, although in 'Separate' mode the two motors will
        rotate at slightly different speeds.

Chorus

    V1/C1 - Lowest chorus speed
    V2/C2 - Medium chorus speed
    V3/C3 - High chorus speed

Percussion:

    Decay Fast/Slow - controls the percussive delay rates.
    Attack Slow Fast - Controls the per note envelope attack time.

The percussives are emulated as per the original design where there was a
single envelope for the whole keyboard and not per note. The envelope will only
restrike for a cleanly pressed note.

Finally there are several parameters affecting the sine wave generation code.
The Hammond used cogged wheels and coil pickups to generate all the harmonics,
but the output was not a pure sine wave. This section primarily adjusts the
waveform generation:

    Preacher:
        The emulator has two modes of operation, one is to generate the 
        harmonics only for each keyed note and another to generate all of
        them and tap of those required for whatever keys have been pressed.
        Both work and have different net results. Firstly, generating each
        note independently is far more efficient than generating all 90 odd
        teeth, as only a few are typically required. This does not have totally
        linked phases between notes and cannot provide for signal damping (see
        below).
        The Preacher algorithm generates all harmonics continuously as per the
        original instrument. It is a better rendition at the expense of large
        chunks of CPU activity. This is discussed further below.

    Compress:
        Time compress the sine wave to produce a slightly sharper leading edge.

    Bright:
        Add additional high frequency harmonics to the sine.

    Click:
        Level of key click noise
    
    Reverb:
        Amount of reverb added by the Leslie
    
    Damping:
        If the same harmonic was reused by different pressed keys then its net
        volume would not be a complete sum, the output gain would decay as the
        pickups would become overloaded. This would dampen the signal strength.
        This is only available with the Preacher algorithm.

The two reverse octaves are presets as per the original, however here they can
just be used to recall the first 23 memories of the current bank. The lower
manual 12 key is the 'save' key for the current settings and must be double
clicked. It should be possible to drive these keys via MIDI, not currently 
tested though. The default presets are a mixture of settings, the lower 
manual being typical 'standard' recital settings, the upper manual being a
mixture of Smith, Argent, Emerson, Winwood and other settings from the well
known Hammond Leslie FAQ. You can overwrite them. As a slight anomaly, which
was intentional, loading a memory from the these keys only adjusts the visible
parameters - the drawbars, leslie, etc. The unseen ones in the options panel
do not change. When you save a memory with a double click on the lower manual
last reverse key then in contrast it saves all the parameters. This will not
change.

The Preacher algorithm supports a diverse set of options for its tonewheel
emulation. These are configured in the file $BRISTOL/memory/profiles/tonewheel
and there is only one copy. The file is a text file and will remain that way,
it is reasonably documented in the file itself. Most settings have two ranges,
one representing the normal setting and the other the bright setting for when
the 'bright' button is pressed. The following settings are currently available:

    ToneNormal: each wheel can be given a waveform setting from 0 (square)
        through to 1.0 (pure sine) to X (sharpening ramp).

    EQNormal: each wheel can be given a gain level across the whole generator.

    DampNormal: each wheel has a damping factor (level robbing/damping/stealing)

    BusNormal: each drawbar can be equalised globally.


    ToneBright: each wheel can be given a waveform setting from 0 (square)
        through to 1.0 (pure sine) to X (sharpening ramp) for the bright button.

    EQBright: each wheel can be given a gain level across the whole generator.

    DampBright: each wheel has a damping factor (level robbing/damping/stealing)

    BusBright: each drawbar can be equalised globally.


    stops: default settings for the eight drawbar gain levels.

        The default is 8 linear stages.

    wheel: enables redefining the frequency and phase of any given tonewheel

        The defaults are the slightly non Even Tempered frequencies of the
        Hammond tonewheels. The tonewheel file redefines the top 6 frequencies
        that were slightly more out of tune due to the 192-teeth wheels and
        a different gear ratio.

    crosstalk: between wheels in a compartment and adjacent drawbar busses.

        This is one area that may need extensions for crosstalk in the wiring
        loom. Currently the level of crosstalk between each of the wheels in
        the compartment can be individually defined, and drawbar bus crosstalk
        also.

    compartment: table of the 24 tonewheel compartments and associated wheels.

    resistors: tapering resister definitions for equalisation of gains per
        wheel by note by drawbar.

    taper: definition of the drawbar taper damping resistor values.

Improvements would come with some other alterations to the sine waveforms and
some more EQ put into the leslie speaker. The speaker has three speeds, two of
which are configurable and the third is 'stopped'. Changes between the different
rates is controlled to emulate inertia.

The net emulation, at least of the preacher algorithm, is reasonable, it is
distinctively a Hammond sound although it does not have quite as much motor
or spindle noise. The Bright button gives a somewhat rawer gearbox. It could do
with a better amplifier emulation for overdrive.

The damping algorithms is not quite correct, it has dependencies on which keys
are pressed (upper/lower manual). Options drop shadow is taken from the wrong
background bitmap so appears in an inconsistent grey.




    Vox Continental
    ---------------

This emulates the original mark-1 Continental, popular in its time with the
Animals on 'House of the Rising Sun', Doors on 'Light my Fire' and most of
their other tracks. Manzarek did use Gibson later, and even played with the
Hammond on their final album, 'LA Woman' but this organ in part defined
the 60's sound and is still used by retro bands for that fact. The Damned
used it in an early revival where Captain Sensible punched the keyboard
wearing gloves to quite good effect. After that The Specials began the Mod/Ska
revival using one. The sharp and strong harmonic content has the ability to
cut into a mix and make its presence known.

The organ was a british design, eventually sold (to Crumar?) and made into a
number of plastic alternatives. Compared to the Hammond this was a fully 
electronic instrument, no moving parts, and much simpler. It had a very
characteristic sound though, sharper and perhaps thinner but was far cheaper
than its larger cousin. It used a master oscillator that was divided down to
each harmonic for each key (as did the later Hammonds for price reasons). This
oscillator division design was used in the first of the polyphonic synthesisers
where the divided note was fead through individual envelope generators and
a shared or individual filter (Polymoog et al).

The Vox is also a drawbar instrument, but far simplified compared to the
Hammond. It has 4 harmonic mixes, 16', 8' and 4' drawbars each with eight
positions. The fourth gave a mix of 2 2/3, 2, 1 1/3 and 1 foot pipes.
An additional two drawbars controlled the overall volume and waveforms, one
for the flute or sine waves and another for the reed or ramp waves. The
resulting sound could be soft and warm (flute) or sharp and rich (reed).

There are two switches on the modulator panel, one for vibrato effect and one
for memories and options. Options give access to an chorus effect rather 
than the simple vibrato, but this actually detracts from the qualities of the
sound which are otherwise very true to the original.

Vox is a trade name owned by Korg Inc. of Japan, and Continental is one of 
their registered trademarks. Bristol does not intend to infringe upon these
registered names and Korg have their own remarkable range of vintage emulations
available. You are directed to their website for further information of true
Korg products.




    Fender Rhodes
    -------------

Again not an instrument that requires much introduction. This emulation is
the DX-7 voiced synth providing a few electric piano effects. The design is 
a Mark-1 Stage-73 that the author has, and the emulation is reasonable if not
exceptional. The Rhodes has always been widely used, Pink Floyd on 'Money',
The Doors on 'Riders on the Storm', Carlos Santana on 'She's not There',
everybody else in the 60's.

The Rhodes piano generated its sound using a full piano action keyboard where
each hammer would hit a 'tine', or metal rod. Next to each rod was a pickup
coil as found on a guitar, and these would be linked together into the output.
The length of each tine defined its frequency and it was tunable using a tight
coiled spring that could be moved along the length of the tine to adjust its
moment. The first one was built mostly out of aircraft parts to amuse injured
pilots during the second world war. The Rhodes company was eventually sold to
Fender and lead to several different versions, the Mark-2 probably being the
most widely acclaimed for its slightly warmer sound.

There is not much to explain regarding functionality. The emulator has a volume
and bass control, and one switch that reveals the memory buttons and algorithm
selector.

The Rhodes would improve with the addition of small amounts of either reverb
or chorus, potentially to be implemented in a future release.

The Rhodes Bass was cobbled together largely for a presentation on Bristol.
It existed and was used be Manzarek when playing with The Doors in
Whiskey-a-GoGo; the owner specified that whilst the music was great they
needed somebody playing the bass. Rather than audition for the part Manzarek
went out and bought a Rhodes Bass and used it for the next couple of years.




    Sequential Circuits Prophet-10
    ------------------------------

The prophet 10 was the troublesome brother of the Pro-5. It is almost two
Prophet-5 in one box, two keyboards and a layering capability. Early models
were not big sellers, they were temperamental and liable to be temperature 
sensitive due to the amount of electronics hidden away inside. The original
layering and 'unison' allowed the original to function as two independent
synths, a pair of layered synths (both keyboards then played the same sound),
as a monophonic synth in 'unison' mode on one keybaord with a second polyphonic
unit on the other, or even all 10 voices on a single keyed note for a humongous
20 oscillator monophonic monster.

Phil Collins used this synth, and plenty of others who might not admit to it.

The emulator uses the same memories as the Prophet-5, shares the same algorithm,
but starts two synths. Each of the two synths can be seen by selecting the U/D
(Up/Down) button in the programmer section. Each of the two synthesisers loads
one of the Pro-5 memories.

There was an added parameter - the Pan or balance of the selected layer, used
to build stereo synths. The lower control panel was extended to select the
playing modes:

    Dual: Two independent keyboards
    Poly: Play note from each layer alternatively
    Layer: Play each layer simultaneously.

In Poly and Layer mode, each keyboard plays the same sounds.

    Mods: Select which of the Mod and Freq wheels control which layers.




    Sequential Circuits Prophet-5
    Sequential Circuits Prophet-52 (the '5' with chorus)
    ----------------------------------------------------

Sequential circuits released amongst the first truly polyphonic synthesisers
where a group of voice circuits (5 in this case) were linked to an onboard
computer that gave the same parameters to each voice and drove the notes to
each voice from the keyboard. The device had some limited memories to allow 
for real live stage work. The synth was amazingly flexible regaring the
oscillator options and modulation routing, producing some of the fattest 
sounds around. They also had some of the fattest pricing as well, putting it
out of reach of all but the select few, something that maintained its mythical
status. David Sylvian of Duran Duran used the synth to wide acclaim in the
early 80's as did many of the new wave of bands.

The -52 is the same as the -5 with the addition of a chorus as it was easy, it
turns the synth stereo for more width to the sound, and others have done it on
the Win platform.

The design of the Prophet synthesisers follows that of the Mini Moog. It has
three oscillators one of them as a dedicated LFO. The second audio oscillator
can also function as a second LFO, and can cross modulate oscillator A for FM 
type effects. The audible oscillators have fixed waveforms with pulse width
modulation of the square wave. These are then mixed and sent to the filter with
two envelopes, for the filter and amplifier.

Modulation bussing is quite rich. There is the wheel modulation which is global,
taking the LFO and Noise as a mixed source, and send it under wheel control to
any of the oscillator frequency and pulse width, plus the filter cutoff. Poly
mods take two sources, the filter envelope and Osc-B output (which are fully
polyphonic, or rather, independent per voice), and can route them through to
Osc-A frequency and Pulse Width, or through to the filter. To get the filter
envelope to actually affect the filter it needs to go through the PolyMod
section. Directing the filter envelope to the PW of Osc-A can make wide, breathy
scanning effects, and when applied to the frequency can give portamento effects.

LFO:

    Frequency: 0.1 to 50 Hz
    Shape: Ramp/Triangle/Square. All can be selected, none selected should
    give a sine wave (*)

    (*) Not yet implemented.

Wheel Mod:

    Mix: LFO/Noise
    Dest: Osc-A Freq/Osc-B Freq/Osc-A PW/Osc-B PW/Filter Cutoff

Poly Mod: These are affected by key velocity.

    Filter Env: Amount of filter envelope applied
    Osc-B: Amount of Osc-B applied:
    Dest: Osc-A Freq/Osc-A PW/Filter Cutoff

Osc-A:

    Freq: 32' to 1' in octave steps
    Shape: Ramp or Square
    Pulse Width: only when Square is active.
    Sync: synchronise to Osc-B

Osc-B:

    Freq: 32' to 1' in octave steps
    Fine: +/- 7 semitones
    Shape: Ramp/Triangle/Square
    Pulse Width: only when Square is active.
    LFO: Lowers frequency by 'several' octaves.
    KBD: enable/disable keyboard tracking.

Mixer:

    Gain for Osc-A, Osc-B, Noise

Filter:

    Cutoff: cuttof frequency
    Res: Resonance/Q/Emphasis
    Env: amount of PolyMod affecting to cutoff.

Envelopes: One each for PolyMod (filter) and amplifier.

    Attack
    Decay
    Sustain
    Release

Global:

    Master Volume
    A440 - stable sine wave at A440 Hz for tuning.
    Midi: channel up/down
    Release: release all notes
    Tune: autotune oscillators.
    Glide: amount of portamento

    Unison: gang all voices to a single 'fat' monophonic synthesiser.

This is one of the fatter of the Bristol synths and the design of the mods
is impressive (not my design, this is as per sequential circuits spec). Some
of the cross modulations are noisy, notably 'Osc-B->Freq Osc-A' for square
waves as dest and worse as source.

The chorus used by the Prophet-52 is a stereo 'Dimension-D' type effect. The
signal is panned from left to right at one rate, and the phasing and depth at
a separate rate to generate subtle chorus through to helicopter flanging.

Memories are loaded by selecting the 'Bank' button and typing in a two digit
bank number followed by load. Once the bank has been selected then 8 memories
from the bank can be loaded by pressing another memory select and pressing
load. The display will show free memories (FRE) or programmed (PRG).




    Oberheim OB-X
    -------------

Oberheim was the biggest competitor of Sequential Circuits, having their OB
range neck and neck with each SC Prophet. The sound is as fat, the OB-X 
similar to the Prophet-5 as the OB-Xa to the Prophet-10. The synths were widely
used in rock music in the late seventies and early 80s. Their early polyphonic
synthesisers had multiple independent voices linked to the keyboard and were
beast to program as each voice was configured independently, something that
prevented much live usage. The OB-X configured all of the voices with the same
parameters and had non-volatile memories for instant recall.

Priced at $6000 upwards, this beast was also sold in limited quantities and
as with its competition gained and maintained a massive reputation for rich,
fat sounds. Considering that it only had 21 continuous controllers they were
used wisely to build its distinctive and flexible sound.

The general design again follows that of the Mini Moog, three oscillators with
one dedicated as an LFO the other two audible. Here there is no mixer though,
the two audible oscillators feed directly into the filter and then the amplifier.

The richness of the sound came from the oscillator options and filter, the 
latter of which is not done justice in the emulator.

Manual:

    Volume
    Auto: autotune the oscillators
    Hold: disable note off events
    Reset: fast decay to zero for envelopes, disregards release parameter.
    Master Tune: up/down one semitone both oscillators.

Control:

    Glide: up to 30 seconds
    Oscillator 2 detune: Up/down one semitone

    Unison: gang all voices to a single 'fat' monophonic synthesiser.

Modulation:

    LFO: rate of oscillation
    Waveform: Sine/Square/Sample&Hold of noise src. Triangle if none selected.

    Depth: Amount of LFO going to:
        Freq Osc-1
        Freq Osc-2
        Filter Cutoff

    PWM: Amount of LFO going to:
        PWM Osc-1
        PWM Osc-2

Oscillators:

    Freq1: 32' to 1' in octave increments.
    PulseWidth: Width of pulse wave (*).
    Freq2: 16' to 1' in semitone increments.

    Saw: sawtooth waveform Osc-1 (**)
    Puls: Pulse waveform Osc-1

    XMod: Osc-1 FW to Osc-2 (***)
    Sync: Osc-2 sync to Osc-1

    Saw: sawtooth waveform Osc-2
    Puls: Pulse waveform Osc-2

    * Although this is a single controller it acts independently on each of the
    oscillators - the most recent to have its square wave selected will be
    affected by this parameter allowing each oscillator to have a different
    pulse width as per the original design.

    ** If no waveform is selected then a triangle is generated.

    *** The original synth had Osc-2 crossmodifying Osc-1, this is not totally
    feasible with the sync options as they are not mutually exclusive here.
    Cross modulation is noisy if the source or dest wave is pulse, something
    that may be fixed in a future release.

Filter:

    Freq: cutoff frequency
    Resonance: emphasis (*)
    Mod: Amount of modulation to filter cutoff (**)

    Osc-1: Osc-1 to cutoff at full swing.
    KDB: Keyboard tracking of cutoff.

    Half/Full: Oscillator 2 to Cutoff at defined levels (***)
    Half/Full: Noise to Cutoff at defined levels (***)

    * In contrast to the original, this filter can self oscillate.

    ** The original had this parameter for the envelope level only, not the
    other modifiers. Due to the filter implementation here it affects total
    depth of the sum of the mods.

    *** These are not mutually exclusive. The 'Half' button gives about 1/4,
    the 'Full' button full, and both on gives 1/2. They could be made mutually
    exclusive, but the same effect can be generated with a little more flexibility
    here.

Envelopes: One each for filter and amplifier.

    Attack
    Decay
    Sustain
    Release

The oscillators appear rather restricted at first sight, but the parametrics
allow for a very rich and cutting sound.

Improvements would be a fatter filter, but this can be argued of all the 
Bristol synthesisers as they all share the same design. It will be altered in
a future release.

The OB-X has its own mod panel (most of the rest share the same frequency and
mod controls). Narrow affects the depth of the two controllers, Osc-2 will 
make frequency only affect Osc-2 rather than both leading to beating, or phasing
effects if the oscillators are in sync. Transpose will raise the keyboard by
one octave.

Memories are quite simple, the first group of 8 buttons is a bank, the second
is for 8 memories in that bank. This is rather restricted for a digital synth
but is reasonably true to the original. If you want more than 64 memories let
me know.




    Oberheim OB-Xa
    --------------

This is almost two OB-X in a single unit. With one keyboard they could provide
the same sounds but with added voicing for split/layers/poly options. The OB-Xa
did at least work with all 10 voices, had a single keyboard, and is renound for
the sounds of van Halen 'Jump' and Stranglers 'Strange Little Girl'. The sound
had the capability to cut through a mix to upstage even guitar solo's. Oberheim
went on to make the most over the top analogue synths before the cut price
alternatives and the age of the DX overcame them.

Parameters are much the same as the OB-X as the algorithm shares the same code,
with a few changes to the mod routing. The main changes will be in the use of
Poly/Split/Layer controllers for splitting the keyboard and layering the sounds
of the two integrated synthesisers and the choice of filter algorithm.

The voice controls apply to the layer being viewed, selected from the D/U
button.

Manual:

    Volume
    Balance
    Auto: autotune the oscillators
    Hold: disable note off
    Reset: fast decay to zero for envelopes, disregards release parameter.
    Master Tune: up/down one semitone both oscillators.

Control:

    Glide: up to 30 seconds
    Oscillator 2 detune: Up/down one semitone

    Unison: gang all voices to a single 'fat' monophonic synthesiser.

Modulation:

    LFO: rate of oscillation
    Waveform: Sine/Square/Sample&Hold of noise src. Triangle if none selected.

    Depth: Amount of LFO going to:
        Freq Osc-1
        Freq Osc-2
        Filter Cutoff

    PWM: Amount of LFO going to:
        PWM Osc-1
        PWM Osc-2
        Tremelo

Oscillators:

    Freq1: 32' to 1' in octave increments.
    PulseWidth: Width of pulse wave (*).
    Freq2: 16' to 1' in semitone increments.

    Saw: sawtooth waveform Osc-1 (**)
    Puls: Pulse waveform Osc-1

    Env: Application of Filter env to frequency
    Sync: Osc-2 sync to Osc-1

    Saw: sawtooth waveform Osc-2
    Puls: Pulse waveform Osc-2

    * Although this is a single controller it acts independently on each of the
    oscillators - the most recent to have its square wave selected will be
    affected by this parameter allowing each oscillator to have a different
    pulse width, as per the original design.

    ** If no waveform is selected then a triangle is generated.

Filter:

    Freq: cutoff frequency
    Resonance: emphasis (*)
    Mod: Amount of modulation to filter cutoff (**)

    Osc-1: Osc-1 to cutoff at full swing.
    KDB: Keyboard tracking of cutoff.

    Half/Full: Oscillator 2 to Cutoff at defined levels (***)

    Noise: to Cutoff at defined levels
    4 Pole: Select 2 pole or 4 pole filter

    * In contrast to the original, this filter will self oscillate.

    ** The original had this parameter for the envelope level only, not the
    other modifiers. Due to the filter implementation here it affects total
    depth of the sum of the mods.

    *** These are not mutually exclusive. The 'Half' button gives about 1/4,
     the 'Full' button full, and both on gives 1/2. They could be made mutually
    exclusive, but the same effect can be generated with a little more flexibility
    here.

Envelopes: One each for filter and amplifier.

    Attack
    Decay
    Sustain
    Release

Mode selection:

    Poly: play one key from each layer alternatively for 10 voices
    Split: Split the keyboard. The next keypress specifies split point
    Layer: Layer each voice on top each other.

    D/U: Select upper and lower layers for editing.

Modifier Panel:

    Rate: Second LFO frequency or Arpeggiator rate (*)
    Depth: Second LFO gain
    Low: Modifiers will affect the lower layer
    Up: Modifiers will affect the upper layer
    Multi: Each voice will implement its own LFO
    Copy: Copy lower layer to upper layer

    Mod 01: Modify Osc-1 in given layer
    Mod 02: Modify Osc-2 in given layer
    PW: Modify Pulse Width
    AMT: Amount (ie, depth) of mods and freq wheels

    Transpose: Up or Down one octave.

The Arpeggiator code integrated into release 0.20.4 has three main parts, the
arpeggiator itself, the arpeggiating sequencer and the chording. All are 
configured from the left of the main panel.

The arpeggiator is governed by the rate control that governs how the code
steps through the available keys, an octave selector for 1, 2 or 3 octaves
of arpeggiation, and finally the Up/Down/Up+Down keys - the last ones start
the arpeggiator. Arpeggiation will only affect the lower layer.

When it has been started you press keys on the keyboard (master controller
or GUI) and the code will step through each note and octaves of each note 
in the order they were pressed and according to the direction buttons. The
key settings are currently reset when you change the direction and you will
have to press the notes again.

The sequencer is a modification of the code. Select the Seq button and then 
a direction. The GUI will program the engine with a series of notes (that can
be redefined) and the GUI will sequence them, also only into the lower layer.
The sequence will only start when you press a key on the keyboard, this is 
the starting point for the sequence. You can press multiple notes to have 
them sequence in unison. Once started you can tweak parameters to control
the sound and memory 88 when loaded has the filter envelope closed down, a
bit of glide and some heavy mods to give you a starting point for some serious
fun.

To reprogram the sequence steps you should stop the sequencer, press the PRG
button, then the Sequence button: enter the notes you want to use one by one
on the keyboard. When finished press the sequence button again, it goes out.
Now enable it again - select Seq and a direction and press a note. Press two
notes.

When you save the memory the OBXa will also save the  sequence however there
is only one sequence memory - that can be changed if you want to have a sequence
memory per voice memory (implemented in 0.20.4).

The chord memory is similar to the Unison mode except that Unison plays all
voices with the same note. Chording will assign one voice to each notes in
the chord for a richer sound. To enable Chording press the 'Hold' button. This
is not the same as the original since it used the hold button as a sustain
option however that does not function well with a Gui and so it was reused.

To reprogram the Chord memory do the following: press the PRG button then the
Hold button. You can then press the keys, up to 8, that you want in the chord,
and finally hit the Hold button again. The default chord is just two notes, 
the one you press plus its octave higher. This results in multiple voices
per keypress (a total of 3 in Layered mode) and with suitable detune will 
give a very rich sound.

There is only one arpeggiator saved for all the memories, not one per memory
as with some of the other implementations. Mail me if you want that changed.



The oscillators appear rather restricted at first sight, but the parametrics
allow for a very rich and cutting sound.

The Copy function on the Mod Panel is to make Poly programming easier - generate the desired sound and then copy the complete parameter set for poly operation. 
It can also be used more subtly, as the copy operation does not affect balance
or detune, so sounds can be copied and immediately panned slightly out of tune to generate natural width in a patch. This is not per the original instrument
that had an arpeggiator on the mod panel.

The Arpeggiator was first integrated into the OBXa in release 0.20.4 but not
widely tested.




    KORG MONOPOLY
    -------------

A synth suite would not be complete without some example of a Korg instrument,
the company was also pivotal in the early synthesiser developments. This is
an implementation of their early attempts at polyphonic synthesis, it was
either this one or the Poly-6 (which may be implemented later). Other choices
would have been the MS series, MS-20, but there are other synth packages that
do a better job of emulating the patching flexibility of that synth - Bristol
is more for fixed configurations.

As with many of the Korg synths (the 800 worked similarly) this is not really
true polyphony, and it is the quirks that make it interesting. The synth had
four audio oscillators, each independently configurable but which are bussed
into a common filter and envelope pair - these are not per voice but rather
per instrument. The unit had different operating modes such that the four
oscillators can be driven together for a phat synth, independently for a form
of polyphony where each is allocated to a different keypress, and a shared
mode where they are assigned in groups depending on the number of keys pressed.
For example, if only 2 notes are held then each key is sounded on two different
oscillators, one key is sounded on all 4 oscillators, and 3 or more have one
each. In addition there are two LFOs for modulation and a basic effects option
for beefing up the sounds. To be honest to the original synth, this emulation
will only request 1 voice from the engine. Korg is one of the few original
manufacturers to have survived the transition to digital synthesis and are
still popular.

One thing that is immediately visible with this synth is that there are a lot
of controllers since each oscillator is configured independently. This is in
contrast to the true polyphonic synths where one set of controls are given to
configure all the oscillators/filters/envelopes. The synth stages do follow the
typical synth design, there are modulation controllers and an FX section
feeding into the oscillators and filter. The effects section is a set of
controllers that can be configured and then enabled/disabled with a button
press. The overall layout is rather kludgy, with some controllers that are
typically grouped being dispersed over the control panel.

Control:

    Volume

    Arpeg:
        Whether arpegiator steps up, down, or down then up. This works in
        conjunction with the 'Hold' mode described later.

    Glide: glissando note to note. Does not operate in all modes

    Octave: Up/Normal/Down one octave transpose of keyboard

    Tune: Global tuning of all oscillators +/- 50 cents (*)
    Detune: Overall detuning of all oscillators +/- 50 cents (*)

    * There is an abundance of 'Tune' controllers. Global Tuning affects all
    the oscillators together, then oscillators 2, 3 and 4 have an independent
    tune controller, and finally there is 'Detune'. The target was to tune all
    the oscillators to Osc-1 using the independent Tune for each, and then use
    the global Tune here to have the synth tuned to other instruments. The
    Detune control can then be applied to introduce some beating between the
    oscillators to fatten the sound without necessarily losing overall tune of
    the instrument.

Modulation wheels:

    Bend:
        Intensity: Depth of modulation
        Destination:
            VCF - Filter cutoff
            Pitch - Frequency of all oscillators
            VCO - Frequency of selected oscillators (FX selection below).

    MG1: Mod Group 1 (LFO)
        Intensity: Depth of modulation
        Destination:
            VCF - Filter cutoff
            Pitch - Frequency of all oscillators
            VCO - Frequency of selected oscillators (FX selection below).

LFO:

    MG-1:
        Frequency
        Waveform - Tri, +ve ramp, -ve ramp, square.

    MG-2:
        Frequency (Triangle wave only).

Pulse Width Control:

    Pulse Width Modulation:
        Source - Env/MG-1/MG-2
        Depth
    
    Pule Width
        Width control
    
    These controllers affect Osc-1 though 4 with they are selected for either
    square of pulse waveforms.

Mode:

    The Mono/Poly had 3 operating modes, plus a 'Hold' option that affects 
    each mode:

        Mono: All oscillators sound same key in unison
        Poly: Each oscillator is assigned independent key - 4 note poly.
        Share: Dynamic assignment:
            1 key - 4 oscillators = Mono mode
            2 key - 2 oscillators per key
            3/4   - 1 oscillator per key = Poly mode

    The Hold function operates in 3 different modes:

        Mono: First 4 keypresses are memorised, further notes are then chorded
            together monophonically.
        Poly:
            Notes are argeggiated in sequence, new note presses are appended
            to the chain. Arpeggiation is up, down or up/down.
        Share:
            First 4 notes are memorised and are then argeggiated in sequence,
            new note presses will transpose the arpeggiation. Stepping is up,
            down or up/down.

    There are several controllers that affect arpeggation:

        Arpeg - direction of stepping
        MG-2 - Frequency of steps from about 10 seconds down to 50 bps.
        Trigger - Multiple will trigger envelopes on each step.

Effects:

    There are three main effects, or perhaps rather modulations, that are
    controlled in this section. These are vibrato, crossmodulated frequency
    and oscillator synchronisation. The application of each mod is configured
    with the controllers and then all of them can be enabled/disabled with
    the 'Effects' button. This allows for big differences in sound to be 
    applied quickly and simply as a typical effect would be. Since these mods
    apply between oscillators it was envisaged they would be applied in Mono
    mode to further fatten the sound, and the Mono mode is actually enabled when
    the Effects key is selected (as per the original instrument). The Mode can
    be changed afterwards for Effects/Poly for example, and they work with the
    arpeggiation function.

    X-Mod: frequency crossmodulation between oscillators
    Freq: frequency modulation by MG-1 (vibrato) or Envlope (sweep)

    Mode:
        Syn: Oscillators are synchronised
        X-M: Oscillators are crossmodulated
        S-X: Oscillators are crossmodulated and synchronised

    SNG:
        Single mode: synth had a master oscillator (1) and three slaves (2/3/4)
    DBL:
        Double mode: synth had two master (1/3) and two slaves (2/4)

    The overall FX routing depends on the SNG/DBL mode and the selection of
    Effects enabled or not according to the table below. This table affects 
    the FX routing and the modulation wheels discussed in the LFO section above:

                     --------------------------------------------------
                     |    FX OFF    |              FX ON              |
                     |              |----------------------------------
                     |              |    Single       |     Double    |
      ---------------+--------------+-----------------+---------------|
      | VCO-1/Slave  |    VCO-1     |    VCO 2/3/4    |     VCO 2/4   |
      |              |              |                 |               |
      | Pitch        |    VCO 1-4   |    VCO 1-4      |     VCO 1-4   |
      |              |              |                 |               |
      | VCF          |    VCF       |    VCF          |     VCF       |
      -----------------------------------------------------------------

    So, glad that is clear. Application of the modulation wheels to Pitch and
    VCF is invariable when they are selected. In contrast, VCO/Slave will have
    different destinations depending on the Effects, ie, when effects are on
    the modwheels will affect different 'slave' oscillators.


Oscillators:

    Each oscillator had the following controllers:

        Tune (*)
        Waveform: Triangle, ramp, pulse, square (**)
        Octave: Transpose 16' to 2'
        Level: output gain/mix of oscillators.

        * Osc-1 tuning is global
        ** width of pulse and square wave is governed by PW controller. The
        modulation of the pulse waveform is then also controlled by PWM.

Noise:

    Level: white noise output gain, mixed with oscillators into filter.

VCF:

    Freq:
        Cutoff frequency

    Res:
        Resonance/emphasis.
    
    Env:
        Amount of contour applied to cutoff
    
    KBD:
        Amount of key tracking applied.

ADSR: Two: filter/PWM/FX, amplifier

    Attack
    Decay
    Sustain
    Release

    Trigger:
        Single: Trigger once until last key release
        Multi: Trigger for each key or arpeggiator step.

    Damp:
        Off: Notes are held in Poly/Share mode until last key is released.
        On: Oscillators are released as keys are released.

This is more a synth to play with than describe. It never managed to be a true
blue synth perhaps largely due to its unusual design: the quasi-poly mode was
never widely accepted, and the effects routing is very strange. This does make
it a synth to be tweaked though.

Some of the mod routings do not conform to the original specification for the
different Slave modes. This is the first and probably the only bristol synth that
will have an inbuilt arpeggiator. The feature was possible here due to the mono
synth specification, and whilst it could be built into the MIDI library for
general use it is left up to the MIDI sequencers (that largely came along to 
replace the 1980s arpeggiators anyway) that are generally availlable on Linux.
[Other instruments emulated by bristol that also included arpeggiation but do
not have in the emulation were the Juno-6, Prophet-10, Oberheim OB-Xa, Poly6].

As of May 06 this synth was in its final stages of development. There are a few
issues with Tune and Detune that need to be fixed, and some of the poly key
assignment may be wrong.




    KORG POLY 6
    -----------

Korg in no way endorses this emulation of their classic synthesiser and have
their own emulation product that gives the features offered here. Korg,
Mono/Poly, Poly-6, MS-20, Vox and Continental are all registered names or
trademarks of Korg Inc of Japan.

Quite a few liberties were taken with this synth. There were extremely few  
differences between the original and the Roland Juno 6, they both had one osc  
with PWM and a suboscillator, one filter and envelope, a chorus effect, and  
inevitably both competed for the same market space for their given price. To  
differentiate this algorithm some alterations were made. There are two separate
envelopes rather than just one, but the option to have a gated amplifier is  
still there. In addition glide and noise were added, both of which were not in  
the original instrument. With respect to the original instrument this was  
perhaps not a wise move, but there seemed little point in making another Juno  
with a different layout. The net results is that the two synths do sound quite  
different. The emulation does not have an arpeggiator.  
 
    Volume: Master volume of the instrument  
 
    Glide: length of portamento  
 
    Tune: Master tuning of instrument  
 
    Bend: Amount of pitch wheel that is applied to the oscillators frequency.  
 
 
    VCO section:  

        Octave: What octave the instrument's keyboard is in.  
 
        Wave: Waveform selection: Triangle, Saw, Pulse and Pulsewidth  
 
        PW PWM: Amount of Pulsewidth (when Pulse is selected) and Pulsewidth
            Modulation (When Pulsewidth is selected).  
 
        Freq: Frequency of PW/PWM  
 
        OFF/SUB1/SUB2; Adds a square sub-oscillator either off, 1 or 2 octaves
            down from a note.  
 
    MG (Modulation Group):  
 
        Freq: Frequency of LFO  

        Delay: Amount of time before the LFO affects the destination when a key
            is pressed.  
        Level: How strongly the LFO affects the destination  
 
        VCO/VCF/VCA: Destinations that the LFO can go to:  
 
            VCO: The Voltage Controlled Oscillator:
                Affects oscillator pitch, producing vibrato  
 
            VCF: The Voltage Controlled Filter:
                Affects Filter, producing a wah effect  
 
            VCA: The Voltage Controlled Amplifier:
                Affects the Amplifier, producing tremolo  
 
    VCF section:  
 
        Freq: Cut off frequency of the filter  
 
        Res: Resonance of the filter  
 
        Env: By how much the filter is affected by the envelope.  
 
        Kbd: How much Keyboard tracking is applied to the envelope. note:

            A low setting doesn't allow the filter to open, making the notes
            seem darker the further you go up the keyboard.  
 
    Hold: prevent key off events  
 
    Mono Mode: Gang all voices to a single 'fat' monophonic synthesiser.  
 
    Poly: One voice per note.  
 
    Envelope Section:  
 
        Top:  
 
        Filter envelope: 
 
            Attack: Amount of time it takes the filter to fully open.
                A high value can produce a 'sweeping filter' effect. 
            Decay: Amount of time it takes for the filter to close to
                the sustain level 
            Sustain: Amount of filter that is sustained when a key is held 

            Release: Amount of time it takes for the filter envelope to stop
                affecting the filter. Combining a low filter release with a
                high amplitude release time can cause an interesting effect. 
 
        Bottom:  
 
        Amplitude envelope:  
 
        Attack: Amount of time it takes for the signal to reach its peak. 

        Decay: Amount of time it takes for the signal to drop to the
            sustain level 
        Sustain: How quickly the sound decays to silence. 

        Release: How long it takes the sound to decay to silence after
            releasing a key. 
 
    VCA:  

        Env: When on, this causes the Amplitude envelope to affect the sound.
            I.E, If you have a long attack time, you get a long attack time. 
        Gate: When on, this causes the Amplitude envelope only (not the filter
            envelope) to be be bypassed.  
        Gain: Gain of signal. 
 
    Effects Section:  

        0: No effects  
        1: Soft Chorus  
        2: Phaser  
        3: Ensemble  
 
        Intensity: How much the effects affect the output. 

There are some mildly anomolous effects possible from the MG section, especially
with the VCA. The MG and the env are summed into the VCA which means if the env
decays to zero then the LFO may end up pumping the volume, something that may
be unexpected. Similarly, if the LFO is pumping and the voice finally stops its
cycle then the closing gate may cause a pop on the MG signal. These can be 
resolved however the current behavious is probably close to the original.

Bristol thanks Andrew Coughlan for patches, bug reports, this manual page and
diverse suggestions to help improve the application.

Korg in no way endorses this emulation of their classic synthesiser and have  
their own emulation product that gives the features offered here. Korg,  
Mono/Poly, Poly-6, MS-20, Vox and Continental are all registered names or  
trademarks of Korg Inc of Japan.




    ARP AXXE
    --------

TBD

At the risk of getting flamed, this is potentially the ugliest synth ever made,
although the competition is strong. It was implemented as a build up to the far
more useful ARP 2600 to understand the ARP components and their implementation.

The implementation is a giveaway written during a week long business trip to 
Athens to keep me busy in the hotel. Its design lead on to the Odyssey and that
was the step towards the final big brother, the ARP 2600.




    ARP ODYSSEY
    -----------

Ring modulation is correct here, it is a multiplier. This deviates from the
original instrument that used an XOR function on the pulsewave outputs of the
two oscillators. The implementation has two models, Mark-I and Mark-II. These
implement different filters as per the original. Although their characteristics
are different it is not suggested they are a particularly close emulation of
the original.

TBD




    Memory Moog
    -----------

TBD.

This is actually a lot warmer than the Mini emulator, largely due to being
later code. The mini should be revisited but I am saving that pleasure for when
some more filters are available. [This was done during the 0.20 stream using the
Houvilainen filters and bandwidth limited oscillators to produce a far richer
sound. Also incorporate a number of fixes to the emulation stages.].




    ARP 2600
    --------

This synth will probably never get a writeup, it is kind of beyond the scope of
this document. There are some discrepancies with the original:

The filters do not self oscillate, they require an input signal. The output
stage is global to all voices so cannot be patched back into the signal path.
The original did not have a chorus, only a spring reverb. The input stage has
not been tested for either signal nor envelope following code. The voltage
manipulators were not in the first bristol upload with this emulation (-60), 
but a future release will include mixing inverters, a lag processor, and
possibly also a Hz->V extractor. The unit has an extra LFO where the original
had just a clock trigger circuit, it produces a TRI wave, can be used to
trigger the AR envelope and be used for modulation. The electroswitch is
unidirectional, two inputs switchable to one output. The sample and hold 
circuit cannot accept an external clock. The Keyboard inputs to the VCO cannot
accept and alternative signal other than the MIDI note with tracking of this 
note either enabled or disabled.

The rest works, ie, all the VCO/VCF/VCA/ENV/AMP and any of the 30 or so outputs
can be repatched into any of the 50 or so inputs. Patches cause no overhead in
the engine as it uses default buffering when not repatched, so feel free to put
in as many cables as you can fit. Patches in the GUI still demand a lot of CPU
cycles. Release -77 improved this about 5-fold and further improvements are in
the pipeline: the 0.10 stream implemented color caching and XImage graphics
interface which massively improved GUI performance.




    REALISTIC MG-1 CONCERTMATE
    --------------------------

This is a pimpy little synth. It was sold through the Realistic electronics 
chain, also known as Radio Shack (and as Tandy, in the UK at least). It was
relatively cheap but had a design from Moog Music (from after Robert Moog
had left?) including the patented ladder filter. It consisted of a monophonic
synth, dual oscillator, lfo, noise, filter, env, and a ring modulator. On top
of that there was an organ circuit to give 'polyphony'. It was not really
polyphonic although different descriptions will tell you it had 10 voices. 
These write-ups are by people who probably only had 10 fingers, the truth is
that the organ circuit was as per any other - it had a master oscillator at
about 2MHz and this was divided using binary counters to deliver a frequency
for every note. The output of the 'poly' section was lamentable at best, it is
a fairly pure square wave passed through the filter and contour. This is fully
emulated although in addition to the contour bristol implements a per note
envelope just to groom the note - this prevents ticks when new keys are pressed
with the mono envelope fully open. There is no access to this env, it just has
fast attack and decay times to smooth the signal and is preconfigured by the
user interface on startup.

The mono section is reasonably fun, the oscillators can be synchronised and
there is a ring modulator so the range of sounds is quite wide. The emulator
uses a chaimberlain filter so is not as warm as the Moog ladder filters.

The list of people who used this is really quite amazing. The promotion for
the product had Elton John holding one up in the air, although seeing as he
probably already had every other synth known to man, holding it up in the
air is likely to be all he ever did with it. Who knows how much they had to
pay him to do it - the photo was nice though, from the days when Elton was
still bald and wearing ridiculously oversized specs.

Tuning:

    One control each for the poly oscillator and mono oscillators

Glide:

    Only affects the monophonic oscillators.

Modulation:

    One LFO with rate and waveshape selection
        produces tri, square and S/H signals.
        can trigger the envelope
    One noise source.
    The modulation can be directed to:
        Oscillators for vibrato
        Filter for wah-wah effects

Oscillator-1:

    Tri or square wave
    Octave from -2 to 0 transposition
    Sync selector (synchronises Osc-2 to Osc-1)

Oscillator-2:

    Tri or pulse wave
    Detune. This interoperates with the sync setting to alter harmonics
    Octave from -1 to +1 transposition

Contour: This is not an ADSR, rather an AR envelope

    Sustain: AR or ASR envelope selector.
    Tracking: controls mono oscillators
        Envelope control
        Key tracking (gate, no env)
        Continuous (always on)
    Rise (attack time)
    Fall (release time)

Filter:

    Cutoff frequency
    Emphasis
    Contour depth
    Keyboard tracking off, 1/2, full.

Mixer: Levels for
    Mono Osc-1
    Mono Osc-2
    Noise
    RingMod of the mono oscillators (called 'bell').
    Poly Osc level.

Master Volume control.

One extra button was added to save the current settings. For the rest the 
controls reflect the simplicity of the original. The implementation is a single
synth, however due to the engine architecture having a pre-operational routine,
a post-operational routine and an operate(polyphonic emulator) the emulation
executes the mono synth in the pre- and post- ops to be mono, these are called
just once per cycle. The poly synth is executed in the operate() code so is 
polyphonic. This leads to one minor deviation from the original routing in
that if you select continuous tone controls then you will also hear one note
from the poly section. This is a minor issue as the poly oscillator can be
zeroed out in the mixer.

It is noted here that this emulation is just a freebie, the interface is kept
simple with no midi channel selection (start it with the -channel option and
it stays there) and no real memories (start it with the -load option and it
will stay on that memory location). There is an extra button on the front
panel (a mod?) and pressing it will save the current settings for next time
it is started. I could have done more, and will if people are interested, but
I built it since the current developments were a granular synth and it was
hard work getting my head around the grain/wave manipulations, so to give 
myself a rest I put this together one weekend. The Rhodesbass and ARP AXXE
were done for similar reasons. I considered adding another mod button, to make
the mono section also truly polyphonic but that kind of detracts from the
original. Perhaps I should put together a Polymoog sometime that did kind of
work like that anyway.

This was perhaps a strange choice, however I like the way it highlights the
difference between monophonic, polyphonic and 'neopolyphonic' synthesised
organs (such as the polymoog). Its a fun synth as well, few people are likely
to every bother buying one as they cost more now than when they were produced
due to being collectable: for the few hundred dollars they would set you back
on eBay you can get a respectable polyphonic unit.
So here is an emulator, for free, for those who want to see how they worked.




    Vox Continental 300
    -------------------

There is an additional emulator for the later Mark-II, Super, 300 or whatever
model you want to call it. This is probably closest to the 300. It was a 
dual manual organ, the lower manual is a Continental and the upper manual had
a different drawbar configuration, using 8', 4' and 2', another two compound
drawbars that represented 5-1/3'+1-3/5', and 2-2/3'+2'+1' respectively. This
gave upper manual a wider tonic range, plus it had the ability to apply some
percussive controls to two of the drawbars. Now, depending on model, some of 
these values could be different and bristol does not emulate all the different
combinations, it uses the harmonics described above and applies percussive to
the 2' and 5-1/3' harmonics (which is arguably incorrect however it gives
a wider combination of percussive harmonics).

The percussive has 4 controls, these are selectors for the harmonics that will
be driven through the percussive decay (and then no longer respond to the 
drawbars), a decay rate called 'L' which acts as a Longer decay when selected,
and a volume selector called 'S' which stands for Soft. The variables are 
adjustable in the mods section. The mods panel is intended to be hidden as
they are just variable parameters. On the original units these were PCB mounted
pots that were not generally accessible either. The panel is visible when you
turn the power control off, not that I suppress the keyboard or anything when
the power is off, but it gave me something useful do to with this button. The
transparency layer is fixed here and is used to apply some drop shadow and a
few beer spills on the cover.

There is an additional Bass section for those who bought the optional Bass
pedals (my old one had them). The emulation allow the selection of Flute and
Reed strengths, and to select 8' or 8'/16' harmonics. The 'Sustain' control
does not currently operate (0.10.9) but that can be fixed if people request
the feature.

The lower manual responds to the MIDI channel on which the emulation was 
started. The upper manual responds to notes greater than MIDI key 48 on the
next channel up. The Bass section also responds to this second channel on keys
lower than #48. Once started you cannot change the midi channel - use the 
'-channel' option at startup to select the one you want. The actual available
max is 15 and that is enforced.

The emulation only contains 6 available presets and a 'Save' button that you 
need to double click to overwrite any preset. The emulation actually uses 
banks, so if you started it with '-load 23' it would start up by selecting
bank 20, and load memory #3 from that bank. Any saved memories are also then
written back to bank 20, still with just 6 memories accessible 20-25. You can
access more via MIDI bank select and program change operations if suitably
linked up.

Vox is a trade name owned by Korg Inc. of Japan, and Continental is one of 
their registered trademarks. Bristol does not intend to infringe upon these
registered names and Korg have their own remarkable range of vintage emulations
available. You are directed to their website for further information of true
Korg products.




    Roland Jupiter 8
    ----------------

This emulator is anticipated in 0.20.4.

The Jupiter synthesizers were the bigger brothers of the Juno series: their
capabilities, sounds and prices were all considerably larger. This is the
larger of the series, the others being the -4 and -6. The -6 and the rack
mounted MKS-80 both came out after the Jupiter-8 and had somewhat more flexible
features. Several of these have been incorporated into the emulation and that
is documented below.

A quick rundown of the synth and emulation:

The synth runs as two layers, each of which is an independent emulator running
the same algorithm, both layers are controlled from the single GUI. The layers
are started with a set of voices, effectively 4+4 per default however bristol
plays with those numbers to give the split/layer at 4 voices each and the 'All'
configuration with 8 voices - it juggles them around depending on the Poly 
mode you select. You can request a different number of voices and the emulator 
will effectively allocate half the number to each layer. If you request 32
voices you will end up with 4+4 though since 32 is interpreted as the default.

The Poly section is used to select between Dual layers, Split keyboard and the
8voice 'All' mode. You can redefine the split point with a double click on the
'Split' button and then pressing a key. If you have linked the GUI up to the
MIDI you should be able to press a key on your master keyboard rather than on
the GUI.

After that you can select the layer as upper or lower to review the parameter 
settings of each layer: they are as follows:

LFO:
    Frequency
    Delay
    Waveform (tri, saw, square, s&h)

VCO Mods:
    LFO and ENV-1
    Destination to modulate frequency of DCO1, DCO2 or both.

PWM:
    PW
    PWM
    Modified by Env-1 or LFO

DCO-1:
    Crossmod (FM) from DCO2 to DCO1
    Modified by Env-1

    Octave range 16' to 2' (all mixable)
    Waveform: Tri, saw, pulse, square (all mixable)

DCO-2:
    Sync (1->2 or 2->1 or off)
    Range: 32' to 2' (all mixable)
    Tuning
    Waveform: tri, saw, pulse, noise (all mixable)

Mix:
    Osc 1 and Osc 2

HPF:
    High pass filter of signal

Filter:
    Cutoff
    Emphasis
    LPF/BPF/HPF
    Env modulation
    Source from Env-1 or Env-2
    LFO mod amount
    Keyboard tracking amount

VCA:
    Env-2 modulation
    LFO modulation

ADSR-1:
    A/D/S/R
    Keyboard tracking amount
    Invert

ADSR-2:
    A/D/S/R
    Keyboard tracking amount

Pan:
    Stereo panning of layer

All of the above 40 or so parameters are part of the layer emulation and are
separately configurable.

The keyboard can operate in several different modes which are selectable from
the Poly and Keyboard mode sections. The first main one is Dual, Split and All.
Dual configure the two synth layers to be placed on top of each other. Split
configures them to be next to each other and by double clicking the split
button you can then enter a new split point by pressing a key. The All setting
gives you a single layer with all 8 voices active. These settings are for the
whole synth.

The Poly section provides different playing modes for each layer independently.
There are 3 settings: Solo give the layer access to a single voice for playing
lead solos. Unison give the layer however many voices it is allowed (8 if in 
the All mode, 4 otherwise) and stacks them all on top of each other. This is
similar to Solo but with multiple voices layered onto each other. Unison is 
good for fat lead sounds, Solo better for mono bass lines where Unison might
have produced unwanted low frequency signal phasing. The third option is Poly
mode 1 where the synth allocates a single voice to each key you press. The
original also had Poly mode 2 which was not available at the first bristol
release - this mode would apply as many notes as available to the keys you
pressed: 1 key = 8 voices as in Unison, with 2 keys pressed each would get 4
voices each, 4 keys pressed would get 2 voices and mixtures in there for other
key combinations. This may be implemented in a future release but it is a
rather left field option and would have to be put into the MIDI library that
controls the voice assignments.

The arpeggiator integrated into bristol is a general design and will differ
from the original. The default settings are 4 buttons controlling the range
of the arpeggiation, from 1 to 4 octaves, 4 buttons controlling the mode as
Up, Down, Up+Down or Random sequencing of the notes available, and 4 notes
that are preloaded into the sequence.

Finally there are two global controls that are outside of the memories - the
rate and clock source (however externally driven MTC has not been implemented
yet). It is noted that the Arpeggiator settings are separate from the sequence
information, ie, Up/Down/Rnd, the range and the arpeggiator clock are not the
same as the note memory, this is discussed further in the memory setting
section below.

It is possible to redefine the arpeggiator sequence: select the function 
button on the right hand side, then select any of the arpeggiator mode buttons,
this will initiate the recording. It does not matter which of the mode is
selected since they will all start the recording sequence. When you have
finished then select the mode button again (you may want to clear the function
key if still active). You can record up to 256 steps, either from the GUI
keyboard or from a master controller and the notes are saved into a midi
key memory.

There is no capability to edit the sequences once they have been entered, that
level of control is left up to separate MIDI sequencers for which there are
many available on Linux. Also, the note memory is actually volatile and will
be lost when the emulation exits. If you want to save the settings then you
have to enter them from the GUI keyboard or make sure that the GUI is linked
up to the master keyboard MIDI interface - you need to be able to see the
GUI keyboard following the notes pressed on the master keyboard since only
when the GUI sees the notes can it store them for later recall. If the GUI
did view the sequence entered here then it will be saved with the patch in
a separate file (so that it can be used with other bristol synths).

In addition to the Arpeggiator there is the 'Chord' control. The original
synth had two green panel buttons labelled 'Hold', they were actually similar
to the sustain pedal on a MIDI keyboard or piano, with one for each layer of
the synth. They have been redefined here as Chord memory. When activated the
layer will play a chord of notes for every key press. The notes are taken from
separate chord memory in the Arpeggiator sequencer. The result is very similar
to the Unison mode where every voice is activated for a single key, the
difference here is that every voice may be playing a different note to give
phat chords. To configure a chord you enable the function key and the target
Hold button to put the synth into chord learning mode, play a set of notes
(you don't have to hold them down), and click again to finalise the chord.
If there are more chord notes than voices available then all voices will
activate. If there are more voices than notes then you will be able to play
these chord polyphonically, for example, if you have 8 voices and entered
just 4 chorded notes then you will have 2 note polyphony left. You should
also be able to play arpeggiations of chords. The maximum number of notes
in a chord is 8.

The synth has a modifier panel which functions as performance options which 
can be applied selectively to different layers:

    Bender:
        This is the depth of the settings and is mapped by the engine to
        continuous controller 1 - the 'Mod' wheel. The emulation also tracks
        the pitch wheel separately.

    Bend to VCO
        This applies an amount of pitch bend from the Mod wheel selectively
        to either VCO-1 and/or VCO-2. These settings only affect the Mod
        layers selected from the main panel. Subtle modifications applied in
        different amounts to each oscillator can widen the sound considerable
        by introducing small amounts of oscillator phasing.
    
    Bend to VCF
        Affects the depth of cutoff to the filter with on/off available. Again
        only applies to layers activate with the Mod setting.

    LFO to VCO:
        The mod panel has a second global LFO producing a sine wave. This can
        be driven in selectable amounts to both VCO simultaneously. Layers are
        selected from the Mod buttons.

    LFO to VCF:
        The LFO can be driven in selectable amounts to both VCO or to the VCF.
        Layers are selected from the Mod buttons.

    Delay:
        This is the rise time of the LFO from the first note pressed. There is
        no apparent frequency control of the second LFO however bristol allows
        the frequency and gain of the LFO to track velocity using function B4
        (see below for function settings). Since there is only one LFO per
        emulation then the velocity tracking can be misleading as it only 
        tracks from a single voice and may not track the last note. For this
        reason it can be disabled. Using a tracking from something like channel
        pressure is for future study.

    Glide:
        Glissando between key frequencies, selectable to be either just to the
        upper layer, off, or to both layers.

    Transpose:
        There are two transpose switches for the lower and upper layers 
        respectively. The range is +/1 one octave.

Modifier panel settings are saved in the synth memories and are loaded with
the first memory (ie, with dual loaded memories discussed below). The ability
to save these settings in memory is an MKS-80 feature that was not available
in either the Jupiter-6 or -8.

There are several parts to the synth memories. Layer parameters govern sound
generation, synth parameters that govern operating modes such Dual/Split,
Solo/Unison etc, Function settings that modify internal operations, the
parameters for the mod panel and finally the Arpeggiator sequences. These
sequences are actually separate from the arpeggiator settings however that
was covered in the notes above.

When a patch is loaded then only the layer parameters are activated so that the
new sound can be played, and the settings are only for the selected layer. This
means any chord or arpeggiation can be tried with the new voice active.

When a memory is 'dual loaded' by a double click on the Load button then all
the memory settings will be read and activated: the current layer settings,
synth settings, operational parameters including the peer layer parameters
for dual/split configurations. Dual loading of the second layer will only 
happen if the memory was saved as a split/double with a peer layer active.

The emulation adds several recognised Jupiter-6 capabilities that were not a
part of the Jupiter-8 product. These are

    1.  PW setting as well as PWM
    2.  Cross modulation can be amplified with envelope-1 for FM type sounds
    3.  Sync can be set in either direction (DCO1 to 2 or DCO2 to 1)
    4.  The waveforms for DCO 1&2 are not exclusive but mixable
    5.  The LFO to VCA is a continuous controller rather than stepped
    6.  The envelope keyboard tracking is continuous rather than on/off
    7.  The filter option is multimode LP/BP/HP rather than 12/24dB
    8.  Layer detune is configurable
    9.  Layer transpose switches are available
    10. Arpeggiator is configurable on both layers

Beyond these recognised mods it is also possible to select any/all DCO
transpositions which further fattens up the sound as it allows for more
harmonics. There is some added detune between the waveforms with its depth
being a function of the -detune setting. Separate Pan and Balance controls
have been implemented, Pan is the stereo positioning and is configurable per
layer. Balance is the relative gain between each of the layers.

There are several options that can be configured from the 'f' button
in the MIDI section. When you push the f(n) button then the patch and bank
buttons will not select memory locations but display the on/off status of 16
algorithmic changes to the emulation. Values are saved in the synth memories.
These are bristol specific modifications and may change between releases unless
otherwise documented.

F(n):

    f(p1): Env-1 retriggers 
    f(p2): Env-1 conditionals
    f(p3): Env-1 attack sensitivity
    f(p4): Env-2 retriggers
    f(p5): Env-2 conditionals
    f(p6): Env-2 attack sensitivity
    f(p7): Noise per voice/layer
    f(p8): Noise white/pink

    f(b1): LFO-1 per voice/layer
    f(b2): LFO-1 Sync to Note ON
    f(b3): LFO-1 velocity tracking
    f(b4): Arpeggiator retrigger
    f(b5): LFO-2 velocity tracking
    f(b6): NRP enable
    f(b7): Debug MIDI
    f(b8): Debug GUI controllers

The same function key also enables the learning function of the arpeggiator 
and chord memory, as explained above. When using the arpeggiator you may want
to test with f(b4) enabled, it will give better control of the filter envelope.


Other differences to the original are the Hold keys on the front panel. These
acted as sustain pedals however for the emulation that does not function very
well. With the original the buttons were readily available whilst playing and
could be used manually, something that does not work well with a GUI and a
mouse. For this reason they were re-used for 'Unison Chording' discussed above.
Implementing them as sustain pedals would have been an easier if less flexible
option and users are advised that the bristol MIDI library does recognise the
sustain controller as the logical alternative here.

Another difference would be the quality of the sound. The emulation is a lot
cleaner and not as phat as the original. You might say it sounds more like
something that comes from Uranus rather than Jupiter and consideration was
indeed given to a tongue in cheek renaming of the emulation..... The author is
allowed this criticism as he wrote the application - as ever, if you want the
original sound then buy the original synth (or get Rolands own emulation?).

A few notes are required on oscillator sync since by default it will seem to 
be quite noisy. The original could only product a single waveform at a single
frequency at any one time. Several emulators, including this one, use a bitone
oscillator which generates complex waveforms. The Bristol Bitone can generate
up to 4 waveforms simultaneously at different levels for 5 different harmonics
and the consequent output is very rich, the waves can be slightly detuned, 
the pulse output can be PW modulated. As with all the bristol oscillators that
support sync, the sync pulse is extracted as a postive leading zero crossing.
Unfortunately if the complex bitone output is used as input to sync another
oscillator then the result is far too many zero crossings to extract a good
sync. For the time being you will have to simplify the sync source to get a
good synchronised output which itself may be complex wave. A future release
will add a sync signal from the bitone which will be a single harmonic at the
base frequency and allow both syncing and synchronised waveform outputs to be
arbitrary.




    CRUMAR BIT-1, BIT-99, BIT-100
    -----------------------------

I was considering the implementation of the Korg-800, a synth I used to borrow
without having due respect for it - it was a late low cost analogue having 
just one filter for all the notes and using the mildly annoying data entry
buttons and parameter selectors. Having only one filter meant that with key
tracking enabled the filter would open up as different keys were pressed,
wildly changing the sound of active notes. Anyway, whilst considering how to
implement the entry keys (and having features like the mouse tracking the
selectors of the parameter graphics) I was reminded of this synthesizer. It
was developed by Crumar but released under the name 'Bit' as the Crumar name
was associated with cheesy roadrunner electric pianos at the time (also
emulated by Bristol). This came out at the same time as the DX-7 but for
half the price, and having the split and layer facilities won it several awards
that would otherwise have gone to the incredibly innovative DX. However with
the different Crumar models being released as the digital era began they kind
of fell between the crack. It has some very nice mod features though, it was
fun to emulate and hopefully enjoyable to play with.

As a side note, the Korg Poly-800 is now also emulated by bristol

A quick rundown of the Bit features are below. The different emulated models
have slightly different default parameter values and/or no access to change
them:

    Two DCO with mixed waveforms.
    VCF with envelope
    VCA with envelope
    Two LFO able to mod all other components, controlled by the wheel and key
    velocity, single waveform, one had ramp and the other sawtooth options.

The envelopes were touch sensitive for gain but also for attack giving plenty 
of expressive capabilities. The bristol envelope, when configured for velocity
sensitive parameters (other than just gain) will also affect the attack rate.

The front panel had a graphic that displayed all the available parameters and
to change then you had to select the "Address" entry point then use the up/down
entry buttons to change its value. Bristol uses this with the addition that the
mouse can click on a parameter to start entering modifications to it.

The emulation includes the 'Compare' and 'Park' features although they are a
little annoying without a control surface. They work like this (not quite the
same as the original): When you select a parameter and change it's value then
the changes are not actually made to the active program, they just change the
current sounds. The Compare button can be used to flip between the last loaded
value and the current modified one to allow you to see if you prefer the sound
before or after the modification.

If you do prefer the changes then to keep them you must "Park" them into the
running program before saving the memory. At the time of writing the emulation
emulated the double click to park&write a memory, however it also has an actual
Save button since 'Save to Tape' is not a feature here. You can use park and
compare over dual loaded voices: unlike the original, which could only support
editing of sounds when not in split/double, this emulation allows you to edit
both layers by selecting the upper/lower entry buttons and then using the
sensitive panel controls to select the addressed parameters. This is not the
default behaviour, you first have to edit address 102 and increment it. Then,
each layer can be simultaneously edited and just needs to be parked and saved
separately. The Park/Compare cache can be disabled by editing parameter DE 101,
changes are then made to the synth memory and not the cache.

The memories are organised differently to the original. It had 99 memories, and
the ones from 75 and above were for Split and Layered memories. Bristol can 
have all memories as Split or Layer. When you save a memory it is written to
memory with a 'peer' program locator. When you load it with a single push on 
the Load button it returns to the active program, but if you double click then
its 'peer' program is loaded into the other layer: press Load once to get the
first program entered, then press it again - the Split/Layer will be set to
the value from the first program and the second layer will be loaded. This 
naturally requires that the first memory was saved with Split/Layer enabled.
It is advised (however not required) that this dual loading is done from the
lower layer. This sequence will allow the lower layer to configure certain
global options that the upper layer will not overwrite, for example the layer
volumes will be select from the lower layer when the upper layer is dual 
loaded.

For MIDI program change then since this quirky interface cannot be emulated
then the memories above 75 will be dual loaded, the rest will be single loaded.

Bristol will also emulate a bit-99 and a Bit-99m2 that includes some parameter
on the front panel that were not available on the original. The engine uses the
exact same algorithm for all emulations but the GUI presents different sets of
parameters on the front panel. Those that are not displayed can only be accessed
from the data entry buttons. The -99m2 put in a few extra features (ie, took a
few liberties) that were not in the original:

    DCO adds PWM from the LFO, not in the original
    DCO-2 adds Sync to DCO-1, also not in the original
    DCO-2 adds FM from DCO-1
    DCO add PWM from Envelope
    Glide has been added
    DCO harmonics are not necessarily exclusive
    Various envelope option for LFO
    S&H LFO modulation

The reason these were added was that bristol could already do them so were
quite easy to incorporate, and at least gave two slightly different emulations.

The oscillators can work slightly differently as well. Since this is a purely
digital emulations then the filters are a bit weak. This is slightly compensated
by the ability to configure more complex DCO. The transpose selectors (32', 16',
8' and 2') were exclusive in the original. That is true here also, however if
controllers 84 and 85 are set to zero then they can all work together to fatten
out the sound. Also, the controllers look like boolean however that is only the
case if the data entry buttons are used, if you change the value with the data
entry pot then they act more like continuous drawbars, a nice effect however 
the display will not show the actual value as the display remains boolean, you
have to use your ear. The square wave is exclusive and will typically only 
function on the most recently selected (ie, typically highest) harmonic.

The same continuous control is also available on the waveform selectors. You
can mix the waveform as per the original however the apparent boolean selectors
are again continuous from 0.0 to 1.0. The net result is that the oscillators 
are quite Voxy having the ability to mix various harmonic levels of different
mixable waveforms.

The stereo mode should be correctly implemented too. The synth was not really
stereo, it had two outputs - one for each layer. As bristol is stereo then each
layer is allocated to the left or right channel. In dual or split they came
out separate feeds if Stereo was selected. This has the rather strange side
effect of single mode with 6 voices. If stereo is not selected then you have
a mono synth. If stereo is selected then voices will exit from a totally 
arbitrary output depending on which voices is allocated to each note.
In contrast to the original the Stereo parameter is saved with the memory and
if you dual load a split/layer it is taken from the first loaded memory only.
The implementation actually uses two different stereo mixes selectable with the
Stereo button: Mono is a centre pan of the signal and Stereo pans hardleft and
hardright respectively. These mixes can be changed with parameters 110 to 117
using extended data entry documented below.

The default emulation takes 6 voices for unison and applies 3+3 for the split
and double modes. You can request more, for example if you used '-voices 16'
at startup then you would be given 8+8. As a slight anomaly you cant request 32
voices - this is currently interpreted as the default and gives you 3+3.

The bit-1 did not have the Stereo control - the controller presented is the
Unison button. You can configure stereo from the extended data entry ID 110 and
111 which give the LR channel panning for 'Mono' setting, it should default to
hard left and right panning. Similarly the -99 emulations do not have a Unison
button, the capability is available from DE 80.

The memories for the bit-1 and bit-99 should be interchangeable however the
code maintains separate directories.

There are three slightly different Bit GUI's. The first is the bit-1 with a 
limited parameter set as it only had 64 parameters. The second is the bit-99
that included midi and split options in the GUI and has the white design that
was an offered by Crumar. The third is a slightly homogenous design that is 
specific to bristol, similar to the black panelled bit99 but with a couple of
extra parameters. All the emulations have the same parameters, some require you
use the data entry controls to access them. This is the same as the original, 
there were diverse parameters that were not in memories that needed to be
entered manually every time you wanted the feature. The Bristol Bit-99m2 has
about all of the parameters selectable from the front panel however all of the
emulations use the same memories so it is not required to configure them at
startup (ie, they are saved). The emulation recognises the following parameters:

    Data Entry  1 LFO-1 triangle wave selector (exclusive switch)
    Data Entry  2 LFO-1 ramp wave selector (exclusive switch)
    Data Entry  3 LFO-1 square wave selector (exclusive switch)
    Data Entry  4 LFO-1 route to DCO-1
    Data Entry  5 LFO-1 route to DCO-2
    Data Entry  6 LFO-1 route to VCF
    Data Entry  7 LFO-1 route to VCA
    Data Entry  8 LFO-1 delay
    Data Entry  9 LFO-1 frequency
    Data Entry 10 LFO-1 velocity to frequency sensitivity
    Data Entry 11 LFO-1 gain
    Data Entry 12 LFO-1 wheel to gain sensitivity

    Data Entry 13 VCF envelope Attack
    Data Entry 14 VCF envelope Decay
    Data Entry 15 VCF envelope Sustain
    Data Entry 16 VCF envelope Release
    Data Entry 17 VCF velocity to attack sensitivity (and decay/release) 
    Data Entry 18 VCF keyboard tracking
    Data Entry 19 VCF cutoff
    Data Entry 20 VCF resonance
    Data Entry 21 VCF envelope amount
    Data Entry 22 VCF velocity to gain sensitivity
    Data Entry 23 VCF envelope invert

    Data Entry 24 DCO-1 32' harmonic
    Data Entry 25 DCO-1 16' harmonic
    Data Entry 26 DCO-1 8' harmonic
    Data Entry 27 DCO-1 4' harmonic
    Data Entry 28 DCO-1 Triangle wave
    Data Entry 29 DCO-1 Ramp wave
    Data Entry 30 DCO-1 Pulse wave
    Data Entry 31 DCO-1 Frequency 24 semitones
    Data Entry 32 DCO-1 Pulse width
    Data Entry 33 DCO-1 Velocity PWM
    Data Entry 34 DCO-1 Noise level

    Data Entry 35 DCO-2 32' harmonic
    Data Entry 36 DCO-2 16' harmonic
    Data Entry 37 DCO-2 8' harmonic
    Data Entry 38 DCO-2 4' harmonic
    Data Entry 39 DCO-2 Triangle wave
    Data Entry 40 DCO-2 Ramp wave
    Data Entry 41 DCO-2 Pulse wave
    Data Entry 42 DCO-2 Frequency 24 semitones
    Data Entry 43 DCO-2 Pulse width
    Data Entry 44 DCO-2 Env to pulse width
    Data Entry 45 DCO-2 Detune

    Data Entry 46 VCA velocity to attack sensitivity (and decay/release) 
    Data Entry 47 VCA velocity to gain sensitivity
    Data Entry 48 VCA overall gain ADSR
    Data Entry 49 VCA Attack
    Data Entry 50 VCA Decay
    Data Entry 51 VCA Sustain
    Data Entry 52 VCA Release

    Data Entry 53 LFO-2 triangle wave selector (exclusive switch)
    Data Entry 54 LFO-2 saw wave selector (exclusive switch)
    Data Entry 55 LFO-2 square wave selector (exclusive switch)
    Data Entry 56 LFO-2 route to DCO-1
    Data Entry 57 LFO-2 route to DCO-2
    Data Entry 58 LFO-2 route to VCF
    Data Entry 59 LFO-2 route to VCA
    Data Entry 60 LFO-2 delay
    Data Entry 61 LFO-2 frequency
    Data Entry 62 LFO-2 velocity to frequency sensitivity
    Data Entry 63 LFO-2 gain
    Data Entry 12 LFO-2 wheel to gain sensitivity

    Data Entry 64 Split
    Data Entry 65 Upper layer transpose
    Data Entry 66 Lower Layer gain
    Data Entry 67 Upper Layer gain

The following were visible in the Bit-99 graphics only:

    Data Entry 68 MIDI Mod wheel depth
    Data Entry 69 MIDI Velocity curve (0 = soft, 10=linear, 25 = hard)
    Data Entry 70 MIDI Enable Debug
    Data Entry 71 MIDI Enable Program Change
    Data Entry 72 MIDI Enable OMNI Mode
    Data Entry 73 MIDI Receive channel

    Data Entry 74 MIDI Mem Search Upwards
    Data Entry 75 MIDI Mem Search Downwards
    Data Entry 76 MIDI Panic (all notes off)

Most of the MIDI options are not as per the original. This is because they are
implemented in the bristol MIDI library and not the emulation.

The following were added which were not really part of the Bit specifications
so are only visible on the front panel of the bit100. For the other emulations
they are accessible from the address entry buttons.

    Data Entry 77 DCO-1->DCO-2 FM
    Data Entry 78 DCO-2 Sync to DCO-1
    Data Entry 79 Keyboard glide
    Data Entry 80 Unison

    Data Entry 81 LFO-1 SH
    Data Entry 82 LFO-1 PWM routing for DCO-1
    Data Entry 83 LFO-1 PWM routing for DCO-2
    Data Entry 84 LFO-1 wheel tracking frequency
    Data Entry 85 LFO-1 velocity tracking gain
    Data Entry 86 LFO-1 per layer or per voice

    Data Entry 87 LFO-2 SH
    Data Entry 88 LFO-2 PWM routing for DCO-1
    Data Entry 89 LFO-2 PWM routing for DCO-2
    Data Entry 90 LFO-2 wheel tracking frequency
    Data Entry 91 LFO-2 velocity tracking gain
    Data Entry 92 LFO-2 per layer or per voice

    Data Entry 93 ENV-1 PWM routing for DCO-1
    Data Entry 94 ENV-1 PWM routing for DCO-2

    Data Entry 95 DCO-1 restricted harmonics
    Data Entry 96 DCO-2 restricted harmonics

    Data Entry 97 VCF Filter type
    Data Entry 98 DCO-1 Mix

    Data Entry 99 Noise per layer

    Data Entry 00 Extended data entry (above 99)
    
Extended data entry is for all parameters above number 99. Since the displays
only have 2 digits it is not totally straightforward to enter these values and
they are only available in Single mode, not dual or split - strangely similar
to the original specification for editing memories. These are only activated for
the lower layer loaded memory, not for dual loaded secondaries or upper layer
loaded memories. You can edit the upper layer voices but they will be saved with
their original extended parameters. This may seem correct however it is possible
to edit an upper layer voice, save it, and have it sound different when next
loaded since the extended parameters were taken from a different lower layer.
This is kind of intentional but if in doubt then only ever dual load voices from
the lower layer and edit them in single mode (not split or layer). Per default
the emulation, as per the original, will not allow voice editing in Split or
Layer modes however it can be enabled with DE 102.

All the Bit emulations recognise extended parameters. They are somewhat in a
disorganised list as they were built in as things developed. For the most part
they should not be needed.
The Bit-100 includes some in its silkscreen, for the others you can access them
as follows:

1. deselect split or double
2. select addr entry
3. use 0..9 buttons to enter address 00
4. increment value to '1'. Last display should show EE (Extended Entry)

5. select last two digits of desired address with 0-9 buttons
6. change value (preferably with pot).

7. when finished, select address 00 again (this is now actually 100) to exit

    Data Entry 100 Exit extended data entry
    Data Entry 101 enable WriteThru scratchpad (disables park and compare)
    Data Entry 102 enable layer edits on Split/Double memories.
    Data Entry 103 LFO-1 Sync to note on
    Data Entry 104 LFO-2 Sync to note on
    Data Entry 105 ENV-1 zero retrigger
    Data Entry 106 ENV-2 zero retrigger
    Data Entry 107 LFO-1 zero retrigger
    Data Entry 108 LFO-2 zero retrigger
    Data Entry 109 Debug enable (0 == none, 5 == persistent)

    Data Entry 110 Left channel Mono gain, Lower
    Data Entry 111 Right channel Mono gain, Lower
    Data Entry 112 Left channel Stereo gain, Lower
    Data Entry 113 Right channel Stereo gain, Lower
    Data Entry 114 Left channel Mono gain, Upper
    Data Entry 115 Right channel Mono gain, Upper
    Data Entry 116 Left channel Stereo gain, Upper
    Data Entry 117 Right channel Stereo gain, Upper
    Data Entry 118 Bit-100 flag
    Data Entry 119 Temperature sensitivity

    Data Entry 120 MIDI Channel tracking layer-2 (same/different channel)
    Data Entry 121 MIDI Split point tracking layer-2 (same/different split)
    Data Entry 122 MIDI Transpose tracking (layer-2 or both layers) N/A
    Data Entry 123 MIDI NRP enable

    Data Entry 130 Free Memory Search Up
    Data Entry 131 Free Memory Search Down
    Data Entry 132 ENV-1 Conditional
    Data Entry 133 ENV-2 Conditional
    Data Entry 134 LFO-1 ENV Conditional
    Data Entry 135 LFO-2 ENV Conditional
    Data Entry 136 Noise white/pink
    Data Entry 137 Noise pink filter (enable DE 136 Pink)
    Data Entry 138 Glide duration 0 to 30 seconds
    Data Entry 139 Emulation gain level

    Data Entry 140 DCO-1 Square wave gain
    Data Entry 141 DCO-1 Subharmonic level
    Data Entry 142 DCO-2 Square wave gain
    Data Entry 143 DCO-2 Subharmonic level

The 150 range will be incorporated when the Arpeggiator code is more stable,
currently in development for the Jupiter. This is anticipated in 0.20.4:

    Data Entry 150 Arpeggiator Start/Stop
    Data Entry 151 Arpeggiator mode D, U/D, U or Random
    Data Entry 152 Arpeggiator range 1, 2, 3, 4 octaves
    Data Entry 153 Arpeggiator rate
    Data Entry 154 Arpeggiator external clock
    Data Entry 155 Arpeggiator retrigger envelopes
    Data Entry 156 Arpeggiator poly-2 mode
    Data Entry 157 Chord Enable
    Data Entry 158 Chord Relearn
    Data Entry 159 Sequencer Start/Stop
    Data Entry 160 Sequencer mode D, U/D, U or Random
    Data Entry 161 Sequencer range 1, 2, 3, 4 octaves
    Data Entry 162 Sequencer rate
    Data Entry 163 Sequencer external clock
    Data Entry 164 Sequencer retrigger envelopes
    Data Entry 165 Sequencer Relearn

The following can be manually configured but are really for internal uses only
and will be overwritten when memories are saved to disk. The Split/Join flag,
for example, is used by dual loading memories to configure the peer layer to
load the memory in DE-198, and the stereo placeholder for configuring the stereo
status of any single loaded memory.

    Data Entry 193 Reserved: save bit-1 formatted memory
    Data Entry 194 Reserved: save bit-99 formatted memory
    Data Entry 195 Reserved: save bit-100 formatted memory
    Data Entry 196 Reserved: Split/Join flag - internal use
    Data Entry 197 Reserved: Stereo placeholder - internal use
    Data Entry 198 Reserved: Peer memory pointer - internal use
    Data Entry 199 Reserved: DCO-2 Wheel mod (masks entry 12) - internal use

The tuning control in the emulation is on the front panel rather than on the
rear panel as in the original. It had a keyboard sensitivity pot however that
is achieved separately with bristol using velocity curves from the MIDI control
settings. The front panel rotary defaults to 0% tuning and is not saved in the
memory. The front panel gain controls are also not saved in the memory and
default to 0.8 at startup.

The net emulation is pretty intensive as it runs with over 150 operational
parameters.

A few notes are required on oscillator sync since by default it may seem to 
be quite noisy. The original could only produce a single waveform at a single
frequency at any one time. Several emulators, including this one, use a bitone
oscillator which generates complex waveforms. The Bristol Bitone can generate
up to 4 waveforms simultaneously at different levels for 5 different harmonics
and the consequent output is very rich, the waves can be slightly detuned, 
the pulse output can be PW modulated. As with all the bristol oscillators that
support sync, the sync pulse is extracted as a postive leading zero crossing.
Unfortunately if the complex bitone output is used as input to sync another
oscillator then the result is far too many zero crossings to extract a good
sync.
Code has been implemented to generate a second sync source using a side output
sync wave which is then fed to a sideband sync input on the oscillator, the
results are far better




    Sequential Circuits Prophet Pro-One
    -----------------------------------

Sequential circuits released amongst the first truly polyphonic synthesisers
where a group of voice circuits (5 to 10 of them) were linked to an onboard
computer that gave the same parameters to each voice and drove the notes to
each voice from the keyboard. The costs were nothing short of exhorbitant and
this lead to Sequential releasing a model with just one voice board as a mono-
phonic equivalent. The sales ran up to 10,000 units, a measure of its success
and it continues to be recognised alongside the Mini Moog as a fat bass synth.

The design of the Prophet synthesisers follows that of the Mini Moog. It has
three oscillators one of them as a dedicated LFO. The second audio oscillator
can also function as a second LFO, and can cross modulate oscillator A for FM 
type effects. The audible oscillators have fixed waveforms with pulse width
modulation of the square wave. These are then mixed and sent to the filter with
two envelopes, for the filter and amplifier.

The Pro-1 had a nice bussing matrix where 3 different sources, LFO, Filter Env
and Oscillator-B could be mixed in varying amounts to two different modulation
busses and each bus could then be chosen as inputs to modulation destinations.
One bus was a direct bus from the mixed parameters, the second bus was under
the modwheel to give configurable expressive control.

LFO:

    Frequency: 0.1 to 50 Hz
    Shape: Ramp/Triangle/Square. All can be selected, none selected should
    give a sine wave

Modulations:

    Source:

        Filter Env amount to Direct or Wheel Mod busses
        Oscillator-B amount to Direct or Wheel Mod busses
        LFO to Direct amount or Wheel Mod busses

    Dest:

        Oscillator-A frequency from Direct or Wheel Mod busses
        Oscillator-A PWM from Direct or Wheel Mod busses
        Oscillator-B frequency from Direct or Wheel Mod busses
        Oscillator-B PWM from Direct or Wheel Mod busses
        Filter Cutoff from Direct or Wheel Mod busses

Osc-A:

    Tune: +/-7 semitones
    Freq: 16' to 2' in octave steps
    Shape: Ramp or Square
    Pulse Width: only when Square is active.
    Sync: synchronise to Osc-B

Osc-B:

    Tune: +/-7 semitones
    Freq: 16' to 2' in octave steps
    Fine: +/- 7 semitones
    Shape: Ramp/Triangle/Square
    Pulse Width: only when Square is active.
    LFO: Lowers frequency by 'several' octaves.
    KBD: enable/disable keyboard tracking.

Mixer:

    Gain for Osc-A, Osc-B, Noise

Filter:

    Cutoff: cuttof frequency
    Res: Resonance/Q/Emphasis
    Env: amount of modulation affecting to cutoff.
    KBD: amount of keyboard trackingn to cutoff

Envelopes: One each for PolyMod (filter) and amplifier.

    Attack
    Decay
    Sustain
    Release

Sequencer:

    On/Off
    Record Play
    Rate configured from LFO

Arpeggiator:

    Up/Off/UpDown
    Rate configured from LFO

Glide:

    Amount of portamento
    Auto/Normal - first key will/not glide.

Global:

    Master Tune
    Master Volume


Memories are loaded by selecting the 'Bank' button and typing in a two digit
bank number followed by load. Once the bank has been selected then 8 memories
from the bank can be loaded by pressing another memory select and pressing
load. The display will show free memories (FRE) or programmed (PRG).
There is an additional Up/Down which scan for the next program and a 'Find'
key which will scan up to the next unused memory location.

The original supported two sequences, Seq1 and Seq2, but these have not been
implemented. Instead the emulator will save a sequence with each memory location
which is a bit more flexible if not totally in the spirit of the original.

The Envelope amount for the filter is actually 'Mod Amount'. To get the filter
envelope to drive the filter it must be routed to the filter via a mod bus. This
may differ from the original.
Arpeggiator range is two octaves.
The Mode options may not be correctly implemented due to the differences in
the original being monophonic and the emulator being polyphonic. The Retrig is
actually 'rezero' since we have separate voices. Drone is a Sustain key that
emulates a sustain pedal.
Osc-B cannot modulate itself in polyphonic mode (well, it could, it's just that
it has not been coded that way).
The filter envelope is configured to ignore velocity.

The default filters are quite expensive. The -lwf option will select the less
computationally expensive lightweight Chamberlain filters which have a colder
response but require zonks fewer CPU cycles.




    Moog Voyager (Bristol "Explorer")
    ---------------------------------

This was Robert Moog's last synth, similar in build to the Mini but created
over a quarter of a century later and having far, far more flexibility. It 
was still monophonic, a flashback to a legendary synth but also a bit like
Bjorn Borg taking his wooden tennis racket back to Wimbledon long after having
retired and carbon fibre having come to pass. I have no idea who uses it and
Bjorn also crashed out in the first round. The modulation routing is exceptional
if not exactly clear.

The Voyager, or Bristol Explorer, is definitely a child of the Mini. It has
the same fold up control panel, three and half octave keyboard and very much
that same look and feel. It follows the same rough design of three oscillators
mixed with noise into a filter with envelopes for the filter and amplifier.
In contrast there is an extra 4th oscillator, a dedicated LFO bus also Osc-3
can still function as a second LFO here. The waveforms are continuously 
selected, changing gradually to each form: bristol uses a form of morphing
get get similar results. The envelopes are 4 stage rather than the 3 stage
Mini, and the effects routing bears no comparison at all, being far more
flexible here.

Just because its funny to know, Robert Moog once stated that the most difficult
part of building and releasing the Voyager was giving it the title 'Moog'. He
had sold his company in the seventies and had to buy back the right to use his
own name to release this synthesiser as a Moog, knowing that without that title
it probably would not sell quite as well as it didn't.

Control:

    LFO:
        Frequency
        Sync: LFO restarted with each keypress.

    Fine tune +/- one note
    Glide 0 to 30 seconds.

Modulation Busses:

    Two busses are implemented. Both have similar capabilities but one is
    controlled by the mod wheel and the other is constantly on. Each bus has
    a selection of sources, shaping, destination selection and amount.

    Wheel Modulation: Depth is controller by mod wheel.

        Source: Triwave/Ramp/Sample&Hold/Osc-3/External
        Shape: Off/Key control/Envelope/On
        Dest: All Osc Frequency/Osc-2/Osc-3/Filter/FilterSpace/Waveform (*)
        Amount: 0 to 1.

    Constant Modulation: Can use Osc-3 as second LFO to fatten sound.

        Source: Triwave/Ramp/Sample&Hold/Osc-3/External
        Shape: Off/Key control/Envelope/On
        Dest: All Osc Frequency/Osc-2/Osc-3/Filter/FilterSpace/Waveform (*)
        Amount: 0 to 1.

        * Destination of filter is the cutoff frequency. Filter space is the 
        difference in cutoff of the two layered filters. Waveform destination 
        affects the continuously variable oscillator waveforms and allows for 
        Pulse Width Modulation type effects with considerably more power since
        it can affect ramp to triangle for example, not just pulse width.

Oscillators:

    Oscillator 1:
        Octave: 32' to 1' in octave steps
        Waveform: Continuous between Triangle/Ramp/Square/Pulse

    Oscillator 2:
        Tune: Continuous up/down 7 semitones.
        Octave: 32' to 1' in octave steps
        Waveform: Continuous between Triangle/Ramp/Square/Pulse

    Oscillator 3:
        Tune: Continuous up/down 7 semitones.
        Octave: 32' to 1' in octave steps
        Waveform: Continuous between Triangle/Ramp/Square/Pulse

    Sync: Synchronise Osc-2 to Osc-1
    FM: Osc-3 frequency modulates Osc-1
    KBD: Keyboard tracking Osc-3
    Freq: Osc-3 as second LFO

Mixer:

    Gain levels for each source: Osc-1/2/3, noise and external input.

Filters:

    There are two filters with different configuration modes:

    1. Two parallel resonant lowpass filters.
    2. Serialised HPF and resonant LPF

    Cutoff: Frequency of cutoff
    Space: Distance between the cutoff of the two filters.
    Resonance: emphasis/Q.
    KBD tracking amount
    Mode: Select between the two operating modes.

Envelopes:

    Attack
    Decay
    Sustain
    Release

    Amount to filter (positive and negative control)

    Velocity sensitivity of amplifier envelope.

Master:

    Volume
    LFO: Single LFO or one per voice (polyphonic operation).
    Glide: On/Off portamento
    Release: On/Off envelope release.

The Explorer has a control wheel and a control pad. The central section has
the memory section plus a panel that can modify any of the synth parameters as
a real time control. Press the first mouse key here and move the mouse around
to adjust the controls. Default values are LFO frequency and filter cutoff 
but values can be changed with the 'panel' button. This is done by selecting
'panel' rather than 'midi', and then using the up/down keys to select parameter
that will be affected by the x and y motion of the mouse. At the moment the
mod routing from the pad controller is not saved to the memories, and it will
remain so since the pad controller is not exactly omnipresent on MIDI master
keyboards - the capabilities was put into the GIU to be 'exact' to the design.

This synth is amazingly flexible and difficult to advise on its best use. Try
starting by mixing just oscillator 1 through to the filter, working on mod 
and filter options to enrich the sound, playing with the oscillator switches
for different effects and then slowly mix in oscillator 2 and 3 as desired.

Memories are available via two grey up/down selector buttons, or a three digit
number can be entered. There are two rows of black buttons where the top row
is 0 to 4 and the second is 5 to 9. When a memory is selected the LCD display
will show whether it is is free (FRE) or programmed already (PRG).




    Moog Sonic-6
    ------------

This original design was made by an engineer who had previously worked with 
Moog on the big modular systems, Gene Zumchek. He tried to get Moog Inc to 
develop a small standalone unit rather than the behemoths however he could 
not get heard. After leaving he built a synth eventually called a Sonic-5 that
did fit the bill but sales volumes were rather small. He had tied up with a
business manager who worked out that the volume was largely due to the name
not being known, muSonics.
This was quickly overcome by accident. Moog managed to run his company into
rather large debt and the company folded. Bill Waytena, working with Zumcheck,
gathered together the funding needed to buy the remains of the failed company
and hence Moog Inc was labled on the rebadged Sonic-6. Zumcheck was eventually
forced to leave this company (or agreed to) as he could not work with Moog.
After a few modifications Bob Moog actually used this unit quite widely for
lecturing on electronic music. For demonstrative purposes it is far more
flexible than any of Moog's own non-modular designs and it was housed in a
transport case rather than needing a shipping crate as the modular systems
required.

The emulation features are given below, but first a few of the differences to 
the original

    Added a mod wheel that can drive GenX/Y.
    PWM is implemented on the oscillator B
    Installed an ADSR rather than AR, selectable.
    No alternative scalings - use scala file support
    Not duo or dia phonic. Primarily poly with separated glide.

The original was duophonic, kind of. It had a keyboard with high note and low
note precedence and the two oscillators could be driven from different notes.
Its not really duophony and was reportedly not nice to play but it added some
flexibility to the instrument. This features was dropped largley because it
is ugly to emulate in a polyphonic environment but the code still has glide
only on Osc-B. It has the two LFO that can be mixed, or at full throw of the 
GenXY mixer they will link X->A and Y->B giving some interesting routing, two
osc each with their own LFO driving the LFO from the mod wheel or shaping it
with the ADSR. Playing around should give access to X driving Osc-A, then 
Osc-A and GenY driving Osc-B with Mod and shaping for some investigation of
FM synthesis. The gruesome direct output mixer is still there, having the osc
and ring-mod bypass the filter and amplifier completely (or can be mixed back
into the 'actuated' signal).

There is currently no likely use for an external signal even though the
graphics are there.

The original envelope was AR or ASR. The emulator has a single ADSR and a 
control switch to select AR (actually AD), ASR, ADSD (MiniMoog envelope) or
ADSR.

Generator-Y has a S/H function on the noise source for a random signal which 
replaced the square wave. Generator-X still has a square wave.

Modulators:

    Two LFO, X and Y:

        Gen X:
            Tri/Ramp/Saw/Square
            Tuning
            Shaping from Envelope or Modwheel

        Gen Y:
            Tri/Ramp/Saw/Rand
            Tuning
            Shaping from Envelope or Modwheel

        Master LFO frequency

        GenXY mixer
    
    Two Oscillators, A and B

        Gen A:
            Tri/Ramp/Pulse
            PulseWidth
            Tuning
            Transpose 16', 8', 4' (*)

            Mods:

                Envelope
                GenXY(or X)
                Low frequency, High Frequency (drone), KBD Tracking

        Gen B:
            Tri/Ramp/Pulse
            PulseWidth
            Tuning
            Transpose 16' 8', 4'

            Mods:
            
                Osc-B
                GenXY(or Y)
                PWM

        GenAB mix

        Ring Mod:

            Osc-B/Ext
            GenXY/Osc-A

        Noise

            Pink/White

        Mixer

            GenAB
            RingMod
            External
            Noise

        Filter (**)

            Cutoff
            Emphasis

            Mods:

                ADSR
                Keyboard tracking
                GenXY

        Envelope:

            AR/ASR/ADSD/ADSR
            Velociy on/off

            Trigger:

                GenX
                GenY
                Kbd (rezero only)

            Bypass (key gated audio)

        Direct Output Mixer

            Osc-A
            Osc-B
            RingMod


The keyboard has controls for

    Glide (Osc-B only)
    Master Volume
    PitchWheel
    ModWheel (gain modifier on LFO)

    Global Tuning

    MultiLFO X and Y

* The oscillator range was +/-2 octave switch and a +/-1 octave pot. This
emulator has +/-1 octave switch and +/-7 note pot. That may change in a future
release to be more like the original, probably having a multiway 5 stage octave
selector.

** The filter will self oscillate at full emphasis however this is less 
prominent at lower frequencies (much like the Moog ladder filter). The filter
is also 'not quite' in tune when played as an oscillator, this will also change
in a future release.

There may be a reverb on the emulator. Or there may not be, that depends on
release. The PitchWheel is not saved in the memories, the unit is tuned on
startup and this will maintain tuning to other instruments. The MultiLFO allow
you to configure single LFO per emulation or one per voice, independently.
Having polyphony means you can have the extra richness of independent LFO per
voice however that does not work well if they are used as triggers, for example,
you end up with a very noisy result. With single triggers for all voices the
result is a lot more predictable.

The Sonic-6 as often described as having bad tuning, that probably depends on 
model since different oscillators were used at times. Also, different units
had different filters (Zumchek used a ladder of diodes to overcome the Moog
ladder of transister patent). The original was often described as only being
useful for sound effects. Personally I don't think that was true however the
design is extremely flexible and the mods are applied with high gains so to
get subtle sounds they only have to be applied lightly. Also, this critique
was in comparison to the Mini which was not great for sound effects since it,
in contrast, had very little in the way of modifiers.

The actual mod routing here is very rich. The two LFO can be mixed to provide
for more complex waves and have independent signal gain from the ADSR. To go
a step further it is possible to take the two mixed LFO into Osc-A, configure
that as an LFO and feed it into Osc-B for some very complex mod signals. That
way you can get a frequency modulated LFO which is not possible from X or Y. As
stated, if these are applied heavily you will get ray guns and car alarms but
in small amounts it is possible to just shape sounds. Most of the mod controls
have been made into power functions to give more control at small values.

The memory panel gives access to 72 banks of 8 memories each. Press the Bank
button and two digits for the bank, then just select the memory and press Load.
You can get the single digit banks by selecting Bank->number->Bank. There is
a save button which should require a double click but does not yet (0.30.0),
a pair of buttons for searching up and down the available memories and a button
called 'Find' which will select the next available free memory.

Midi options include channel, channel down and, er, thats it.




    CRUMAR TRILOGY
    --------------

This text is primarily that of the Stratus since the two synths were very 
similar. This is the bigger brother having all the same features with the added
string section. There were some minor differences in the synth circuits for
switchable or mixable waveforms.

This unit is a hybrid synth/organ/string combo, an early polyphonic using an
organ divider circuit rather than independent VCO and having a set of filters
and envelope for the synth sounds, most manufacturers came out with similar
designs. The organ section was generally regarded as pretty bad here, there
were just five controls, four used for the volume of 16, 8, 4 and 2 foot 
harmonics and a fifth for overall organ volume. The synth section had 6 voices
and some quite neat little features for a glide circuitry and legato playing
modes. The string section could mix 3 waveforms with vibrato on some so when
mixed with the straight waveform would produce phasing.

The emulator consists of two totally separate layers, one emulating the organ
circuitry and another the synth. The organ has maximum available polyphony as
the algorithm is quite lightweight even though diverse liberties have been
taken to beef up the sound. The synth section is limited to 6 voices unless
otherwise specified at run time. The organ circuitry is used to generate the
string section.

The legato playing modes affects three sections, the LFO modulation, VCO 
selection and glide:

LFO: this mod has a basic envelope to control the gain of the LFO using delay,
slope and gain. In 'multi' mode the envelope is triggered for every note that
is played and in the emulator this is actually a separate LFO per voice, a bit
fatter than the original. In 'Mono' mode there is only one LFO that all voices
will share and the envelope is triggered in Legato style, ie, only once for
a sequence of notes - all have to be released for the envelope to recover.

VCO: The original allowed for wavaeform selection to alternate between notes, 
something that is rather ugly to do with the bristol architecture. This is 
replaced with a VCO selector where each note will only take the output from
one of the two avalable oscillators and gives the ntoes a little more
separation. The legato mode works whereby the oscillator selection is only
made for the first note in a sequence to give a little more sound consistency.

Glide: This is probably the coolest feature of the synth. Since it used an
organ divider circuit it was not possible to actually glide from one note to
another - there are really only two oscillators in the synth section, not two
per voice. In contrast the glide section could glide up or down from a selected
amount to the real frequency. Selected from down with suitable values would
give a nice 'blue note' effect for example. In Legato mode this is done only
for the first keypress rather than all of the since the effect can be a bit
over the top if applied to each keystroke in a sequence. At the same time it
was possible to Sync the two oscillators, so having only one of them glide 
and be in sync then without legato this gave a big phasing entrance to each
note, a very interesting effect. The Glide has 4 modes:

    A. Both oscillators glide up to the target frequency
    B. Only oscillator-2 glides up to the target frequency
    C. Only oscillator-2 glides down to the target frequency
    D. Both oscillators glide down to the target frequency

These glide options with different sync and legato lead to some very unique
sounds and are emulated here with only minor differences.

The features, then notes on the differences to the original:

    A. Organ Section

        16, 8, 4 and 2 foot harmonic strengths.
        Volume.

    B. Synth Section

        LFO Modulation

            Rate - 0.1 to 50Hz approx
            Slope - up to 10 seconds
            Delay - up to 10 seconds
            Gain

            Routing selector: VCO, VCF, VCA
            Mono/Multi legato mode
            Shape - Tri/Ramp/Saw/Square

        Oscillator 1

            Tuning
            Sync 2 to 1
            Octave selector

        Oscillator 2

            Tuning
            Octave trill
            Octave selector

        Waveform Ramp and Square mix
        Alternate on/off
        Mono/Multi legato mode VCO selection

        Glide

            Amount up or down from true frequency
            Speed of glide
            Mono/Multi legato mode
            Direction A, B, C, D

        Filter

            Cutoff frequency
            Resonance
            Envelope tracking -ve to +ve
            Pedal tracking on/off

        Envelope

            Attack
            Decay
            Sustain
            Release

        Gain

    C. String Section

        16' 8' mix
        Subharmonic gain
        Attack
        Release

        Volume

Diverse liberties were taken with the reproduction, these are manageable from
the options panel by selecting the button next to the keyboard. This opens up
a graphic of a PCB, mostly done for humorous effect as it not in the least bit
representative of the actual hardware. Here there are a number of surface
mounted controllers. These are as below but may change by release:

   P1  Master volume

   P2  Organ pan
   P3  Organ waveform distorts
   P4  Organ spacialisation
   P5  Organ mod level
   J1  Organ key grooming 
   P6  Organ tuning (currently inactive *)

   P7  Synth pan
   P8  Synth tuning
   P9  Synth osc1 harmonics
   P10 Synth osc2 harmonics
   J2  Synth velocity sensitivity
   J3  Synth filter type
   P11 Synth filter tracking

   P12 String pan
   P13 String harmonics
   P14 String spacialisation
   P15 String mod level
   P16 String waveform distorts

*: To make the organ tunable the keymap file has to be removed.

Master (P1) volume affects both layers simultaneously and each layer can be
panned (P2/P7) and tuned (P8) separately to give phasing and spacialisation.
The synth layer has the default frequency map of equal temperament however the
organ section uses a 2MHz divider frequency map that is a few cents out for
each key. The Trilogy actually has this map for both layers and that can easily
be done with the emulator, details on request.

It is currently not possible to retune the organ divider circuit, it has a
private microtonal mapping to emulate the few percent anomalies of the divider
circuit and the frequencies are predefined. The pot is still visible in P6 and
can be activated by removing the related microtonal mapping file, details from
the author on request.

Diverse liberties were taken with the Organ section since the original only 
produced 4 pure (infinite bandwidth) square waves that were mixed together, 
an overly weak result. The emulator adds a waveform distort (P3), an notched
control that produces a pure sine wave at centre point. Going down it will
generate gradually increasing 3rd and 5th harmonics to give it a squarey wave
with a distinct hammond tone. The distortion actually came from the B3 emulator
which models the distort on the shape of the hammond tonewheels themselves.
Going up from centre point will produce gradually sharper sawtooth waves using
a different phase distortion.

Organ spacialisation (P4) will separate out the 4 harmonics to give them 
slightly different left and right positions to fatten out the sound. This works
in conjunction with the mod level (P5) where one of the stereo components of
each wave is modified by the LFO to give phasing changes up to vibrato.

The organ key grooming (J1) will either give a groomed wave to remove any
audible clicks from the key on and off events or when selected will produce 
something akin to a percussive ping for the start of the note.

The result for the organ section is that it can produce some quite nice sounds
reminiscent of the farfisa range to not quite hammond, either way far more
useful than the flat, honking square waves. The original sound can be made by
waveform to a quarter turn or less, spacialisation and mod to zero, key
grooming off.

The synth has 5 modifications at the first release. The oscillator harmonics
can be fattened at the top or bottom using P9 and P10, one control for each
oscillator, low is more bass, high is more treble. Some of the additional
harmonics will be automatically detuned a little to fatten out the sound as a
function of the -detune parameter defaulting to 100.

The envelope can have its velocity sensitively to the filter enabled or disabled
(J2) and the filter type can be a light weight filter for a thinner sound but at
far lower CPU load (J3).

The filter keyboard tracking is configurable (P11), this was outside of the spec
of the Trilogy however it was implemented here to correct the keyboard tracking
of the filter for all the emulations and the filter should now be playable.
The envelope touch will affect this depending on J2 since velocity affects the
cut off frequency and that is noticeable when playing the filter. This jumper
is there so that the envelope does not adversely affect tuning but can still be
used to have the filter open up with velocity if desired.

The mod application is different from the original. It had a three way selector
for routing the LFO to either VCO, VCA or VCF but only a single route. This
emulation uses a continuous notched control where full off is VCO only, notch
is VCF only and full on is VCA however the intermidiate positions will route
proportional amounts to two components.

The LFO has more options (Ramp and Saw) than the original (Tri and Square).

The extra options are saved with each memory however they are only loaded at
initialisation and when the 'Load' button is double-clicked. This allows you to
have them as global settings or per memory as desired. The MemUp and MemDown 
will not load the options, only the main settings.

VCO mod routing is a little bit arbitrary in this first release however I could
not find details of the actual implementation. The VCO mod routing only goes
to Osc-1 which also takes mod from the joystick downward motion. Mod routing
to Osc-2 only happens if 'trill' is selected. This seemed to give the most
flexibility, directing the LFO to VCF/VCA and controlling vibrato from the 
stick, then having Osc-2 separate so that it can be modified and sync'ed to
give some interesting phasing.

As of the first release there are possibly some issues with the oscillator 
Sync selector, it is perhaps a bit noisy with a high content of square wave.
Also, there are a couple of minor improvements that could be made to the 
legato features but they will be done in a future release. They regard how
the glide is applied to the first or all in a sequence of notes.

The joystick does not always pick up correctly however it is largely for 
presentation, doing actual work you would use a real joystick or just use the
modwheel (the stick generates and tracks continuous controller 1 - mod). The
modwheel tracking is also a bit odd but reflects the original architecture - 
at midpoint on the wheel there is no net modulation, going down affects VCO
in increasing amounts and going up from mid affect the VCF. The control feels
like it should be notched however generally that is not the case with mod
wheels.

A few notes are required on oscillator sync since by default it will seem to 
be quite noisy. The original could only product a single waveform at a single
frequency at any one time. Several emulators, including this one, use a bitone
oscillator which generates complex waveforms. The Bristol Bitone can generate
up to 4 waveforms simultaneously at different levels for 5 different harmonics
and the consequent output is very rich, the waves can be slightly detuned, 
the pulse output can be PW modulated. As with all the bristol oscillators that
support sync, the sync pulse is extracted as a postive leading zero crossing.
Unfortunately if the complex bitone output is used as input to sync another
oscillator then the result is far too many zero crossings to extract a good
sync. For the time being you will have to simplify the sync source to get a
good synchronised output which itself may be complex wave. A future release
will add a sync signal from the bitone which will be a single harmonic at the
base frequency and allow both syncing and synchronised waveform outputs to be
arbitrary. For the Trilogy this simplification of the sync waveform is done
automatically by the Sync switch, this means the synchronised output sounds
correct but the overall waveform may be simpler.




    CRUMAR STRATUS
    --------------

This unit is a hybrid synth/organ combo, an early polyphonic synth using an
organ divider circuit rather than independent VCO and having a set of filters
and envelope for the synth sounds, most manufacturers came out with similar
designs. The organ section was generally regarded as pretty bad here, there
were just five controls, four used for the volume of 16, 8, 4 and 2 foot 
harmonics and a fifth for overall organ volume. The synth section had 6 voices
and some quite neat little features for a glide circuitry and legato playing
modes.

The emulator consists of two totally separate layers, one emulating the organ
circuitry and another the synth. The organ has maximum available polyphony as
the algorithm is quite lightweight even though diverse liberties have been
taken to beef up the sound. The synth section is limited to 6 voices unless
otherwise specified at run time.

The legato playing modes affects three sections, the LFO modulation, VCO 
selection and glide:

LFO: this mod has a basic envelope to control the gain of the LFO using delay,
slope and gain. In 'multi' mode the envelope is triggered for every note that
is played and in the emulator this is actually a separate LFO per voice, a bit
fatter than the original. In 'Mono' mode there is only one LFO that all voices
will share and the envelope is triggered in Legato style, ie, only once for
a sequence of notes - all have to be released for the envelope to recover.

VCO: The original allowed for wavaeform selection to alternate between notes, 
something that is rather ugly to do with the bristol architecture. This is 
replaced with a VCO selector where each note will only take the output from
one of the two avalable oscillators and gives the ntoes a little more
separation. The legato mode works whereby the oscillator selection is only
made for the first note in a sequence to give a little more sound consistency.

Glide: This is probably the coolest feature of the synth. Since it used an
organ divider circuit it was not possible to actually glide from one note to
another - there are really only two oscillators in the synth section, not two
per voice. In contrast the glide section could glide up or down from a selected
amount to the real frequency. Selected from down with suitable values would
give a nice 'blue note' effect for example. In Legato mode this is done only
for the first keypress rather than all of the since the effect can be a bit
over the top if applied to each keystroke in a sequence. At the same time it
was possible to Sync the two oscillators, so having only one of them glide 
and be in sync then without legato this gave a big phasing entrance to each
note, a very interesting effect. The Glide has 4 modes:

    A. Both oscillators glide up to the target frequency
    B. Only oscillator-2 glides up to the target frequency
    C. Only oscillator-2 glides down to the target frequency
    D. Both oscillators glide down to the target frequency

These glide options with different sync and legato lead to some very unique
sounds and are emulated here with only minor differences.

The features, then notes on the differences to the original:

    A. Organ Section

        16, 8, 4 and 2 foot harmonic strengths.
        Volume.

    B. Synth Section

        LFO Modulation

            Rate - 0.1 to 50Hz approx
            Slope - up to 10 seconds
            Delay - up to 10 seconds
            Gain

            Routing selector: VCO, VCF, VCA
            Mono/Multi legato mode
            Shape - Tri/Ramp/Saw/Square

        Oscillator 1

            Tuning
            Sync 2 to 1
            Octave selector

        Oscillator 2

            Tuning
            Octave trill
            Octave selector

        Waveform Ramp and Square mix
        Alternate on/off
        Mono/Multi legato mode VCO selection

        Glide

            Amount up or down from true frequency
            Speed of glide
            Mono/Multi legato mode
            Direction A, B, C, D

        Filter

            Cutoff frequency
            Resonance
            Envelope tracking -ve to +ve
            Pedal tracking on/off

        Envelope

            Attack
            Decay
            Sustain
            Release

        Gain

Diverse liberties were taken with the reproduction, these are manageable from
the options panel by selecting the button next to the keyboard. This opens up
a graphic of a PCB, mostly done for humorous effect as it not in the least bit
representative of the actual hardware. Here there are a number of surface
mounted controllers. These are as below but may change by release:

   P1 Master volume

   P2  Organ pan
   P3  Organ waveform distorts
   P4  Organ spacialisation
   P5  Organ mod level
   J1  Organ key grooming 
   P6  Organ tuning (currently inactive *)

   P7  Synth pan
   P8  Synth tuning
   P9  Synth osc1 harmonics
   P10 Synth osc2 harmonics
   J2  Synth velocity sensitivity
   J3  Synth filter type
   P11 Synth filter tracking

*: To make the organ tunable the keymap file has to be removed.

Master (P1) volume affects both layers simultaneously and each layer can be
panned (P2/P7) and tuned (P8) separately to give phasing and spacialisation.
The synth layer has the default frequency map of equal temperament however the
organ section uses a 2MHz divider frequency map that is a few cents out for
each key. The Stratus actually has this map for both layers and that can easily
be done with the emulator, details on request.

It is currently not possible to retune the organ divider circuit, it has a
private microtonal mapping to emulate the few percent anomalies of the divider
circuit and the frequencies are predefined. The pot is still visible in P6 and
can be activated by removing the related microtonal mapping file, details from
the author on request.

Diverse liberties were taken with the Organ section since the original only 
produced 4 pure (infinite bandwidth) square waves that were mixed together, 
an overly weak result. The emulator adds a waveform distort (P3), an notched
control that produces a pure sine wave at centre point. Going down it will
generate gradually increasing 3rd and 5th harmonics to give it a squarey wave
with a distinct hammond tone. The distortion actually came from the B3 emulator
which models the distort on the shape of the hammond tonewheels themselves.
Going up from centre point will produce gradually sharper sawtooth waves using
a different phase distortion.

Organ spacialisation (P4) will separate out the 4 harmonics to give them 
slightly different left and right positions to fatten out the sound. This works
in conjunction with the mod level (P5) where one of the stereo components of
each wave is modified by the LFO to give phasing changes up to vibrato.

The organ key grooming (J1) will either give a groomed wave to remove any
audible clicks from the key on and off events or when selected will produce 
something akin to a percussive ping for the start of the note.

The result for the organ section is that it can produce some quite nice sounds
reminiscent of the farfisa range to not quite hammond, either way far more
useful than the flat, honking square waves. The original sound can be made by
waveform to a quarter turn or less, spacialisation and mod to zero, key
grooming off.

The synth has 5 modifications at the first release. The oscillator harmonics
can be fattened at the top or bottom using P9 and P10, one control for each
oscillator, low is more bass, high is more treble. Some of the additional
harmonics will be automatically detuned a little to fatten out the sound as a
function of the -detune parameter defaulting to 100.

The envelope can have its velocity sensitively to the filter enabled or disabled
(J2) and the filter type can be a light weight filter for a thinner sound but at
far lower CPU load (J3).

The filter keyboard tracking is configurable (P11), this was outside of the spec
of the Stratus however it was implemented here to correct the keyboard tracking
of the filter for all the emulations and the filter should now be playable.
The envelope touch will affect this depending on J2 since velocity affects the
cut off frequency and that is noticeable when playing the filter. This jumper
is there so that the envelope does not adversely affect tuning but can still be
used to have the filter open up with velocity if desired.

The mod application is different from the original. It had a three way selector
for routing the LFO to either VCO, VCA or VCF but only a single route. This
emulation uses a continuous notched control where full off is VCO only, notch
is VCF only and full on is VCA however the intermidiate positions will route
proportional amounts to two components.

The LFO has more options (Ramp and Saw) than the original (Tri and Square).

The extra options are saved with each memory however they are only loaded at
initialisation and when the 'Load' button is double-clicked. This allows you to
have them as global settings or per memory as desired. The MemUp and MemDown 
will not load the options, only the main settings.

VCO mod routing is a little bit arbitrary in this first release however I could
not find details of the actual implementation. The VCO mod routing only goes
to Osc-1 which also takes mod from the joystick downward motion. Mod routing
to Osc-2 only happens if 'trill' is selected. This seemed to give the most
flexibility, directing the LFO to VCF/VCA and controlling vibrato from the 
stick, then having Osc-2 separate so that it can be modified and sync'ed to
give some interesting phasing.

As of the first release there are possibly some issues with the oscillator 
Sync selector, it is perhaps a bit noisy with a high content of square wave.
Also, there are a couple of minor improvements that could be made to the 
legato features but they will be done in a future release. They regard how
the glide is applied to the first or all in a sequence of notes.

The joystick does not always pick up correctly however it is largely for 
presentation, doing actual work you would use a real joystick or just use the
modwheel (the stick generates and tracks continuous controller 1 - mod). The
modwheel tracking is also a bit odd but reflects the original architecture - 
at midpoint on the wheel there is no net modulation, going down affects VCO
in increasing amounts and going up from mid affect the VCF. The control feels
like it should be notched however generally that is not the case with mod
wheels.

A few notes are required on oscillator sync since by default it will seem to 
be quite noisy. The original could only product a single waveform at a single
frequency at any one time. Several emulators, including this one, use a bitone
oscillator which generates complex waveforms. The Bristol Bitone can generate
up to 4 waveforms simultaneously at different levels for 5 different harmonics
and the consequent output is very rich, the waves can be slightly detuned, 
the pulse output can be PW modulated. As with all the bristol oscillators that
support sync, the sync pulse is extracted as a postive leading zero crossing.
Unfortunately if the complex bitone output is used as input to sync another
oscillator then the result is far too many zero crossings to extract a good
sync. For the time being you will have to simplify the sync source to get a
good synchronised output which itself may be complex wave. A future release
will add a sync signal from the bitone which will be a single harmonic at the
base frequency and allow both syncing and synchronised waveform outputs to be
arbitrary. For the Stratus this simplification of the sync waveform is done
automatically by the Sync switch, this means the synchronised output sounds
correct but the overall waveform may be simpler.




    KORG POLY 800
    -------------

This is a low cost hybrid synth, somewhere between the Korg PolySix and their
Mono/Poly in that is polyphonic but only has one filter rather than one per
voice that came with the PolySix. It may have also used organ divider circuits
rather than individual oscillators - it did not have glide as a feature which
would be indicative of a divider circuit.

It featured 8 oscillators that could be applied as either 4 voices with dual
osc or 8 voices with a single osc. The architecture was verging on the
interesting since each oscillator was fead into an individual envelope generator
(described below) and then summed into the single filter, the filter having
another envelope generator, 9 in total. This lead to cost reduction over having
a filter per voice however the single filter leads to breathing, also discussed
below. The envelopes were digitally generated by an on-board CPU.

The control panel has a volume, global tuning control and a 'Bend' control
that governs the depth of the pitch bend from the joystick and the overall 
amount of DCO modulation applied by the joystick. There is no sequencer in
this emulation largely because there are far better options now available than
this had but also due to a shortage of onscreen realestate.

The Poly, Chord and Hold keys are emulated, hold being a sustain key. The
Chord relearn function works follows:

    Press the Hold key
    Press the Chord key with 2 seconds
        Press the notes on the keyboard (*)
    Press the Chord key again

After that the single chord can be played from a single note as a monophonic
instrument. The Chord is saved individually with each memory.
* Note that the chord is only saved if (a) it was played from the GUI keyboard
or (b) the GUI was linked up to any MIDI device as well as the engine. The 
reason is that the GUI maintains memories and so if a chord is played on your
actual keyboard then both the engine and the GUI needs a copy, the engine to
be able to play the notes and the GUI to be able to save them.

The keypanel should function very similar to the original. There is a Prog 
button that selects between Program selection or Parameter selection and an 
LED should show where the action is. There is the telephone keyboard to enter
program or parameters numbers and an up/down selector for parameter value.
The Bank/Hold selector also works, it fixes the bank number so programs can
be recalled from a single bank with a single button press. The Write function
is as per the original - Press Write, then two digits to save a memory.

The front panel consists of a data entry panel and a silkscreen of the parameter
numbers (this silkscreen is active in the emulation). Fifty parameters are
available from the original instrument:

    DE 11 DCO1 Octave transposition +2 octaves
    DE 12 DCO1 Waveform Square or Ramp
    DE 13 DCO1 16' harmonic
    DE 14 DCO1 8' harmonic
    DE 15 DCO1 4' harmonic
    DE 16 DCO1 2' harmonic
    DE 17 DCO1 level

    DE 18 DCO Double (4 voice) or Single (8 voice)

    DE 21 DCO2 Octave transposition +2 octaves
    DE 22 DCO2 Waveform Square or Ramp
    DE 23 DCO2 16' harmonic
    DE 24 DCO2 8' harmonic
    DE 25 DCO2 4' harmonic
    DE 26 DCO2 2' harmonic
    DE 27 DCO2 level
    DE 31 DCO2 semitone transpose
    DE 32 DCO2 detune

    DE 33 Noise level

    DE 41 Filter cutoff frequency
    DE 42 Filter Resonance
    DE 43 Filter Keyboard tracking off/half/full
    DE 44 Filter Envelope polarity
    DE 45 Filter Envelope amount
    DE 46 Filter Envelope retrigger

    DE 48 Chorus On/Off

    DE 51 Env-1 DCO1 Attack
    DE 52 Env-1 DCO1 Decay
    DE 53 Env-1 DCO1 Breakpoint
    DE 54 Env-1 DCO1 Slope
    DE 55 Env-1 DCO1 Sustain
    DE 56 Env-1 DCO1 Release

    DE 61 Env-2 DCO2 Attack
    DE 62 Env-2 DCO2 Decay
    DE 63 Env-2 DCO2 Breakpoint
    DE 64 Env-2 DCO2 Slope
    DE 65 Env-2 DCO2 Sustain
    DE 66 Env-2 DCO2 Release

    DE 71 Env-3 Filter Attack
    DE 72 Env-3 Filter Decay
    DE 73 Env-3 Filter Breakpoint
    DE 74 Env-3 Filter Slope
    DE 75 Env-3 Filter Sustain
    DE 76 Env-3 Filter Release

    DE 81 Mod LFO Frequency
    DE 82 Mod Delay
    DE 83 Mod DCO
    DE 84 Mod VCF

    DE 86 Midi channel
    DE 87 Midi program change enable
    DE 88 Midi OMNI

Of these 25 pararmeters, the emulation has changed 88 to be OMNI mode rather 
than the original sequence clock as internal or external. This is because the
sequencer function was dropped as explained above.

Additional to the original many of the controls which are depicted as on/off
are actually continuous. For example, the waveform appears to be either square
or ramp. The emulator allows you to use the up/down Value keys to reproduce
this however if you use the potentiometer then you can gradually move from one
wave to the next. The different harmonics are also not on/off, you can mix
each of them together with different amounts and if you configure a mixture
of waveforms and a bit of detune the sound should widen due to addition of a
bit of phasing within the actual oscillator.

The envelope generators are not typical ADSR. There is an initial attack from
zero to max gain then decay to a 'Breakpoint'. When this has been reached then
the 'Slope' parameter will take the signal to the Sustain level, then finally
the release rate. The extra step of breakpoint and slope give plenty of extra
flexibility to try and adjust for the loss of a filter per voice and the 
emulation has a linear step which should be the same as the original. The
ninth envelope is applied to the single filter and also as the envelope for 
the noise signal level.

The single filter always responded to the highest note on the keyboard. This
gives a weaker overall sound and if playing with two hands then there is a
noticible effect with keytracking - left hand held chords will cause filter
breathing as the right hand plays solos and the keyboard tracking changes 
from high to low octaves. Note that the emulator will implement a single
filter if you select DE 46 filter envelope retrigger to be single trigger, it
will be played legato style. If multiple triggers are selected then the
emulator will produce a filter and envelope for each voice.

Bristol adds a number of extra parameters to the emulator that are not
available from the mouse on the silkscreen and were not a part of the design
of the poly800. You have to select Prog such that the LED is lit next to the
Param display, then select the two digit parameter from the telephone keyboard:

    DE 28 DCO Sync 2 to 1
    DE 34 DCO-1 PW
    DE 35 DCO-1 PWM
    DE 36 DCO-2 PW
    DE 37 DCO-2 PWM
    DE 38 DCO temperature sensitivity
    DE 67 DCO Glide

    DE 85 Mod - Uni/Multi per voice or globally

    DE 57 Envelope Touch response

    DE 47 Chorus Parameter 0
    DE 58 Chorus Parameter 1
    DE 68 Chorus Parameter 2
    DE 78 Chorus Parameter 3

If DataEntry 28 is selected for oscillator sync then LFO MOD to DCO-1 is no
longer applied, it only goes to DCO-2. This allows for the interesting sync
modulated slow vibrato of DCO-2. The LFO mod is still applied via the joystick.

DE 38 global detune will apply both temperature sensitivity to each oscillator
but also fatten out the harmonics by detuning them independently. It is only 
calculated at 'note on' which can be misleading - it has no effect on existing
notes which is intentional if misleading.

DE 57 is a bitmask for the three envelopes to define which ones will give a
response to velocity with a default to '3' for velocity tracking oscillator
gain:

    value    DEG1    DEG2    DEG3
             DCO1    DCO2    FILT

      0       -        -       -
      1       V        -       -
      2       -        V       -
      3       V        V       -
      4       -        -       V
      5       V        -       V
      6       -        V       V
      7       V        V       V

This gives some interesting velocity tracking capabilities where just one osc
can track velocity to introduce harmonic content keeping the filter at a fixed
cutoff frequence. Having a bit of detune applied globally and locally will keep
the sound reasonably fat for each oscillator.

The filter envelope does not track velocity for any of the distributed voices,
this was intentional since when using high resonance it is not desirable that
the filter cutoff changes with velocity, it tends to be inconsistently 
disonant.

If you want to use this synth with controller mappings then map the value 
entry pot to your easiest to find rotary, then click the mouse on the membrane
switch to select which parameter you want to adjust with that control each time.
The emulator is naturally not limited to just 4/8 voices, you can request more
in which case single oscillator will give you the requested number of voices
and double will give you half that amount.

The Bristol Poly-800 is dedicated to Mark.




    Baumann BME-700
    ---------------

This unusual German synth had a build volume of about 500 units and only one
useful source of information could be found on it: a report on repair work for
one of the few existing examples at www.bluesynths.com. The BME systems were
hand built and judging by some reports on build quality may have been sold in
kit form. The unit was produced in the mid 1970's.

The synth has a very interesting design, somewhat reminiscent of the Moog Sonic
and Explorer synths. It has two modulating LFO with fairly high top frequency,
two filter and two envelopes. The envelopes are either AR or ASR but they can
be mixed together to generate amongst other features an ADSR, very innovative.
There is only one oscillator but the sound is fattened out by the use of two
parallel filters, one acting as a pure resonator and the other as a full VCF.

The synth has been left with a minimum of overhead. There are just 8 memory
locations on the front panel with Load, Save and Increment buttons and one
panel of options to adjust a few parameters on the oscillator and filters. It
is possible to get extra memories by loading banks with -load: if you request
starting in memory #21 the emulator will stuff 20 into the bank and 1 into the
memory location. There is no apparant midi channel selector, use -channel <n>
and then stay on it. This could have been put into the options panel however 
having midi channel in a memory is generally a bad idea.

    A. MOD

        Two LFO:

            frequency from 0.1 to 100 Hz
            Triangle and Square wave outputs

        Mix control

            Mod-1/2 into the VCO FM
            Env-1/Mod-2 into the VCO FM

    B. Oscillator

        Single VCO

            Glide 0 to 10s, on/off.
            PW Man: 5 to 50% duty cycle
            Auto depth:

                    Envelope-1
                    Mod-1, Mod-1/2, Tri/Square

            Vibrato depth
            Tuning
                8', 4', 16' transposition

            Shape

                continuous control from Square to Tri wave.

        Mix of noise or VCO output

    C. Res Filter

        Sharp (24db/Oct), Flat (12dB/Oct)
        5 frequency switches

    D. Envelopes

        Two envelopes

            Rise time
            Fall Time
            AR/ASR selector

            Two independent mixes of Env, for VCF and VCA.

    E. Filter

        Frequency
        Resonance
        Env/Mod selector

        Modulation

            KBD tracking
            Mod-1 or Mod-2, Tri/Square

    F. Amplifier

        Mix resonator/filter.
        Volume

        Mod depth

            Mod-1 or Mod-2, Tri/Square

The oscillator is implemented as a non-resampling signal generator, this means
it uses heuristics to estimate the wave at any given time. The harmonic content
is a little thin and although the generation method seems to be correct in how
it interprets signal ramps and drains from an analogue circuit this is one area
of improvement in the emulator. There are options to produce multiple waveforms
described below.

The resonant filter is implemented with a single Houvilainen and actually only
runs at 24dB/Oct. There are controls for remixing the different taps, a form
of feedforward and when in 'Flat' mod there is more remixing of the poles, this
does generate a slower roll off but gives the signal a bit more warmth than a
pure 12dB/Oct would.


There is a selector in the Memory section to access some options:

    G. Options

        LFO

            Synchronise wave to key on events
            Multi LFO (per voice).

        Oscillator

            Detune (temperature sensitivity)
            Multi - remix 8' with 16' or 4'.

        Noise

            Multi Noise (per voice).
            White/Pink
            Pink Filter

        ResFilter

            Sharp Resonance/Remix
            Flat Resonance/Remix

        Envelope

            Velocity Sensitive
            Rezero for note on
            Gain

        Filter

            Remix
            KBD tracking depth

The emulator probably gives the best results with the following:

startBristol -bme700 -mono -hnp -retrig -channel 1

This gives a monophonic emulation with high note preference and multiple
triggers.

The options from section G are only loaded under two circumstances: at system
start from the first selected memory location and if the Load button is given
a DoubleClick. All other memory load functions will inherrit the settings that
are currently active.




    Bristol BassMaker
    -----------------

The BassMaker is not actually an emulator, it is a bespoke sequencer design but
based on the capabilities of some of the early analogue sequencers such as the
Korg SQ-10. Supplying this probably leaves bristol open to a lot of feature
requests for sequencer functionaliity and it is stated here that the BassMaker
is supposed to be simple so excess functionality will probably be declined as
there are plenty of other sequencing applications that can provide a richer
feature set.

The main page gives access to a screen of controls for 16 steps and a total of
4 pages are available for a total of 64 steps. The pages are named 'A' through
'D'. Each step has 5 options:

    Note: one octave of note selection
    Transpose: +/- one octave transposition of the note.
    Volume: MIDI note velocity
    Controller: MIDI modulation, discussed further below
    Triggers: Note On/Off enablers

The trigger button gives 4 options indicated by the LED:

    off: note on/off are sent
    red: only send note_on
    green: only send note_off
    yellow: do not send note on/off

The 'Controllers' setting has multiple functions which can be selected from
the menu as explained below. The options available are as follows:

    Send semitone tuning

    Send glide rate

    Send modwheel

    Send expression pedal (controller value)

    Send Note: the controller will be 12 discrete steps as per the 'Note' 
    setting and this note will be sent on the Secondary MIDI channel.

The semitone tuning and glide work for the majority of the emulations. Some do
not support fine tune controls (Vox, Hammond, others). If you are missing these
capabilities for specific emulators raise a change request on Sourceforge.net.

At the top of the window there is a panel to manage the sequencer. It has the
following functions:

    Speed: step rate through the notes
    DutyCycle: ratio of note-on to note-off

    Start/Pause
    Stop: stop and return to first step/page

    Direction:
        Up
        Down
        Up/Down
        Random

    Select: which of the pages to include in the sequence.
    Edit: which page is currently displayed to be edited.

    Memory:
        0..9 key entry buttons, 1000 memories available
        Load
        Save: doubleclick to save current sequence

    Menu Panel
        Up, Down menu
        Function (return to previous level)
        Enter: enter submenu or enter value if in submenu

The menu consists of several tables, these can be stepped through using the Up
and Down arrows to move through the menu and the 'Enter' arrow to select a sub
menu or activate any option. The 'Fn' button returns one level:

    Memory:

        Find next free memory upwards
        Find next memory upwards
        Find next memory downwards

    Copy:

        Copy current edit page to 'A', 'B', 'C' or 'D'.

    Control - Set the control value to send:

        semitone tuning
        glide rate
        modwheel
        expression pedal (controller value)
        note events

    First midi channel

        Primary midi channel for note events

    Second midi channel

        Secondary midi channel when 'Control' configured to 'Note' events.

    Global Transpose

        Transpose the whole sequence up or down 12 semitones

    Clear - configure default value for all of the:

        Notes to zero
        Transpose to zero (midpoint)
        Volume to 0.8
        Control to midpoint
        Triggers to on/off

As of the first release in 0.30.8 large parts of the Controllers functionality
was only lightly tested. If you do not get the results you anticipate you may
require a fix.




    Bristol SID
    -----------

In release 0.40 bristol introduced a piece of code that emulated the Commodore
C64 6581 SID chip. The interface uses byte settings of the 31 chip registers to
be close to the original plus some floating point IO for extracting the audio
signal and configuring some analogue parameters and the 'softSID' is clocked
by the sample extraction process.

The chip uses integer maths and logic for the oscillators, ring mod, sync and
envelopes and emulates the analogue components of the 6581 with floating point
code, for the filter and S/N generation.

The oscillators will run as per the original using a single phase accumulator
and 16 bit frequency space. All the waveforms are extracted logically from the
ramp waveform generated by the phase accumulation. Sync and RingMod are also
extracted with the same methods. The noise generation is exor/add as per the
original however the noise signal will not degenerate when mixing waveforms.
The output waves are ANDed together. The bristol control register has an option
for Multi waveforms and when selected each oscillator will have its own phase
accumulator, can have a detune applied and will be mixed by summation rather
than using an AND function.

The envelope is an 8 bit up/down counter with a single gate bit. All the 4 bit
parameters give rates taken from the chip specifications including the slightly
exponential decay and release. Attack is a linear function and the sustain level
can only be decreased when active as the counter also refuses to count back up
when passed its peak.

The filter implements a 12dB/Octave multimode chamberlain filter providing LP,
BP and HP signals. This is not the best filter in the world however neither was
the original. An additional 24dB/Octave LP filter has been added, optionally 
available and with feedforward to provide 12/18dB signals. Between them the 
output can be quite rich.

The emulator provides some control over the 'analogue' section. The S/N ratio
can be configured from inaudible (just used to prevent denormal of the filter)
up to irritating levels. Oscillator leakage is configurable from none up to 
audible levels and the oscillator detune is configurable in cents although
this is a digital parameter and was not a part of the original.

Voice-3 provides an 8 bit output of its oscillator and envelope via the normal
output registers and the otherwise unused X and Y Analogue registers contain
the Voice-1 and Voice-2 oscillator output.

The bristol -sid emulator uses two softSID, one generating three audio voices
and a second one providing modulation signals by sampling the voice-3 osc and
env outputs and also by configuring voice-1 to generate noise to the output, 
resampling this noise and gating it from voice-3 to get sample and hold. This
would have been possible with the original as well if the output signal were
suitably coupled back on to one of the X/Y_Analogue inputs.

The emulator has several key assignment modes. The emulator is always just
monophonic but uses internal logic to assign voices. It can be played as a big
mono synth with three voices/oscillators, polyphonically with all voices either
sounding the same or optionally configured individually, and as of this release
a single arpeggiating mode - Poly-3. Poly-3 will assign Voice-1 to the lowest
note, voice-3 to the highest note and will arpeggiate Voice-2 through all other
keys that are pressed with a very high step rate. This is to provide some of
the sounds of the original C64 where fast arpeggiation was used to sounds 
chords rather than having to use all the voices. This first implementation 
does not play very well in Poly-3, a subsequent release will probably have a
split keyboard option where one half will arpeggiate and the other half will
play notes.

This is NOT a SID player, that would require large parts of the C64 to also be
emulated and there are plenty of SID players already available.

Bristol again thanks Andrew Coughlan, here for proposing the implementation of
a SID chip which turned out to be a very interesting project.





For the sake of being complete, given below is the verbose help output



A synthesiser emulation package.

    You should start this package with the startBristol script. This script
    will start up the bristol synthesiser binaries evaluating the correct
    library paths and executable paths. There are emulation, synthesiser,
    operational and GUI parameters:

    Emulation:

        -mini              - moog mini
        -explorer          - moog voyager
        -voyager           - moog voyager electric blue
        -memory            - moog memory
        -sonic6            - moog sonic 6
        -mg1               - moog/realistic mg-1 concertmate
        -hammond           - hammond module (deprecated, use -b3)
        -b3                - hammond B3 (default)
        -prophet           - sequential circuits prophet-5
        -pro52             - sequential circuits prophet-5/fx
        -pro10             - sequential circuits prophet-10
        -pro1              - sequential circuits pro-one
        -rhodes            - fender rhodes mark-I stage 73
        -rhodesbass        - fender rhodes bass piano
        -roadrunner        - crumar roadrunner electric piano
        -bitone            - crumar bit 01
        -bit99             - crumar bit 99
        -bit100            - crumar bit + mods
        -stratus           - crumar stratus synth/organ combo
        -trilogy           - crumar trilogy synth/organ/string combo
        -obx               - oberheim OB-X
        -obxa              - oberheim OB-Xa
        -axxe              - arp axxe
        -odyssey           - arp odyssey
        -arp2600           - arp 2600
        -solina            - arp/solina string ensemble
        -polysix           - korg polysix
        -poly800           - korg poly-800
        -monopoly          - korg mono/poly
        -ms20              - korg ms20 (unfinished: -libtest only)
        -vox               - vox continental
        -voxM2             - vox continental super/300/II
        -juno              - roland juno-60
        -jupiter           - roland jupiter-8
        -bme700            - baumann bme-700
        -bm                - bristol bassmaker sequencer
        -dx                - yamaha dx-7
        -cs80              - yamaha cs-80 (unfinished)
        -sidney            - commodore-64 SID chip synth
        -melbourne         - commodore-64 SID polyphonic synth (unfinished)
        -granular          - granular synthesiser (unfinished)
        -aks               - ems synthi-a (unfinished)
        -mixer             - 16 track mixer (unfinished: -libtest only)

    Synthesiser:

        -voices <n>        - operate with a total of 'n' voices (32)
        -mono              - operate with a single voice (-voices 1)
        -lnp               - low note preference (-mono)
        -hnp               - high note preference (-mono)
        -nnp               - no/last note preference (-mono)
        -retrig            - monophonic note logic legato trigger (-mono)
        -lvel              - monophonic note logic legato velocity (-mono)
        -channel <c>       - initial midi channel selected to 'c' (default 1)
        -lowkey <n>        - minimum MIDI note response (0)
        -highkey <n>       - maximum MIDI note response (127)
        -detune <%>        - 'temperature sensitivity' of emulation (0)
        -gain <gn>         - emulator output signal gain (default 1)
        -pwd <s>           - pitch wheel depth (2 semitones)
        -velocity <v>      - MIDI velocity mapping curve (510) (-mvc)
        -glide <s>         - MIDI glide duration (5)
        -emulate <name>    - search for the named synth or exit
        -register <name>   - name used for jack and alsa device regisration
        -lwf               - emulator lightweight filters
        -nwf               - emulator default filters
        -wwf               - emulator welterweight filters
        -hwf               - emulator heavyweight filters
        -blo <h>           - maximum # band limited harmonics (31)
        -blofraction <f>   - band limiting nyquist fraction (0.8)
        -scala <file>      - read the scala .scl tonal mapping table

    User Interface:

        -quality <n>       - color cache depth (bbp 2..8) (6)
        -grayscale <n>     - color or BW display (0..5) (0 = color)
        -antialias <n>     - antialias depth (0..100%) (30)
        -aliastype <s>     - antialias type (pre/texture/all)
        -opacity <n>       - opacity of the patch layer 20..100% (50)
        -scale <s>         - initial windowsize, fs = fullscreen (1.0)
        -width <n>         - the pixel width of the GUI window
        -autozoom          - flip between min and max window on Enter/Leave
        -raise             - disable auto raise on max resize
        -lower             - disable auto lower on min resize
        -rud               - constrain rotary tracking to up/down
        -pixmap            - use the pixmap interface rather than ximage
        -dct <ms>          - double click timeout (250 ms)
        -tracking          - disable MIDI keyboard tracking in GUI
        -load <m>          - load memory number 'm' (default 0)
        -import <pathname> - import memory from file into synth
        -mbi <m>           - master bank index (0)
        -activesense <m>   - active sense rate (2000 ms)
        -ast <m>           - active sense timeout (15000 ms)
        -mct <m>           - midi cycle timeout (50 ms)
        -ar|-aspect        - ignore emulator requested aspect ratio
        -iconify           - start with iconified window
        -window            - toggle switch to enable X11 window interfacen
        -cli               - enable command line interface
        -libtest           - gui test option, engine not invoked

        Gui keyboard shortcuts:

            <Ctrl> 's'     - save settings to current memory
            <Ctrl> 'l'     - (re)load current memory
            <Ctrl> 'x'     - exchange current with previous memory
            <Ctrl> '+'     - load next memory
            <Ctrl> '-'     - load previous memory
            <Ctrl> '?'     - show emulator help information
            <Ctrl> 'h'     - show emulator help information
            <Ctrl> 'r'     - show application readme information
            <Ctrl> 'k'     - show keyboard shortcuts
            <Ctrl> 'p'     - screendump to /tmp/<synth>.xpm
            <Ctrl> 't'     - toggle opacity
            <Ctrl> 'o'     - decrease opacity of patch layer
            <Ctrl> 'O'     - increase opacity of patch layer
            <Ctrl> 'w'     - display warranty
            <Ctrl> 'g'     - display GPL (copying conditions)
            <Shift> '+'    - increase window size
            <Shift> '-'    - decrease window size
            <Shift> 'Enter'- toggle window between full screen size
            'UpArrow'      - controller motion up (shift key accelerator)
            'DownArrow'    - controller motion down (shift key accelerator)
            'RightArrow'   - more controller motion up (shift key accelerator)
            'LeftArrow'    - more controller motion down (shift key accelerator)

    Operational:

        General:

            -engine        - don't start engine (connect to existing engine)
            -gui           - don't start gui (only start engine)
            -server        - run engine as a permanant server
            -daemon        - run engine as a detached permanant server
            -log           - redirect diagnostic to $HOME/.bristol/log
            -syslog        - redirect diagnostic to syslog
            -console       - log all messages to console (must be 1st option)
            -cache <path>  - memory and profile cache location (~/.bristol)
            -exec          - run all subprocesses in background
            -debug <1-16>  - debuging level (0)
            -readme [-<e>] - show readme [for emulator <e>] to console
            -glwf          - global lightweight filters - no overrides
            -host <h>      - connect to engine on host 'h' (localhost)
            -port <p>      - connect to engine on TCP port 'p' (default 5028)
            -quiet         - redirect diagnostic output to /dev/null
            -gmc           - open a MIDI connection to the brighton GUI
            -oss           - use OSS defaults for audio and MIDI
            -alsa          - use ALSA defaults for audio and MIDI (default)
            -jack          - use Jack defaults for audio and MIDI
            -jsmuuid <UUID>- jack session unique identifier
            -jsmfile <path>- jack session setting path
            -jsmd <ms>     - jack session file load delay (5000)
            -session       - disable session management
            -jdo           - use separate Jack clients for audio and MIDI
            -osc           - use OSC for control interface (unfinished)
            -forward       - disable MIDI event forwarding globally
            -localforward  - disable emulator gui->engine event forwarding
            -remoteforward - disable emulator engine->gui event forwarding
            -o <filename>  - Duplicate raw audio output data to file
            -nrp           - enable NPR support globally
            -enrp          - enable NPR/DE support in engine
            -gnrp          - enable NPR/RP/DE support in GUI
            -nrpcc <n>     - size of NRP controller table (128)

        Audio driver:

            -audio [oss|alsa|jack] - audio driver selection (alsa)
            -audiodev <dev>        - audio device selection
            -count <samples>       - sample period count (256)
            -outgain <gn>          - digital output signal gain (default 4)
            -ingain <gn>           - digital input signal gain (default 4)
            -preload <periods>     - configure preload buffer count (default 4)
            -rate <hz>             - sample rate (44100)
            -priority <p>          - audio RT priority, 0=no realtime (75)
            -autoconn              - attempt jack port auto-connect
            -multi <c>             - register 'c' IO channels (jack only)
            -migc <f>              - multi IO input gain scaling (jack only)
            -mogc <f>              - multi IO output gain scaling (jack only)

        Midi driver:

            -midi [oss|[raw]alsa|jack] - midi driver selection (alsa)
            -mididev <dev>             - midi device selection
            -seq                       - use the ALSA SEQ interface (default)
            -mididbg                   - midi debug-1 enable
            -mididbg2                  - midi debug-2 enable
            -sysid                     - MIDI SYSEX system identifier

        LADI driver (level 1 compliant):

            -ladi brighton             - only execute LADI in GUI
            -ladi bristol              - only execute LADI in engine
            -ladi <memory>             - LADI state memory index (1024)

    Audio drivers are PCM/PCM_plug or Jack. Midi drivers are either OSS/ALSA
    rawmidi interface, or ALSA SEQ. Multiple GUIs can connect to the single
    audio engine which then operates multitimbrally.

    The LADI interfaces does not use a state file but a memory in the normal
    memory locations. This should typically be outside of the range of the
    select buttons for the synth and the default of 1024 is taken for this
    reason.

    Examples:

    startBristol

        Print a terse help message.

    startBristol -v -h

        Hm, if you're reading this you found these switches already.

    startBristol -mini

        Run a minimoog using ALSA interface for audio and midi seq. This is
        equivalent to all the following options:
        -mini -alsa -audiodev plughw:0,0 -midi seq -count 256 -preload 8 
        -port 5028 -voices 32 -channel 1 -rate 44100 -gain 4 -ingain 4

    startBristol -alsa -mini

        Run a minimoog using ALSA interface for audio and midi. This is
        equivalent to all the following options:
        -mini -audio alsa -audiodev plughw:0,0 -midi alsa -mididev hw:0
        -count 256 -preload 8 -port 5028 -voices 32 -channel 1 -rate 44100

    startBristol -explorer -voices 1 -oss

        Run a moog explorer as a monophonic instrument, using OSS interface for
        audio and midi.

    startBristol -prophet -channel 3

        Run a prophet-5 using ALSA for audio and midi on channel 3.

    startBristol -b3 -count 512 -preload 2

        Run a hammond b3 with a buffer size of 512 samples, and preload two 
        such buffers before going active. Some Live! cards need this larger
        buffer size with ALSA drivers.

    startBristol -oss -audiodev /dev/dsp1 -vox -voices 8

        Run a vox continental using OSS device 1, and default midi device
        /dev/midi0. Operate with just 8 voices.

    startBristol -b3 -audio alsa -audiodev plughw:0,0 -seq -mididev 128.0

        Run a B3 emulation over the ALSA PCM plug interface, using the ALSA
        sequencer over client 128, port 0.

    startBristol -juno &
    startBristol -prophet -channel 2 -engine

        Start two synthesisers, a juno and a prophet. Both synthesisers will
        will be executed on one engine (multitimbral) with 32 voices between 
        them. The juno will be on default midi channel (1), and the prophet on
        channel 2. Output over the same default ALSA audio device.

    startBristol -juno &
    startBristol -port 5029 -audio oss -audiodev /dev/dsp1 -mididev /dev/midi1

        Start two synthesisers, a juno on the first ALSA soundcard, and a
        mini on the second OSS soundcard. Each synth is totally independent
        and runs with 32 voice polyphony (looks nice, not been tested).

The location of the bristol binaries can be specified in the BRISTOL
environment variable. Private memory and MIDI controller mapping files can
be found in the directory BRISTOL_CACHE and defaults to $HOME/.bristol

Setting the environment variable BRISTOL_LOG_CONSOLE to any value will result
in the bristol logging output going to your console window without formatted
timestamps

Korg Inc. of Japan is the rightful owner of the Korg and Vox trademarks, and
the Polysix, Mono/Poly, Poly-800, MS-20 and Continental tradenames. Their own
Vintage Collection provides emulations for a selection of their classic
synthesiser range, this product is in no manner related to Korg other than
giving homage to their great instruments.

Bristol is in no manner associated with any of the original manufacturers of
any of the emulated instruments. All names and trademarks are property of
their respective owners.

    author:   Nick Copeland
    email:    nickycopeland@hotmail.com

    http://bristol.sourceforge.net

