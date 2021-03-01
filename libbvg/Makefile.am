AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/brighton -DBRISOL_HAS_ALSA=@BRISTOL_HAS_ALSA@

noinst_LIBRARIES = libbvg.a
#libbvg_a_LDFLAGS=-export-dynamic -version-info @BRISTOL_SO_VERSION@
libbvg_a_SOURCES = bvg.c font1.h

#bin_PROGRAMS = sidtest
#sidtest_LDFLAGS = -Bdynamic -L../libbvg/.libs
#sidtest_LDADD = @JACK_LIBS@ @ALSA_LIBS@ -lbvg -lm -lpthread
#sidtest_SOURCES = sidtest.c

