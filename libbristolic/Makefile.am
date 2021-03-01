AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/bristol -DBRISOL_HAS_ALSA=@BRISTOL_HAS_ALSA@
#libbristolic_la_LDFLAGS=-export-dynamic -version-info @BRISTOL_SO_VERSION@

noinst_LIBRARIES = libbristolic.a
libbristolic_a_SOURCES = sid.c

#bin_PROGRAMS = sidtest
#sidtest_LDFLAGS = -Bdynamic -L../libbristolic/.libs
#sidtest_LDADD = @JACK_LIBS@ @ALSA_LIBS@ -lbristolic -lm -lpthread
#sidtest_SOURCES = sidtest.c

