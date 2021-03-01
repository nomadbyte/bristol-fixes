AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/slab -I$(srcdir)/../include/bristol -DBRISTOL_HAS_OSS=@BRISTOL_HAS_OSS@ @BRISTOL_HAS_DRAIN@ -DBRISTOL_HAS_ALSA=@BRISTOL_HAS_ALSA@ @BRISTOL_HAS_JACK@ @BRISTOL_HAS_JACK_MIDI@ @BRISTOL_HAS_JACK_SESSION@ @ALSA_CFLAGS@ @JACK_CFLAGS@ @BRISTOL_HAS_PA@

#libbristolaudio_la_LDFLAGS=-export-dynamic -version-info @BRISTOL_SO_VERSION@
#libbristolaudio_la_LIBADD=@ALSA_LIBS@ @JACK_LIBS@

noinst_LIBRARIES = libbristolaudio.a
libbristolaudio_a_SOURCES = audioEngineALSA.c audioGUIALSA.c audioMastering.c audioEngine.c audioGUI.c audioEngineOSS.c audioGUIOSS.c audioEngineJack.c

