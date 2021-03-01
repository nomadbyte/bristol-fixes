AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/brighton -I/usr/X11R6/include/ -I. -O0 @BRIGHTON_HAS_X11@ @BRIGHTON_HAS_AUTOZOOM@

noinst_LIBRARIES = libbrighton.a
#libbrighton_a_LDFLAGS=-export-dynamic -version-info @BRISTOL_SO_VERSION@ @BRIGHTON_LIBXLIBS@ @BRIGHTON_LIBXLIBS@
#libbrighton_a_LIBADD= @BRIGHTON_LIBB11@ -lm
libbrighton_a_SOURCES = brightonBitmaps.c brightonButton.c brightonC.c brightonColorMgt.c brightonDevice.c brightonDispatch.c brightonDisplay.c brightonDisplayMgt.c brightonEventHandlers.c brightonHammond.c brightonInterface.c brightonKbd.c brightonLayer.c brightonPanelMgt.c brightonPic.c brightonRender.c brightonRotary.c brightonScale.c brightonShadowMgt.c brightonTouchpanel.c brightonVu.c brightonWindowMgt.c brightonXpmRead.c brightonkeymappings.h brightonMenu.c brightonLedBlock.c brightonHButton.c brightonLever.c brightonModWheel.c brightonLed.c brightonSlowTimer.c brightonFastTimer.c brightonRibbonKbd.c

