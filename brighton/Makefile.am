AUTOMAKE_OPTIONS = foreign

AM_CFLAGS = -pthread -Wall -g -I$(srcdir)/../include/brighton -I$(srcdir)/../include/bristol -DBRISTOL_HAS_ALSA=@BRISTOL_HAS_ALSA@ @BRIGHTON_HAS_X11@ -DBRISTOL_VOICECOUNT=@_BRISTOL_VOICES@

bin_PROGRAMS = brighton
brighton_LDFLAGS = -Bdynamic -L../libbrighton/ -L../libbristolmidi/.libs @BRIGHTON_LIBXLIBS@ -L/usr/X11R6/lib -L../libbvg
brighton_LDADD = -lbrighton -lbvg @BRIGHTON_LIBB11@ @BRIGHTON_LIBX11@ @BRIGHTON_LIBXEXT@ -lbristolmidi @ALSA_LIBS@ -lm -lpthread

brighton_SOURCES = brightonArp2600.c brightonAxxe.c brighton.c brightonControllers.c brightonDX.c brightonExplorer.c brightonHammondB3.c brightonHammond.c brightonJuno.c brightonMemoryMoog.c brightonMini.c brightonMixer.c brightonMixerMemory.c brightonMixerMenu.c brightonMS20.c brightonOBXa.c brightonOBX.c brightonOdyssey.c brightonPoly6.c brightonPoly.c brightonProphet10.c brightonProphet52.c brightonProphet.c brightonRhodesBass.c brightonRhodes.c brightonRoutines.c brightonSAks.c brightonVox.c brightonKeyboards.h brightonKeys.h brightonMini.h brightonMixer.h brightonMixerMemory.h brightonhelp.h brightonSolina.c brightonRoadRunner.c brightonGranular.c brightonRealistic.c brightonVoxM2.c brightonJupiter.c brightonBitOne.c brightonMaster.c brightonCS80.c brightonProOne.c brightonVoyager.c brightonSonic6.c brightonTrilogy.c brightonStratus.c brightonPoly800.c brightonBME700.c brightonBassMaker.c brightonSID.c brightonSID2.c brightonSID2.h brightonreadme.h brightonCLI.c brightonVImages.h

