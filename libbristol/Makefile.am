AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/bristol -I$(srcdir)/../include/slab -DBRISTOL_HAS_ALSA=@BRISTOL_HAS_ALSA@ @BRISTOL_HAS_PA@
#libbristol_a_LDFLAGS=-export-dynamic -version-info @BRISTOL_SO_VERSION@ -march=core2 -msse -msse2 -mfpmath=sse -ffast-math -fomit-frame-pointer -O3
#libbristol_la_LIBADD=$(top_builddir)/libbristol/libbristol.la

noinst_LIBRARIES = libbristol.a
libbristol_a_SOURCES = audioRoutines.c bristolcdefs.c debugging.c mixroutines.c opmgt.c

