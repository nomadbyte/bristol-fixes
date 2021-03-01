AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/slab -I$(srcdir)/../include/bristol @ALSA_CFLAGS@ @JACK_CFLAGS@ @BRISTOL_HAS_JACK_MIDI@ -DBRISTOL_HAS_ALSA=@BRISTOL_HAS_ALSA@ @BRISTOL_HAS_JACK@ -O1
libbristolmidi_la_LDFLAGS=-export-dynamic -version-info @BRISTOL_SO_VERSION@
libbristolmidi_la_LIBADD=@ALSA_LIBS@ @JACK_LIBS@ -lm

lib_LTLIBRARIES = libbristolmidi.la
libbristolmidi_la_SOURCES = inetClient.c inetServer.c interface.c midiALSADevMan.c midiDevMan.c midiMsgMan.c midiOSSDevMan.c midiParse.c midiRoutines.c midiSeqDevMan.c midiTCPDevMan.c slabmessages.h socketToolKit.c slabmessages.h midiFileMgt.c midiJACKDevMan.c scalaParse.c

