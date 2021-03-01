AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/brighton @BRIGHTON_HAS_XIMAGE@

#libC11_la_LDFLAGS= -export-dynamic -version-info @BRISTOL_SO_VERSION@
#libC11_la_LIBADD=

noinst_LIBRARIES = libC11.a
libC11_a_SOURCES = bColor.c bDisplay.c bEvent.c bRender.c bRoutines.c bWindow.c

