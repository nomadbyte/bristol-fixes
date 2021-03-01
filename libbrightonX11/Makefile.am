AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/brighton -I/usr/X11R6/include/ @BRIGHTON_HAS_XIMAGE@ @BRIGHTON_HAS_SHMIMAGE@ @BRIGHTON_HAS_X11@

#libB11_a_LDFLAGS= -export-dynamic -version-info @BRISTOL_SO_VERSION@
#libB11_a_LIBADD= -L/usr/X11R6/lib -lX11

noinst_LIBRARIES = libB11.a
libB11_a_SOURCES = bColor.c bDisplay.c bEvent.c bRender.c bRoutines.c bWindow.c

