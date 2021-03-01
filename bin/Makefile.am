AUTOMAKE_OPTIONS = foreign

LDFLAGS = -Bdynamic -lm -lpthread `pkg-config --silence-errors --libs jack`
AM_CFLAGS = -pthread -Wall -g -I. -I$(srcdir)/../include/bristol @BRISTOL_HAS_JACK@ @BRISTOL_HAS_JACK_MIDI@ @JACK_CFLAGS@

#bin_PROGRAMS = bristoljackstats bristolnotegen bristolNRPgen
bin_PROGRAMS = bristoljackstats

bristoljackstats_LDFLAGS = -Bdynamic
bristoljackstats_LDADD = @JACK_LIBS@ -lm -lpthread

#bristolnotegen_LDFLAGS = -Bdynamic
#bristolnotegen_LDADD = @JACK_LIBS@ -lm -lpthread -lbristolmidi -L../libbristolmidi

#bristolNRPgen_LDFLAGS = -Bdynamic
#bristolNRPgen_LDADD = @JACK_LIBS@ -lm -lpthread -lbristolmidi -L../libbristolmidi

bristoljackstats_SOURCES = bristoljackstats.c
#bristolnotegen_SOURCES = bristolnotegen.c

#install:
#	cp $(srcdir)/startBristol
#	$(INSTALL) -m 755 $(srcdir)/startBristol $(DESTDIR)$(bindir)
#	$(INSTALL) -m 755 $(srcdir)/bristoljackstats $(DESTDIR)$(bindir)

