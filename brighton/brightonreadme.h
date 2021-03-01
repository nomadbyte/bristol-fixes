
/*
 *  Diverse Bristol audio routines.
 *  Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

char *readmeheader = "Bristol Emulations\n\
------------------\n\
\n\
This is a write-up of each of the emulated synthesisers. The algorithms\n\
employed were 'gleaned' from a variety of sources including the original\n\
owners manuals, so they may be a better source of information. The author\n\
has owned and used a selection but far from all of the originals. Some of them\n\
were built just from descriptions of their operation, or from understanding\n\
how synths work - most of them were based on the Mini Moog anyway. Many of\n\
the synths share components: the filter covers most of them, the Prophets and\n\
Oberheims share a common oscillator and the same LFO is used in many of them.\n\
Having said that each one differs considerably in the resulting sound that is\n\
generated, more so than initially expected. Each release refines each of the\n\
components and the result is that all emulations benefit from the improvements.\n\
All the emulations have distinctive sounds, not least due to that the original\n\
instruments used different modulations and mod routing.\n\
The filter, which is a large defining factor in the tonal qualities of any\n\
synth, is common to all the emulations. The filter implements a few different \n\
algorithms and these do separate each of the synths: the Explorer layering\n\
two low pass filters on top of each other: the OB-Xa using different types\n\
depending on 'Pole' selection. Since release 0.20.8 the emulator has had a\n\
Houvillainen non-linear ladder filter integrated which massively improves \n\
the quality at considerable expense to the CPU.\n\
There is one further filter algorithm used solely for the Leslie rotary \n\
emulator crossover, this is a butterworth type filter.\n\
\n\
Bristol is in no way related to any of the original manufacturers whose \n\
products are emulated by the engine and represented by the user interface,\n\
bristol does not suggest that the emulation is a like representation of the\n\
original instrument, and the author maintains that if you want the original\n\
sound then you are advised to seek out the original product. Alternatively a\n\
number of the original manufacturers now provide their own vintage collections\n\
which are anticipated to be more authentic. All names and trademarks used by\n\
Bristol are ownership of the respective companies and it is not inteded to \n\
misappropriate their use here. If you have concerns you are kindly requested\n\
to contact the author.\n\
\n\
The write-up includes the parameter operations, modulations, a description of\n\
the original instrument and a brief list of the kind of sounds you can expect\n\
by describing a few of the well known users of the synth.\n\
\n\
Several emulations have not been written up. Since the APR 2600 was implemented\n\
it became a rather large job to actually describe what was going on. If you \n\
really want to know about the synths that are not in this document then you\n\
might want to search for their owners manuals.\n\
\n\
All emulations are available from the same engine, just launch multiple GUIs\n\
and adjust the midi channels for multi timbrality and layering.\n\
\n\
It is noted here that the engine is relatively 'dumb'. Ok, it generates a very\n\
broad range of sounds, currently about 25 different synthesisers and organs,\n\
but it is not really intelligent. Memories are a part of the GUI specification\n\
- it tells the engine which algorithm to use on which MIDI channel, then it\n\
calls a memory routine that configures all the GUI controllers and a side effect\n\
of setting the controllers is that their values are sent to the engine. This is\n\
arguably the correct model but it can affect the use of MIDI master keyboards.\n\
The reason is that the GUI is really just a master keyboard for the engine and\n\
drives it with MIDI SYSEX messages over TCP sessions. If you were to alter the\n\
keyboard transpose, for example, this would result in the GUI sending different\n\
'key' numbers to the engine when you press a note. If you were already driving\n\
the synth from a master keyboard then the transpose button in the Brighton GUI\n\
would have no effect - the master keyboard would have to be transposed instead.\n\
This apparent anomaly is exacerbated by the fact that some parameters still are\n\
in the engine, for example master tuning is in the engine for the pure fact that\n\
MIDI does not have a very good concept of master tuning (only autotuning).\n\
Irrespective of this, bristol is a synthesiser so it needs to be played, \n\
tweaked, driven. If you think that any of the behaviour is anomalous then let\n\
me know. One known issue in this area is that if you press a key, transpose\n\
the GUI, then release the key - it will not go off in the engine since the GUI\n\
sends a different key code for the note off event - the transposed key. This\n\
cannot be related to the original keypress. This could be fixed with a MIDI all\n\
notes off event on 'transpose', but I don't like them. Also, since the 0.20\n\
stream the problem only affects a few of the emulations, the rest now sending\n\
a transpose message to the engine and letting it do the work.\n\
\n\
Since release 0.30.6 the engine correctly implements monophonic note logic.\n\
Prior to this the whole engine was polyphonic and playing with one voice only\n\
gave last note preference which dramatically affects playing styles - none of\n\
the cool legato effects of the early monophonics. The quoted release fix this\n\
limitation where the engine will keep a keymap of all played keys (one per\n\
emulation) when started with a single voice and uses this map to provide\n\
consistent note precedence, high note logic, low note logic or just using the\n\
previously implemented last note logic. In this release the keymap was only\n\
maintained with monophonic emulations, this is a potential extension as even\n\
in polyphonic mode it would be useful for arpeggiation (which is currently\n\
implemented using a FIFO rather than an ordered keymap).\n\
\n";

char *readmetrailer = "For the sake of being complete, given below is the verbose help output\n\n";

char *readme[BRISTOL_SYNTHCOUNT] = {

"    Moog Mini\n\
    ---------\n\
\n\
It is perhaps not possible to write up who used this synth, the list is endless.\n\
Popular as it was about the first non-modular synthesiser, built as a fixed\n\
configuration of the racked or modular predecessors.\n\
\n\
Best known at the time on Pink Floyd 'Dark Side of the Moon' and other albums.\n\
Rick Wakefield used it as did Jean Michel Jarre. Wakefield could actually\n\
predict the sound it would make by just looking at the settings, nice to be\n\
able to do if a little unproductive but it went to show how this was treated\n\
as an instrument in its own right. It takes a bit of work to get the same sweet,\n\
rich sounds out of the emulation, but it can be done with suitable tweaking.\n\
\n\
The original was monophonic, although a polyphonic version was eventually made\n\
after Moog sold the company - the MultiMoog. This emulation is more comparable\n\
to that model as the sound is a bit thinner and can be polyphonic. The design\n\
of this synth became the pole bearer for the following generations: it had \n\
three oscillators, one of which could become a low frequency modulator. They\n\
were fed into a mixer with a noise source, and were then fed into a filter\n\
with 2 envelope generators to contour the wave. Modulation capabilities were\n\
not extensive, but interestingly enough it did have a frequency modulation (FM)\n\
capability, eventually used by Yamaha to revolutionise the synthesiser market\n\
starting the downfall of analogue synthesis twenty years later.\n\
\n\
All the analogue synths were temperature sensitive. It was not unusual for the\n\
synths to 'detune' between sound test and performance as the evening set in.\n\
To overcome this they could optionally produce a stable A-440Hz signal for \n\
tuning the oscillators manually - eventually being an automated option in the\n\
newer synths. Whilst this digital version has stable frequency generation the\n\
A-440 is still employed here for the sake of it.\n\
\n\
Modifiers and mod routing are relatively simple, Osc-3 and noise can be mixed,\n\
and this signal routed to the oscillator 1 and 2 frequency or filter cutoff.\n\
\n\
The synth had 5 main stages as follows:\n\
\n\
Control:\n\
\n\
    Master tuning: up/down one note.\n\
\n\
    Glide: (glissando, portamento). The rate at which one key will change its\n\
    frequency to the next played key, 0 to 30 seconds.\n\
\n\
    Mod: source changes between Osc-3 and noise.\n\
\n\
    Release: The envelope generators had only 3 parameters. This governed whether\n\
    a key would release immediately or would use Decay to die out.\n\
\n\
    Multi: Controls whether the envelope will retrigger for each new keypress.\n\
\n\
Oscillators:\n\
\n\
    There are three oscillators. One and two are keyboard tracking, the third\n\
    can be decoupled and used as an LFO modulation source.\n\
\n\
    Oscillator 1:\n\
        Octave step from 32' to 1'.\n\
        Waveform selection: sine/square/pulse/ramp/tri/splitramp\n\
        Mod: controls whether Osc-3/noise modulates frequency\n\
\n\
    Oscillator 2:\n\
        Octave step from 32' to 1'.\n\
        Fine tune up/down 7 half notes.\n\
        Waveform selection: sine/square/pulse/ramp/tri/splitramp\n\
        Mod: controls whether Osc-3/noise modulates frequency\n\
\n\
    Oscillator 3:\n\
        Octave step from 32' to 1'.\n\
        Fine tune up/down 7 half notes.\n\
        Waveform selection: sine/square/pulse/ramp/tri/splitramp\n\
        LFO switch to decouple from keytracking.\n\
\n\
Mixer:\n\
\n\
    Gain levels for Oscillator 1/2/3\n\
    Mixing of the external input source into filter\n\
    Noise source with white/pink switch.\n\
\n\
    Note: The level at which Osc-3 and noise modulates sound depends on its\n\
    gain here, similarly the noise. The modulator mix also affects this, but\n\
    allows Osc-3 to mod as well as sound. The modwheel also affect depth.\n\
\n\
Filter:\n\
\n\
    Cutoff frequency\n\
\n\
    Emphasis (affects Q and resonance of filter).\n\
\n\
    Contour: defines how much the filter envelope affects cutoff.\n\
\n\
    Mod - Keyboard tracking of cutoff frequency.\n\
\n\
    Mod - Osc-3/noise modulation of cutoff frequency.\n\
\n\
Contour:\n\
\n\
    The synth had two envelope generators, one for the filter and one for the\n\
    amplifier. Release is affected by the release switch. If off the the sound\n\
    will release at the rate of the decay control.\n\
\n\
    Attack: initial ramp up of gain.\n\
\n\
    Decay: fall off of maximum signal down to:\n\
\n\
    Sustain: gain level for constant key-on level.\n\
\n\
    Key: Touch sensitivity of amplifier envelope.\n\
\n\
Improvements to the Mini would be some better oscillator waveforms, plus an\n\
alternative filter as this is a relatively simple synthesiser and could do\n\
with a warmer filter (this was fixed with integration of the houvillanen filters\n\
although the do consume a lot of CPU to do it).\n\
\n\
The Output selection has a Midi channel up/down selector and memory selector.\n\
To read a memory either use the up/down arrows to go to the next available\n\
memory, or type in a 3 digit number on the telephone keypad and press 'L' for\n\
load or 'S' for save.\n\
\n\
As of release 0.20.5 Vasiliy Basic contributed his Mini memory banks and they\n\
are now a part of the distribution:\n\
\n\
Programs for Bristol's \"Mini\" (from 50 to 86 PRG)\n\
\n\
List of programs:\n\
\n\
    -Melodic-\n\
    50 - Trumpet\n\
    51 - Cello\n\
    52 - Guitar 1\n\
    53 - Guitar 2\n\
    54 - Fingered Bass\n\
    55 - Picked Bass\n\
    56 - Harmonica\n\
    57 - Accordion\n\
    58 - Tango Accordion\n\
    59 - Super Accordion\n\
    60 - Piano\n\
    61 - Dark Organ\n\
    62 - Flute\n\
    63 - Music Box\n\
    64 - Glass Xylo\n\
    65 - Glass Pad\n\
    66 - Acid Bass\n\
    \n\
    -Drums-\n\
    67 - Bass Drum 1 \n\
    68 - Bass Drum 2\n\
    69 - Bass Drum 3\n\
    70 - Bass Drum 4\n\
    71 - Tom\n\
    72 - Snare 1\n\
    73 - Snare 2\n\
    74 - Snare 3\n\
    75 - Snare 4\n\
    76 - Cl HH 1\n\
    77 - Op HH 1\n\
    78 - Crash Cym 1\n\
    79 - Crash Cym 2\n\
    80 - Cl HH 2\n\
    81 - Op HH 2\n\
    \n\
    -FX-\n\
    82 - Sea Shore\n\
    83 - Helicopter 1\n\
    84 - Helicopter 2\n\
    85 - Bird Tweet\n\
    86 - Birds Tweet\n\n",

NULL,

"    Sequential Circuits Prophet-5\n\
    Sequential Circuits Prophet-52 (the '5' with chorus)\n\
    ----------------------------------------------------\n\
\n\
Sequential circuits released amongst the first truly polyphonic synthesisers\n\
where a group of voice circuits (5 in this case) were linked to an onboard\n\
computer that gave the same parameters to each voice and drove the notes to\n\
each voice from the keyboard. The device had some limited memories to allow \n\
for real live stage work. The synth was amazingly flexible regaring the\n\
oscillator options and modulation routing, producing some of the fattest \n\
sounds around. They also had some of the fattest pricing as well, putting it\n\
out of reach of all but the select few, something that maintained its mythical\n\
status. David Sylvian of Duran Duran used the synth to wide acclaim in the\n\
early 80's as did many of the new wave of bands.\n\
\n\
The -52 is the same as the -5 with the addition of a chorus as it was easy, it\n\
turns the synth stereo for more width to the sound, and others have done it on\n\
the Win platform.\n\
\n\
The design of the Prophet synthesisers follows that of the Mini Moog. It has\n\
three oscillators one of them as a dedicated LFO. The second audio oscillator\n\
can also function as a second LFO, and can cross modulate oscillator A for FM \n\
type effects. The audible oscillators have fixed waveforms with pulse width\n\
modulation of the square wave. These are then mixed and sent to the filter with\n\
two envelopes, for the filter and amplifier.\n\
\n\
Modulation bussing is quite rich. There is the wheel modulation which is global,\n\
taking the LFO and Noise as a mixed source, and send it under wheel control to\n\
any of the oscillator frequency and pulse width, plus the filter cutoff. Poly\n\
mods take two sources, the filter envelope and Osc-B output (which are fully\n\
polyphonic, or rather, independent per voice), and can route them through to\n\
Osc-A frequency and Pulse Width, or through to the filter. To get the filter\n\
envelope to actually affect the filter it needs to go through the PolyMod\n\
section. Directing the filter envelope to the PW of Osc-A can make wide, breathy\n\
scanning effects, and when applied to the frequency can give portamento effects.\n\
\n\
LFO:\n\
\n\
    Frequency: 0.1 to 50 Hz\n\
    Shape: Ramp/Triangle/Square. All can be selected, none selected should\n\
    give a sine wave (*)\n\
\n\
    (*) Not yet implemented.\n\
\n\
Wheel Mod:\n\
\n\
    Mix: LFO/Noise\n\
    Dest: Osc-A Freq/Osc-B Freq/Osc-A PW/Osc-B PW/Filter Cutoff\n\
\n\
Poly Mod: These are affected by key velocity.\n\
\n\
    Filter Env: Amount of filter envelope applied\n\
    Osc-B: Amount of Osc-B applied:\n\
    Dest: Osc-A Freq/Osc-A PW/Filter Cutoff\n\
\n\
Osc-A:\n\
\n\
    Freq: 32' to 1' in octave steps\n\
    Shape: Ramp or Square\n\
    Pulse Width: only when Square is active.\n\
    Sync: synchronise to Osc-B\n\
\n\
Osc-B:\n\
\n\
    Freq: 32' to 1' in octave steps\n\
    Fine: +/- 7 semitones\n\
    Shape: Ramp/Triangle/Square\n\
    Pulse Width: only when Square is active.\n\
    LFO: Lowers frequency by 'several' octaves.\n\
    KBD: enable/disable keyboard tracking.\n\
\n\
Mixer:\n\
\n\
    Gain for Osc-A, Osc-B, Noise\n\
\n\
Filter:\n\
\n\
    Cutoff: cuttof frequency\n\
    Res: Resonance/Q/Emphasis\n\
    Env: amount of PolyMod affecting to cutoff.\n\
\n\
Envelopes: One each for PolyMod (filter) and amplifier.\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
Global:\n\
\n\
    Master Volume\n\
    A440 - stable sine wave at A440 Hz for tuning.\n\
    Midi: channel up/down\n\
    Release: release all notes\n\
    Tune: autotune oscillators.\n\
    Glide: amount of portamento\n\
\n\
    Unison: gang all voices to a single 'fat' monophonic synthesiser.\n\
\n\
This is one of the fatter of the Bristol synths and the design of the mods\n\
is impressive (not my design, this is as per sequential circuits spec). Some\n\
of the cross modulations are noisy, notably 'Osc-B->Freq Osc-A' for square\n\
waves as dest and worse as source.\n\
\n\
The chorus used by the Prophet-52 is a stereo 'Dimension-D' type effect. The\n\
signal is panned from left to right at one rate, and the phasing and depth at\n\
a separate rate to generate subtle chorus through to helicopter flanging.\n\
\n\
Memories are loaded by selecting the 'Bank' button and typing in a two digit\n\
bank number followed by load. Once the bank has been selected then 8 memories\n\
from the bank can be loaded by pressing another memory select and pressing\n\
load. The display will show free memories (FRE) or programmed (PRG).\n\
\n",

"    Yamaha DX-7\n\
    -----------\n\
\n\
Released in the '80s this synth quickly became the most popular of all time.\n\
It was the first fully digital synth, employed a revolutionary frequency \n\
modulated algorithm and was priced much lower than the analogue monsters\n\
that preceded it. Philip Glass used it to wide effect for Miami Vice, Prince\n\
had it on many of his albums, Howard Jones produced albums filled with its\n\
library sounds. The whole of the 80's were loaded with this synth, almost to\n\
the point of saturation. There was generally wide use of its library sounds\n\
due to the fact that it was nigh on impossible to programme, only having entry\n\
buttons and the algorithm itself was not exactly intuitive, but also because\n\
the library was exceptional and the voices very playable. The emulation is a\n\
6 operator per voice, and all the parameters are directly accessible to ease\n\
programming.\n\
\n\
The original DX had six operators although cheaper models were release with\n\
just 4 operators and a consequently thinner sound. Each operator is a sine\n\
wave oscillator with its own envelope generator for amplification and a few \n\
parameters that adjusted its modulators. It used a number of different \n\
algorithms where operators were mixed together and then used to adjust the\n\
frequency of the next set of operators. The sequence of the operators affected\n\
the net harmonics of the overall sound. Each operator has a seven stage \n\
envelope - 'ramp' to 'level 1', 'ramp' to 'level 2', 'decay' to 'sustain',\n\
and finally 'release' when a key is released. The input gain to the frequency\n\
modulation is controllable, the output gain is also adjustable, and the final\n\
stage operators can be panned left and right.\n\
\n\
Each operator has:\n\
\n\
    Envelope:\n\
\n\
        Attack: Ramp rate to L1\n\
        L1: First target gain level\n\
        Attack: Ramp rate from L2 to L2\n\
        L2: Second target gain level\n\
        Decay: Ramp rate to sustain level\n\
        Sustain: Continuous gain level\n\
        Release: Key release ramp rate\n\
\n\
    Tuning:\n\
\n\
        Tune: +/- 7 semitones\n\
        Transpose: 32' to 1' in octave increments\n\
\n\
        LFO: Low frequency oscillation with no keyboard control\n\
\n\
    Gain controls:\n\
\n\
        Touch: Velocity sensitivity of operator.\n\
\n\
        In gain: Amount of frequency modulation from input\n\
        Out gain: Output signal level\n\
\n\
        IGC: Input gain under Mod control\n\
        OGC: Output gain under Mod control\n\
\n\
        Pan: L/R pan of final stage operators.\n\
\n\
Global and Algorithms:\n\
\n\
    24 different operator staging algorithms\n\
    Pitchwheel: Depth of pitch modifier\n\
    Glide: Polyphonic portamento\n\
    Volume\n\
    Tune: Autotune all operators\n\
\n\
Memories can be selected with either submitting a 3 digit number on the keypad,\n\
or selecting the orange up/down buttons.\n\
\n\
An improvement could be more preset memories with different sounds that can\n\
then be modified, ie, more library sounds. There are some improvements that\n\
could be made to polyphonic mods from key velocity and channel/poly pressure\n\
that would not be difficult to implement.\n\
\n\
The addition of triangle of other complex waveforms could be a fun development\n\
effort (if anyone were to want to do it).\n\
\n\
The DX still has a prependancy to seg fault, especially when large gains are\n\
applied to input signals. This is due to loose bounds checking that will be\n\
extended in a present release.\n\
\n",

"    Roland Juno-60\n\
    --------------\n\
\n\
Roland was one of the main pacemakers in analogue synthesis, also competing\n\
with the Sequential and Oberheim products. They did anticipate the moving\n\
market and produced the Juno-6 relatively early. This was one of the first\n\
accessible synths, having a reasonably fat analogue sound without the price\n\
card of the monster predecessors. It brought synthesis to the mass market that\n\
marked the decline of Sequential Circuits and Oberheim who continued to make\n\
their products bigger and fatter. The reduced price tag meant it had a slightly\n\
thinner sound, and a chorus was added to extend this, to be a little more\n\
comparable.\n\
\n\
The synth again follows the Mini Moog design of oscillators into filter into\n\
amp. The single oscillator is fattened out with harmonics and pulse width\n\
modulation. There is only one envelope generator that can apply to both the\n\
filter and amplifier.\n\
\n\
Control:\n\
\n\
    DCO: Amount of pitch wheel that is applied to the oscillators frequency.\n\
    VCF: Amount of pitch wheel that is applied to the filter frequency.\n\
\n\
    Tune: Master tuning of instrument\n\
\n\
    Glide: length of portamento\n\
\n\
    LFO: Manual control for start of LFO operation.\n\
\n\
Hold: (*)\n\
\n\
    Transpose: Up/Down one octave\n\
    Hold: prevent key off events\n\
\n\
LFO:\n\
\n\
    Rate: Frequency of LFO\n\
    Delay: Period before LFO is activated\n\
    Man/Auto: Manual or Automatic cut in of LFO\n\
\n\
DCO:\n\
\n\
    LFO: Amount of LFO affecting frequency. Affected by mod wheel.\n\
    PWM: Amount of LFO affecting PWM. Affected by mod wheel.\n\
\n\
    ENV/LFO/MANUAL: Modulator for PWM\n\
\n\
    Waveform:\n\
        Pulse or Ramp wave. Pulse has PWM capabily.\n\
    \n\
    Sub oscillator:\n\
        On/Off first fundamental square wave.\n\
    \n\
    Sub:\n\
        Mixer for fundamental\n\
\n\
    Noise:\n\
        Mixer of white noise source.\n\
\n\
HPF: High Pass Filter\n\
\n\
    Freq:\n\
        Frequency of cutoff.\n\
\n\
VCF:\n\
\n\
    Freq:\n\
        Cutoff frequency\n\
\n\
    Res:\n\
        Resonance/emphasis.\n\
    \n\
    Envelope:\n\
        +ve/-ve application\n\
    \n\
    Env:\n\
        Amount of contour applied to cutoff\n\
    \n\
    LFO:\n\
        Depth of LFO modulation applied.\n\
    \n\
    KBD:\n\
        Amount of key tracking applied.\n\
\n\
VCA:\n\
\n\
    Env/Gate:\n\
        Contour is either gated or modulated by ADSR\n\
    \n\
    Level:\n\
        Overall volume\n\
\n\
ADSR:\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
Chorus:\n\
\n\
    8 Selectable levels of Dimension-D type helicopter flanger.\n\
\n\
* The original instrument had a basic sequencer on board for arpeggio effects\n\
on each key. In fact, so did the Prophet-10 and Oberheims. This was only \n\
implemented in 0.10.11.\n\
\n\
The LFO cut in and gain is adjusted by a timer and envelope that it triggers.\n\
\n\
The Juno would improve from the use of the prophet DCO rather than its own one.\n\
It would require a second oscillator for the sub frequency, but the prophet DCO\n\
can do all the Juno does with better resampling and PWM generation.\n\
\n",

"    Moog Voyager (Bristol \"Explorer\")\n\
    ---------------------------------\n\
\n\
This was Robert Moog's last synth, similar in build to the Mini but created\n\
over a quarter of a century later and having far, far more flexibility. It \n\
was still monophonic, a flashback to a legendary synth but also a bit like\n\
Bjorn Borg taking his wooden tennis racket back to Wimbledon long after having\n\
retired and carbon fibre having come to pass. I have no idea who uses it and\n\
Bjorn also crashed out in the first round. The modulation routing is exceptional\n\
if not exactly clear.\n\
\n\
The Voyager, or Bristol Explorer, is definitely a child of the Mini. It has\n\
the same fold up control panel, three and half octave keyboard and very much\n\
that same look and feel. It follows the same rough design of three oscillators\n\
mixed with noise into a filter with envelopes for the filter and amplifier.\n\
In contrast there is an extra 4th oscillator, a dedicated LFO bus also Osc-3\n\
can still function as a second LFO here. The waveforms are continuously \n\
selected, changing gradually to each form: bristol uses a form of morphing\n\
get get similar results. The envelopes are 4 stage rather than the 3 stage\n\
Mini, and the effects routing bears no comparison at all, being far more\n\
flexible here.\n\
\n\
Just because its funny to know, Robert Moog once stated that the most difficult\n\
part of building and releasing the Voyager was giving it the title 'Moog'. He\n\
had sold his company in the seventies and had to buy back the right to use his\n\
own name to release this synthesiser as a Moog, knowing that without that title\n\
it probably would not sell quite as well as it didn't.\n\
\n\
Control:\n\
\n\
    LFO:\n\
        Frequency\n\
        Sync: LFO restarted with each keypress.\n\
\n\
    Fine tune +/- one note\n\
    Glide 0 to 30 seconds.\n\
\n\
Modulation Busses:\n\
\n\
    Two busses are implemented. Both have similar capabilities but one is\n\
    controlled by the mod wheel and the other is constantly on. Each bus has\n\
    a selection of sources, shaping, destination selection and amount.\n\
\n\
    Wheel Modulation: Depth is controller by mod wheel.\n\
\n\
        Source: Triwave/Ramp/Sample&Hold/Osc-3/External\n\
        Shape: Off/Key control/Envelope/On\n\
        Dest: All Osc Frequency/Osc-2/Osc-3/Filter/FilterSpace/Waveform (*)\n\
        Amount: 0 to 1.\n\
\n\
    Constant Modulation: Can use Osc-3 as second LFO to fatten sound.\n\
\n\
        Source: Triwave/Ramp/Sample&Hold/Osc-3/External\n\
        Shape: Off/Key control/Envelope/On\n\
        Dest: All Osc Frequency/Osc-2/Osc-3/Filter/FilterSpace/Waveform (*)\n\
        Amount: 0 to 1.\n\
\n\
        * Destination of filter is the cutoff frequency. Filter space is the \n\
        difference in cutoff of the two layered filters. Waveform destination \n\
        affects the continuously variable oscillator waveforms and allows for \n\
        Pulse Width Modulation type effects with considerably more power since\n\
        it can affect ramp to triangle for example, not just pulse width.\n\
\n\
Oscillators:\n\
\n\
    Oscillator 1:\n\
        Octave: 32' to 1' in octave steps\n\
        Waveform: Continuous between Triangle/Ramp/Square/Pulse\n\
\n\
    Oscillator 2:\n\
        Tune: Continuous up/down 7 semitones.\n\
        Octave: 32' to 1' in octave steps\n\
        Waveform: Continuous between Triangle/Ramp/Square/Pulse\n\
\n\
    Oscillator 3:\n\
        Tune: Continuous up/down 7 semitones.\n\
        Octave: 32' to 1' in octave steps\n\
        Waveform: Continuous between Triangle/Ramp/Square/Pulse\n\
\n\
    Sync: Synchronise Osc-2 to Osc-1\n\
    FM: Osc-3 frequency modulates Osc-1\n\
    KBD: Keyboard tracking Osc-3\n\
    Freq: Osc-3 as second LFO\n\
\n\
Mixer:\n\
\n\
    Gain levels for each source: Osc-1/2/3, noise and external input.\n\
\n\
Filters:\n\
\n\
    There are two filters with different configuration modes:\n\
\n\
    1. Two parallel resonant lowpass filters.\n\
    2. Serialised HPF and resonant LPF\n\
\n\
    Cutoff: Frequency of cutoff\n\
    Space: Distance between the cutoff of the two filters.\n\
    Resonance: emphasis/Q.\n\
    KBD tracking amount\n\
    Mode: Select between the two operating modes.\n\
\n\
Envelopes:\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
    Amount to filter (positive and negative control)\n\
\n\
    Velocity sensitivity of amplifier envelope.\n\
\n\
Master:\n\
\n\
    Volume\n\
    LFO: Single LFO or one per voice (polyphonic operation).\n\
    Glide: On/Off portamento\n\
    Release: On/Off envelope release.\n\
\n\
The Explorer has a control wheel and a control pad. The central section has\n\
the memory section plus a panel that can modify any of the synth parameters as\n\
a real time control. Press the first mouse key here and move the mouse around\n\
to adjust the controls. Default values are LFO frequency and filter cutoff \n\
but values can be changed with the 'panel' button. This is done by selecting\n\
'panel' rather than 'midi', and then using the up/down keys to select parameter\n\
that will be affected by the x and y motion of the mouse. At the moment the\n\
mod routing from the pad controller is not saved to the memories, and it will\n\
remain so since the pad controller is not exactly omnipresent on MIDI master\n\
keyboards - the capabilities was put into the GIU to be 'exact' to the design.\n\
\n\
This synth is amazingly flexible and difficult to advise on its best use. Try\n\
starting by mixing just oscillator 1 through to the filter, working on mod \n\
and filter options to enrich the sound, playing with the oscillator switches\n\
for different effects and then slowly mix in oscillator 2 and 3 as desired.\n\
\n\
Memories are available via two grey up/down selector buttons, or a three digit\n\
number can be entered. There are two rows of black buttons where the top row\n\
is 0 to 4 and the second is 5 to 9. When a memory is selected the LCD display\n\
will show whether it is is free (FRE) or programmed already (PRG).\n\
\n" ,

"   Hammond B3 (dual manual)\n\
    ------------------------\n\
\n\
The author first implemented the Hammond module, then extended it to the B3\n\
emulation. Users of this are too numerous to mention and the organ is still\n\
popular. Jimmy Smith, Screaming Jay Hawkins, Kieth Emerson, Doors and \n\
almost all american gospel blues. Smith was profuse, using the instrument for\n\
a jazz audience, even using its defects (key noise) to great effect. Emerson\n\
had two on stage, one to play and another to kick around, even including\n\
stabbing the keyboard with a knife to force keylock during performances\n\
(Emerson was also a Moog fan with some of the first live performances). He\n\
also used the defects of the system to great effect, giving life to the over-\n\
driven Hammond sound.\n\
\n\
The Hammond was historically a mechanical instrument although later cheaper\n\
models used electronics. The unit had a master motor that rotated at\n\
the speed of the mains supply. It drove a spindle of cog wheels and next to \n\
each cog was a pickup. The pickup output went into the matrix of the harmonic\n\
drawbars. It was originally devised to replace the massive pipe organs in\n\
churches - Hammond marketed their instruments with claims that they could not be\n\
differentiated from the mechanical pipe equivalent. He was taken to court by\n\
the US government for misrepresentation, finally winning his case using a double\n\
blind competitive test against a pipe organ, in a cathedral, with speakers\n\
mounted behind the organ pipes and an array of music scholars, students and \n\
professionals listening. The results spoke for themselves - students would\n\
have scored better by simply guessing which was which, the professionals\n\
fared only a little better than that. The age of the Hammond organ had arrived.\n\
\n\
The company had a love/hate relationship with the Leslie speaker company - the\n\
latter making money by selling their rotary speakers along with the organ to\n\
wide acceptance. The fat hammond 'chorus' was a failed attempt to distance\n\
themselves from Leslie. That was never achieved due to the acceptance of the\n\
Leslie, but the chorus did add another unique sound to the already awesome\n\
instrument. The rotary speaker itself still added an extra something to the\n\
unique sound that is difficult imagine one without the other. It has a wide\n\
range of operating modes most of which are included in this emulator.\n\
\n\
The chorus emulation is an 8 stage phase shifting filter algorithm with a \n\
linear rotor between the taps.\n\
\n\
Parameterisation of the first B3 window follows the original design:\n\
\n\
    Leslie: Rotary speaker on/off\n\
    Reverb: Reverb on/off\n\
    VibraChorus: 3 levels of vibrato, 3 of chorus.\n\
    Bright: Added upper harmonics to waveforms.\n\
\n\
Lower and Upper Manual Drawbars: The drawbars are colour coded into white for\n\
even harmonics and black for odd harmonics. There are two subfrequencies in \n\
brown. The number given here are the length of organ pipe that would \n\
correspond to the given desired frequency.\n\
\n\
    16    - Lower fundamental\n\
    5 1/3 - Lower 3rd fundamental\n\
    8     - Fundamental\n\
    4     - First even harmonic\n\
    2 2/3 - First odd harmonic\n\
    2     - Second even harmonic\n\
    1 3/5 - Second odd harmonic\n\
    1 1/3 - Third odd harmonic\n\
    1     - Third even harmonic\n\
\n\
The drawbars are effectively mixed for each note played. The method by which\n\
the mixing is done is controlled in the options section below. There were \n\
numerous anomalies shown by the instrument and most of them are emulated.\n\
\n\
The Hammond could provide percussives effect the first even and odd harmonics.\n\
This gave a piano like effect and is emulated with Attack/Decay envelope.\n\
\n\
    Perc 4'     - Apply percussive to the first even harmonic\n\
    Perc 2 2/3' - Apply percussive to the first odd harmonic\n\
    Slow        - Adjust rate of decay from about 1/2 second to 4 seconds.\n\
\n\
    Soft        - Provide a soft attack to each note.\n\
\n\
The soft attack is an attempt to reduce the level of undesired key noise. The\n\
keyboard consisted of a metal bar under each key that made physical contact \n\
with 9 sprung teeth to tap off the harmonics. The initial contact would generate\n\
noise that did not really accord to the pipe organ comparison. This was \n\
reduced by adding a slow start to each key, but the jazz musicians had used\n\
this defect to great effect, terming it 'key click' and it became a part of\n\
the Hammond characteristics. Some musicians would even brag about how noisy\n\
there organ was.\n\
\n\
On the left had side are three more controls:\n\
\n\
    Volume potentiometer\n\
\n\
    Options switch discussed below.\n\
\n\
    Rotary Speed: low/high speed Leslie rotation. Shifts between the speeds\n\
    are suppressed to emulate the spin up and down periods of the leslie motors.\n\
\n\
The options section, under control of the options button, has the parameters\n\
used to control the emulation. These are broken into sections and discussed\n\
individually.\n\
\n\
Leslie:\n\
\n\
The Leslie rotary speaker consisted of a large cabinet with a bass speaker and\n\
a pair of high frequency air horns. Each were mounted on its own rotating table\n\
and driven around inside the cabinet by motors. A crossover filter was used to\n\
separate the frequencies driven to either speaker. Each pair was typically \n\
isolated physically from the other. As the speaker rotated it would generate\n\
chorus type effects, but far richer in quality. Depending on where the speaker\n\
was with respect to the listener the sound would also appear to rotate. There\n\
would be different phasing effects based on signal reflections, different\n\
filtering effects depending on where the speaker was in respect to the cabinet\n\
producing differences resonances with respect to the internal baffling.\n\
\n\
    Separate:\n\
    Sync:\n\
    No Bass:\n\
        The Leslie had two motors, one for the horns and one for the voice coil\n\
        speaker. These rotated at different speeds. Some players preferred to \n\
        have both rotate at the same speed, would remove the second motor and\n\
        bind the spindles of each speaker table, this had the added effect\n\
        that both would also spin up at the same rate, not true of the \n\
        separated motors since each table had a very different rotary moment.\n\
        The 'No Bass' option does not rotate the voice coil speaker. This was\n\
        typically done since it would respond only slowly to speed changes,\n\
        this left just the horns rotating but able to spin up and down faster.\n\
\n\
    Brake:\n\
        Some cabinets had a brake applied to the tables such that when the\n\
        motor stopped the speakers slowed down faster.\n\
\n\
    X-Over:\n\
        This is the cross over frequency between the voice coil and air horns.\n\
        Uses a butterworth filter design.\n\
\n\
    Inertia:\n\
        Rate at which speaker rotational speed will respond to changes.\n\
\n\
    Overdrive:\n\
        Amount by which the amplifier is overdriven into distortion.\n\
\n\
    H-Depth/Frequency/Phase\n\
    L-Depth/Frequency/Phase\n\
        These parameters control the rotary phasing effect. The algorithm used\n\
        has three differently phased rotations used for filtering, phasing and\n\
        reverberation of the sound. These parameters are used to control the\n\
        depth and general phasing of each of them, giving different parameters\n\
        for the high and low speed rotations. There are no separate parameters\n\
        for the voice coil or air horns, these parameters are for the two\n\
        different speeds only, although in 'Separate' mode the two motors will\n\
        rotate at slightly different speeds.\n\
\n\
Chorus\n\
\n\
    V1/C1 - Lowest chorus speed\n\
    V2/C2 - Medium chorus speed\n\
    V3/C3 - High chorus speed\n\
\n\
Percussion:\n\
\n\
    Decay Fast/Slow - controls the percussive delay rates.\n\
    Attack Slow Fast - Controls the per note envelope attack time.\n\
\n\
The percussives are emulated as per the original design where there was a\n\
single envelope for the whole keyboard and not per note. The envelope will only\n\
restrike for a cleanly pressed note.\n\
\n\
Finally there are several parameters affecting the sine wave generation code.\n\
The Hammond used cogged wheels and coil pickups to generate all the harmonics,\n\
but the output was not a pure sine wave. This section primarily adjusts the\n\
waveform generation:\n\
\n\
    Preacher:\n\
        The emulator has two modes of operation, one is to generate the \n\
        harmonics only for each keyed note and another to generate all of\n\
        them and tap of those required for whatever keys have been pressed.\n\
        Both work and have different net results. Firstly, generating each\n\
        note independently is far more efficient than generating all 90 odd\n\
        teeth, as only a few are typically required. This does not have totally\n\
        linked phases between notes and cannot provide for signal damping (see\n\
        below).\n\
        The Preacher algorithm generates all harmonics continuously as per the\n\
        original instrument. It is a better rendition at the expense of large\n\
        chunks of CPU activity. This is discussed further below.\n\
\n\
    Compress:\n\
        Time compress the sine wave to produce a slightly sharper leading edge.\n\
\n\
    Bright:\n\
        Add additional high frequency harmonics to the sine.\n\
\n\
    Click:\n\
        Level of key click noise\n\
    \n\
    Reverb:\n\
        Amount of reverb added by the Leslie\n\
    \n\
    Damping:\n\
        If the same harmonic was reused by different pressed keys then its net\n\
        volume would not be a complete sum, the output gain would decay as the\n\
        pickups would become overloaded. This would dampen the signal strength.\n\
        This is only available with the Preacher algorithm.\n\
\n\
The two reverse octaves are presets as per the original, however here they can\n\
just be used to recall the first 23 memories of the current bank. The lower\n\
manual 12 key is the 'save' key for the current settings and must be double\n\
clicked. It should be possible to drive these keys via MIDI, not currently \n\
tested though. The default presets are a mixture of settings, the lower \n\
manual being typical 'standard' recital settings, the upper manual being a\n\
mixture of Smith, Argent, Emerson, Winwood and other settings from the well\n\
known Hammond Leslie FAQ. You can overwrite them. As a slight anomaly, which\n\
was intentional, loading a memory from the these keys only adjusts the visible\n\
parameters - the drawbars, leslie, etc. The unseen ones in the options panel\n\
do not change. When you save a memory with a double click on the lower manual\n\
last reverse key then in contrast it saves all the parameters. This will not\n\
change.\n\
\n\
The Preacher algorithm supports a diverse set of options for its tonewheel\n\
emulation. These are configured in the file $BRISTOL/memory/profiles/tonewheel\n\
and there is only one copy. The file is a text file and will remain that way,\n\
it is reasonably documented in the file itself. Most settings have two ranges,\n\
one representing the normal setting and the other the bright setting for when\n\
the 'bright' button is pressed. The following settings are currently available:\n\
\n\
    ToneNormal: each wheel can be given a waveform setting from 0 (square)\n\
        through to 1.0 (pure sine) to X (sharpening ramp).\n\
\n\
    EQNormal: each wheel can be given a gain level across the whole generator.\n\
\n\
    DampNormal: each wheel has a damping factor (level robbing/damping/stealing)\n\
\n\
    BusNormal: each drawbar can be equalised globally.\n\
\n\
\n\
    ToneBright: each wheel can be given a waveform setting from 0 (square)\n\
        through to 1.0 (pure sine) to X (sharpening ramp) for the bright button.\n\
\n\
    EQBright: each wheel can be given a gain level across the whole generator.\n\
\n\
    DampBright: each wheel has a damping factor (level robbing/damping/stealing)\n\
\n\
    BusBright: each drawbar can be equalised globally.\n\
\n\
\n\
    stops: default settings for the eight drawbar gain levels.\n\
\n\
        The default is 8 linear stages.\n\
\n\
    wheel: enables redefining the frequency and phase of any given tonewheel\n\
\n\
        The defaults are the slightly non Even Tempered frequencies of the\n\
        Hammond tonewheels. The tonewheel file redefines the top 6 frequencies\n\
        that were slightly more out of tune due to the 192-teeth wheels and\n\
        a different gear ratio.\n\
\n\
    crosstalk: between wheels in a compartment and adjacent drawbar busses.\n\
\n\
        This is one area that may need extensions for crosstalk in the wiring\n\
        loom. Currently the level of crosstalk between each of the wheels in\n\
        the compartment can be individually defined, and drawbar bus crosstalk\n\
        also.\n\
\n\
    compartment: table of the 24 tonewheel compartments and associated wheels.\n\
\n\
    resistors: tapering resister definitions for equalisation of gains per\n\
        wheel by note by drawbar.\n\
\n\
    taper: definition of the drawbar taper damping resistor values.\n\
\n\
Improvements would come with some other alterations to the sine waveforms and\n\
some more EQ put into the leslie speaker. The speaker has three speeds, two of\n\
which are configurable and the third is 'stopped'. Changes between the different\n\
rates is controlled to emulate inertia.\n\
\n\
The net emulation, at least of the preacher algorithm, is reasonable, it is\n\
distinctively a Hammond sound although it does not have quite as much motor\n\
or spindle noise. The Bright button gives a somewhat rawer gearbox. It could do\n\
with a better amplifier emulation for overdrive.\n\
\n\
The damping algorithms is not quite correct, it has dependencies on which keys\n\
are pressed (upper/lower manual). Options drop shadow is taken from the wrong\n\
background bitmap so appears in an inconsistent grey.\n\
\n",

"    Vox Continental\n\
    ---------------\n\
\n\
This emulates the original mark-1 Continental, popular in its time with the\n\
Animals on 'House of the Rising Sun', Doors on 'Light my Fire' and most of\n\
their other tracks. Manzarek did use Gibson later, and even played with the\n\
Hammond on their final album, 'LA Woman' but this organ in part defined\n\
the 60's sound and is still used by retro bands for that fact. The Damned\n\
used it in an early revival where Captain Sensible punched the keyboard\n\
wearing gloves to quite good effect. After that The Specials began the Mod/Ska\n\
revival using one. The sharp and strong harmonic content has the ability to\n\
cut into a mix and make its presence known.\n\
\n\
The organ was a british design, eventually sold (to Crumar?) and made into a\n\
number of plastic alternatives. Compared to the Hammond this was a fully \n\
electronic instrument, no moving parts, and much simpler. It had a very\n\
characteristic sound though, sharper and perhaps thinner but was far cheaper\n\
than its larger cousin. It used a master oscillator that was divided down to\n\
each harmonic for each key (as did the later Hammonds for price reasons). This\n\
oscillator division design was used in the first of the polyphonic synthesisers\n\
where the divided note was fead through individual envelope generators and\n\
a shared or individual filter (Polymoog et al).\n\
\n\
The Vox is also a drawbar instrument, but far simplified compared to the\n\
Hammond. It has 4 harmonic mixes, 16', 8' and 4' drawbars each with eight\n\
positions. The fourth gave a mix of 2 2/3, 2, 1 1/3 and 1 foot pipes.\n\
An additional two drawbars controlled the overall volume and waveforms, one\n\
for the flute or sine waves and another for the reed or ramp waves. The\n\
resulting sound could be soft and warm (flute) or sharp and rich (reed).\n\
\n\
There are two switches on the modulator panel, one for vibrato effect and one\n\
for memories and options. Options give access to an chorus effect rather \n\
than the simple vibrato, but this actually detracts from the qualities of the\n\
sound which are otherwise very true to the original.\n\
\n\
Vox is a trade name owned by Korg Inc. of Japan, and Continental is one of \n\
their registered trademarks. Bristol does not intend to infringe upon these\n\
registered names and Korg have their own remarkable range of vintage emulations\n\
available. You are directed to their website for further information of true\n\
Korg products.\n\
\n",

"    Fender Rhodes\n\
    -------------\n\
\n\
Again not an instrument that requires much introduction. This emulation is\n\
the DX-7 voiced synth providing a few electric piano effects. The design is \n\
a Mark-1 Stage-73 that the author has, and the emulation is reasonable if not\n\
exceptional. The Rhodes has always been widely used, Pink Floyd on 'Money',\n\
The Doors on 'Riders on the Storm', Carlos Santana on 'She's not There',\n\
everybody else in the 60's.\n\
\n\
The Rhodes piano generated its sound using a full piano action keyboard where\n\
each hammer would hit a 'tine', or metal rod. Next to each rod was a pickup\n\
coil as found on a guitar, and these would be linked together into the output.\n\
The length of each tine defined its frequency and it was tunable using a tight\n\
coiled spring that could be moved along the length of the tine to adjust its\n\
moment. The first one was built mostly out of aircraft parts to amuse injured\n\
pilots during the second world war. The Rhodes company was eventually sold to\n\
Fender and lead to several different versions, the Mark-2 probably being the\n\
most widely acclaimed for its slightly warmer sound.\n\
\n\
There is not much to explain regarding functionality. The emulator has a volume\n\
and bass control, and one switch that reveals the memory buttons and algorithm\n\
selector.\n\
\n\
The Rhodes would improve with the addition of small amounts of either reverb\n\
or chorus, potentially to be implemented in a future release.\n\
\n\
The Rhodes Bass was cobbled together largely for a presentation on Bristol.\n\
It existed and was used be Manzarek when playing with The Doors in\n\
Whiskey-a-GoGo; the owner specified that whilst the music was great they\n\
needed somebody playing the bass. Rather than audition for the part Manzarek\n\
went out and bought a Rhodes Bass and used it for the next couple of years.\n\
\n",

"    Sequential Circuits Prophet-10\n\
    ------------------------------\n\
\n\
The prophet 10 was the troublesome brother of the Pro-5. It is almost two\n\
Prophet-5 in one box, two keyboards and a layering capability. Early models\n\
were not big sellers, they were temperamental and liable to be temperature \n\
sensitive due to the amount of electronics hidden away inside. The original\n\
layering and 'unison' allowed the original to function as two independent\n\
synths, a pair of layered synths (both keyboards then played the same sound),\n\
as a monophonic synth in 'unison' mode on one keybaord with a second polyphonic\n\
unit on the other, or even all 10 voices on a single keyed note for a humongous\n\
20 oscillator monophonic monster.\n\
\n\
Phil Collins used this synth, and plenty of others who might not admit to it.\n\
\n\
The emulator uses the same memories as the Prophet-5, shares the same algorithm,\n\
but starts two synths. Each of the two synths can be seen by selecting the U/D\n\
(Up/Down) button in the programmer section. Each of the two synthesisers loads\n\
one of the Pro-5 memories.\n\
\n\
There was an added parameter - the Pan or balance of the selected layer, used\n\
to build stereo synths. The lower control panel was extended to select the\n\
playing modes:\n\
\n\
    Dual: Two independent keyboards\n\
    Poly: Play note from each layer alternatively\n\
    Layer: Play each layer simultaneously.\n\
\n\
In Poly and Layer mode, each keyboard plays the same sounds.\n\
\n\
    Mods: Select which of the Mod and Freq wheels control which layers.\n\
\n",

NULL, /* Mixer */
NULL, /* Sampler */
NULL, /* Bristol */
NULL, /* DDD */
NULL, /* Pro-52 - stuff later */

"    Oberheim OB-X\n\
    -------------\n\
\n\
Oberheim was the biggest competitor of Sequential Circuits, having their OB\n\
range neck and neck with each SC Prophet. The sound is as fat, the OB-X \n\
similar to the Prophet-5 as the OB-Xa to the Prophet-10. The synths were widely\n\
used in rock music in the late seventies and early 80s. Their early polyphonic\n\
synthesisers had multiple independent voices linked to the keyboard and were\n\
beast to program as each voice was configured independently, something that\n\
prevented much live usage. The OB-X configured all of the voices with the same\n\
parameters and had non-volatile memories for instant recall.\n\
\n\
Priced at $6000 upwards, this beast was also sold in limited quantities and\n\
as with its competition gained and maintained a massive reputation for rich,\n\
fat sounds. Considering that it only had 21 continuous controllers they were\n\
used wisely to build its distinctive and flexible sound.\n\
\n\
The general design again follows that of the Mini Moog, three oscillators with\n\
one dedicated as an LFO the other two audible. Here there is no mixer though,\n\
the two audible oscillators feed directly into the filter and then the amplifier.\n\
\n\
The richness of the sound came from the oscillator options and filter, the \n\
latter of which is not done justice in the emulator.\n\
\n\
Manual:\n\
\n\
    Volume\n\
    Auto: autotune the oscillators\n\
    Hold: disable note off events\n\
    Reset: fast decay to zero for envelopes, disregards release parameter.\n\
    Master Tune: up/down one semitone both oscillators.\n\
\n\
Control:\n\
\n\
    Glide: up to 30 seconds\n\
    Oscillator 2 detune: Up/down one semitone\n\
\n\
    Unison: gang all voices to a single 'fat' monophonic synthesiser.\n\
\n\
Modulation:\n\
\n\
    LFO: rate of oscillation\n\
    Waveform: Sine/Square/Sample&Hold of noise src. Triangle if none selected.\n\
\n\
    Depth: Amount of LFO going to:\n\
        Freq Osc-1\n\
        Freq Osc-2\n\
        Filter Cutoff\n\
\n\
    PWM: Amount of LFO going to:\n\
        PWM Osc-1\n\
        PWM Osc-2\n\
\n\
Oscillators:\n\
\n\
    Freq1: 32' to 1' in octave increments.\n\
    PulseWidth: Width of pulse wave (*).\n\
    Freq2: 16' to 1' in semitone increments.\n\
\n\
    Saw: sawtooth waveform Osc-1 (**)\n\
    Puls: Pulse waveform Osc-1\n\
\n\
    XMod: Osc-1 FW to Osc-2 (***)\n\
    Sync: Osc-2 sync to Osc-1\n\
\n\
    Saw: sawtooth waveform Osc-2\n\
    Puls: Pulse waveform Osc-2\n\
\n\
    * Although this is a single controller it acts independently on each of the\n\
    oscillators - the most recent to have its square wave selected will be\n\
    affected by this parameter allowing each oscillator to have a different\n\
    pulse width as per the original design.\n\
\n\
    ** If no waveform is selected then a triangle is generated.\n\
\n\
    *** The original synth had Osc-2 crossmodifying Osc-1, this is not totally\n\
    feasible with the sync options as they are not mutually exclusive here.\n\
    Cross modulation is noisy if the source or dest wave is pulse, something\n\
    that may be fixed in a future release.\n\
\n\
Filter:\n\
\n\
    Freq: cutoff frequency\n\
    Resonance: emphasis (*)\n\
    Mod: Amount of modulation to filter cutoff (**)\n\
\n\
    Osc-1: Osc-1 to cutoff at full swing.\n\
    KDB: Keyboard tracking of cutoff.\n\
\n\
    Half/Full: Oscillator 2 to Cutoff at defined levels (***)\n\
    Half/Full: Noise to Cutoff at defined levels (***)\n\
\n\
    * In contrast to the original, this filter can self oscillate.\n\
\n\
    ** The original had this parameter for the envelope level only, not the\n\
    other modifiers. Due to the filter implementation here it affects total\n\
    depth of the sum of the mods.\n\
\n\
    *** These are not mutually exclusive. The 'Half' button gives about 1/4,\n\
    the 'Full' button full, and both on gives 1/2. They could be made mutually\n\
    exclusive, but the same effect can be generated with a little more flexibility\n\
    here.\n\
\n\
Envelopes: One each for filter and amplifier.\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
The oscillators appear rather restricted at first sight, but the parametrics\n\
allow for a very rich and cutting sound.\n\
\n\
Improvements would be a fatter filter, but this can be argued of all the \n\
Bristol synthesisers as they all share the same design. It will be altered in\n\
a future release.\n\
\n\
The OB-X has its own mod panel (most of the rest share the same frequency and\n\
mod controls). Narrow affects the depth of the two controllers, Osc-2 will \n\
make frequency only affect Osc-2 rather than both leading to beating, or phasing\n\
effects if the oscillators are in sync. Transpose will raise the keyboard by\n\
one octave.\n\
\n\
Memories are quite simple, the first group of 8 buttons is a bank, the second\n\
is for 8 memories in that bank. This is rather restricted for a digital synth\n\
but is reasonably true to the original. If you want more than 64 memories let\n\
me know.\n\
\n",

"    Oberheim OB-Xa\n\
    --------------\n\
\n\
This is almost two OB-X in a single unit. With one keyboard they could provide\n\
the same sounds but with added voicing for split/layers/poly options. The OB-Xa\n\
did at least work with all 10 voices, had a single keyboard, and is renound for\n\
the sounds of van Halen 'Jump' and Stranglers 'Strange Little Girl'. The sound\n\
had the capability to cut through a mix to upstage even guitar solo's. Oberheim\n\
went on to make the most over the top analogue synths before the cut price\n\
alternatives and the age of the DX overcame them.\n\
\n\
Parameters are much the same as the OB-X as the algorithm shares the same code,\n\
with a few changes to the mod routing. The main changes will be in the use of\n\
Poly/Split/Layer controllers for splitting the keyboard and layering the sounds\n\
of the two integrated synthesisers and the choice of filter algorithm.\n\
\n\
The voice controls apply to the layer being viewed, selected from the D/U\n\
button.\n\
\n\
Manual:\n\
\n\
    Volume\n\
    Balance\n\
    Auto: autotune the oscillators\n\
    Hold: disable note off\n\
    Reset: fast decay to zero for envelopes, disregards release parameter.\n\
    Master Tune: up/down one semitone both oscillators.\n\
\n\
Control:\n\
\n\
    Glide: up to 30 seconds\n\
    Oscillator 2 detune: Up/down one semitone\n\
\n\
    Unison: gang all voices to a single 'fat' monophonic synthesiser.\n\
\n\
Modulation:\n\
\n\
    LFO: rate of oscillation\n\
    Waveform: Sine/Square/Sample&Hold of noise src. Triangle if none selected.\n\
\n\
    Depth: Amount of LFO going to:\n\
        Freq Osc-1\n\
        Freq Osc-2\n\
        Filter Cutoff\n\
\n\
    PWM: Amount of LFO going to:\n\
        PWM Osc-1\n\
        PWM Osc-2\n\
        Tremelo\n\
\n\
Oscillators:\n\
\n\
    Freq1: 32' to 1' in octave increments.\n\
    PulseWidth: Width of pulse wave (*).\n\
    Freq2: 16' to 1' in semitone increments.\n\
\n\
    Saw: sawtooth waveform Osc-1 (**)\n\
    Puls: Pulse waveform Osc-1\n\
\n\
    Env: Application of Filter env to frequency\n\
    Sync: Osc-2 sync to Osc-1\n\
\n\
    Saw: sawtooth waveform Osc-2\n\
    Puls: Pulse waveform Osc-2\n\
\n\
    * Although this is a single controller it acts independently on each of the\n\
    oscillators - the most recent to have its square wave selected will be\n\
    affected by this parameter allowing each oscillator to have a different\n\
    pulse width, as per the original design.\n\
\n\
    ** If no waveform is selected then a triangle is generated.\n\
\n\
Filter:\n\
\n\
    Freq: cutoff frequency\n\
    Resonance: emphasis (*)\n\
    Mod: Amount of modulation to filter cutoff (**)\n\
\n\
    Osc-1: Osc-1 to cutoff at full swing.\n\
    KDB: Keyboard tracking of cutoff.\n\
\n\
    Half/Full: Oscillator 2 to Cutoff at defined levels (***)\n\
\n\
    Noise: to Cutoff at defined levels\n\
    4 Pole: Select 2 pole or 4 pole filter\n\
\n\
    * In contrast to the original, this filter will self oscillate.\n\
\n\
    ** The original had this parameter for the envelope level only, not the\n\
    other modifiers. Due to the filter implementation here it affects total\n\
    depth of the sum of the mods.\n\
\n\
    *** These are not mutually exclusive. The 'Half' button gives about 1/4,\n\
     the 'Full' button full, and both on gives 1/2. They could be made mutually\n\
    exclusive, but the same effect can be generated with a little more flexibility\n\
    here.\n\
\n\
Envelopes: One each for filter and amplifier.\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
Mode selection:\n\
\n\
    Poly: play one key from each layer alternatively for 10 voices\n\
    Split: Split the keyboard. The next keypress specifies split point\n\
    Layer: Layer each voice on top each other.\n\
\n\
    D/U: Select upper and lower layers for editing.\n\
\n\
Modifier Panel:\n\
\n\
    Rate: Second LFO frequency or Arpeggiator rate (*)\n\
    Depth: Second LFO gain\n\
    Low: Modifiers will affect the lower layer\n\
    Up: Modifiers will affect the upper layer\n\
    Multi: Each voice will implement its own LFO\n\
    Copy: Copy lower layer to upper layer\n\
\n\
    Mod 01: Modify Osc-1 in given layer\n\
    Mod 02: Modify Osc-2 in given layer\n\
    PW: Modify Pulse Width\n\
    AMT: Amount (ie, depth) of mods and freq wheels\n\
\n\
    Transpose: Up or Down one octave.\n\
\n\
The Arpeggiator code integrated into release 0.20.4 has three main parts, the\n\
arpeggiator itself, the arpeggiating sequencer and the chording. All are \n\
configured from the left of the main panel.\n\
\n\
The arpeggiator is governed by the rate control that governs how the code\n\
steps through the available keys, an octave selector for 1, 2 or 3 octaves\n\
of arpeggiation, and finally the Up/Down/Up+Down keys - the last ones start\n\
the arpeggiator. Arpeggiation will only affect the lower layer.\n\
\n\
When it has been started you press keys on the keyboard (master controller\n\
or GUI) and the code will step through each note and octaves of each note \n\
in the order they were pressed and according to the direction buttons. The\n\
key settings are currently reset when you change the direction and you will\n\
have to press the notes again.\n\
\n\
The sequencer is a modification of the code. Select the Seq button and then \n\
a direction. The GUI will program the engine with a series of notes (that can\n\
be redefined) and the GUI will sequence them, also only into the lower layer.\n\
The sequence will only start when you press a key on the keyboard, this is \n\
the starting point for the sequence. You can press multiple notes to have \n\
them sequence in unison. Once started you can tweak parameters to control\n\
the sound and memory 88 when loaded has the filter envelope closed down, a\n\
bit of glide and some heavy mods to give you a starting point for some serious\n\
fun.\n\
\n\
To reprogram the sequence steps you should stop the sequencer, press the PRG\n\
button, then the Sequence button: enter the notes you want to use one by one\n\
on the keyboard. When finished press the sequence button again, it goes out.\n\
Now enable it again - select Seq and a direction and press a note. Press two\n\
notes.\n\
\n\
When you save the memory the OBXa will also save the  sequence however there\n\
is only one sequence memory - that can be changed if you want to have a sequence\n\
memory per voice memory (implemented in 0.20.4).\n\
\n\
The chord memory is similar to the Unison mode except that Unison plays all\n\
voices with the same note. Chording will assign one voice to each notes in\n\
the chord for a richer sound. To enable Chording press the 'Hold' button. This\n\
is not the same as the original since it used the hold button as a sustain\n\
option however that does not function well with a Gui and so it was reused.\n\
\n\
To reprogram the Chord memory do the following: press the PRG button then the\n\
Hold button. You can then press the keys, up to 8, that you want in the chord,\n\
and finally hit the Hold button again. The default chord is just two notes, \n\
the one you press plus its octave higher. This results in multiple voices\n\
per keypress (a total of 3 in Layered mode) and with suitable detune will \n\
give a very rich sound.\n\
\n\
There is only one arpeggiator saved for all the memories, not one per memory\n\
as with some of the other implementations. Mail me if you want that changed.\n\
\n\
\n\
\n\
The oscillators appear rather restricted at first sight, but the parametrics\n\
allow for a very rich and cutting sound.\n\
\n\
The Copy function on the Mod Panel is to make Poly programming easier - generate the desired sound and then copy the complete parameter set for poly operation. \n\
It can also be used more subtly, as the copy operation does not affect balance\n\
or detune, so sounds can be copied and immediately panned slightly out of tune to generate natural width in a patch. This is not per the original instrument\n\
that had an arpeggiator on the mod panel.\n\
\n\
The Arpeggiator was first integrated into the OBXa in release 0.20.4 but not\n\
widely tested.\n\
\n",

NULL, /* Rhodes Bass */

"    KORG MONOPOLY\n\
    -------------\n\
\n\
A synth suite would not be complete without some example of a Korg instrument,\n\
the company was also pivotal in the early synthesiser developments. This is\n\
an implementation of their early attempts at polyphonic synthesis, it was\n\
either this one or the Poly-6 (which may be implemented later). Other choices\n\
would have been the MS series, MS-20, but there are other synth packages that\n\
do a better job of emulating the patching flexibility of that synth - Bristol\n\
is more for fixed configurations.\n\
\n\
As with many of the Korg synths (the 800 worked similarly) this is not really\n\
true polyphony, and it is the quirks that make it interesting. The synth had\n\
four audio oscillators, each independently configurable but which are bussed\n\
into a common filter and envelope pair - these are not per voice but rather\n\
per instrument. The unit had different operating modes such that the four\n\
oscillators can be driven together for a phat synth, independently for a form\n\
of polyphony where each is allocated to a different keypress, and a shared\n\
mode where they are assigned in groups depending on the number of keys pressed.\n\
For example, if only 2 notes are held then each key is sounded on two different\n\
oscillators, one key is sounded on all 4 oscillators, and 3 or more have one\n\
each. In addition there are two LFOs for modulation and a basic effects option\n\
for beefing up the sounds. To be honest to the original synth, this emulation\n\
will only request 1 voice from the engine. Korg is one of the few original\n\
manufacturers to have survived the transition to digital synthesis and are\n\
still popular.\n\
\n\
One thing that is immediately visible with this synth is that there are a lot\n\
of controllers since each oscillator is configured independently. This is in\n\
contrast to the true polyphonic synths where one set of controls are given to\n\
configure all the oscillators/filters/envelopes. The synth stages do follow the\n\
typical synth design, there are modulation controllers and an FX section\n\
feeding into the oscillators and filter. The effects section is a set of\n\
controllers that can be configured and then enabled/disabled with a button\n\
press. The overall layout is rather kludgy, with some controllers that are\n\
typically grouped being dispersed over the control panel.\n\
\n\
Control:\n\
\n\
    Volume\n\
\n\
    Arpeg:\n\
        Whether arpegiator steps up, down, or down then up. This works in\n\
        conjunction with the 'Hold' mode described later.\n\
\n\
    Glide: glissando note to note. Does not operate in all modes\n\
\n\
    Octave: Up/Normal/Down one octave transpose of keyboard\n\
\n\
    Tune: Global tuning of all oscillators +/- 50 cents (*)\n\
    Detune: Overall detuning of all oscillators +/- 50 cents (*)\n\
\n\
    * There is an abundance of 'Tune' controllers. Global Tuning affects all\n\
    the oscillators together, then oscillators 2, 3 and 4 have an independent\n\
    tune controller, and finally there is 'Detune'. The target was to tune all\n\
    the oscillators to Osc-1 using the independent Tune for each, and then use\n\
    the global Tune here to have the synth tuned to other instruments. The\n\
    Detune control can then be applied to introduce some beating between the\n\
    oscillators to fatten the sound without necessarily losing overall tune of\n\
    the instrument.\n\
\n\
Modulation wheels:\n\
\n\
    Bend:\n\
        Intensity: Depth of modulation\n\
        Destination:\n\
            VCF - Filter cutoff\n\
            Pitch - Frequency of all oscillators\n\
            VCO - Frequency of selected oscillators (FX selection below).\n\
\n\
    MG1: Mod Group 1 (LFO)\n\
        Intensity: Depth of modulation\n\
        Destination:\n\
            VCF - Filter cutoff\n\
            Pitch - Frequency of all oscillators\n\
            VCO - Frequency of selected oscillators (FX selection below).\n\
\n\
LFO:\n\
\n\
    MG-1:\n\
        Frequency\n\
        Waveform - Tri, +ve ramp, -ve ramp, square.\n\
\n\
    MG-2:\n\
        Frequency (Triangle wave only).\n\
\n\
Pulse Width Control:\n\
\n\
    Pulse Width Modulation:\n\
        Source - Env/MG-1/MG-2\n\
        Depth\n\
    \n\
    Pule Width\n\
        Width control\n\
    \n\
    These controllers affect Osc-1 though 4 with they are selected for either\n\
    square of pulse waveforms.\n\
\n\
Mode:\n\
\n\
    The Mono/Poly had 3 operating modes, plus a 'Hold' option that affects \n\
    each mode:\n\
\n\
        Mono: All oscillators sound same key in unison\n\
        Poly: Each oscillator is assigned independent key - 4 note poly.\n\
        Share: Dynamic assignment:\n\
            1 key - 4 oscillators = Mono mode\n\
            2 key - 2 oscillators per key\n\
            3/4   - 1 oscillator per key = Poly mode\n\
\n\
    The Hold function operates in 3 different modes:\n\
\n\
        Mono: First 4 keypresses are memorised, further notes are then chorded\n\
            together monophonically.\n\
        Poly:\n\
            Notes are argeggiated in sequence, new note presses are appended\n\
            to the chain. Arpeggiation is up, down or up/down.\n\
        Share:\n\
            First 4 notes are memorised and are then argeggiated in sequence,\n\
            new note presses will transpose the arpeggiation. Stepping is up,\n\
            down or up/down.\n\
\n\
    There are several controllers that affect arpeggation:\n\
\n\
        Arpeg - direction of stepping\n\
        MG-2 - Frequency of steps from about 10 seconds down to 50 bps.\n\
        Trigger - Multiple will trigger envelopes on each step.\n\
\n\
Effects:\n\
\n\
    There are three main effects, or perhaps rather modulations, that are\n\
    controlled in this section. These are vibrato, crossmodulated frequency\n\
    and oscillator synchronisation. The application of each mod is configured\n\
    with the controllers and then all of them can be enabled/disabled with\n\
    the 'Effects' button. This allows for big differences in sound to be \n\
    applied quickly and simply as a typical effect would be. Since these mods\n\
    apply between oscillators it was envisaged they would be applied in Mono\n\
    mode to further fatten the sound, and the Mono mode is actually enabled when\n\
    the Effects key is selected (as per the original instrument). The Mode can\n\
    be changed afterwards for Effects/Poly for example, and they work with the\n\
    arpeggiation function.\n\
\n\
    X-Mod: frequency crossmodulation between oscillators\n\
    Freq: frequency modulation by MG-1 (vibrato) or Envlope (sweep)\n\
\n\
    Mode:\n\
        Syn: Oscillators are synchronised\n\
        X-M: Oscillators are crossmodulated\n\
        S-X: Oscillators are crossmodulated and synchronised\n\
\n\
    SNG:\n\
        Single mode: synth had a master oscillator (1) and three slaves (2/3/4)\n\
    DBL:\n\
        Double mode: synth had two master (1/3) and two slaves (2/4)\n\
\n\
    The overall FX routing depends on the SNG/DBL mode and the selection of\n\
    Effects enabled or not according to the table below. This table affects \n\
    the FX routing and the modulation wheels discussed in the LFO section above:\n\
\n\
                     --------------------------------------------------\n\
                     |    FX OFF    |              FX ON              |\n\
                     |              |----------------------------------\n\
                     |              |    Single       |     Double    |\n\
      ---------------+--------------+-----------------+---------------|\n\
      | VCO-1/Slave  |    VCO-1     |    VCO 2/3/4    |     VCO 2/4   |\n\
      |              |              |                 |               |\n\
      | Pitch        |    VCO 1-4   |    VCO 1-4      |     VCO 1-4   |\n\
      |              |              |                 |               |\n\
      | VCF          |    VCF       |    VCF          |     VCF       |\n\
      -----------------------------------------------------------------\n\
\n\
    So, glad that is clear. Application of the modulation wheels to Pitch and\n\
    VCF is invariable when they are selected. In contrast, VCO/Slave will have\n\
    different destinations depending on the Effects, ie, when effects are on\n\
    the modwheels will affect different 'slave' oscillators.\n\
\n\
\n\
Oscillators:\n\
\n\
    Each oscillator had the following controllers:\n\
\n\
        Tune (*)\n\
        Waveform: Triangle, ramp, pulse, square (**)\n\
        Octave: Transpose 16' to 2'\n\
        Level: output gain/mix of oscillators.\n\
\n\
        * Osc-1 tuning is global\n\
        ** width of pulse and square wave is governed by PW controller. The\n\
        modulation of the pulse waveform is then also controlled by PWM.\n\
\n\
Noise:\n\
\n\
    Level: white noise output gain, mixed with oscillators into filter.\n\
\n\
VCF:\n\
\n\
    Freq:\n\
        Cutoff frequency\n\
\n\
    Res:\n\
        Resonance/emphasis.\n\
    \n\
    Env:\n\
        Amount of contour applied to cutoff\n\
    \n\
    KBD:\n\
        Amount of key tracking applied.\n\
\n\
ADSR: Two: filter/PWM/FX, amplifier\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
    Trigger:\n\
        Single: Trigger once until last key release\n\
        Multi: Trigger for each key or arpeggiator step.\n\
\n\
    Damp:\n\
        Off: Notes are held in Poly/Share mode until last key is released.\n\
        On: Oscillators are released as keys are released.\n\
\n\
This is more a synth to play with than describe. It never managed to be a true\n\
blue synth perhaps largely due to its unusual design: the quasi-poly mode was\n\
never widely accepted, and the effects routing is very strange. This does make\n\
it a synth to be tweaked though.\n\
\n\
Some of the mod routings do not conform to the original specification for the\n\
different Slave modes. This is the first and probably the only bristol synth that\n\
will have an inbuilt arpeggiator. The feature was possible here due to the mono\n\
synth specification, and whilst it could be built into the MIDI library for\n\
general use it is left up to the MIDI sequencers (that largely came along to \n\
replace the 1980s arpeggiators anyway) that are generally availlable on Linux.\n\
[Other instruments emulated by bristol that also included arpeggiation but do\n\
not have in the emulation were the Juno-6, Prophet-10, Oberheim OB-Xa, Poly6].\n\
\n\
As of May 06 this synth was in its final stages of development. There are a few\n\
issues with Tune and Detune that need to be fixed, and some of the poly key\n\
assignment may be wrong.\n\
\n",

"    KORG POLY 6\n\
    -----------\n\
\n\
Korg in no way endorses this emulation of their classic synthesiser and have\n\
their own emulation product that gives the features offered here. Korg,\n\
Mono/Poly, Poly-6, MS-20, Vox and Continental are all registered names or\n\
trademarks of Korg Inc of Japan.\n\
\n\
Quite a few liberties were taken with this synth. There were extremely few  \n\
differences between the original and the Roland Juno 6, they both had one osc  \n\
with PWM and a suboscillator, one filter and envelope, a chorus effect, and  \n\
inevitably both competed for the same market space for their given price. To  \n\
differentiate this algorithm some alterations were made. There are two separate\n\
envelopes rather than just one, but the option to have a gated amplifier is  \n\
still there. In addition glide and noise were added, both of which were not in  \n\
the original instrument. With respect to the original instrument this was  \n\
perhaps not a wise move, but there seemed little point in making another Juno  \n\
with a different layout. The net results is that the two synths do sound quite  \n\
different. The emulation does not have an arpeggiator.  \n\
 \n\
    Volume: Master volume of the instrument  \n\
 \n\
    Glide: length of portamento  \n\
 \n\
    Tune: Master tuning of instrument  \n\
 \n\
    Bend: Amount of pitch wheel that is applied to the oscillators frequency.  \n\
 \n\
 \n\
    VCO section:  \n\
\n\
        Octave: What octave the instrument's keyboard is in.  \n\
 \n\
        Wave: Waveform selection: Triangle, Saw, Pulse and Pulsewidth  \n\
 \n\
        PW PWM: Amount of Pulsewidth (when Pulse is selected) and Pulsewidth\n\
            Modulation (When Pulsewidth is selected).  \n\
 \n\
        Freq: Frequency of PW/PWM  \n\
 \n\
        OFF/SUB1/SUB2; Adds a square sub-oscillator either off, 1 or 2 octaves\n\
            down from a note.  \n\
 \n\
    MG (Modulation Group):  \n\
 \n\
        Freq: Frequency of LFO  \n\
\n\
        Delay: Amount of time before the LFO affects the destination when a key\n\
            is pressed.  \n\
        Level: How strongly the LFO affects the destination  \n\
 \n\
        VCO/VCF/VCA: Destinations that the LFO can go to:  \n\
 \n\
            VCO: The Voltage Controlled Oscillator:\n\
                Affects oscillator pitch, producing vibrato  \n\
 \n\
            VCF: The Voltage Controlled Filter:\n\
                Affects Filter, producing a wah effect  \n\
 \n\
            VCA: The Voltage Controlled Amplifier:\n\
                Affects the Amplifier, producing tremolo  \n\
 \n\
    VCF section:  \n\
 \n\
        Freq: Cut off frequency of the filter  \n\
 \n\
        Res: Resonance of the filter  \n\
 \n\
        Env: By how much the filter is affected by the envelope.  \n\
 \n\
        Kbd: How much Keyboard tracking is applied to the envelope. note:\n\
\n\
            A low setting doesn't allow the filter to open, making the notes\n\
            seem darker the further you go up the keyboard.  \n\
 \n\
    Hold: prevent key off events  \n\
 \n\
    Mono Mode: Gang all voices to a single 'fat' monophonic synthesiser.  \n\
 \n\
    Poly: One voice per note.  \n\
 \n\
    Envelope Section:  \n\
 \n\
        Top:  \n\
 \n\
        Filter envelope: \n\
 \n\
            Attack: Amount of time it takes the filter to fully open.\n\
                A high value can produce a 'sweeping filter' effect. \n\
            Decay: Amount of time it takes for the filter to close to\n\
                the sustain level \n\
            Sustain: Amount of filter that is sustained when a key is held \n\
\n\
            Release: Amount of time it takes for the filter envelope to stop\n\
                affecting the filter. Combining a low filter release with a\n\
                high amplitude release time can cause an interesting effect. \n\
 \n\
        Bottom:  \n\
 \n\
        Amplitude envelope:  \n\
 \n\
        Attack: Amount of time it takes for the signal to reach its peak. \n\
\n\
        Decay: Amount of time it takes for the signal to drop to the\n\
            sustain level \n\
        Sustain: How quickly the sound decays to silence. \n\
\n\
        Release: How long it takes the sound to decay to silence after\n\
            releasing a key. \n\
 \n\
    VCA:  \n\
\n\
        Env: When on, this causes the Amplitude envelope to affect the sound.\n\
            I.E, If you have a long attack time, you get a long attack time. \n\
        Gate: When on, this causes the Amplitude envelope only (not the filter\n\
            envelope) to be be bypassed.  \n\
        Gain: Gain of signal. \n\
 \n\
    Effects Section:  \n\
\n\
        0: No effects  \n\
        1: Soft Chorus  \n\
        2: Phaser  \n\
        3: Ensemble  \n\
 \n\
        Intensity: How much the effects affect the output. \n\
\n\
There are some mildly anomolous effects possible from the MG section, especially\n\
with the VCA. The MG and the env are summed into the VCA which means if the env\n\
decays to zero then the LFO may end up pumping the volume, something that may\n\
be unexpected. Similarly, if the LFO is pumping and the voice finally stops its\n\
cycle then the closing gate may cause a pop on the MG signal. These can be \n\
resolved however the current behavious is probably close to the original.\n\
\n\
Bristol thanks Andrew Coughlan for patches, bug reports, this manual page and\n\
diverse suggestions to help improve the application.\n\
\n\
Korg in no way endorses this emulation of their classic synthesiser and have  \n\
their own emulation product that gives the features offered here. Korg,  \n\
Mono/Poly, Poly-6, MS-20, Vox and Continental are all registered names or  \n\
trademarks of Korg Inc of Japan.\n\
\n",

"    ARP AXXE\n\
    --------\n\
\n\
TBD\n\
\n\
At the risk of getting flamed, this is potentially the ugliest synth ever made,\n\
although the competition is strong. It was implemented as a build up to the far\n\
more useful ARP 2600 to understand the ARP components and their implementation.\n\
\n\
The implementation is a giveaway written during a week long business trip to \n\
Athens to keep me busy in the hotel. Its design lead on to the Odyssey and that\n\
was the step towards the final big brother, the ARP 2600.\n\
\n",

"    ARP ODYSSEY\n\
    -----------\n\
\n\
Ring modulation is correct here, it is a multiplier. This deviates from the\n\
original instrument that used an XOR function on the pulsewave outputs of the\n\
two oscillators. The implementation has two models, Mark-I and Mark-II. These\n\
implement different filters as per the original. Although their characteristics\n\
are different it is not suggested they are a particularly close emulation of\n\
the original.\n\
\n\
TBD\n\
\n",

"    Memory Moog\n\
    -----------\n\
\n\
TBD.\n\
\n\
This is actually a lot warmer than the Mini emulator, largely due to being\n\
later code. The mini should be revisited but I am saving that pleasure for when\n\
some more filters are available. [This was done during the 0.20 stream using the\n\
Houvilainen filters and bandwidth limited oscillators to produce a far richer\n\
sound. Also incorporate a number of fixes to the emulation stages.].\n\
\n",

"    ARP 2600\n\
    --------\n\
\n\
This synth will probably never get a writeup, it is kind of beyond the scope of\n\
this document. There are some discrepancies with the original:\n\
\n\
The filters do not self oscillate, they require an input signal. The output\n\
stage is global to all voices so cannot be patched back into the signal path.\n\
The original did not have a chorus, only a spring reverb. The input stage has\n\
not been tested for either signal nor envelope following code. The voltage\n\
manipulators were not in the first bristol upload with this emulation (-60), \n\
but a future release will include mixing inverters, a lag processor, and\n\
possibly also a Hz->V extractor. The unit has an extra LFO where the original\n\
had just a clock trigger circuit, it produces a TRI wave, can be used to\n\
trigger the AR envelope and be used for modulation. The electroswitch is\n\
unidirectional, two inputs switchable to one output. The sample and hold \n\
circuit cannot accept an external clock. The Keyboard inputs to the VCO cannot\n\
accept and alternative signal other than the MIDI note with tracking of this \n\
note either enabled or disabled.\n\
\n\
The rest works, ie, all the VCO/VCF/VCA/ENV/AMP and any of the 30 or so outputs\n\
can be repatched into any of the 50 or so inputs. Patches cause no overhead in\n\
the engine as it uses default buffering when not repatched, so feel free to put\n\
in as many cables as you can fit. Patches in the GUI still demand a lot of CPU\n\
cycles. Release -77 improved this about 5-fold and further improvements are in\n\
the pipeline: the 0.10 stream implemented color caching and XImage graphics\n\
interface which massively improved GUI performance.\n\
\n",

NULL, /* SAKs */
NULL, /* MS-20 */
NULL, /* Solina */
NULL, /* RoadRunner */
NULL, /* Granular */

"    REALISTIC MG-1 CONCERTMATE\n\
    --------------------------\n\
\n\
This is a pimpy little synth. It was sold through the Realistic electronics \n\
chain, also known as Radio Shack (and as Tandy, in the UK at least). It was\n\
relatively cheap but had a design from Moog Music (from after Robert Moog\n\
had left?) including the patented ladder filter. It consisted of a monophonic\n\
synth, dual oscillator, lfo, noise, filter, env, and a ring modulator. On top\n\
of that there was an organ circuit to give 'polyphony'. It was not really\n\
polyphonic although different descriptions will tell you it had 10 voices. \n\
These write-ups are by people who probably only had 10 fingers, the truth is\n\
that the organ circuit was as per any other - it had a master oscillator at\n\
about 2MHz and this was divided using binary counters to deliver a frequency\n\
for every note. The output of the 'poly' section was lamentable at best, it is\n\
a fairly pure square wave passed through the filter and contour. This is fully\n\
emulated although in addition to the contour bristol implements a per note\n\
envelope just to groom the note - this prevents ticks when new keys are pressed\n\
with the mono envelope fully open. There is no access to this env, it just has\n\
fast attack and decay times to smooth the signal and is preconfigured by the\n\
user interface on startup.\n\
\n\
The mono section is reasonably fun, the oscillators can be synchronised and\n\
there is a ring modulator so the range of sounds is quite wide. The emulator\n\
uses a chaimberlain filter so is not as warm as the Moog ladder filters.\n\
\n\
The list of people who used this is really quite amazing. The promotion for\n\
the product had Elton John holding one up in the air, although seeing as he\n\
probably already had every other synth known to man, holding it up in the\n\
air is likely to be all he ever did with it. Who knows how much they had to\n\
pay him to do it - the photo was nice though, from the days when Elton was\n\
still bald and wearing ridiculously oversized specs.\n\
\n\
Tuning:\n\
\n\
    One control each for the poly oscillator and mono oscillators\n\
\n\
Glide:\n\
\n\
    Only affects the monophonic oscillators.\n\
\n\
Modulation:\n\
\n\
    One LFO with rate and waveshape selection\n\
        produces tri, square and S/H signals.\n\
        can trigger the envelope\n\
    One noise source.\n\
    The modulation can be directed to:\n\
        Oscillators for vibrato\n\
        Filter for wah-wah effects\n\
\n\
Oscillator-1:\n\
\n\
    Tri or square wave\n\
    Octave from -2 to 0 transposition\n\
    Sync selector (synchronises Osc-2 to Osc-1)\n\
\n\
Oscillator-2:\n\
\n\
    Tri or pulse wave\n\
    Detune. This interoperates with the sync setting to alter harmonics\n\
    Octave from -1 to +1 transposition\n\
\n\
Contour: This is not an ADSR, rather an AR envelope\n\
\n\
    Sustain: AR or ASR envelope selector.\n\
    Tracking: controls mono oscillators\n\
        Envelope control\n\
        Key tracking (gate, no env)\n\
        Continuous (always on)\n\
    Rise (attack time)\n\
    Fall (release time)\n\
\n\
Filter:\n\
\n\
    Cutoff frequency\n\
    Emphasis\n\
    Contour depth\n\
    Keyboard tracking off, 1/2, full.\n\
\n\
Mixer: Levels for\n\
    Mono Osc-1\n\
    Mono Osc-2\n\
    Noise\n\
    RingMod of the mono oscillators (called 'bell').\n\
    Poly Osc level.\n\
\n\
Master Volume control.\n\
\n\
One extra button was added to save the current settings. For the rest the \n\
controls reflect the simplicity of the original. The implementation is a single\n\
synth, however due to the engine architecture having a pre-operational routine,\n\
a post-operational routine and an operate(polyphonic emulator) the emulation\n\
executes the mono synth in the pre- and post- ops to be mono, these are called\n\
just once per cycle. The poly synth is executed in the operate() code so is \n\
polyphonic. This leads to one minor deviation from the original routing in\n\
that if you select continuous tone controls then you will also hear one note\n\
from the poly section. This is a minor issue as the poly oscillator can be\n\
zeroed out in the mixer.\n\
\n\
It is noted here that this emulation is just a freebie, the interface is kept\n\
simple with no midi channel selection (start it with the -channel option and\n\
it stays there) and no real memories (start it with the -load option and it\n\
will stay on that memory location). There is an extra button on the front\n\
panel (a mod?) and pressing it will save the current settings for next time\n\
it is started. I could have done more, and will if people are interested, but\n\
I built it since the current developments were a granular synth and it was\n\
hard work getting my head around the grain/wave manipulations, so to give \n\
myself a rest I put this together one weekend. The Rhodesbass and ARP AXXE\n\
were done for similar reasons. I considered adding another mod button, to make\n\
the mono section also truly polyphonic but that kind of detracts from the\n\
original. Perhaps I should put together a Polymoog sometime that did kind of\n\
work like that anyway.\n\
\n\
This was perhaps a strange choice, however I like the way it highlights the\n\
difference between monophonic, polyphonic and 'neopolyphonic' synthesised\n\
organs (such as the polymoog). Its a fun synth as well, few people are likely\n\
to every bother buying one as they cost more now than when they were produced\n\
due to being collectable: for the few hundred dollars they would set you back\n\
on eBay you can get a respectable polyphonic unit.\n\
So here is an emulator, for free, for those who want to see how they worked.\n\
\n",

"    Vox Continental 300\n\
    -------------------\n\
\n\
There is an additional emulator for the later Mark-II, Super, 300 or whatever\n\
model you want to call it. This is probably closest to the 300. It was a \n\
dual manual organ, the lower manual is a Continental and the upper manual had\n\
a different drawbar configuration, using 8', 4' and 2', another two compound\n\
drawbars that represented 5-1/3'+1-3/5', and 2-2/3'+2'+1' respectively. This\n\
gave upper manual a wider tonic range, plus it had the ability to apply some\n\
percussive controls to two of the drawbars. Now, depending on model, some of \n\
these values could be different and bristol does not emulate all the different\n\
combinations, it uses the harmonics described above and applies percussive to\n\
the 2' and 5-1/3' harmonics (which is arguably incorrect however it gives\n\
a wider combination of percussive harmonics).\n\
\n\
The percussive has 4 controls, these are selectors for the harmonics that will\n\
be driven through the percussive decay (and then no longer respond to the \n\
drawbars), a decay rate called 'L' which acts as a Longer decay when selected,\n\
and a volume selector called 'S' which stands for Soft. The variables are \n\
adjustable in the mods section. The mods panel is intended to be hidden as\n\
they are just variable parameters. On the original units these were PCB mounted\n\
pots that were not generally accessible either. The panel is visible when you\n\
turn the power control off, not that I suppress the keyboard or anything when\n\
the power is off, but it gave me something useful do to with this button. The\n\
transparency layer is fixed here and is used to apply some drop shadow and a\n\
few beer spills on the cover.\n\
\n\
There is an additional Bass section for those who bought the optional Bass\n\
pedals (my old one had them). The emulation allow the selection of Flute and\n\
Reed strengths, and to select 8' or 8'/16' harmonics. The 'Sustain' control\n\
does not currently operate (0.10.9) but that can be fixed if people request\n\
the feature.\n\
\n\
The lower manual responds to the MIDI channel on which the emulation was \n\
started. The upper manual responds to notes greater than MIDI key 48 on the\n\
next channel up. The Bass section also responds to this second channel on keys\n\
lower than #48. Once started you cannot change the midi channel - use the \n\
'-channel' option at startup to select the one you want. The actual available\n\
max is 15 and that is enforced.\n\
\n\
The emulation only contains 6 available presets and a 'Save' button that you \n\
need to double click to overwrite any preset. The emulation actually uses \n\
banks, so if you started it with '-load 23' it would start up by selecting\n\
bank 20, and load memory #3 from that bank. Any saved memories are also then\n\
written back to bank 20, still with just 6 memories accessible 20-25. You can\n\
access more via MIDI bank select and program change operations if suitably\n\
linked up.\n\
\n\
Vox is a trade name owned by Korg Inc. of Japan, and Continental is one of \n\
their registered trademarks. Bristol does not intend to infringe upon these\n\
registered names and Korg have their own remarkable range of vintage emulations\n\
available. You are directed to their website for further information of true\n\
Korg products.\n\
\n",

"    Roland Jupiter 8\n\
    ----------------\n\
\n\
This emulator is anticipated in 0.20.4.\n\
\n\
The Jupiter synthesizers were the bigger brothers of the Juno series: their\n\
capabilities, sounds and prices were all considerably larger. This is the\n\
larger of the series, the others being the -4 and -6. The -6 and the rack\n\
mounted MKS-80 both came out after the Jupiter-8 and had somewhat more flexible\n\
features. Several of these have been incorporated into the emulation and that\n\
is documented below.\n\
\n\
A quick rundown of the synth and emulation:\n\
\n\
The synth runs as two layers, each of which is an independent emulator running\n\
the same algorithm, both layers are controlled from the single GUI. The layers\n\
are started with a set of voices, effectively 4+4 per default however bristol\n\
plays with those numbers to give the split/layer at 4 voices each and the 'All'\n\
configuration with 8 voices - it juggles them around depending on the Poly \n\
mode you select. You can request a different number of voices and the emulator \n\
will effectively allocate half the number to each layer. If you request 32\n\
voices you will end up with 4+4 though since 32 is interpreted as the default.\n\
\n\
The Poly section is used to select between Dual layers, Split keyboard and the\n\
8voice 'All' mode. You can redefine the split point with a double click on the\n\
'Split' button and then pressing a key. If you have linked the GUI up to the\n\
MIDI you should be able to press a key on your master keyboard rather than on\n\
the GUI.\n\
\n\
After that you can select the layer as upper or lower to review the parameter \n\
settings of each layer: they are as follows:\n\
\n\
LFO:\n\
    Frequency\n\
    Delay\n\
    Waveform (tri, saw, square, s&h)\n\
\n\
VCO Mods:\n\
    LFO and ENV-1\n\
    Destination to modulate frequency of DCO1, DCO2 or both.\n\
\n\
PWM:\n\
    PW\n\
    PWM\n\
    Modified by Env-1 or LFO\n\
\n\
DCO-1:\n\
    Crossmod (FM) from DCO2 to DCO1\n\
    Modified by Env-1\n\
\n\
    Octave range 16' to 2' (all mixable)\n\
    Waveform: Tri, saw, pulse, square (all mixable)\n\
\n\
DCO-2:\n\
    Sync (1->2 or 2->1 or off)\n\
    Range: 32' to 2' (all mixable)\n\
    Tuning\n\
    Waveform: tri, saw, pulse, noise (all mixable)\n\
\n\
Mix:\n\
    Osc 1 and Osc 2\n\
\n\
HPF:\n\
    High pass filter of signal\n\
\n\
Filter:\n\
    Cutoff\n\
    Emphasis\n\
    LPF/BPF/HPF\n\
    Env modulation\n\
    Source from Env-1 or Env-2\n\
    LFO mod amount\n\
    Keyboard tracking amount\n\
\n\
VCA:\n\
    Env-2 modulation\n\
    LFO modulation\n\
\n\
ADSR-1:\n\
    A/D/S/R\n\
    Keyboard tracking amount\n\
    Invert\n\
\n\
ADSR-2:\n\
    A/D/S/R\n\
    Keyboard tracking amount\n\
\n\
Pan:\n\
    Stereo panning of layer\n\
\n\
All of the above 40 or so parameters are part of the layer emulation and are\n\
separately configurable.\n\
\n\
The keyboard can operate in several different modes which are selectable from\n\
the Poly and Keyboard mode sections. The first main one is Dual, Split and All.\n\
Dual configure the two synth layers to be placed on top of each other. Split\n\
configures them to be next to each other and by double clicking the split\n\
button you can then enter a new split point by pressing a key. The All setting\n\
gives you a single layer with all 8 voices active. These settings are for the\n\
whole synth.\n\
\n\
The Poly section provides different playing modes for each layer independently.\n\
There are 3 settings: Solo give the layer access to a single voice for playing\n\
lead solos. Unison give the layer however many voices it is allowed (8 if in \n\
the All mode, 4 otherwise) and stacks them all on top of each other. This is\n\
similar to Solo but with multiple voices layered onto each other. Unison is \n\
good for fat lead sounds, Solo better for mono bass lines where Unison might\n\
have produced unwanted low frequency signal phasing. The third option is Poly\n\
mode 1 where the synth allocates a single voice to each key you press. The\n\
original also had Poly mode 2 which was not available at the first bristol\n\
release - this mode would apply as many notes as available to the keys you\n\
pressed: 1 key = 8 voices as in Unison, with 2 keys pressed each would get 4\n\
voices each, 4 keys pressed would get 2 voices and mixtures in there for other\n\
key combinations. This may be implemented in a future release but it is a\n\
rather left field option and would have to be put into the MIDI library that\n\
controls the voice assignments.\n\
\n\
The arpeggiator integrated into bristol is a general design and will differ\n\
from the original. The default settings are 4 buttons controlling the range\n\
of the arpeggiation, from 1 to 4 octaves, 4 buttons controlling the mode as\n\
Up, Down, Up+Down or Random sequencing of the notes available, and 4 notes\n\
that are preloaded into the sequence.\n\
\n\
Finally there are two global controls that are outside of the memories - the\n\
rate and clock source (however externally driven MTC has not been implemented\n\
yet). It is noted that the Arpeggiator settings are separate from the sequence\n\
information, ie, Up/Down/Rnd, the range and the arpeggiator clock are not the\n\
same as the note memory, this is discussed further in the memory setting\n\
section below.\n\
\n\
It is possible to redefine the arpeggiator sequence: select the function \n\
button on the right hand side, then select any of the arpeggiator mode buttons,\n\
this will initiate the recording. It does not matter which of the mode is\n\
selected since they will all start the recording sequence. When you have\n\
finished then select the mode button again (you may want to clear the function\n\
key if still active). You can record up to 256 steps, either from the GUI\n\
keyboard or from a master controller and the notes are saved into a midi\n\
key memory.\n\
\n\
There is no capability to edit the sequences once they have been entered, that\n\
level of control is left up to separate MIDI sequencers for which there are\n\
many available on Linux. Also, the note memory is actually volatile and will\n\
be lost when the emulation exits. If you want to save the settings then you\n\
have to enter them from the GUI keyboard or make sure that the GUI is linked\n\
up to the master keyboard MIDI interface - you need to be able to see the\n\
GUI keyboard following the notes pressed on the master keyboard since only\n\
when the GUI sees the notes can it store them for later recall. If the GUI\n\
did view the sequence entered here then it will be saved with the patch in\n\
a separate file (so that it can be used with other bristol synths).\n\
\n\
In addition to the Arpeggiator there is the 'Chord' control. The original\n\
synth had two green panel buttons labelled 'Hold', they were actually similar\n\
to the sustain pedal on a MIDI keyboard or piano, with one for each layer of\n\
the synth. They have been redefined here as Chord memory. When activated the\n\
layer will play a chord of notes for every key press. The notes are taken from\n\
separate chord memory in the Arpeggiator sequencer. The result is very similar\n\
to the Unison mode where every voice is activated for a single key, the\n\
difference here is that every voice may be playing a different note to give\n\
phat chords. To configure a chord you enable the function key and the target\n\
Hold button to put the synth into chord learning mode, play a set of notes\n\
(you don't have to hold them down), and click again to finalise the chord.\n\
If there are more chord notes than voices available then all voices will\n\
activate. If there are more voices than notes then you will be able to play\n\
these chord polyphonically, for example, if you have 8 voices and entered\n\
just 4 chorded notes then you will have 2 note polyphony left. You should\n\
also be able to play arpeggiations of chords. The maximum number of notes\n\
in a chord is 8.\n\
\n\
The synth has a modifier panel which functions as performance options which \n\
can be applied selectively to different layers:\n\
\n\
    Bender:\n\
        This is the depth of the settings and is mapped by the engine to\n\
        continuous controller 1 - the 'Mod' wheel. The emulation also tracks\n\
        the pitch wheel separately.\n\
\n\
    Bend to VCO\n\
        This applies an amount of pitch bend from the Mod wheel selectively\n\
        to either VCO-1 and/or VCO-2. These settings only affect the Mod\n\
        layers selected from the main panel. Subtle modifications applied in\n\
        different amounts to each oscillator can widen the sound considerable\n\
        by introducing small amounts of oscillator phasing.\n\
    \n\
    Bend to VCF\n\
        Affects the depth of cutoff to the filter with on/off available. Again\n\
        only applies to layers activate with the Mod setting.\n\
\n\
    LFO to VCO:\n\
        The mod panel has a second global LFO producing a sine wave. This can\n\
        be driven in selectable amounts to both VCO simultaneously. Layers are\n\
        selected from the Mod buttons.\n\
\n\
    LFO to VCF:\n\
        The LFO can be driven in selectable amounts to both VCO or to the VCF.\n\
        Layers are selected from the Mod buttons.\n\
\n\
    Delay:\n\
        This is the rise time of the LFO from the first note pressed. There is\n\
        no apparent frequency control of the second LFO however bristol allows\n\
        the frequency and gain of the LFO to track velocity using function B4\n\
        (see below for function settings). Since there is only one LFO per\n\
        emulation then the velocity tracking can be misleading as it only \n\
        tracks from a single voice and may not track the last note. For this\n\
        reason it can be disabled. Using a tracking from something like channel\n\
        pressure is for future study.\n\
\n\
    Glide:\n\
        Glissando between key frequencies, selectable to be either just to the\n\
        upper layer, off, or to both layers.\n\
\n\
    Transpose:\n\
        There are two transpose switches for the lower and upper layers \n\
        respectively. The range is +/1 one octave.\n\
\n\
Modifier panel settings are saved in the synth memories and are loaded with\n\
the first memory (ie, with dual loaded memories discussed below). The ability\n\
to save these settings in memory is an MKS-80 feature that was not available\n\
in either the Jupiter-6 or -8.\n\
\n\
There are several parts to the synth memories. Layer parameters govern sound\n\
generation, synth parameters that govern operating modes such Dual/Split,\n\
Solo/Unison etc, Function settings that modify internal operations, the\n\
parameters for the mod panel and finally the Arpeggiator sequences. These\n\
sequences are actually separate from the arpeggiator settings however that\n\
was covered in the notes above.\n\
\n\
When a patch is loaded then only the layer parameters are activated so that the\n\
new sound can be played, and the settings are only for the selected layer. This\n\
means any chord or arpeggiation can be tried with the new voice active.\n\
\n\
When a memory is 'dual loaded' by a double click on the Load button then all\n\
the memory settings will be read and activated: the current layer settings,\n\
synth settings, operational parameters including the peer layer parameters\n\
for dual/split configurations. Dual loading of the second layer will only \n\
happen if the memory was saved as a split/double with a peer layer active.\n\
\n\
The emulation adds several recognised Jupiter-6 capabilities that were not a\n\
part of the Jupiter-8 product. These are\n\
\n\
    1.  PW setting as well as PWM\n\
    2.  Cross modulation can be amplified with envelope-1 for FM type sounds\n\
    3.  Sync can be set in either direction (DCO1 to 2 or DCO2 to 1)\n\
    4.  The waveforms for DCO 1&2 are not exclusive but mixable\n\
    5.  The LFO to VCA is a continuous controller rather than stepped\n\
    6.  The envelope keyboard tracking is continuous rather than on/off\n\
    7.  The filter option is multimode LP/BP/HP rather than 12/24dB\n\
    8.  Layer detune is configurable\n\
    9.  Layer transpose switches are available\n\
    10. Arpeggiator is configurable on both layers\n\
\n\
Beyond these recognised mods it is also possible to select any/all DCO\n\
transpositions which further fattens up the sound as it allows for more\n\
harmonics. There is some added detune between the waveforms with its depth\n\
being a function of the -detune setting. Separate Pan and Balance controls\n\
have been implemented, Pan is the stereo positioning and is configurable per\n\
layer. Balance is the relative gain between each of the layers.\n\
\n\
There are several options that can be configured from the 'f' button\n\
in the MIDI section. When you push the f(n) button then the patch and bank\n\
buttons will not select memory locations but display the on/off status of 16\n\
algorithmic changes to the emulation. Values are saved in the synth memories.\n\
These are bristol specific modifications and may change between releases unless\n\
otherwise documented.\n\
\n\
F(n):\n\
\n\
    f(p1): Env-1 retriggers \n\
    f(p2): Env-1 conditionals\n\
    f(p3): Env-1 attack sensitivity\n\
    f(p4): Env-2 retriggers\n\
    f(p5): Env-2 conditionals\n\
    f(p6): Env-2 attack sensitivity\n\
    f(p7): Noise per voice/layer\n\
    f(p8): Noise white/pink\n\
\n\
    f(b1): LFO-1 per voice/layer\n\
    f(b2): LFO-1 Sync to Note ON\n\
    f(b3): LFO-1 velocity tracking\n\
    f(b4): Arpeggiator retrigger\n\
    f(b5): LFO-2 velocity tracking\n\
    f(b6): NRP enable\n\
    f(b7): Debug MIDI\n\
    f(b8): Debug GUI controllers\n\
\n\
The same function key also enables the learning function of the arpeggiator \n\
and chord memory, as explained above. When using the arpeggiator you may want\n\
to test with f(b4) enabled, it will give better control of the filter envelope.\n\
\n\
\n\
Other differences to the original are the Hold keys on the front panel. These\n\
acted as sustain pedals however for the emulation that does not function very\n\
well. With the original the buttons were readily available whilst playing and\n\
could be used manually, something that does not work well with a GUI and a\n\
mouse. For this reason they were re-used for 'Unison Chording' discussed above.\n\
Implementing them as sustain pedals would have been an easier if less flexible\n\
option and users are advised that the bristol MIDI library does recognise the\n\
sustain controller as the logical alternative here.\n\
\n\
Another difference would be the quality of the sound. The emulation is a lot\n\
cleaner and not as phat as the original. You might say it sounds more like\n\
something that comes from Uranus rather than Jupiter and consideration was\n\
indeed given to a tongue in cheek renaming of the emulation..... The author is\n\
allowed this criticism as he wrote the application - as ever, if you want the\n\
original sound then buy the original synth (or get Rolands own emulation?).\n\
\n\
A few notes are required on oscillator sync since by default it will seem to \n\
be quite noisy. The original could only product a single waveform at a single\n\
frequency at any one time. Several emulators, including this one, use a bitone\n\
oscillator which generates complex waveforms. The Bristol Bitone can generate\n\
up to 4 waveforms simultaneously at different levels for 5 different harmonics\n\
and the consequent output is very rich, the waves can be slightly detuned, \n\
the pulse output can be PW modulated. As with all the bristol oscillators that\n\
support sync, the sync pulse is extracted as a postive leading zero crossing.\n\
Unfortunately if the complex bitone output is used as input to sync another\n\
oscillator then the result is far too many zero crossings to extract a good\n\
sync. For the time being you will have to simplify the sync source to get a\n\
good synchronised output which itself may be complex wave. A future release\n\
will add a sync signal from the bitone which will be a single harmonic at the\n\
base frequency and allow both syncing and synchronised waveform outputs to be\n\
arbitrary.\n\
\n",

"    CRUMAR BIT-1, BIT-99, BIT-100\n\
    -----------------------------\n\
\n\
I was considering the implementation of the Korg-800, a synth I used to borrow\n\
without having due respect for it - it was a late low cost analogue having \n\
just one filter for all the notes and using the mildly annoying data entry\n\
buttons and parameter selectors. Having only one filter meant that with key\n\
tracking enabled the filter would open up as different keys were pressed,\n\
wildly changing the sound of active notes. Anyway, whilst considering how to\n\
implement the entry keys (and having features like the mouse tracking the\n\
selectors of the parameter graphics) I was reminded of this synthesizer. It\n\
was developed by Crumar but released under the name 'Bit' as the Crumar name\n\
was associated with cheesy roadrunner electric pianos at the time (also\n\
emulated by Bristol). This came out at the same time as the DX-7 but for\n\
half the price, and having the split and layer facilities won it several awards\n\
that would otherwise have gone to the incredibly innovative DX. However with\n\
the different Crumar models being released as the digital era began they kind\n\
of fell between the crack. It has some very nice mod features though, it was\n\
fun to emulate and hopefully enjoyable to play with.\n\
\n\
As a side note, the Korg Poly-800 is now also emulated by bristol\n\
\n\
A quick rundown of the Bit features are below. The different emulated models\n\
have slightly different default parameter values and/or no access to change\n\
them:\n\
\n\
    Two DCO with mixed waveforms.\n\
    VCF with envelope\n\
    VCA with envelope\n\
    Two LFO able to mod all other components, controlled by the wheel and key\n\
    velocity, single waveform, one had ramp and the other sawtooth options.\n\
\n\
The envelopes were touch sensitive for gain but also for attack giving plenty \n\
of expressive capabilities. The bristol envelope, when configured for velocity\n\
sensitive parameters (other than just gain) will also affect the attack rate.\n\
\n\
The front panel had a graphic that displayed all the available parameters and\n\
to change then you had to select the \"Address\" entry point then use the up/down\n\
entry buttons to change its value. Bristol uses this with the addition that the\n\
mouse can click on a parameter to start entering modifications to it.\n\
\n\
The emulation includes the 'Compare' and 'Park' features although they are a\n\
little annoying without a control surface. They work like this (not quite the\n\
same as the original): When you select a parameter and change it's value then\n\
the changes are not actually made to the active program, they just change the\n\
current sounds. The Compare button can be used to flip between the last loaded\n\
value and the current modified one to allow you to see if you prefer the sound\n\
before or after the modification.\n\
\n\
If you do prefer the changes then to keep them you must \"Park\" them into the\n\
running program before saving the memory. At the time of writing the emulation\n\
emulated the double click to park&write a memory, however it also has an actual\n\
Save button since 'Save to Tape' is not a feature here. You can use park and\n\
compare over dual loaded voices: unlike the original, which could only support\n\
editing of sounds when not in split/double, this emulation allows you to edit\n\
both layers by selecting the upper/lower entry buttons and then using the\n\
sensitive panel controls to select the addressed parameters. This is not the\n\
default behaviour, you first have to edit address 102 and increment it. Then,\n\
each layer can be simultaneously edited and just needs to be parked and saved\n\
separately. The Park/Compare cache can be disabled by editing parameter DE 101,\n\
changes are then made to the synth memory and not the cache.\n\
\n\
The memories are organised differently to the original. It had 99 memories, and\n\
the ones from 75 and above were for Split and Layered memories. Bristol can \n\
have all memories as Split or Layer. When you save a memory it is written to\n\
memory with a 'peer' program locator. When you load it with a single push on \n\
the Load button it returns to the active program, but if you double click then\n\
its 'peer' program is loaded into the other layer: press Load once to get the\n\
first program entered, then press it again - the Split/Layer will be set to\n\
the value from the first program and the second layer will be loaded. This \n\
naturally requires that the first memory was saved with Split/Layer enabled.\n\
It is advised (however not required) that this dual loading is done from the\n\
lower layer. This sequence will allow the lower layer to configure certain\n\
global options that the upper layer will not overwrite, for example the layer\n\
volumes will be select from the lower layer when the upper layer is dual \n\
loaded.\n\
\n\
For MIDI program change then since this quirky interface cannot be emulated\n\
then the memories above 75 will be dual loaded, the rest will be single loaded.\n\
\n\
Bristol will also emulate a bit-99 and a Bit-99m2 that includes some parameter\n\
on the front panel that were not available on the original. The engine uses the\n\
exact same algorithm for all emulations but the GUI presents different sets of\n\
parameters on the front panel. Those that are not displayed can only be accessed\n\
from the data entry buttons. The -99m2 put in a few extra features (ie, took a\n\
few liberties) that were not in the original:\n\
\n\
    DCO adds PWM from the LFO, not in the original\n\
    DCO-2 adds Sync to DCO-1, also not in the original\n\
    DCO-2 adds FM from DCO-1\n\
    DCO add PWM from Envelope\n\
    Glide has been added\n\
    DCO harmonics are not necessarily exclusive\n\
    Various envelope option for LFO\n\
    S&H LFO modulation\n\
\n\
The reason these were added was that bristol could already do them so were\n\
quite easy to incorporate, and at least gave two slightly different emulations.\n\
\n\
The oscillators can work slightly differently as well. Since this is a purely\n\
digital emulations then the filters are a bit weak. This is slightly compensated\n\
by the ability to configure more complex DCO. The transpose selectors (32', 16',\n\
8' and 2') were exclusive in the original. That is true here also, however if\n\
controllers 84 and 85 are set to zero then they can all work together to fatten\n\
out the sound. Also, the controllers look like boolean however that is only the\n\
case if the data entry buttons are used, if you change the value with the data\n\
entry pot then they act more like continuous drawbars, a nice effect however \n\
the display will not show the actual value as the display remains boolean, you\n\
have to use your ear. The square wave is exclusive and will typically only \n\
function on the most recently selected (ie, typically highest) harmonic.\n\
\n\
The same continuous control is also available on the waveform selectors. You\n\
can mix the waveform as per the original however the apparent boolean selectors\n\
are again continuous from 0.0 to 1.0. The net result is that the oscillators \n\
are quite Voxy having the ability to mix various harmonic levels of different\n\
mixable waveforms.\n\
\n\
The stereo mode should be correctly implemented too. The synth was not really\n\
stereo, it had two outputs - one for each layer. As bristol is stereo then each\n\
layer is allocated to the left or right channel. In dual or split they came\n\
out separate feeds if Stereo was selected. This has the rather strange side\n\
effect of single mode with 6 voices. If stereo is not selected then you have\n\
a mono synth. If stereo is selected then voices will exit from a totally \n\
arbitrary output depending on which voices is allocated to each note.\n\
In contrast to the original the Stereo parameter is saved with the memory and\n\
if you dual load a split/layer it is taken from the first loaded memory only.\n\
The implementation actually uses two different stereo mixes selectable with the\n\
Stereo button: Mono is a centre pan of the signal and Stereo pans hardleft and\n\
hardright respectively. These mixes can be changed with parameters 110 to 117\n\
using extended data entry documented below.\n\
\n\
The default emulation takes 6 voices for unison and applies 3+3 for the split\n\
and double modes. You can request more, for example if you used '-voices 16'\n\
at startup then you would be given 8+8. As a slight anomaly you cant request 32\n\
voices - this is currently interpreted as the default and gives you 3+3.\n\
\n\
The bit-1 did not have the Stereo control - the controller presented is the\n\
Unison button. You can configure stereo from the extended data entry ID 110 and\n\
111 which give the LR channel panning for 'Mono' setting, it should default to\n\
hard left and right panning. Similarly the -99 emulations do not have a Unison\n\
button, the capability is available from DE 80.\n\
\n\
The memories for the bit-1 and bit-99 should be interchangeable however the\n\
code maintains separate directories.\n\
\n\
There are three slightly different Bit GUI's. The first is the bit-1 with a \n\
limited parameter set as it only had 64 parameters. The second is the bit-99\n\
that included midi and split options in the GUI and has the white design that\n\
was an offered by Crumar. The third is a slightly homogenous design that is \n\
specific to bristol, similar to the black panelled bit99 but with a couple of\n\
extra parameters. All the emulations have the same parameters, some require you\n\
use the data entry controls to access them. This is the same as the original, \n\
there were diverse parameters that were not in memories that needed to be\n\
entered manually every time you wanted the feature. The Bristol Bit-99m2 has\n\
about all of the parameters selectable from the front panel however all of the\n\
emulations use the same memories so it is not required to configure them at\n\
startup (ie, they are saved). The emulation recognises the following parameters:\n\
\n\
    Data Entry  1 LFO-1 triangle wave selector (exclusive switch)\n\
    Data Entry  2 LFO-1 ramp wave selector (exclusive switch)\n\
    Data Entry  3 LFO-1 square wave selector (exclusive switch)\n\
    Data Entry  4 LFO-1 route to DCO-1\n\
    Data Entry  5 LFO-1 route to DCO-2\n\
    Data Entry  6 LFO-1 route to VCF\n\
    Data Entry  7 LFO-1 route to VCA\n\
    Data Entry  8 LFO-1 delay\n\
    Data Entry  9 LFO-1 frequency\n\
    Data Entry 10 LFO-1 velocity to frequency sensitivity\n\
    Data Entry 11 LFO-1 gain\n\
    Data Entry 12 LFO-1 wheel to gain sensitivity\n\
\n\
    Data Entry 13 VCF envelope Attack\n\
    Data Entry 14 VCF envelope Decay\n\
    Data Entry 15 VCF envelope Sustain\n\
    Data Entry 16 VCF envelope Release\n\
    Data Entry 17 VCF velocity to attack sensitivity (and decay/release) \n\
    Data Entry 18 VCF keyboard tracking\n\
    Data Entry 19 VCF cutoff\n\
    Data Entry 20 VCF resonance\n\
    Data Entry 21 VCF envelope amount\n\
    Data Entry 22 VCF velocity to gain sensitivity\n\
    Data Entry 23 VCF envelope invert\n\
\n\
    Data Entry 24 DCO-1 32' harmonic\n\
    Data Entry 25 DCO-1 16' harmonic\n\
    Data Entry 26 DCO-1 8' harmonic\n\
    Data Entry 27 DCO-1 4' harmonic\n\
    Data Entry 28 DCO-1 Triangle wave\n\
    Data Entry 29 DCO-1 Ramp wave\n\
    Data Entry 30 DCO-1 Pulse wave\n\
    Data Entry 31 DCO-1 Frequency 24 semitones\n\
    Data Entry 32 DCO-1 Pulse width\n\
    Data Entry 33 DCO-1 Velocity PWM\n\
    Data Entry 34 DCO-1 Noise level\n\
\n\
    Data Entry 35 DCO-2 32' harmonic\n\
    Data Entry 36 DCO-2 16' harmonic\n\
    Data Entry 37 DCO-2 8' harmonic\n\
    Data Entry 38 DCO-2 4' harmonic\n\
    Data Entry 39 DCO-2 Triangle wave\n\
    Data Entry 40 DCO-2 Ramp wave\n\
    Data Entry 41 DCO-2 Pulse wave\n\
    Data Entry 42 DCO-2 Frequency 24 semitones\n\
    Data Entry 43 DCO-2 Pulse width\n\
    Data Entry 44 DCO-2 Env to pulse width\n\
    Data Entry 45 DCO-2 Detune\n\
\n\
    Data Entry 46 VCA velocity to attack sensitivity (and decay/release) \n\
    Data Entry 47 VCA velocity to gain sensitivity\n\
    Data Entry 48 VCA overall gain ADSR\n\
    Data Entry 49 VCA Attack\n\
    Data Entry 50 VCA Decay\n\
    Data Entry 51 VCA Sustain\n\
    Data Entry 52 VCA Release\n\
\n\
    Data Entry 53 LFO-2 triangle wave selector (exclusive switch)\n\
    Data Entry 54 LFO-2 saw wave selector (exclusive switch)\n\
    Data Entry 55 LFO-2 square wave selector (exclusive switch)\n\
    Data Entry 56 LFO-2 route to DCO-1\n\
    Data Entry 57 LFO-2 route to DCO-2\n\
    Data Entry 58 LFO-2 route to VCF\n\
    Data Entry 59 LFO-2 route to VCA\n\
    Data Entry 60 LFO-2 delay\n\
    Data Entry 61 LFO-2 frequency\n\
    Data Entry 62 LFO-2 velocity to frequency sensitivity\n\
    Data Entry 63 LFO-2 gain\n\
    Data Entry 12 LFO-2 wheel to gain sensitivity\n\
\n\
    Data Entry 64 Split\n\
    Data Entry 65 Upper layer transpose\n\
    Data Entry 66 Lower Layer gain\n\
    Data Entry 67 Upper Layer gain\n\
\n\
The following were visible in the Bit-99 graphics only:\n\
\n\
    Data Entry 68 MIDI Mod wheel depth\n\
    Data Entry 69 MIDI Velocity curve (0 = soft, 10=linear, 25 = hard)\n\
    Data Entry 70 MIDI Enable Debug\n\
    Data Entry 71 MIDI Enable Program Change\n\
    Data Entry 72 MIDI Enable OMNI Mode\n\
    Data Entry 73 MIDI Receive channel\n\
\n\
    Data Entry 74 MIDI Mem Search Upwards\n\
    Data Entry 75 MIDI Mem Search Downwards\n\
    Data Entry 76 MIDI Panic (all notes off)\n\
\n\
Most of the MIDI options are not as per the original. This is because they are\n\
implemented in the bristol MIDI library and not the emulation.\n\
\n\
The following were added which were not really part of the Bit specifications\n\
so are only visible on the front panel of the bit100. For the other emulations\n\
they are accessible from the address entry buttons.\n\
\n\
    Data Entry 77 DCO-1->DCO-2 FM\n\
    Data Entry 78 DCO-2 Sync to DCO-1\n\
    Data Entry 79 Keyboard glide\n\
    Data Entry 80 Unison\n\
\n\
    Data Entry 81 LFO-1 SH\n\
    Data Entry 82 LFO-1 PWM routing for DCO-1\n\
    Data Entry 83 LFO-1 PWM routing for DCO-2\n\
    Data Entry 84 LFO-1 wheel tracking frequency\n\
    Data Entry 85 LFO-1 velocity tracking gain\n\
    Data Entry 86 LFO-1 per layer or per voice\n\
\n\
    Data Entry 87 LFO-2 SH\n\
    Data Entry 88 LFO-2 PWM routing for DCO-1\n\
    Data Entry 89 LFO-2 PWM routing for DCO-2\n\
    Data Entry 90 LFO-2 wheel tracking frequency\n\
    Data Entry 91 LFO-2 velocity tracking gain\n\
    Data Entry 92 LFO-2 per layer or per voice\n\
\n\
    Data Entry 93 ENV-1 PWM routing for DCO-1\n\
    Data Entry 94 ENV-1 PWM routing for DCO-2\n\
\n\
    Data Entry 95 DCO-1 restricted harmonics\n\
    Data Entry 96 DCO-2 restricted harmonics\n\
\n\
    Data Entry 97 VCF Filter type\n\
    Data Entry 98 DCO-1 Mix\n\
\n\
    Data Entry 99 Noise per layer\n\
\n\
    Data Entry 00 Extended data entry (above 99)\n\
    \n\
Extended data entry is for all parameters above number 99. Since the displays\n\
only have 2 digits it is not totally straightforward to enter these values and\n\
they are only available in Single mode, not dual or split - strangely similar\n\
to the original specification for editing memories. These are only activated for\n\
the lower layer loaded memory, not for dual loaded secondaries or upper layer\n\
loaded memories. You can edit the upper layer voices but they will be saved with\n\
their original extended parameters. This may seem correct however it is possible\n\
to edit an upper layer voice, save it, and have it sound different when next\n\
loaded since the extended parameters were taken from a different lower layer.\n\
This is kind of intentional but if in doubt then only ever dual load voices from\n\
the lower layer and edit them in single mode (not split or layer). Per default\n\
the emulation, as per the original, will not allow voice editing in Split or\n\
Layer modes however it can be enabled with DE 102.\n\
\n\
All the Bit emulations recognise extended parameters. They are somewhat in a\n\
disorganised list as they were built in as things developed. For the most part\n\
they should not be needed.\n\
The Bit-100 includes some in its silkscreen, for the others you can access them\n\
as follows:\n\
\n\
1. deselect split or double\n\
2. select addr entry\n\
3. use 0..9 buttons to enter address 00\n\
4. increment value to '1'. Last display should show EE (Extended Entry)\n\
\n\
5. select last two digits of desired address with 0-9 buttons\n\
6. change value (preferably with pot).\n\
\n\
7. when finished, select address 00 again (this is now actually 100) to exit\n\
\n\
    Data Entry 100 Exit extended data entry\n\
    Data Entry 101 enable WriteThru scratchpad (disables park and compare)\n\
    Data Entry 102 enable layer edits on Split/Double memories.\n\
    Data Entry 103 LFO-1 Sync to note on\n\
    Data Entry 104 LFO-2 Sync to note on\n\
    Data Entry 105 ENV-1 zero retrigger\n\
    Data Entry 106 ENV-2 zero retrigger\n\
    Data Entry 107 LFO-1 zero retrigger\n\
    Data Entry 108 LFO-2 zero retrigger\n\
    Data Entry 109 Debug enable (0 == none, 5 == persistent)\n\
\n\
    Data Entry 110 Left channel Mono gain, Lower\n\
    Data Entry 111 Right channel Mono gain, Lower\n\
    Data Entry 112 Left channel Stereo gain, Lower\n\
    Data Entry 113 Right channel Stereo gain, Lower\n\
    Data Entry 114 Left channel Mono gain, Upper\n\
    Data Entry 115 Right channel Mono gain, Upper\n\
    Data Entry 116 Left channel Stereo gain, Upper\n\
    Data Entry 117 Right channel Stereo gain, Upper\n\
    Data Entry 118 Bit-100 flag\n\
    Data Entry 119 Temperature sensitivity\n\
\n\
    Data Entry 120 MIDI Channel tracking layer-2 (same/different channel)\n\
    Data Entry 121 MIDI Split point tracking layer-2 (same/different split)\n\
    Data Entry 122 MIDI Transpose tracking (layer-2 or both layers) N/A\n\
    Data Entry 123 MIDI NRP enable\n\
\n\
    Data Entry 130 Free Memory Search Up\n\
    Data Entry 131 Free Memory Search Down\n\
    Data Entry 132 ENV-1 Conditional\n\
    Data Entry 133 ENV-2 Conditional\n\
    Data Entry 134 LFO-1 ENV Conditional\n\
    Data Entry 135 LFO-2 ENV Conditional\n\
    Data Entry 136 Noise white/pink\n\
    Data Entry 137 Noise pink filter (enable DE 136 Pink)\n\
    Data Entry 138 Glide duration 0 to 30 seconds\n\
    Data Entry 139 Emulation gain level\n\
\n\
    Data Entry 140 DCO-1 Square wave gain\n\
    Data Entry 141 DCO-1 Subharmonic level\n\
    Data Entry 142 DCO-2 Square wave gain\n\
    Data Entry 143 DCO-2 Subharmonic level\n\
\n\
The 150 range will be incorporated when the Arpeggiator code is more stable,\n\
currently in development for the Jupiter. This is anticipated in 0.20.4:\n\
\n\
    Data Entry 150 Arpeggiator Start/Stop\n\
    Data Entry 151 Arpeggiator mode D, U/D, U or Random\n\
    Data Entry 152 Arpeggiator range 1, 2, 3, 4 octaves\n\
    Data Entry 153 Arpeggiator rate\n\
    Data Entry 154 Arpeggiator external clock\n\
    Data Entry 155 Arpeggiator retrigger envelopes\n\
    Data Entry 156 Arpeggiator poly-2 mode\n\
    Data Entry 157 Chord Enable\n\
    Data Entry 158 Chord Relearn\n\
    Data Entry 159 Sequencer Start/Stop\n\
    Data Entry 160 Sequencer mode D, U/D, U or Random\n\
    Data Entry 161 Sequencer range 1, 2, 3, 4 octaves\n\
    Data Entry 162 Sequencer rate\n\
    Data Entry 163 Sequencer external clock\n\
    Data Entry 164 Sequencer retrigger envelopes\n\
    Data Entry 165 Sequencer Relearn\n\
\n\
The following can be manually configured but are really for internal uses only\n\
and will be overwritten when memories are saved to disk. The Split/Join flag,\n\
for example, is used by dual loading memories to configure the peer layer to\n\
load the memory in DE-198, and the stereo placeholder for configuring the stereo\n\
status of any single loaded memory.\n\
\n\
    Data Entry 193 Reserved: save bit-1 formatted memory\n\
    Data Entry 194 Reserved: save bit-99 formatted memory\n\
    Data Entry 195 Reserved: save bit-100 formatted memory\n\
    Data Entry 196 Reserved: Split/Join flag - internal use\n\
    Data Entry 197 Reserved: Stereo placeholder - internal use\n\
    Data Entry 198 Reserved: Peer memory pointer - internal use\n\
    Data Entry 199 Reserved: DCO-2 Wheel mod (masks entry 12) - internal use\n\
\n\
The tuning control in the emulation is on the front panel rather than on the\n\
rear panel as in the original. It had a keyboard sensitivity pot however that\n\
is achieved separately with bristol using velocity curves from the MIDI control\n\
settings. The front panel rotary defaults to 0% tuning and is not saved in the\n\
memory. The front panel gain controls are also not saved in the memory and\n\
default to 0.8 at startup.\n\
\n\
The net emulation is pretty intensive as it runs with over 150 operational\n\
parameters.\n\
\n\
A few notes are required on oscillator sync since by default it may seem to \n\
be quite noisy. The original could only produce a single waveform at a single\n\
frequency at any one time. Several emulators, including this one, use a bitone\n\
oscillator which generates complex waveforms. The Bristol Bitone can generate\n\
up to 4 waveforms simultaneously at different levels for 5 different harmonics\n\
and the consequent output is very rich, the waves can be slightly detuned, \n\
the pulse output can be PW modulated. As with all the bristol oscillators that\n\
support sync, the sync pulse is extracted as a postive leading zero crossing.\n\
Unfortunately if the complex bitone output is used as input to sync another\n\
oscillator then the result is far too many zero crossings to extract a good\n\
sync.\n\
Code has been implemented to generate a second sync source using a side output\n\
sync wave which is then fed to a sideband sync input on the oscillator, the\n\
results are far better\n\
\n",

NULL, /* Master */
NULL, /* CS-80 */

"    Sequential Circuits Prophet Pro-One\n\
    -----------------------------------\n\
\n\
Sequential circuits released amongst the first truly polyphonic synthesisers\n\
where a group of voice circuits (5 to 10 of them) were linked to an onboard\n\
computer that gave the same parameters to each voice and drove the notes to\n\
each voice from the keyboard. The costs were nothing short of exhorbitant and\n\
this lead to Sequential releasing a model with just one voice board as a mono-\n\
phonic equivalent. The sales ran up to 10,000 units, a measure of its success\n\
and it continues to be recognised alongside the Mini Moog as a fat bass synth.\n\
\n\
The design of the Prophet synthesisers follows that of the Mini Moog. It has\n\
three oscillators one of them as a dedicated LFO. The second audio oscillator\n\
can also function as a second LFO, and can cross modulate oscillator A for FM \n\
type effects. The audible oscillators have fixed waveforms with pulse width\n\
modulation of the square wave. These are then mixed and sent to the filter with\n\
two envelopes, for the filter and amplifier.\n\
\n\
The Pro-1 had a nice bussing matrix where 3 different sources, LFO, Filter Env\n\
and Oscillator-B could be mixed in varying amounts to two different modulation\n\
busses and each bus could then be chosen as inputs to modulation destinations.\n\
One bus was a direct bus from the mixed parameters, the second bus was under\n\
the modwheel to give configurable expressive control.\n\
\n\
LFO:\n\
\n\
    Frequency: 0.1 to 50 Hz\n\
    Shape: Ramp/Triangle/Square. All can be selected, none selected should\n\
    give a sine wave\n\
\n\
Modulations:\n\
\n\
    Source:\n\
\n\
        Filter Env amount to Direct or Wheel Mod busses\n\
        Oscillator-B amount to Direct or Wheel Mod busses\n\
        LFO to Direct amount or Wheel Mod busses\n\
\n\
    Dest:\n\
\n\
        Oscillator-A frequency from Direct or Wheel Mod busses\n\
        Oscillator-A PWM from Direct or Wheel Mod busses\n\
        Oscillator-B frequency from Direct or Wheel Mod busses\n\
        Oscillator-B PWM from Direct or Wheel Mod busses\n\
        Filter Cutoff from Direct or Wheel Mod busses\n\
\n\
Osc-A:\n\
\n\
    Tune: +/-7 semitones\n\
    Freq: 16' to 2' in octave steps\n\
    Shape: Ramp or Square\n\
    Pulse Width: only when Square is active.\n\
    Sync: synchronise to Osc-B\n\
\n\
Osc-B:\n\
\n\
    Tune: +/-7 semitones\n\
    Freq: 16' to 2' in octave steps\n\
    Fine: +/- 7 semitones\n\
    Shape: Ramp/Triangle/Square\n\
    Pulse Width: only when Square is active.\n\
    LFO: Lowers frequency by 'several' octaves.\n\
    KBD: enable/disable keyboard tracking.\n\
\n\
Mixer:\n\
\n\
    Gain for Osc-A, Osc-B, Noise\n\
\n\
Filter:\n\
\n\
    Cutoff: cuttof frequency\n\
    Res: Resonance/Q/Emphasis\n\
    Env: amount of modulation affecting to cutoff.\n\
    KBD: amount of keyboard trackingn to cutoff\n\
\n\
Envelopes: One each for PolyMod (filter) and amplifier.\n\
\n\
    Attack\n\
    Decay\n\
    Sustain\n\
    Release\n\
\n\
Sequencer:\n\
\n\
    On/Off\n\
    Record Play\n\
    Rate configured from LFO\n\
\n\
Arpeggiator:\n\
\n\
    Up/Off/UpDown\n\
    Rate configured from LFO\n\
\n\
Glide:\n\
\n\
    Amount of portamento\n\
    Auto/Normal - first key will/not glide.\n\
\n\
Global:\n\
\n\
    Master Tune\n\
    Master Volume\n\
\n\
\n\
Memories are loaded by selecting the 'Bank' button and typing in a two digit\n\
bank number followed by load. Once the bank has been selected then 8 memories\n\
from the bank can be loaded by pressing another memory select and pressing\n\
load. The display will show free memories (FRE) or programmed (PRG).\n\
There is an additional Up/Down which scan for the next program and a 'Find'\n\
key which will scan up to the next unused memory location.\n\
\n\
The original supported two sequences, Seq1 and Seq2, but these have not been\n\
implemented. Instead the emulator will save a sequence with each memory location\n\
which is a bit more flexible if not totally in the spirit of the original.\n\
\n\
The Envelope amount for the filter is actually 'Mod Amount'. To get the filter\n\
envelope to drive the filter it must be routed to the filter via a mod bus. This\n\
may differ from the original.\n\
Arpeggiator range is two octaves.\n\
The Mode options may not be correctly implemented due to the differences in\n\
the original being monophonic and the emulator being polyphonic. The Retrig is\n\
actually 'rezero' since we have separate voices. Drone is a Sustain key that\n\
emulates a sustain pedal.\n\
Osc-B cannot modulate itself in polyphonic mode (well, it could, it's just that\n\
it has not been coded that way).\n\
The filter envelope is configured to ignore velocity.\n\
\n\
The default filters are quite expensive. The -lwf option will select the less\n\
computationally expensive lightweight Chamberlain filters which have a colder\n\
response but require zonks fewer CPU cycles.\n\
\n",

NULL, /* Voyager = explorer, stuff later */

"    Moog Sonic-6\n\
    ------------\n\
\n\
This original design was made by an engineer who had previously worked with \n\
Moog on the big modular systems, Gene Zumchek. He tried to get Moog Inc to \n\
develop a small standalone unit rather than the behemoths however he could \n\
not get heard. After leaving he built a synth eventually called a Sonic-5 that\n\
did fit the bill but sales volumes were rather small. He had tied up with a\n\
business manager who worked out that the volume was largely due to the name\n\
not being known, muSonics.\n\
This was quickly overcome by accident. Moog managed to run his company into\n\
rather large debt and the company folded. Bill Waytena, working with Zumcheck,\n\
gathered together the funding needed to buy the remains of the failed company\n\
and hence Moog Inc was labled on the rebadged Sonic-6. Zumcheck was eventually\n\
forced to leave this company (or agreed to) as he could not work with Moog.\n\
After a few modifications Bob Moog actually used this unit quite widely for\n\
lecturing on electronic music. For demonstrative purposes it is far more\n\
flexible than any of Moog's own non-modular designs and it was housed in a\n\
transport case rather than needing a shipping crate as the modular systems\n\
required.\n\
\n\
The emulation features are given below, but first a few of the differences to \n\
the original\n\
\n\
    Added a mod wheel that can drive GenX/Y.\n\
    PWM is implemented on the oscillator B\n\
    Installed an ADSR rather than AR, selectable.\n\
    No alternative scalings - use scala file support\n\
    Not duo or dia phonic. Primarily poly with separated glide.\n\
\n\
The original was duophonic, kind of. It had a keyboard with high note and low\n\
note precedence and the two oscillators could be driven from different notes.\n\
Its not really duophony and was reportedly not nice to play but it added some\n\
flexibility to the instrument. This features was dropped largley because it\n\
is ugly to emulate in a polyphonic environment but the code still has glide\n\
only on Osc-B. It has the two LFO that can be mixed, or at full throw of the \n\
GenXY mixer they will link X->A and Y->B giving some interesting routing, two\n\
osc each with their own LFO driving the LFO from the mod wheel or shaping it\n\
with the ADSR. Playing around should give access to X driving Osc-A, then \n\
Osc-A and GenY driving Osc-B with Mod and shaping for some investigation of\n\
FM synthesis. The gruesome direct output mixer is still there, having the osc\n\
and ring-mod bypass the filter and amplifier completely (or can be mixed back\n\
into the 'actuated' signal).\n\
\n\
There is currently no likely use for an external signal even though the\n\
graphics are there.\n\
\n\
The original envelope was AR or ASR. The emulator has a single ADSR and a \n\
control switch to select AR (actually AD), ASR, ADSD (MiniMoog envelope) or\n\
ADSR.\n\
\n\
Generator-Y has a S/H function on the noise source for a random signal which \n\
replaced the square wave. Generator-X still has a square wave.\n\
\n\
Modulators:\n\
\n\
    Two LFO, X and Y:\n\
\n\
        Gen X:\n\
            Tri/Ramp/Saw/Square\n\
            Tuning\n\
            Shaping from Envelope or Modwheel\n\
\n\
        Gen Y:\n\
            Tri/Ramp/Saw/Rand\n\
            Tuning\n\
            Shaping from Envelope or Modwheel\n\
\n\
        Master LFO frequency\n\
\n\
        GenXY mixer\n\
    \n\
    Two Oscillators, A and B\n\
\n\
        Gen A:\n\
            Tri/Ramp/Pulse\n\
            PulseWidth\n\
            Tuning\n\
            Transpose 16', 8', 4' (*)\n\
\n\
            Mods:\n\
\n\
                Envelope\n\
                GenXY(or X)\n\
                Low frequency, High Frequency (drone), KBD Tracking\n\
\n\
        Gen B:\n\
            Tri/Ramp/Pulse\n\
            PulseWidth\n\
            Tuning\n\
            Transpose 16' 8', 4'\n\
\n\
            Mods:\n\
            \n\
                Osc-B\n\
                GenXY(or Y)\n\
                PWM\n\
\n\
        GenAB mix\n\
\n\
        Ring Mod:\n\
\n\
            Osc-B/Ext\n\
            GenXY/Osc-A\n\
\n\
        Noise\n\
\n\
            Pink/White\n\
\n\
        Mixer\n\
\n\
            GenAB\n\
            RingMod\n\
            External\n\
            Noise\n\
\n\
        Filter (**)\n\
\n\
            Cutoff\n\
            Emphasis\n\
\n\
            Mods:\n\
\n\
                ADSR\n\
                Keyboard tracking\n\
                GenXY\n\
\n\
        Envelope:\n\
\n\
            AR/ASR/ADSD/ADSR\n\
            Velociy on/off\n\
\n\
            Trigger:\n\
\n\
                GenX\n\
                GenY\n\
                Kbd (rezero only)\n\
\n\
            Bypass (key gated audio)\n\
\n\
        Direct Output Mixer\n\
\n\
            Osc-A\n\
            Osc-B\n\
            RingMod\n\
\n\
\n\
The keyboard has controls for\n\
\n\
    Glide (Osc-B only)\n\
    Master Volume\n\
    PitchWheel\n\
    ModWheel (gain modifier on LFO)\n\
\n\
    Global Tuning\n\
\n\
    MultiLFO X and Y\n\
\n\
* The oscillator range was +/-2 octave switch and a +/-1 octave pot. This\n\
emulator has +/-1 octave switch and +/-7 note pot. That may change in a future\n\
release to be more like the original, probably having a multiway 5 stage octave\n\
selector.\n\
\n\
** The filter will self oscillate at full emphasis however this is less \n\
prominent at lower frequencies (much like the Moog ladder filter). The filter\n\
is also 'not quite' in tune when played as an oscillator, this will also change\n\
in a future release.\n\
\n\
There may be a reverb on the emulator. Or there may not be, that depends on\n\
release. The PitchWheel is not saved in the memories, the unit is tuned on\n\
startup and this will maintain tuning to other instruments. The MultiLFO allow\n\
you to configure single LFO per emulation or one per voice, independently.\n\
Having polyphony means you can have the extra richness of independent LFO per\n\
voice however that does not work well if they are used as triggers, for example,\n\
you end up with a very noisy result. With single triggers for all voices the\n\
result is a lot more predictable.\n\
\n\
The Sonic-6 as often described as having bad tuning, that probably depends on \n\
model since different oscillators were used at times. Also, different units\n\
had different filters (Zumchek used a ladder of diodes to overcome the Moog\n\
ladder of transister patent). The original was often described as only being\n\
useful for sound effects. Personally I don't think that was true however the\n\
design is extremely flexible and the mods are applied with high gains so to\n\
get subtle sounds they only have to be applied lightly. Also, this critique\n\
was in comparison to the Mini which was not great for sound effects since it,\n\
in contrast, had very little in the way of modifiers.\n\
\n\
The actual mod routing here is very rich. The two LFO can be mixed to provide\n\
for more complex waves and have independent signal gain from the ADSR. To go\n\
a step further it is possible to take the two mixed LFO into Osc-A, configure\n\
that as an LFO and feed it into Osc-B for some very complex mod signals. That\n\
way you can get a frequency modulated LFO which is not possible from X or Y. As\n\
stated, if these are applied heavily you will get ray guns and car alarms but\n\
in small amounts it is possible to just shape sounds. Most of the mod controls\n\
have been made into power functions to give more control at small values.\n\
\n\
The memory panel gives access to 72 banks of 8 memories each. Press the Bank\n\
button and two digits for the bank, then just select the memory and press Load.\n\
You can get the single digit banks by selecting Bank->number->Bank. There is\n\
a save button which should require a double click but does not yet (0.30.0),\n\
a pair of buttons for searching up and down the available memories and a button\n\
called 'Find' which will select the next available free memory.\n\
\n\
Midi options include channel, channel down and, er, thats it.\n\
\n",

"    CRUMAR TRILOGY\n\
    --------------\n\
\n\
This text is primarily that of the Stratus since the two synths were very \n\
similar. This is the bigger brother having all the same features with the added\n\
string section. There were some minor differences in the synth circuits for\n\
switchable or mixable waveforms.\n\
\n\
This unit is a hybrid synth/organ/string combo, an early polyphonic using an\n\
organ divider circuit rather than independent VCO and having a set of filters\n\
and envelope for the synth sounds, most manufacturers came out with similar\n\
designs. The organ section was generally regarded as pretty bad here, there\n\
were just five controls, four used for the volume of 16, 8, 4 and 2 foot \n\
harmonics and a fifth for overall organ volume. The synth section had 6 voices\n\
and some quite neat little features for a glide circuitry and legato playing\n\
modes. The string section could mix 3 waveforms with vibrato on some so when\n\
mixed with the straight waveform would produce phasing.\n\
\n\
The emulator consists of two totally separate layers, one emulating the organ\n\
circuitry and another the synth. The organ has maximum available polyphony as\n\
the algorithm is quite lightweight even though diverse liberties have been\n\
taken to beef up the sound. The synth section is limited to 6 voices unless\n\
otherwise specified at run time. The organ circuitry is used to generate the\n\
string section.\n\
\n\
The legato playing modes affects three sections, the LFO modulation, VCO \n\
selection and glide:\n\
\n\
LFO: this mod has a basic envelope to control the gain of the LFO using delay,\n\
slope and gain. In 'multi' mode the envelope is triggered for every note that\n\
is played and in the emulator this is actually a separate LFO per voice, a bit\n\
fatter than the original. In 'Mono' mode there is only one LFO that all voices\n\
will share and the envelope is triggered in Legato style, ie, only once for\n\
a sequence of notes - all have to be released for the envelope to recover.\n\
\n\
VCO: The original allowed for wavaeform selection to alternate between notes, \n\
something that is rather ugly to do with the bristol architecture. This is \n\
replaced with a VCO selector where each note will only take the output from\n\
one of the two avalable oscillators and gives the ntoes a little more\n\
separation. The legato mode works whereby the oscillator selection is only\n\
made for the first note in a sequence to give a little more sound consistency.\n\
\n\
Glide: This is probably the coolest feature of the synth. Since it used an\n\
organ divider circuit it was not possible to actually glide from one note to\n\
another - there are really only two oscillators in the synth section, not two\n\
per voice. In contrast the glide section could glide up or down from a selected\n\
amount to the real frequency. Selected from down with suitable values would\n\
give a nice 'blue note' effect for example. In Legato mode this is done only\n\
for the first keypress rather than all of the since the effect can be a bit\n\
over the top if applied to each keystroke in a sequence. At the same time it\n\
was possible to Sync the two oscillators, so having only one of them glide \n\
and be in sync then without legato this gave a big phasing entrance to each\n\
note, a very interesting effect. The Glide has 4 modes:\n\
\n\
    A. Both oscillators glide up to the target frequency\n\
    B. Only oscillator-2 glides up to the target frequency\n\
    C. Only oscillator-2 glides down to the target frequency\n\
    D. Both oscillators glide down to the target frequency\n\
\n\
These glide options with different sync and legato lead to some very unique\n\
sounds and are emulated here with only minor differences.\n\
\n\
The features, then notes on the differences to the original:\n\
\n\
    A. Organ Section\n\
\n\
        16, 8, 4 and 2 foot harmonic strengths.\n\
        Volume.\n\
\n\
    B. Synth Section\n\
\n\
        LFO Modulation\n\
\n\
            Rate - 0.1 to 50Hz approx\n\
            Slope - up to 10 seconds\n\
            Delay - up to 10 seconds\n\
            Gain\n\
\n\
            Routing selector: VCO, VCF, VCA\n\
            Mono/Multi legato mode\n\
            Shape - Tri/Ramp/Saw/Square\n\
\n\
        Oscillator 1\n\
\n\
            Tuning\n\
            Sync 2 to 1\n\
            Octave selector\n\
\n\
        Oscillator 2\n\
\n\
            Tuning\n\
            Octave trill\n\
            Octave selector\n\
\n\
        Waveform Ramp and Square mix\n\
        Alternate on/off\n\
        Mono/Multi legato mode VCO selection\n\
\n\
        Glide\n\
\n\
            Amount up or down from true frequency\n\
            Speed of glide\n\
            Mono/Multi legato mode\n\
            Direction A, B, C, D\n\
\n\
        Filter\n\
\n\
            Cutoff frequency\n\
            Resonance\n\
            Envelope tracking -ve to +ve\n\
            Pedal tracking on/off\n\
\n\
        Envelope\n\
\n\
            Attack\n\
            Decay\n\
            Sustain\n\
            Release\n\
\n\
        Gain\n\
\n\
    C. String Section\n\
\n\
        16' 8' mix\n\
        Subharmonic gain\n\
        Attack\n\
        Release\n\
\n\
        Volume\n\
\n\
Diverse liberties were taken with the reproduction, these are manageable from\n\
the options panel by selecting the button next to the keyboard. This opens up\n\
a graphic of a PCB, mostly done for humorous effect as it not in the least bit\n\
representative of the actual hardware. Here there are a number of surface\n\
mounted controllers. These are as below but may change by release:\n\
\n\
   P1  Master volume\n\
\n\
   P2  Organ pan\n\
   P3  Organ waveform distorts\n\
   P4  Organ spacialisation\n\
   P5  Organ mod level\n\
   J1  Organ key grooming \n\
   P6  Organ tuning (currently inactive *)\n\
\n\
   P7  Synth pan\n\
   P8  Synth tuning\n\
   P9  Synth osc1 harmonics\n\
   P10 Synth osc2 harmonics\n\
   J2  Synth velocity sensitivity\n\
   J3  Synth filter type\n\
   P11 Synth filter tracking\n\
\n\
   P12 String pan\n\
   P13 String harmonics\n\
   P14 String spacialisation\n\
   P15 String mod level\n\
   P16 String waveform distorts\n\
\n\
*: To make the organ tunable the keymap file has to be removed.\n\
\n\
Master (P1) volume affects both layers simultaneously and each layer can be\n\
panned (P2/P7) and tuned (P8) separately to give phasing and spacialisation.\n\
The synth layer has the default frequency map of equal temperament however the\n\
organ section uses a 2MHz divider frequency map that is a few cents out for\n\
each key. The Trilogy actually has this map for both layers and that can easily\n\
be done with the emulator, details on request.\n\
\n\
It is currently not possible to retune the organ divider circuit, it has a\n\
private microtonal mapping to emulate the few percent anomalies of the divider\n\
circuit and the frequencies are predefined. The pot is still visible in P6 and\n\
can be activated by removing the related microtonal mapping file, details from\n\
the author on request.\n\
\n\
Diverse liberties were taken with the Organ section since the original only \n\
produced 4 pure (infinite bandwidth) square waves that were mixed together, \n\
an overly weak result. The emulator adds a waveform distort (P3), an notched\n\
control that produces a pure sine wave at centre point. Going down it will\n\
generate gradually increasing 3rd and 5th harmonics to give it a squarey wave\n\
with a distinct hammond tone. The distortion actually came from the B3 emulator\n\
which models the distort on the shape of the hammond tonewheels themselves.\n\
Going up from centre point will produce gradually sharper sawtooth waves using\n\
a different phase distortion.\n\
\n\
Organ spacialisation (P4) will separate out the 4 harmonics to give them \n\
slightly different left and right positions to fatten out the sound. This works\n\
in conjunction with the mod level (P5) where one of the stereo components of\n\
each wave is modified by the LFO to give phasing changes up to vibrato.\n\
\n\
The organ key grooming (J1) will either give a groomed wave to remove any\n\
audible clicks from the key on and off events or when selected will produce \n\
something akin to a percussive ping for the start of the note.\n\
\n\
The result for the organ section is that it can produce some quite nice sounds\n\
reminiscent of the farfisa range to not quite hammond, either way far more\n\
useful than the flat, honking square waves. The original sound can be made by\n\
waveform to a quarter turn or less, spacialisation and mod to zero, key\n\
grooming off.\n\
\n\
The synth has 5 modifications at the first release. The oscillator harmonics\n\
can be fattened at the top or bottom using P9 and P10, one control for each\n\
oscillator, low is more bass, high is more treble. Some of the additional\n\
harmonics will be automatically detuned a little to fatten out the sound as a\n\
function of the -detune parameter defaulting to 100.\n\
\n\
The envelope can have its velocity sensitively to the filter enabled or disabled\n\
(J2) and the filter type can be a light weight filter for a thinner sound but at\n\
far lower CPU load (J3).\n\
\n\
The filter keyboard tracking is configurable (P11), this was outside of the spec\n\
of the Trilogy however it was implemented here to correct the keyboard tracking\n\
of the filter for all the emulations and the filter should now be playable.\n\
The envelope touch will affect this depending on J2 since velocity affects the\n\
cut off frequency and that is noticeable when playing the filter. This jumper\n\
is there so that the envelope does not adversely affect tuning but can still be\n\
used to have the filter open up with velocity if desired.\n\
\n\
The mod application is different from the original. It had a three way selector\n\
for routing the LFO to either VCO, VCA or VCF but only a single route. This\n\
emulation uses a continuous notched control where full off is VCO only, notch\n\
is VCF only and full on is VCA however the intermidiate positions will route\n\
proportional amounts to two components.\n\
\n\
The LFO has more options (Ramp and Saw) than the original (Tri and Square).\n\
\n\
The extra options are saved with each memory however they are only loaded at\n\
initialisation and when the 'Load' button is double-clicked. This allows you to\n\
have them as global settings or per memory as desired. The MemUp and MemDown \n\
will not load the options, only the main settings.\n\
\n\
VCO mod routing is a little bit arbitrary in this first release however I could\n\
not find details of the actual implementation. The VCO mod routing only goes\n\
to Osc-1 which also takes mod from the joystick downward motion. Mod routing\n\
to Osc-2 only happens if 'trill' is selected. This seemed to give the most\n\
flexibility, directing the LFO to VCF/VCA and controlling vibrato from the \n\
stick, then having Osc-2 separate so that it can be modified and sync'ed to\n\
give some interesting phasing.\n\
\n\
As of the first release there are possibly some issues with the oscillator \n\
Sync selector, it is perhaps a bit noisy with a high content of square wave.\n\
Also, there are a couple of minor improvements that could be made to the \n\
legato features but they will be done in a future release. They regard how\n\
the glide is applied to the first or all in a sequence of notes.\n\
\n\
The joystick does not always pick up correctly however it is largely for \n\
presentation, doing actual work you would use a real joystick or just use the\n\
modwheel (the stick generates and tracks continuous controller 1 - mod). The\n\
modwheel tracking is also a bit odd but reflects the original architecture - \n\
at midpoint on the wheel there is no net modulation, going down affects VCO\n\
in increasing amounts and going up from mid affect the VCF. The control feels\n\
like it should be notched however generally that is not the case with mod\n\
wheels.\n\
\n\
A few notes are required on oscillator sync since by default it will seem to \n\
be quite noisy. The original could only product a single waveform at a single\n\
frequency at any one time. Several emulators, including this one, use a bitone\n\
oscillator which generates complex waveforms. The Bristol Bitone can generate\n\
up to 4 waveforms simultaneously at different levels for 5 different harmonics\n\
and the consequent output is very rich, the waves can be slightly detuned, \n\
the pulse output can be PW modulated. As with all the bristol oscillators that\n\
support sync, the sync pulse is extracted as a postive leading zero crossing.\n\
Unfortunately if the complex bitone output is used as input to sync another\n\
oscillator then the result is far too many zero crossings to extract a good\n\
sync. For the time being you will have to simplify the sync source to get a\n\
good synchronised output which itself may be complex wave. A future release\n\
will add a sync signal from the bitone which will be a single harmonic at the\n\
base frequency and allow both syncing and synchronised waveform outputs to be\n\
arbitrary. For the Trilogy this simplification of the sync waveform is done\n\
automatically by the Sync switch, this means the synchronised output sounds\n\
correct but the overall waveform may be simpler.\n\
\n",

NULL, /* Trilogy ODC */

"    CRUMAR STRATUS\n\
    --------------\n\
\n\
This unit is a hybrid synth/organ combo, an early polyphonic synth using an\n\
organ divider circuit rather than independent VCO and having a set of filters\n\
and envelope for the synth sounds, most manufacturers came out with similar\n\
designs. The organ section was generally regarded as pretty bad here, there\n\
were just five controls, four used for the volume of 16, 8, 4 and 2 foot \n\
harmonics and a fifth for overall organ volume. The synth section had 6 voices\n\
and some quite neat little features for a glide circuitry and legato playing\n\
modes.\n\
\n\
The emulator consists of two totally separate layers, one emulating the organ\n\
circuitry and another the synth. The organ has maximum available polyphony as\n\
the algorithm is quite lightweight even though diverse liberties have been\n\
taken to beef up the sound. The synth section is limited to 6 voices unless\n\
otherwise specified at run time.\n\
\n\
The legato playing modes affects three sections, the LFO modulation, VCO \n\
selection and glide:\n\
\n\
LFO: this mod has a basic envelope to control the gain of the LFO using delay,\n\
slope and gain. In 'multi' mode the envelope is triggered for every note that\n\
is played and in the emulator this is actually a separate LFO per voice, a bit\n\
fatter than the original. In 'Mono' mode there is only one LFO that all voices\n\
will share and the envelope is triggered in Legato style, ie, only once for\n\
a sequence of notes - all have to be released for the envelope to recover.\n\
\n\
VCO: The original allowed for wavaeform selection to alternate between notes, \n\
something that is rather ugly to do with the bristol architecture. This is \n\
replaced with a VCO selector where each note will only take the output from\n\
one of the two avalable oscillators and gives the ntoes a little more\n\
separation. The legato mode works whereby the oscillator selection is only\n\
made for the first note in a sequence to give a little more sound consistency.\n\
\n\
Glide: This is probably the coolest feature of the synth. Since it used an\n\
organ divider circuit it was not possible to actually glide from one note to\n\
another - there are really only two oscillators in the synth section, not two\n\
per voice. In contrast the glide section could glide up or down from a selected\n\
amount to the real frequency. Selected from down with suitable values would\n\
give a nice 'blue note' effect for example. In Legato mode this is done only\n\
for the first keypress rather than all of the since the effect can be a bit\n\
over the top if applied to each keystroke in a sequence. At the same time it\n\
was possible to Sync the two oscillators, so having only one of them glide \n\
and be in sync then without legato this gave a big phasing entrance to each\n\
note, a very interesting effect. The Glide has 4 modes:\n\
\n\
    A. Both oscillators glide up to the target frequency\n\
    B. Only oscillator-2 glides up to the target frequency\n\
    C. Only oscillator-2 glides down to the target frequency\n\
    D. Both oscillators glide down to the target frequency\n\
\n\
These glide options with different sync and legato lead to some very unique\n\
sounds and are emulated here with only minor differences.\n\
\n\
The features, then notes on the differences to the original:\n\
\n\
    A. Organ Section\n\
\n\
        16, 8, 4 and 2 foot harmonic strengths.\n\
        Volume.\n\
\n\
    B. Synth Section\n\
\n\
        LFO Modulation\n\
\n\
            Rate - 0.1 to 50Hz approx\n\
            Slope - up to 10 seconds\n\
            Delay - up to 10 seconds\n\
            Gain\n\
\n\
            Routing selector: VCO, VCF, VCA\n\
            Mono/Multi legato mode\n\
            Shape - Tri/Ramp/Saw/Square\n\
\n\
        Oscillator 1\n\
\n\
            Tuning\n\
            Sync 2 to 1\n\
            Octave selector\n\
\n\
        Oscillator 2\n\
\n\
            Tuning\n\
            Octave trill\n\
            Octave selector\n\
\n\
        Waveform Ramp and Square mix\n\
        Alternate on/off\n\
        Mono/Multi legato mode VCO selection\n\
\n\
        Glide\n\
\n\
            Amount up or down from true frequency\n\
            Speed of glide\n\
            Mono/Multi legato mode\n\
            Direction A, B, C, D\n\
\n\
        Filter\n\
\n\
            Cutoff frequency\n\
            Resonance\n\
            Envelope tracking -ve to +ve\n\
            Pedal tracking on/off\n\
\n\
        Envelope\n\
\n\
            Attack\n\
            Decay\n\
            Sustain\n\
            Release\n\
\n\
        Gain\n\
\n\
Diverse liberties were taken with the reproduction, these are manageable from\n\
the options panel by selecting the button next to the keyboard. This opens up\n\
a graphic of a PCB, mostly done for humorous effect as it not in the least bit\n\
representative of the actual hardware. Here there are a number of surface\n\
mounted controllers. These are as below but may change by release:\n\
\n\
   P1 Master volume\n\
\n\
   P2  Organ pan\n\
   P3  Organ waveform distorts\n\
   P4  Organ spacialisation\n\
   P5  Organ mod level\n\
   J1  Organ key grooming \n\
   P6  Organ tuning (currently inactive *)\n\
\n\
   P7  Synth pan\n\
   P8  Synth tuning\n\
   P9  Synth osc1 harmonics\n\
   P10 Synth osc2 harmonics\n\
   J2  Synth velocity sensitivity\n\
   J3  Synth filter type\n\
   P11 Synth filter tracking\n\
\n\
*: To make the organ tunable the keymap file has to be removed.\n\
\n\
Master (P1) volume affects both layers simultaneously and each layer can be\n\
panned (P2/P7) and tuned (P8) separately to give phasing and spacialisation.\n\
The synth layer has the default frequency map of equal temperament however the\n\
organ section uses a 2MHz divider frequency map that is a few cents out for\n\
each key. The Stratus actually has this map for both layers and that can easily\n\
be done with the emulator, details on request.\n\
\n\
It is currently not possible to retune the organ divider circuit, it has a\n\
private microtonal mapping to emulate the few percent anomalies of the divider\n\
circuit and the frequencies are predefined. The pot is still visible in P6 and\n\
can be activated by removing the related microtonal mapping file, details from\n\
the author on request.\n\
\n\
Diverse liberties were taken with the Organ section since the original only \n\
produced 4 pure (infinite bandwidth) square waves that were mixed together, \n\
an overly weak result. The emulator adds a waveform distort (P3), an notched\n\
control that produces a pure sine wave at centre point. Going down it will\n\
generate gradually increasing 3rd and 5th harmonics to give it a squarey wave\n\
with a distinct hammond tone. The distortion actually came from the B3 emulator\n\
which models the distort on the shape of the hammond tonewheels themselves.\n\
Going up from centre point will produce gradually sharper sawtooth waves using\n\
a different phase distortion.\n\
\n\
Organ spacialisation (P4) will separate out the 4 harmonics to give them \n\
slightly different left and right positions to fatten out the sound. This works\n\
in conjunction with the mod level (P5) where one of the stereo components of\n\
each wave is modified by the LFO to give phasing changes up to vibrato.\n\
\n\
The organ key grooming (J1) will either give a groomed wave to remove any\n\
audible clicks from the key on and off events or when selected will produce \n\
something akin to a percussive ping for the start of the note.\n\
\n\
The result for the organ section is that it can produce some quite nice sounds\n\
reminiscent of the farfisa range to not quite hammond, either way far more\n\
useful than the flat, honking square waves. The original sound can be made by\n\
waveform to a quarter turn or less, spacialisation and mod to zero, key\n\
grooming off.\n\
\n\
The synth has 5 modifications at the first release. The oscillator harmonics\n\
can be fattened at the top or bottom using P9 and P10, one control for each\n\
oscillator, low is more bass, high is more treble. Some of the additional\n\
harmonics will be automatically detuned a little to fatten out the sound as a\n\
function of the -detune parameter defaulting to 100.\n\
\n\
The envelope can have its velocity sensitively to the filter enabled or disabled\n\
(J2) and the filter type can be a light weight filter for a thinner sound but at\n\
far lower CPU load (J3).\n\
\n\
The filter keyboard tracking is configurable (P11), this was outside of the spec\n\
of the Stratus however it was implemented here to correct the keyboard tracking\n\
of the filter for all the emulations and the filter should now be playable.\n\
The envelope touch will affect this depending on J2 since velocity affects the\n\
cut off frequency and that is noticeable when playing the filter. This jumper\n\
is there so that the envelope does not adversely affect tuning but can still be\n\
used to have the filter open up with velocity if desired.\n\
\n\
The mod application is different from the original. It had a three way selector\n\
for routing the LFO to either VCO, VCA or VCF but only a single route. This\n\
emulation uses a continuous notched control where full off is VCO only, notch\n\
is VCF only and full on is VCA however the intermidiate positions will route\n\
proportional amounts to two components.\n\
\n\
The LFO has more options (Ramp and Saw) than the original (Tri and Square).\n\
\n\
The extra options are saved with each memory however they are only loaded at\n\
initialisation and when the 'Load' button is double-clicked. This allows you to\n\
have them as global settings or per memory as desired. The MemUp and MemDown \n\
will not load the options, only the main settings.\n\
\n\
VCO mod routing is a little bit arbitrary in this first release however I could\n\
not find details of the actual implementation. The VCO mod routing only goes\n\
to Osc-1 which also takes mod from the joystick downward motion. Mod routing\n\
to Osc-2 only happens if 'trill' is selected. This seemed to give the most\n\
flexibility, directing the LFO to VCF/VCA and controlling vibrato from the \n\
stick, then having Osc-2 separate so that it can be modified and sync'ed to\n\
give some interesting phasing.\n\
\n\
As of the first release there are possibly some issues with the oscillator \n\
Sync selector, it is perhaps a bit noisy with a high content of square wave.\n\
Also, there are a couple of minor improvements that could be made to the \n\
legato features but they will be done in a future release. They regard how\n\
the glide is applied to the first or all in a sequence of notes.\n\
\n\
The joystick does not always pick up correctly however it is largely for \n\
presentation, doing actual work you would use a real joystick or just use the\n\
modwheel (the stick generates and tracks continuous controller 1 - mod). The\n\
modwheel tracking is also a bit odd but reflects the original architecture - \n\
at midpoint on the wheel there is no net modulation, going down affects VCO\n\
in increasing amounts and going up from mid affect the VCF. The control feels\n\
like it should be notched however generally that is not the case with mod\n\
wheels.\n\
\n\
A few notes are required on oscillator sync since by default it will seem to \n\
be quite noisy. The original could only product a single waveform at a single\n\
frequency at any one time. Several emulators, including this one, use a bitone\n\
oscillator which generates complex waveforms. The Bristol Bitone can generate\n\
up to 4 waveforms simultaneously at different levels for 5 different harmonics\n\
and the consequent output is very rich, the waves can be slightly detuned, \n\
the pulse output can be PW modulated. As with all the bristol oscillators that\n\
support sync, the sync pulse is extracted as a postive leading zero crossing.\n\
Unfortunately if the complex bitone output is used as input to sync another\n\
oscillator then the result is far too many zero crossings to extract a good\n\
sync. For the time being you will have to simplify the sync source to get a\n\
good synchronised output which itself may be complex wave. A future release\n\
will add a sync signal from the bitone which will be a single harmonic at the\n\
base frequency and allow both syncing and synchronised waveform outputs to be\n\
arbitrary. For the Stratus this simplification of the sync waveform is done\n\
automatically by the Sync switch, this means the synchronised output sounds\n\
correct but the overall waveform may be simpler.\n\
\n",

"    KORG POLY 800\n\
    -------------\n\
\n\
This is a low cost hybrid synth, somewhere between the Korg PolySix and their\n\
Mono/Poly in that is polyphonic but only has one filter rather than one per\n\
voice that came with the PolySix. It may have also used organ divider circuits\n\
rather than individual oscillators - it did not have glide as a feature which\n\
would be indicative of a divider circuit.\n\
\n\
It featured 8 oscillators that could be applied as either 4 voices with dual\n\
osc or 8 voices with a single osc. The architecture was verging on the\n\
interesting since each oscillator was fead into an individual envelope generator\n\
(described below) and then summed into the single filter, the filter having\n\
another envelope generator, 9 in total. This lead to cost reduction over having\n\
a filter per voice however the single filter leads to breathing, also discussed\n\
below. The envelopes were digitally generated by an on-board CPU.\n\
\n\
The control panel has a volume, global tuning control and a 'Bend' control\n\
that governs the depth of the pitch bend from the joystick and the overall \n\
amount of DCO modulation applied by the joystick. There is no sequencer in\n\
this emulation largely because there are far better options now available than\n\
this had but also due to a shortage of onscreen realestate.\n\
\n\
The Poly, Chord and Hold keys are emulated, hold being a sustain key. The\n\
Chord relearn function works follows:\n\
\n\
    Press the Hold key\n\
    Press the Chord key with 2 seconds\n\
        Press the notes on the keyboard (*)\n\
    Press the Chord key again\n\
\n\
After that the single chord can be played from a single note as a monophonic\n\
instrument. The Chord is saved individually with each memory.\n\
* Note that the chord is only saved if (a) it was played from the GUI keyboard\n\
or (b) the GUI was linked up to any MIDI device as well as the engine. The \n\
reason is that the GUI maintains memories and so if a chord is played on your\n\
actual keyboard then both the engine and the GUI needs a copy, the engine to\n\
be able to play the notes and the GUI to be able to save them.\n\
\n\
The keypanel should function very similar to the original. There is a Prog \n\
button that selects between Program selection or Parameter selection and an \n\
LED should show where the action is. There is the telephone keyboard to enter\n\
program or parameters numbers and an up/down selector for parameter value.\n\
The Bank/Hold selector also works, it fixes the bank number so programs can\n\
be recalled from a single bank with a single button press. The Write function\n\
is as per the original - Press Write, then two digits to save a memory.\n\
\n\
The front panel consists of a data entry panel and a silkscreen of the parameter\n\
numbers (this silkscreen is active in the emulation). Fifty parameters are\n\
available from the original instrument:\n\
\n\
    DE 11 DCO1 Octave transposition +2 octaves\n\
    DE 12 DCO1 Waveform Square or Ramp\n\
    DE 13 DCO1 16' harmonic\n\
    DE 14 DCO1 8' harmonic\n\
    DE 15 DCO1 4' harmonic\n\
    DE 16 DCO1 2' harmonic\n\
    DE 17 DCO1 level\n\
\n\
    DE 18 DCO Double (4 voice) or Single (8 voice)\n\
\n\
    DE 21 DCO2 Octave transposition +2 octaves\n\
    DE 22 DCO2 Waveform Square or Ramp\n\
    DE 23 DCO2 16' harmonic\n\
    DE 24 DCO2 8' harmonic\n\
    DE 25 DCO2 4' harmonic\n\
    DE 26 DCO2 2' harmonic\n\
    DE 27 DCO2 level\n\
    DE 31 DCO2 semitone transpose\n\
    DE 32 DCO2 detune\n\
\n\
    DE 33 Noise level\n\
\n\
    DE 41 Filter cutoff frequency\n\
    DE 42 Filter Resonance\n\
    DE 43 Filter Keyboard tracking off/half/full\n\
    DE 44 Filter Envelope polarity\n\
    DE 45 Filter Envelope amount\n\
    DE 46 Filter Envelope retrigger\n\
\n\
    DE 48 Chorus On/Off\n\
\n\
    DE 51 Env-1 DCO1 Attack\n\
    DE 52 Env-1 DCO1 Decay\n\
    DE 53 Env-1 DCO1 Breakpoint\n\
    DE 54 Env-1 DCO1 Slope\n\
    DE 55 Env-1 DCO1 Sustain\n\
    DE 56 Env-1 DCO1 Release\n\
\n\
    DE 61 Env-2 DCO2 Attack\n\
    DE 62 Env-2 DCO2 Decay\n\
    DE 63 Env-2 DCO2 Breakpoint\n\
    DE 64 Env-2 DCO2 Slope\n\
    DE 65 Env-2 DCO2 Sustain\n\
    DE 66 Env-2 DCO2 Release\n\
\n\
    DE 71 Env-3 Filter Attack\n\
    DE 72 Env-3 Filter Decay\n\
    DE 73 Env-3 Filter Breakpoint\n\
    DE 74 Env-3 Filter Slope\n\
    DE 75 Env-3 Filter Sustain\n\
    DE 76 Env-3 Filter Release\n\
\n\
    DE 81 Mod LFO Frequency\n\
    DE 82 Mod Delay\n\
    DE 83 Mod DCO\n\
    DE 84 Mod VCF\n\
\n\
    DE 86 Midi channel\n\
    DE 87 Midi program change enable\n\
    DE 88 Midi OMNI\n\
\n\
Of these 25 pararmeters, the emulation has changed 88 to be OMNI mode rather \n\
than the original sequence clock as internal or external. This is because the\n\
sequencer function was dropped as explained above.\n\
\n\
Additional to the original many of the controls which are depicted as on/off\n\
are actually continuous. For example, the waveform appears to be either square\n\
or ramp. The emulator allows you to use the up/down Value keys to reproduce\n\
this however if you use the potentiometer then you can gradually move from one\n\
wave to the next. The different harmonics are also not on/off, you can mix\n\
each of them together with different amounts and if you configure a mixture\n\
of waveforms and a bit of detune the sound should widen due to addition of a\n\
bit of phasing within the actual oscillator.\n\
\n\
The envelope generators are not typical ADSR. There is an initial attack from\n\
zero to max gain then decay to a 'Breakpoint'. When this has been reached then\n\
the 'Slope' parameter will take the signal to the Sustain level, then finally\n\
the release rate. The extra step of breakpoint and slope give plenty of extra\n\
flexibility to try and adjust for the loss of a filter per voice and the \n\
emulation has a linear step which should be the same as the original. The\n\
ninth envelope is applied to the single filter and also as the envelope for \n\
the noise signal level.\n\
\n\
The single filter always responded to the highest note on the keyboard. This\n\
gives a weaker overall sound and if playing with two hands then there is a\n\
noticible effect with keytracking - left hand held chords will cause filter\n\
breathing as the right hand plays solos and the keyboard tracking changes \n\
from high to low octaves. Note that the emulator will implement a single\n\
filter if you select DE 46 filter envelope retrigger to be single trigger, it\n\
will be played legato style. If multiple triggers are selected then the\n\
emulator will produce a filter and envelope for each voice.\n\
\n\
Bristol adds a number of extra parameters to the emulator that are not\n\
available from the mouse on the silkscreen and were not a part of the design\n\
of the poly800. You have to select Prog such that the LED is lit next to the\n\
Param display, then select the two digit parameter from the telephone keyboard:\n\
\n\
    DE 28 DCO Sync 2 to 1\n\
    DE 34 DCO-1 PW\n\
    DE 35 DCO-1 PWM\n\
    DE 36 DCO-2 PW\n\
    DE 37 DCO-2 PWM\n\
    DE 38 DCO temperature sensitivity\n\
    DE 67 DCO Glide\n\
\n\
    DE 85 Mod - Uni/Multi per voice or globally\n\
\n\
    DE 57 Envelope Touch response\n\
\n\
    DE 47 Chorus Parameter 0\n\
    DE 58 Chorus Parameter 1\n\
    DE 68 Chorus Parameter 2\n\
    DE 78 Chorus Parameter 3\n\
\n\
If DataEntry 28 is selected for oscillator sync then LFO MOD to DCO-1 is no\n\
longer applied, it only goes to DCO-2. This allows for the interesting sync\n\
modulated slow vibrato of DCO-2. The LFO mod is still applied via the joystick.\n\
\n\
DE 38 global detune will apply both temperature sensitivity to each oscillator\n\
but also fatten out the harmonics by detuning them independently. It is only \n\
calculated at 'note on' which can be misleading - it has no effect on existing\n\
notes which is intentional if misleading.\n\
\n\
DE 57 is a bitmask for the three envelopes to define which ones will give a\n\
response to velocity with a default to '3' for velocity tracking oscillator\n\
gain:\n\
\n\
    value    DEG1    DEG2    DEG3\n\
             DCO1    DCO2    FILT\n\
\n\
      0       -        -       -\n\
      1       V        -       -\n\
      2       -        V       -\n\
      3       V        V       -\n\
      4       -        -       V\n\
      5       V        -       V\n\
      6       -        V       V\n\
      7       V        V       V\n\
\n\
This gives some interesting velocity tracking capabilities where just one osc\n\
can track velocity to introduce harmonic content keeping the filter at a fixed\n\
cutoff frequence. Having a bit of detune applied globally and locally will keep\n\
the sound reasonably fat for each oscillator.\n\
\n\
The filter envelope does not track velocity for any of the distributed voices,\n\
this was intentional since when using high resonance it is not desirable that\n\
the filter cutoff changes with velocity, it tends to be inconsistently \n\
disonant.\n\
\n\
If you want to use this synth with controller mappings then map the value \n\
entry pot to your easiest to find rotary, then click the mouse on the membrane\n\
switch to select which parameter you want to adjust with that control each time.\n\
The emulator is naturally not limited to just 4/8 voices, you can request more\n\
in which case single oscillator will give you the requested number of voices\n\
and double will give you half that amount.\n\
\n\
The Bristol Poly-800 is dedicated to Mark.\n\
\n",

"    Baumann BME-700\n\
    ---------------\n\
\n\
This unusual German synth had a build volume of about 500 units and only one\n\
useful source of information could be found on it: a report on repair work for\n\
one of the few existing examples at www.bluesynths.com. The BME systems were\n\
hand built and judging by some reports on build quality may have been sold in\n\
kit form. The unit was produced in the mid 1970's.\n\
\n\
The synth has a very interesting design, somewhat reminiscent of the Moog Sonic\n\
and Explorer synths. It has two modulating LFO with fairly high top frequency,\n\
two filter and two envelopes. The envelopes are either AR or ASR but they can\n\
be mixed together to generate amongst other features an ADSR, very innovative.\n\
There is only one oscillator but the sound is fattened out by the use of two\n\
parallel filters, one acting as a pure resonator and the other as a full VCF.\n\
\n\
The synth has been left with a minimum of overhead. There are just 8 memory\n\
locations on the front panel with Load, Save and Increment buttons and one\n\
panel of options to adjust a few parameters on the oscillator and filters. It\n\
is possible to get extra memories by loading banks with -load: if you request\n\
starting in memory #21 the emulator will stuff 20 into the bank and 1 into the\n\
memory location. There is no apparant midi channel selector, use -channel <n>\n\
and then stay on it. This could have been put into the options panel however \n\
having midi channel in a memory is generally a bad idea.\n\
\n\
    A. MOD\n\
\n\
        Two LFO:\n\
\n\
            frequency from 0.1 to 100 Hz\n\
            Triangle and Square wave outputs\n\
\n\
        Mix control\n\
\n\
            Mod-1/2 into the VCO FM\n\
            Env-1/Mod-2 into the VCO FM\n\
\n\
    B. Oscillator\n\
\n\
        Single VCO\n\
\n\
            Glide 0 to 10s, on/off.\n\
            PW Man: 5 to 50% duty cycle\n\
            Auto depth:\n\
\n\
                    Envelope-1\n\
                    Mod-1, Mod-1/2, Tri/Square\n\
\n\
            Vibrato depth\n\
            Tuning\n\
                8', 4', 16' transposition\n\
\n\
            Shape\n\
\n\
                continuous control from Square to Tri wave.\n\
\n\
        Mix of noise or VCO output\n\
\n\
    C. Res Filter\n\
\n\
        Sharp (24db/Oct), Flat (12dB/Oct)\n\
        5 frequency switches\n\
\n\
    D. Envelopes\n\
\n\
        Two envelopes\n\
\n\
            Rise time\n\
            Fall Time\n\
            AR/ASR selector\n\
\n\
            Two independent mixes of Env, for VCF and VCA.\n\
\n\
    E. Filter\n\
\n\
        Frequency\n\
        Resonance\n\
        Env/Mod selector\n\
\n\
        Modulation\n\
\n\
            KBD tracking\n\
            Mod-1 or Mod-2, Tri/Square\n\
\n\
    F. Amplifier\n\
\n\
        Mix resonator/filter.\n\
        Volume\n\
\n\
        Mod depth\n\
\n\
            Mod-1 or Mod-2, Tri/Square\n\
\n\
The oscillator is implemented as a non-resampling signal generator, this means\n\
it uses heuristics to estimate the wave at any given time. The harmonic content\n\
is a little thin and although the generation method seems to be correct in how\n\
it interprets signal ramps and drains from an analogue circuit this is one area\n\
of improvement in the emulator. There are options to produce multiple waveforms\n\
described below.\n\
\n\
The resonant filter is implemented with a single Houvilainen and actually only\n\
runs at 24dB/Oct. There are controls for remixing the different taps, a form\n\
of feedforward and when in 'Flat' mod there is more remixing of the poles, this\n\
does generate a slower roll off but gives the signal a bit more warmth than a\n\
pure 12dB/Oct would.\n\
\n\
\n\
There is a selector in the Memory section to access some options:\n\
\n\
    G. Options\n\
\n\
        LFO\n\
\n\
            Synchronise wave to key on events\n\
            Multi LFO (per voice).\n\
\n\
        Oscillator\n\
\n\
            Detune (temperature sensitivity)\n\
            Multi - remix 8' with 16' or 4'.\n\
\n\
        Noise\n\
\n\
            Multi Noise (per voice).\n\
            White/Pink\n\
            Pink Filter\n\
\n\
        ResFilter\n\
\n\
            Sharp Resonance/Remix\n\
            Flat Resonance/Remix\n\
\n\
        Envelope\n\
\n\
            Velocity Sensitive\n\
            Rezero for note on\n\
            Gain\n\
\n\
        Filter\n\
\n\
            Remix\n\
            KBD tracking depth\n\
\n\
The emulator probably gives the best results with the following:\n\
\n\
startBristol -bme700 -mono -hnp -retrig -channel 1\n\
\n\
This gives a monophonic emulation with high note preference and multiple\n\
triggers.\n\
\n\
The options from section G are only loaded under two circumstances: at system\n\
start from the first selected memory location and if the Load button is given\n\
a DoubleClick. All other memory load functions will inherrit the settings that\n\
are currently active.\n\
\n",

"    Bristol BassMaker\n\
    -----------------\n\
\n\
The BassMaker is not actually an emulator, it is a bespoke sequencer design but\n\
based on the capabilities of some of the early analogue sequencers such as the\n\
Korg SQ-10. Supplying this probably leaves bristol open to a lot of feature\n\
requests for sequencer functionaliity and it is stated here that the BassMaker\n\
is supposed to be simple so excess functionality will probably be declined as\n\
there are plenty of other sequencing applications that can provide a richer\n\
feature set.\n\
\n\
The main page gives access to a screen of controls for 16 steps and a total of\n\
4 pages are available for a total of 64 steps. The pages are named 'A' through\n\
'D'. Each step has 5 options:\n\
\n\
    Note: one octave of note selection\n\
    Transpose: +/- one octave transposition of the note.\n\
    Volume: MIDI note velocity\n\
    Controller: MIDI modulation, discussed further below\n\
    Triggers: Note On/Off enablers\n\
\n\
The trigger button gives 4 options indicated by the LED:\n\
\n\
    off: note on/off are sent\n\
    red: only send note_on\n\
    green: only send note_off\n\
    yellow: do not send note on/off\n\
\n\
The 'Controllers' setting has multiple functions which can be selected from\n\
the menu as explained below. The options available are as follows:\n\
\n\
    Send semitone tuning\n\
\n\
    Send glide rate\n\
\n\
    Send modwheel\n\
\n\
    Send expression pedal (controller value)\n\
\n\
    Send Note: the controller will be 12 discrete steps as per the 'Note' \n\
    setting and this note will be sent on the Secondary MIDI channel.\n\
\n\
The semitone tuning and glide work for the majority of the emulations. Some do\n\
not support fine tune controls (Vox, Hammond, others). If you are missing these\n\
capabilities for specific emulators raise a change request on Sourceforge.net.\n\
\n\
At the top of the window there is a panel to manage the sequencer. It has the\n\
following functions:\n\
\n\
    Speed: step rate through the notes\n\
    DutyCycle: ratio of note-on to note-off\n\
\n\
    Start/Pause\n\
    Stop: stop and return to first step/page\n\
\n\
    Direction:\n\
        Up\n\
        Down\n\
        Up/Down\n\
        Random\n\
\n\
    Select: which of the pages to include in the sequence.\n\
    Edit: which page is currently displayed to be edited.\n\
\n\
    Memory:\n\
        0..9 key entry buttons, 1000 memories available\n\
        Load\n\
        Save: doubleclick to save current sequence\n\
\n\
    Menu Panel\n\
        Up, Down menu\n\
        Function (return to previous level)\n\
        Enter: enter submenu or enter value if in submenu\n\
\n\
The menu consists of several tables, these can be stepped through using the Up\n\
and Down arrows to move through the menu and the 'Enter' arrow to select a sub\n\
menu or activate any option. The 'Fn' button returns one level:\n\
\n\
    Memory:\n\
\n\
        Find next free memory upwards\n\
        Find next memory upwards\n\
        Find next memory downwards\n\
\n\
    Copy:\n\
\n\
        Copy current edit page to 'A', 'B', 'C' or 'D'.\n\
\n\
    Control - Set the control value to send:\n\
\n\
        semitone tuning\n\
        glide rate\n\
        modwheel\n\
        expression pedal (controller value)\n\
        note events\n\
\n\
    First midi channel\n\
\n\
        Primary midi channel for note events\n\
\n\
    Second midi channel\n\
\n\
        Secondary midi channel when 'Control' configured to 'Note' events.\n\
\n\
    Global Transpose\n\
\n\
        Transpose the whole sequence up or down 12 semitones\n\
\n\
    Clear - configure default value for all of the:\n\
\n\
        Notes to zero\n\
        Transpose to zero (midpoint)\n\
        Volume to 0.8\n\
        Control to midpoint\n\
        Triggers to on/off\n\
\n\
As of the first release in 0.30.8 large parts of the Controllers functionality\n\
was only lightly tested. If you do not get the results you anticipate you may\n\
require a fix.\n\
\n",

"    Bristol SID\n\
    -----------\n\
\n\
In release 0.40 bristol introduced a piece of code that emulated the Commodore\n\
C64 6581 SID chip. The interface uses byte settings of the 31 chip registers to\n\
be close to the original plus some floating point IO for extracting the audio\n\
signal and configuring some analogue parameters and the 'softSID' is clocked\n\
by the sample extraction process.\n\
\n\
The chip uses integer maths and logic for the oscillators, ring mod, sync and\n\
envelopes and emulates the analogue components of the 6581 with floating point\n\
code, for the filter and S/N generation.\n\
\n\
The oscillators will run as per the original using a single phase accumulator\n\
and 16 bit frequency space. All the waveforms are extracted logically from the\n\
ramp waveform generated by the phase accumulation. Sync and RingMod are also\n\
extracted with the same methods. The noise generation is exor/add as per the\n\
original however the noise signal will not degenerate when mixing waveforms.\n\
The output waves are ANDed together. The bristol control register has an option\n\
for Multi waveforms and when selected each oscillator will have its own phase\n\
accumulator, can have a detune applied and will be mixed by summation rather\n\
than using an AND function.\n\
\n\
The envelope is an 8 bit up/down counter with a single gate bit. All the 4 bit\n\
parameters give rates taken from the chip specifications including the slightly\n\
exponential decay and release. Attack is a linear function and the sustain level\n\
can only be decreased when active as the counter also refuses to count back up\n\
when passed its peak.\n\
\n\
The filter implements a 12dB/Octave multimode chamberlain filter providing LP,\n\
BP and HP signals. This is not the best filter in the world however neither was\n\
the original. An additional 24dB/Octave LP filter has been added, optionally \n\
available and with feedforward to provide 12/18dB signals. Between them the \n\
output can be quite rich.\n\
\n\
The emulator provides some control over the 'analogue' section. The S/N ratio\n\
can be configured from inaudible (just used to prevent denormal of the filter)\n\
up to irritating levels. Oscillator leakage is configurable from none up to \n\
audible levels and the oscillator detune is configurable in cents although\n\
this is a digital parameter and was not a part of the original.\n\
\n\
Voice-3 provides an 8 bit output of its oscillator and envelope via the normal\n\
output registers and the otherwise unused X and Y Analogue registers contain\n\
the Voice-1 and Voice-2 oscillator output.\n\
\n\
The bristol -sid emulator uses two softSID, one generating three audio voices\n\
and a second one providing modulation signals by sampling the voice-3 osc and\n\
env outputs and also by configuring voice-1 to generate noise to the output, \n\
resampling this noise and gating it from voice-3 to get sample and hold. This\n\
would have been possible with the original as well if the output signal were\n\
suitably coupled back on to one of the X/Y_Analogue inputs.\n\
\n\
The emulator has several key assignment modes. The emulator is always just\n\
monophonic but uses internal logic to assign voices. It can be played as a big\n\
mono synth with three voices/oscillators, polyphonically with all voices either\n\
sounding the same or optionally configured individually, and as of this release\n\
a single arpeggiating mode - Poly-3. Poly-3 will assign Voice-1 to the lowest\n\
note, voice-3 to the highest note and will arpeggiate Voice-2 through all other\n\
keys that are pressed with a very high step rate. This is to provide some of\n\
the sounds of the original C64 where fast arpeggiation was used to sounds \n\
chords rather than having to use all the voices. This first implementation \n\
does not play very well in Poly-3, a subsequent release will probably have a\n\
split keyboard option where one half will arpeggiate and the other half will\n\
play notes.\n\
\n\
This is NOT a SID player, that would require large parts of the C64 to also be\n\
emulated and there are plenty of SID players already available.\n\
\n\
Bristol again thanks Andrew Coughlan, here for proposing the implementation of\n\
a SID chip which turned out to be a very interesting project.\n\
\n",

NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */
NULL, /* Not used */

};
