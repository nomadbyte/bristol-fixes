
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

char *helptext = "\nA synthesiser emulation package.\n\
\n\
    You should start this package with the startBristol script. This script\n\
    will start up the bristol synthesiser binaries evaluating the correct\n\
    library paths and executable paths. There are emulation, synthesiser,\n\
    operational and GUI parameters:\n\
\n\
    Emulation:\n\
\n\
        -mini              - moog mini\n\
        -explorer          - moog voyager\n\
        -voyager           - moog voyager electric blue\n\
        -memory            - moog memory\n\
        -sonic6            - moog sonic 6\n\
        -mg1               - moog/realistic mg-1 concertmate\n\
        -hammond           - hammond module (deprecated, use -b3)\n\
        -b3                - hammond B3 (default)\n\
        -prophet           - sequential circuits prophet-5\n\
        -pro52             - sequential circuits prophet-5/fx\n\
        -pro10             - sequential circuits prophet-10\n\
        -pro1              - sequential circuits pro-one\n\
        -rhodes            - fender rhodes mark-I stage 73\n\
        -rhodesbass        - fender rhodes bass piano\n\
        -roadrunner        - crumar roadrunner electric piano\n\
        -bitone            - crumar bit 01\n\
        -bit99             - crumar bit 99\n\
        -bit100            - crumar bit + mods\n\
        -stratus           - crumar stratus synth/organ combo\n\
        -trilogy           - crumar trilogy synth/organ/string combo\n\
        -obx               - oberheim OB-X\n\
        -obxa              - oberheim OB-Xa\n\
        -axxe              - arp axxe\n\
        -odyssey           - arp odyssey\n\
        -arp2600           - arp 2600\n\
        -solina            - arp/solina string ensemble\n\
        -polysix           - korg polysix\n\
        -poly800           - korg poly-800\n\
        -monopoly          - korg mono/poly\n\
        -ms20              - korg ms20 (unfinished: -libtest only)\n\
        -vox               - vox continental\n\
        -voxM2             - vox continental super/300/II\n\
        -juno              - roland juno-60\n\
        -jupiter           - roland jupiter-8\n\
        -bme700            - baumann bme-700\n\
        -bm                - bristol bassmaker sequencer\n\
        -dx                - yamaha dx-7\n\
        -cs80              - yamaha cs-80 (unfinished)\n\
        -sidney            - commodore-64 SID chip synth\n\
        -melbourne         - commodore-64 SID polyphonic synth (unfinished)\n\
        -granular          - granular synthesiser (unfinished)\n\
        -aks               - ems synthi-a (unfinished)\n\
        -mixer             - 16 track mixer (unfinished: -libtest only)\n\
\n\
    Synthesiser:\n\
\n\
        -voices <n>        - operate with a total of 'n' voices (32)\n\
        -mono              - operate with a single voice (-voices 1)\n\
        -lnp               - low note preference (-mono)\n\
        -hnp               - high note preference (-mono)\n\
        -nnp               - no/last note preference (-mono)\n\
        -retrig            - monophonic note logic legato trigger (-mono)\n\
        -lvel              - monophonic note logic legato velocity (-mono)\n\
        -channel <c>       - initial midi channel selected to 'c' (default 1)\n\
        -lowkey <n>        - minimum MIDI note response (0)\n\
        -highkey <n>       - maximum MIDI note response (127)\n\
        -detune <%>        - 'temperature sensitivity' of emulation (0)\n\
        -gain <gn>         - emulator output signal gain (default 1)\n\
        -pwd <s>           - pitch wheel depth (2 semitones)\n\
        -velocity <v>      - MIDI velocity mapping curve (510) (-mvc)\n\
        -glide <s>         - MIDI glide duration (5)\n\
        -emulate <name>    - search for the named synth or exit\n\
        -register <name>   - name used for jack and alsa device regisration\n\
        -lwf               - emulator lightweight filters\n\
        -nwf               - emulator default filters\n\
        -wwf               - emulator welterweight filters\n\
        -hwf               - emulator heavyweight filters\n\
        -blo <h>           - maximum # band limited harmonics (31)\n\
        -blofraction <f>   - band limiting nyquist fraction (0.8)\n\
        -scala <file>      - read the scala .scl tonal mapping table\n\
\n\
    User Interface:\n\
\n\
        -quality <n>       - color cache depth (bbp 2..8) (6)\n\
        -grayscale <n>     - color or BW display (0..5) (0 = color)\n\
        -antialias <n>     - antialias depth (0..100%) (30)\n\
        -aliastype <s>     - antialias type (pre/texture/all)\n\
        -opacity <n>       - opacity of the patch layer 20..100% (50)\n\
        -scale <s>         - initial windowsize, fs = fullscreen (1.0)\n\
        -width <n>         - the pixel width of the GUI window\n\
        -autozoom          - flip between min and max window on Enter/Leave\n\
        -raise             - disable auto raise on max resize\n\
        -lower             - disable auto lower on min resize\n\
        -rud               - constrain rotary tracking to up/down\n\
        -pixmap            - use the pixmap interface rather than ximage\n\
        -dct <ms>          - double click timeout (250 ms)\n\
        -tracking          - disable MIDI keyboard latching state\n\
        -keytoggle         - disable MIDI \n\
        -load <m>          - load memory number 'm' (default 0)\n\
        -neutral           - initialise the emulator with a 'null' patch\n\
        -import <pathname> - import memory from file into synth\n\
        -mbi <m>           - master bank index (0)\n\
        -activesense <m>   - active sense rate (2000 ms)\n\
        -ast <m>           - active sense timeout (15000 ms)\n\
        -mct <m>           - midi cycle timeout (50 ms)\n\
        -ar|-aspect        - ignore emulator requested aspect ratio\n\
        -iconify           - start with iconified window\n\
        -window            - toggle switch to enable X11 window interfacen\n\
        -cli               - enable command line interface\n\
        -libtest           - gui test option, engine not invoked\n\
\n\
        Gui keyboard shortcuts:\n\
\n\
            <Ctrl> 's'     - save settings to current memory\n\
            <Ctrl> 'l'     - (re)load current memory\n\
            <Ctrl> 'x'     - exchange current with previous memory\n\
            <Ctrl> '+'     - load next memory\n\
            <Ctrl> '-'     - load previous memory\n\
            <Ctrl> '?'     - show emulator help information\n\
            <Ctrl> 'h'     - show emulator help information\n\
            <Ctrl> 'r'     - show application readme information\n\
            <Ctrl> 'k'     - show keyboard shortcuts\n\
            <Ctrl> 'p'     - screendump to /tmp/<synth>.xpm\n\
            <Ctrl> 't'     - toggle opacity\n\
            <Ctrl> 'o'     - decrease opacity of patch layer\n\
            <Ctrl> 'O'     - increase opacity of patch layer\n\
            <Ctrl> 'w'     - display warranty\n\
            <Ctrl> 'g'     - display GPL (copying conditions)\n\
            <Shift> '+'    - increase window size\n\
            <Shift> '-'    - decrease window size\n\
            <Shift> 'Enter'- toggle window between full screen size\n\
            'UpArrow'      - controller motion up (shift key accelerator)\n\
            'DownArrow'    - controller motion down (shift key accelerator)\n\
            'RightArrow'   - more controller motion up (shift key accelerator)\n\
            'LeftArrow'    - more controller motion down (shift key accelerator)\n\
\n\
    Operational:\n\
\n\
        General:\n\
\n\
            -engine        - don't start engine (connect to existing engine)\n\
            -gui           - don't start gui (only start engine)\n\
            -server        - run engine as a permanant server\n\
            -daemon        - run engine as a detached permanant server\n\
            -watchdog <s>  - audio thread initialisation timeout (30s)\n\
            -log           - redirect diagnostic to $HOME/.bristol/log\n\
            -syslog        - redirect diagnostic to syslog\n\
            -console       - log all messages to console (must be 1st option)\n\
            -exec          - run all subprocesses in background\n\
            -stop          - terminate all bristol engines\n\
            -exit          - terminate all bristol engines and GUI\n\
            -kill <-emu>   - terminate all bristol processes emulating -emu\n\
            -cache <path>  - memory and profile cache location (~/.bristol)\n\
            -memdump <path>- copy full set of memories to <path>, with -emulate\n\
            -debug <1-16>  - debuging level (0)\n\
            -readme [-<e>] - show readme [for emulator <e>] to console\n\
            -glwf          - global lightweight filters - no overrides\n\
            -host <h>      - connect to engine on host 'h' (localhost)\n\
            -port <p>      - connect to engine on TCP port 'p' (default 5028)\n\
            -quiet         - redirect diagnostic output to /dev/null\n\
            -gmc           - open a MIDI connection to the brighton GUI\n\
            -oss           - use OSS defaults for audio and MIDI\n\
            -alsa          - use ALSA defaults for audio and MIDI (default)\n\
            -jack          - use Jack defaults for audio and MIDI\n\
            -jackstats     - avoid use of bristoljackstats\n\
            -jsmuuid <UUID>- jack session unique identifier\n\
            -jsmfile <path>- jack session setting path\n\
            -jsmd <ms>     - jack session file load delay (5000)\n\
            -sleep <n>     - delay init for 'n' seconds (jsm patch)\n\
            -session       - disable session management\n\
            -jdo           - use separate Jack clients for audio and MIDI\n\
            -osc           - use OSC for control interface (unfinished)\n\
            -forward       - disable MIDI event forwarding globally\n\
            -localforward  - disable emulator gui->engine event forwarding\n\
            -remoteforward - disable emulator engine->gui event forwarding\n\
            -o <filename>  - Duplicate raw audio output data to file\n\
            -nrp           - enable NPR support globally\n\
            -enrp          - enable NPR/DE support in engine\n\
            -gnrp          - enable NPR/RP/DE support in GUI\n\
            -nrpcc <n>     - size of NRP controller table (128)\n\
\n\
        Audio driver:\n\
\n\
            -audio [oss|alsa|jack] - audio driver selection (alsa)\n\
            -audiodev <dev>        - audio device selection\n\
            -count <samples>       - sample period count (256)\n\
            -outgain <gn>          - digital output signal gain (default 4)\n\
            -ingain <gn>           - digital input signal gain (default 4)\n\
            -preload <periods>     - configure preload buffer count (default 4)\n\
            -rate <hz>             - sample rate (44100)\n\
            -priority <p>          - audio RT priority, 0=no realtime (75)\n\
            -autoconn              - attempt jack port auto-connect\n\
            -multi <c>             - register 'c' IO channels (jack only)\n\
            -migc <f>              - multi IO input gain scaling (jack only)\n\
            -mogc <f>              - multi IO output gain scaling (jack only)\n\
\n\
        Midi driver:\n\
\n\
            -midi [oss|[raw]alsa|jack] - midi driver selection (alsa)\n\
            -mididev <dev>             - midi device selection\n\
            -seq                       - use the ALSA SEQ interface (default)\n\
            -mididbg                   - midi debug-1 enable\n\
            -mididbg2                  - midi debug-2 enable\n\
            -sysid                     - MIDI SYSEX system identifier\n\
\n\
        LADI driver (level 1 compliant):\n\
\n\
            -ladi brighton             - only execute LADI in GUI\n\
            -ladi bristol              - only execute LADI in engine\n\
            -ladi <memory>             - LADI state memory index (1024)\n\
\n\
    Audio drivers are PCM/PCM_plug or Jack. Midi drivers are either OSS/ALSA\n\
    rawmidi interface, or ALSA SEQ. Multiple GUIs can connect to the single\n\
    audio engine which then operates multitimbrally.\n\
\n\
    The LADI interfaces does not use a state file but a memory in the normal\n\
    memory locations. This should typically be outside of the range of the\n\
    select buttons for the synth and the default of 1024 is taken for this\n\
    reason.\n\
\n\
    Examples:\n\
\n\
    startBristol\n\
\n\
        Print a terse help message.\n\
\n\
    startBristol -v -h\n\
\n\
        Hm, if you're reading this you found these switches already.\n\
\n\
    startBristol -mini\n\
\n\
        Run a minimoog using ALSA interface for audio and midi seq. This is\n\
        equivalent to all the following options:\n\
        -mini -alsa -audiodev plughw:0,0 -midi seq -count 256 -preload 8 \n\
        -port 5028 -voices 32 -channel 1 -rate 44100 -gain 4 -ingain 4\n\
\n\
    startBristol -alsa -mini\n\
\n\
        Run a minimoog using ALSA interface for audio and midi. This is\n\
        equivalent to all the following options:\n\
        -mini -audio alsa -audiodev plughw:0,0 -midi alsa -mididev hw:0\n\
        -count 256 -preload 8 -port 5028 -voices 32 -channel 1 -rate 44100\n\
\n\
    startBristol -explorer -voices 1 -oss\n\
\n\
        Run a moog explorer as a monophonic instrument, using OSS interface for\n\
        audio and midi.\n\
\n\
    startBristol -prophet -channel 3\n\
\n\
        Run a prophet-5 using ALSA for audio and midi on channel 3.\n\
\n\
    startBristol -b3 -count 512 -preload 2\n\
\n\
        Run a hammond b3 with a buffer size of 512 samples, and preload two \n\
        such buffers before going active. Some Live! cards need this larger\n\
        buffer size with ALSA drivers.\n\
\n\
    startBristol -oss -audiodev /dev/dsp1 -vox -voices 8\n\
\n\
        Run a vox continental using OSS device 1, and default midi device\n\
        /dev/midi0. Operate with just 8 voices.\n\
\n\
    startBristol -b3 -audio alsa -audiodev plughw:0,0 -seq -mididev 128.0\n\
\n\
        Run a B3 emulation over the ALSA PCM plug interface, using the ALSA\n\
        sequencer over client 128, port 0.\n\
\n\
    startBristol -juno &\n\
    startBristol -prophet -channel 2 -engine\n\
\n\
        Start two synthesisers, a juno and a prophet. Both synthesisers will\n\
        will be executed on one engine (multitimbral) with 32 voices between \n\
        them. The juno will be on default midi channel (1), and the prophet on\n\
        channel 2. Output over the same default ALSA audio device.\n\
\n\
    startBristol -juno &\n\
    startBristol -port 5029 -audio oss -audiodev /dev/dsp1 -mididev /dev/midi1\n\
\n\
        Start two synthesisers, a juno on the first ALSA soundcard, and a\n\
        mini on the second OSS soundcard. Each synth is totally independent\n\
        and runs with 32 voice polyphony (looks nice, not been tested).\n\
\n\
The location of the bristol binaries can be specified in the BRISTOL\n\
environment variable. Private memory and MIDI controller mapping files can\n\
be found in the directory BRISTOL_CACHE and defaults to $HOME/.bristol\n\
\n\
Setting the environment variable BRISTOL_LOG_CONSOLE to any value will result\n\
in the bristol logging output going to your console window without formatted\n\
timestamps\n\
\n\
Korg Inc. of Japan is the rightful owner of the Korg and Vox trademarks, and\n\
the Polysix, Mono/Poly, Poly-800, MS-20 and Continental tradenames. Their own\n\
Vintage Collection provides emulations for a selection of their classic\n\
synthesiser range, this product is in no manner related to Korg other than\n\
giving homage to their great instruments.\n\
\n\
Bristol is in no manner associated with any of the original manufacturers of\n\
any of the emulated instruments. All names and trademarks are property of\n\
their respective owners.\n\
\n\
    author:   Nick Copeland\n\
    email:    nickycopeland@hotmail.com\n\
\n\
    http://bristol.sourceforge.net\n\
\n\
";

char *gplnotice = "\
Copyright (c) by Nick Copeland <nickycopeland@hotmail.com> 1996,2012\n\
This program comes with ABSOLUTELY NO WARRANTY; for details type `<Ctrl> w'.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions; type `<Ctrl> g' for details of GPL terms.\n";

char *gplwarranty = "\
THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.\n\
EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n\
PROVIDE THE PROGRAM “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR\n\
IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY\n\
AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND\n\
PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU\n\
ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n";

char *gplconditions = "\n\
The following are terms and conditions of the GNU General Public License. For\n\
full details please see <http://www.gnu.org/licenses/>\n\n\
4. Conveying Verbatim Copies.\n\
\n\
You may convey verbatim copies of the Program's source code as you receive it, in any medium, provided that you conspicuously and appropriately publish on each copy an appropriate copyright notice; keep intact all notices stating that this License and any non-permissive terms added in accord with section 7 apply to the code; keep intact all notices of the absence of any warranty; and give all recipients a copy of this License along with the Program.\n\
\n\
You may charge any price or no price for each copy that you convey, and you may offer support or warranty protection for a fee.\n\
5. Conveying Modified Source Versions.\n\
\n\
You may convey a work based on the Program, or the modifications to produce it from the Program, in the form of source code under the terms of section 4, provided that you also meet all of these conditions:\n\
\n\
    * a) The work must carry prominent notices stating that you modified it, and giving a relevant date.\n\
    * b) The work must carry prominent notices stating that it is released under this License and any conditions added under section 7. This requirement modifies the requirement in section 4 to “keep intact all notices”.\n\
    * c) You must license the entire work, as a whole, under this License to anyone who comes into possession of a copy. This License will therefore apply, along with any applicable section 7 additional terms, to the whole of the work, and all its parts, regardless of how they are packaged. This License gives no permission to license the work in any other way, but it does not invalidate such permission if you have separately received it.\n\
    * d) If the work has interactive user interfaces, each must display Appropriate Legal Notices; however, if the Program has interactive interfaces that do not display Appropriate Legal Notices, your work need not make them do so.\n\
\n\
A compilation of a covered work with other separate and independent works, which are not by their nature extensions of the covered work, and which are not combined with it such as to form a larger program, in or on a volume of a storage or distribution medium, is called an “aggregate” if the compilation and its resulting copyright are not used to limit the access or legal rights of the compilation's users beyond what the individual works permit. Inclusion of a covered work in an aggregate does not cause this License to apply to the other parts of the aggregate.\n\
6. Conveying Non-Source Forms.\n\
\n\
You may convey a covered work in object code form under the terms of sections 4 and 5, provided that you also convey the machine-readable Corresponding Source under the terms of this License, in one of these ways:\n\
\n\
    * a) Convey the object code in, or embodied in, a physical product (including a physical distribution medium), accompanied by the Corresponding Source fixed on a durable physical medium customarily used for software interchange.\n\
    * b) Convey the object code in, or embodied in, a physical product (including a physical distribution medium), accompanied by a written offer, valid for at least three years and valid for as long as you offer spare parts or customer support for that product model, to give anyone who possesses the object code either (1) a copy of the Corresponding Source for all the software in the product that is covered by this License, on a durable physical medium customarily used for software interchange, for a price no more than your reasonable cost of physically performing this conveying of source, or (2) access to copy the Corresponding Source from a network server at no charge.\n\
    * c) Convey individual copies of the object code with a copy of the written offer to provide the Corresponding Source. This alternative is allowed only occasionally and noncommercially, and only if you received the object code with such an offer, in accord with subsection 6b.\n\
    * d) Convey the object code by offering access from a designated place (gratis or for a charge), and offer equivalent access to the Corresponding Source in the same way through the same place at no further charge. You need not require recipients to copy the Corresponding Source along with the object code. If the place to copy the object code is a network server, the Corresponding Source may be on a different server (operated by you or a third party) that supports equivalent copying facilities, provided you maintain clear directions next to the object code saying where to find the Corresponding Source. Regardless of what server hosts the Corresponding Source, you remain obligated to ensure that it is available for as long as needed to satisfy these requirements.\n\
    * e) Convey the object code using peer-to-peer transmission, provided you inform other peers where the object code and Corresponding Source of the work are being offered to the general public at no charge under subsection 6d.\n\
\n\
A separable portion of the object code, whose source code is excluded from the Corresponding Source as a System Library, need not be included in conveying the object code work.\n\
\n\
A “User Product” is either (1) a “consumer product”, which means any tangible personal property which is normally used for personal, family, or household purposes, or (2) anything designed or sold for incorporation into a dwelling. In determining whether a product is a consumer product, doubtful cases shall be resolved in favor of coverage. For a particular product received by a particular user, “normally used” refers to a typical or common use of that class of product, regardless of the status of the particular user or of the way in which the particular user actually uses, or expects or is expected to use, the product. A product is a consumer product regardless of whether the product has substantial commercial, industrial or non-consumer uses, unless such uses represent the only significant mode of use of the product.\n\
\n\
“Installation Information” for a User Product means any methods, procedures, authorization keys, or other information required to install and execute modified versions of a covered work in that User Product from a modified version of its Corresponding Source. The information must suffice to ensure that the continued functioning of the modified object code is in no case prevented or interfered with solely because modification has been made.\n\
\n\
If you convey an object code work under this section in, or with, or specifically for use in, a User Product, and the conveying occurs as part of a transaction in which the right of possession and use of the User Product is transferred to the recipient in perpetuity or for a fixed term (regardless of how the transaction is characterized), the Corresponding Source conveyed under this section must be accompanied by the Installation Information. But this requirement does not apply if neither you nor any third party retains the ability to install modified object code on the User Product (for example, the work has been installed in ROM).\n\
\n\
The requirement to provide Installation Information does not include a requirement to continue to provide support service, warranty, or updates for a work that has been modified or installed by the recipient, or for the User Product in which it has been modified or installed. Access to a network may be denied when the modification itself materially and adversely affects the operation of the network or violates the rules and protocols for communication across the network.\n\
\n\
Corresponding Source conveyed, and Installation Information provided, in accord with this section must be in a format that is publicly documented (and with an implementation available to the public in source code form), and must require no special password or key for unpacking, reading or copying.\n\n";

char *summarytext = "\
arp2600 \
axxe \
bassmaker \
bitone \
bit99 \
bit100 \
BME700 \
dx \
explorer \
hammondB3 \
juno \
jupiter8 \
memoryMoog \
mini \
monopoly \
obx \
obxa \
odyssey \
poly \
poly800 \
pro1 \
prophet \
prophet10 \
prophet52 \
realistic \
rhodes \
rhodesbass \
roadrunner \
sidney \
solina \
sonic6 \
stratus \
trilogy \
vox \
voxM2 \
voyager \
";

char *oldsummarytext = "\
arp2600 \
axxe \
b3 \
bm \
bme700 \
bit1 \
bit99 \
bit100 \
dx \
explorer \
juno \
jupiter \
mg1 \
memoryMoog \
mini \
monopoly \
obx \
obxa \
odyssey \
poly800 \
polysix \
pro1 \
pro10 \
pro5 \
pro52 \
rhodes \
rhodesbass \
roadrunner \
sidney \
solina \
sonic6 \
stratus \
trilogy \
vox \
voxm2 \
voyager \
";

char *summarytextu = "\
mini \
explorer \
voyager \
memory \
sonic6 \
mg1 \
b3 \
prophet \
pro52 \
pro10 \
pro1 \
rhodes \
rhodesbass \
roadrunner \
bitone \
bit99 \
bit100 \
stratus \
trilogy \
obx \
obxa \
axxe \
odyssey \
arp2600 \
solina \
polysix \
poly800 \
monopoly \
vox \
voxm2 \
juno \
jupiter \
bme700 \
bm \
dx \
sidney \
";

