#											   -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ(2.60)
AC_INIT(brighton/brightonhelp.h)

BRISTOL_MAJOR_VERSION=0
BRISTOL_MINOR_VERSION=60
BRISTOL_MICRO_VERSION=11

BRISTOL_API_CURRENT=0
BRISTOL_API_REVISION=611
BRISTOL_API_AGE=0

AC_SUBST(BRISTOL_MAJOR_VERSION)
AC_SUBST(BRISTOL_MINOR_VERSION)
AC_SUBST(BRISTOL_MICRO_VERSION)

BRISTOL_VERSION=${BRISTOL_MAJOR_VERSION}.${BRISTOL_MINOR_VERSION}.${BRISTOL_MICRO_VERSION}
AC_SUBST(BRISTOL_VERSION)

BRISTOL_SO_VERSION=${BRISTOL_API_CURRENT}:${BRISTOL_API_REVISION}:${BRISTOL_API_AGE}
AC_SUBST(BRISTOL_SO_VERSION)

#AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_INIT_AUTOMAKE(bristol,${BRISTOL_VERSION})

AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
# Checks for libraries.
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# if we have a previous version pan the build
showErrMsg1()
{
	echo
	echo "*************************************************************"
	echo "*                                                           *"
	echo "*  A previous bristol installation exists on this system.   *"
	echo "*                                                           *"
	echo "*  If you have bristol installed from your package manager  *"
	echo "*  you really need to remove it first. If you have it from  *"
	echo "*  a previous bristol build then you can remove it with a   *"
	echo "*  'make uninstall' from the previous build directory.      *"
	echo "*                                                           *"
	echo "*  If you understand the risks or just want to be lazy then *"
	echo "*  you can override this test with --disable-version-check  *"
	echo "*  however the author advises against such a workaround.    *"
	echo "*                                                           *"
	echo "*************************************************************"
	echo
	exit 1
}

TRY_VC=yes
AC_ARG_ENABLE(version_check, [  --disable-version-check	ignore preinstalled versions], TRY_VC=no , TRY_VC=yes )
if test "x$TRY_VC" = 'xyes'
then
	which startBristol > /dev/null 2>&1
	if test $? == 0; then showErrMsg1; fi
	which bristol > /dev/null 2>&1
	if test $? == 0; then showErrMsg1; fi
	if test -f ${prefix}/bin/bristol; then showErrMsg1; fi
fi

# Checks for header files.

AC_ARG_ENABLE(oss, [  --disable-oss			ignore OSS driver ],
			TRY_OSS=$enableval , TRY_OSS=yes )

if test "x$TRY_OSS" = "xyes"
then
HAVE_OSS=true
BRISTOL_HAS_OSS=1
AC_CHECK_HEADER(sys/soundcard.h, [], [HAVE_OSS=false])

if test $HAVE_OSS = "false"; then
	BRISTOL_HAS_OSS=0
fi
else
HAVE_OSS=false
BRISTOL_HAS_OSS=0
fi
AC_SUBST(BRISTOL_HAS_OSS)

# The brighton library requires 4 X11 header files and cannot proceed without
# them. We should check here for their availability.
CFLAGS="$CFLAGS -I/usr/X11R6/include"
CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
HAVE_XHEADERS=true
AC_CHECK_HEADER(X11/X.h, [], [HAVE_XHEADERS=false])
AC_CHECK_HEADER(X11/Xlib.h, [], [HAVE_XHEADERS=false])
AC_CHECK_HEADER(X11/Xutil.h, [], [HAVE_XHEADERS=false])
AC_CHECK_HEADER(X11/Xos.h, [], [HAVE_XHEADERS=false])
AC_CHECK_HEADER(X11/Xatom.h, [], [HAVE_XHEADERS=false])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for X11 library functions.
BRIGHTON_HAS_X11=
BRIGHTON_X11_DIR=
BRIGHTON_LIBX11=
BRIGHTON_LIBB11=-lC11
BRIGHTON_LIBXLIBS=-L../libbrightonC11
AC_ARG_ENABLE(x11, [  --disable-x11			disable X11 interface ],
	TRY_X11=$enableval , TRY_X11=yes )

if test $HAVE_XHEADERS = "false"
then
	TRY_X11=no
fi

AC_CHECK_HEADERS([X11/extensions/XShm.h], [x11_shm_found="true"], [x11_shm_found="false"], 
[
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
])
# Force this for now - pure build time options, no header dependency

if test "x$TRY_X11" = "xyes"
then
	BRIGHTON_HAS_X11=-DBRIGHTON_HAS_X11
	BRIGHTON_X11_DIR=libbrightonX11
	BRIGHTON_LIBB11=-lB11
	BRIGHTON_LIBX11=-lX11
	BRIGHTON_LIBXEXT=
	BRIGHTON_LIBXLIBS=-L../libbrightonX11

	AC_ARG_ENABLE(ximage, [  --disable-ximage		ignore XImage interface ],
			TRY_XIMAGE=$enableval , TRY_XIMAGE=yes )
	if test "x$TRY_XIMAGE" = "xyes"
	then
		BRIGHTON_HAS_XIMAGE=-DBRIGHTON_XIMAGE
		AC_SUBST(BRIGHTON_HAS_XIMAGE)
		if test "x$x11_shm_found" = "xtrue"
		then
		AC_ARG_ENABLE(shmimage, [  --disable-shmimage		ignore XShmImage interface ],
			TRY_SHMIMAGE=$enableval , TRY_SHMIMAGE=no )
			if test "x$TRY_SHMIMAGE" = "xyes"
			then
				BRIGHTON_HAS_SHMIMAGE=-DBRIGHTON_SHMIMAGE
				BRIGHTON_LIBXEXT=-lXext
				AC_SUBST(BRIGHTON_HAS_SHMIMAGE)
			fi
		fi
	fi
fi
AC_SUBST(BRIGHTON_X11_DIR)
AC_SUBST(BRIGHTON_HAS_X11)
AC_SUBST(BRIGHTON_LIBX11)
AC_SUBST(BRIGHTON_LIBB11)
AC_SUBST(BRIGHTON_LIBXEXT)
AC_SUBST(BRIGHTON_LIBXLIBS)

BRR=${BRISTOL_RAMP_RATE:-10}
AC_SUBST(BRR)

_BRISTOL_VOICES=${BRISTOL_VOICECOUNT:-32}
AC_SUBST(_BRISTOL_VOICES)

BRISTOL_LIN_ATTACK=-DBRISTOL_LIN_ATTACK
AC_ARG_ENABLE(exp-attack, [  --enable-exp-attack		enable exponential attack], USE_EXP_ATTACK=yes , USE_EXP_ATTACK=no )
if test "x$USE_EXP_ATTACK" == "xyes"
then
	BRISTOL_LIN_ATTACK=
fi
AC_SUBST(BRISTOL_LIN_ATTACK)

BRISTOL_SEM_OPEN=
BRISTOL_SEMAPHORE=
AC_ARG_ENABLE(semaphore, [  --enable-semaphore		enable engine semaphores ],
			USE_SEMAPHORE=yes , USE_SEMAPHORE=no )
if test "x$USE_SEMAPHORE" == "xyes"
then
	BRISTOL_SEMAPHORE=-DBRISTOL_SEMAPHORE

	# Checks for sem_open use.
	AC_ARG_ENABLE(sem-open, [  --enable-sem-open		use sem_open interface ],
			   USE_SEM_OPEN=yes , USE_SEM_OPEN=no )
	if test "x$USE_SEM_OPEN" = "xyes"
	then
		BRISTOL_SEM_OPEN=-DBRISTOL_SEM_OPEN
	fi
fi
AC_SUBST(BRISTOL_SEM_OPEN)
AC_SUBST(BRISTOL_SEMAPHORE)

BRISTOL_BARRIER=
AC_ARG_ENABLE(memory-barrier, [  --enable-memory-barrier	enable ringbuffer barrier],
			USE_BARRIER=yes , USE_BARRIER=no )
if test "x$USE_BARRIER" == "xyes"
then
	BRISTOL_BARRIER=-DUSE_MLOCK
fi
AC_SUBST(BRISTOL_BARRIER)

# Checks for autozoom feature requested
BRIGHTON_HAS_AUTOZOOM=-DBRIGHTON_HAS_ZOOM=1
AC_ARG_ENABLE(autozoom, [  --disable-autozoom		disable window autozoom on Enter],
			TRY_AZ=no , TRY_AZ=yes )
if test "x$TRY_AZ" = "xno"
then
	BRIGHTON_HAS_AUTOZOOM=-DBRIGHTON_HAS_ZOOM=0
fi
AC_SUBST(BRIGHTON_HAS_AUTOZOOM)


# Checks for old audio device close/open on error.
AC_ARG_ENABLE(drain, [  --disable-drain		no reopen of audio dev on error ],
			TRY_DRAIN=$enableval , TRY_DRAIN=yes )
if test "x$TRY_DRAIN" = "xyes"
then
	BRISTOL_HAS_DRAIN=-D_BRISTOL_DRAIN
	AC_SUBST(BRISTOL_HAS_DRAIN)
fi

PKG_PROG_PKG_CONFIG

# Checks for library functions.
AC_ARG_ENABLE(alsa, [  --disable-alsa		ignore ALSA driver ],
	TRY_ALSA=$enableval , TRY_ALSA=yes )
AC_CHECK_HEADER(alsa/asoundlib.h, alsa_found="true", alsa_found="false") 

HAVE_ALSA=false
BRISTOL_HAS_ALSA=0
if test "x$TRY_ALSA" = "xyes" -a "x$alsa_found" = "xtrue";
then
	# check for ALSA >= 1.0.0
	PKG_CHECK_MODULES(ALSA, alsa >= 1.0.0,
		 [HAVE_ALSA=true
			  BRISTOL_HAS_ALSA=1
			 ], [true]
	)
else
	ALSA_CFLAGS=
	ALSA_LIBS=
	AC_SUBST(ALSA_CFLAGS)
	AC_SUBST(ALSA_LIBS)
fi
AC_SUBST(BRISTOL_HAS_ALSA)

AC_ARG_ENABLE(liblo, [  --disable-liblo		ignore LIBLO driver ],
			TRY_LIBLO=$enableval , TRY_LIBLO=yes )

# needs liblo and liblo-dev to work - have to see the header files anyway.
HAVE_LIBLO=false
BRISTOL_HAS_LIBLO=0
if test "x$TRY_LIBLO" = "xyes" ;
then
	# check for LIBLO >= 1.0.0
	PKG_CHECK_MODULES(LIBLO, liblo >= 0.22.0,
		 [HAVE_LIBLO=true
			  BRISTOL_HAS_LIBLO=1
			 ], [true]
	)
else
	LIBLO_CFLAGS=
	LIBLO_LIBS=
	AC_SUBST(LIBLO_CFLAGS)
	AC_SUBST(LIBLO_LIBS)
fi
AC_SUBST(BRISTOL_HAS_LIBLO)

jack_session_found="false"
HAVE_JACK_MIDI="false"

AC_ARG_ENABLE(jack, [  --disable-jack		ignore JACK driver ],
	TRY_JACK=$enableval , TRY_JACK=yes )
AC_CHECK_HEADER(jack/jack.h, jack_found="true", jack_found="false") 

DEFAULT_AUDIO_FLAG=-alsa
DEFAULT_JACK=no
DEFAULT_JACK_MIDI=no
BRISTOL_JACK_DEFAULT=
BRISTOL_JACK_DEFAULT_MIDI=
BRISTOL_JACK_MULTI_CLOSE=-DBRISTOL_JACK_MULTI_CLOSE
HAVE_JACK="false"
JACK_SC=yes
if test "x$TRY_JACK" = "xyes" -a "x$jack_found" = "xtrue";
then
	# check for JACK >= 0.99.0
	PKG_CHECK_MODULES(JACK, jack >= 0.99.0,
		 [HAVE_JACK="true"
			  JACK_DEFAULT_DRIVER="jack"
			  BRISTOL_HAS_JACK=-D_BRISTOL_JACK
			 ], [true]
	)
	AC_SUBST(JACK_LIBS)
	AC_SUBST(BRISTOL_HAS_JACK)

	AC_CHECK_HEADER(jack/session.h, jack_session_found="true", jack_session_found="false") 
 	if test "$jack_session_found" = "true"; then 
		AC_DEFINE(JACK_SESSION, 1, [Define to enable jack session support]) 
		BRISTOL_HAS_JACK_SESSION=-D_BRISTOL_JACK_SESSION
		AC_SUBST(BRISTOL_HAS_JACK_SESSION)
	fi 

	AC_ARG_ENABLE(jack-midi, [  --disable-jack-midi		ignore JACK MIDI driver ],
			TRY_JACK_MIDI=$enableval , TRY_JACK_MIDI=yes )
	if test "x$TRY_JACK_MIDI" = "xyes" ;
	then
		# check for JACK >= 0.99.0
		PKG_CHECK_MODULES(JACK, jack >= 0.109.0,
			 [HAVE_JACK_MIDI="true"
				  BRISTOL_HAS_JACK_MIDI=-D_BRISTOL_JACK_MIDI
				 ], [true]
		)
		AC_SUBST(BRISTOL_HAS_JACK_MIDI)
	else
		HAVE_JACK_MIDI="false"
	fi
	AM_CONDITIONAL(HAVE_JACK_MIDI, $HAVE_JACK_MIDI)

	AC_ARG_ENABLE(jack-single-close, [  --disable-jack-single-close	disable Jack selective closedown],
		JACK_SC=$enableval , JACK_SC=yes )
	if test "x$JACK_SC" = "xyes"
	then
		BRISTOL_JACK_MULTI_CLOSE=
	fi

	AC_ARG_ENABLE(jack-default-audio, [  --enable-jack-default-audio	enable Jack as default Audio driver],
		DEFAULT_JACK=$enableval , DEFAULT_JACK=no )
	if test "x$DEFAULT_JACK" = "xyes"
	then
		DEFAULT_AUDIO_FLAG=-jack
		BRISTOL_JACK_DEFAULT=-DBRISTOL_JACK_DEFAULT
		AC_SUBST(BRISTOL_JACK_DEFAULT)
	fi

	AC_ARG_ENABLE(jack-default-midi, [  --enable-jack-default-midi	enable Jack as default Midi driver],
		DEFAULT_JACK_MIDI=$enableval , DEFAULT_JACK_MIDI=no )
	if test "x$DEFAULT_JACK_MIDI" = "xyes"
	then
		BRISTOL_JACK_DEFAULT_MIDI=-DBRISTOL_JACK_DEFAULT_MIDI
		AC_SUBST(BRISTOL_JACK_DEFAULT_MIDI)
	fi

else
	JACK_CFLAGS=
	JACK_LIBS=
	AC_SUBST(JACK_CFLAGS)
	AC_SUBST(JACK_LIBS)
fi
AM_CONDITIONAL(HAVE_JACK, $HAVE_JACK)
AM_CONDITIONAL(HAVE_JACK_MIDI, $HAVE_JACK_MIDI)
AC_SUBST(DEFAULT_AUDIO_FLAG)
AC_SUBST(BRISTOL_JACK_MULTI_CLOSE)

## See about pulse audio
TRY_PA=no
BRISTOL_HAS_PA=
BRISTOL_PA_DIR=
BRISTOL_LIB_PA=
BRISTOL_LIBPALIBS=
AC_ARG_ENABLE(pulseaudio, [  --enable-pulseaudio		enable pulse audio drivers], TRY_PA=yes , TRY_PA=no )
AC_CHECK_HEADER(pulse/pulseaudio.h, [HAVE_PA_HEADERS=true], [HAVE_PA_HEADERS=false])
if test x$TRY_PA = 'xyes'
then
	if test $HAVE_PA_HEADERS = 'true'
	then
		BRISTOL_HAS_PA=-DBRISTOL_PA
		BRISTOL_PA_DIR=libbristolpulse
		BRISTOL_LIB_PA='-lbristolpulse -lpulse -lpulse-simple'
		BRISTOL_LIBPALIBS=-L$BPALIBS
	else
		$TRY_PA=no
	fi
fi
AC_SUBST(BRISTOL_HAS_PA)
AC_SUBST(BRISTOL_PA_DIR)
AC_SUBST(BRISTOL_LIB_PA)
AC_SUBST(BRISTOL_LIBPALIBS)

if test $prefix = "NONE"; then
	prefix=$ac_default_prefix
fi

BRISTOL_DIR=$prefix/share/bristol
AC_SUBST(BRISTOL_DIR)

AC_CONFIG_FILES([Makefile libbrightonC11/Makefile libbrightonX11/Makefile libbrighton/Makefile libbvg/Makefile libbristolaudio/Makefile libbristolmidi/Makefile libbristolic/Makefile libbristol/Makefile brighton/Makefile bristol/Makefile bin/startBristol bin/Makefile])
AC_OUTPUT

echo
echo $PACKAGE $VERSION :
echo
echo \| Build with OSS support ......................... : $HAVE_OSS
echo \| Build with ALSA support ........................ : $HAVE_ALSA
if test $HAVE_ALSA = "false" && test $TRY_ALSA = "yes" ; then
	echo \| Bristol needs libasound2-dev to be installed for ALSA support.
fi
echo \| Build with JACK support ........................ : $HAVE_JACK
if test $HAVE_JACK = "false" && test $TRY_JACK = "yes" ; then
	echo \| Bristol needs jackd and libjack-dev to be installed for JACK support.
fi
echo \| Build with JACK MIDI support ................... : $HAVE_JACK_MIDI
#
# This will be 0.50.5
echo \| Build with JACK Session support ................ : $jack_session_found

if test x$TRY_PA = 'xyes' ; then
echo \| Build with PA support .......................... : true
fi

if test $DEFAULT_JACK = "yes"; then
echo \| Default audio drivers .......................... : jack
else
echo \| Default audio drivers .......................... : alsa
fi

if test $DEFAULT_JACK_MIDI = "yes"; then
echo \| Default MIDI drivers ........................... : jack
else
echo \| Default MIDI drivers ........................... : alsa
fi

if test $USE_SEMAPHORE = "yes"; then
echo \| Build with semaphore support.................... : true
if test $USE_SEM_OPEN = "yes"; then
echo \| Build with sem_open ............................ : true
fi
fi

if test $USE_BARRIER == "yes"; then
echo \| Build with jrb memory barrier .................. : true
fi

echo \| Build with Graphical Interface ................. : $HAVE_XHEADERS
if test "x$TRY_X11" = "xyes"; then
echo \| Compile with GUI support ....................... : true
if test "x$TRY_SHMIMAGE" = "xyes"; then
echo \| Build with XShmImage support ................... : true
fi
if test $TRY_XIMAGE != "yes"; then
echo \| Build with XImage support ...................... : false
else
if test $HAVE_XHEADERS = "false"; then
	echo \| Bristol needs libX11-dev installed to support a grpahical interface
fi
fi
else
echo \| Compile with X11 support ....................... : false
fi
echo \| Bin directory .................................. : ${prefix}/bin
echo \| Lib directory .................................. : ${prefix}/lib
echo \| Data directory ................................. : $BRISTOL_DIR
echo \| Default voicecount ............................. : BRISTOL_VOICECOUNT=$_BRISTOL_VOICES
if test $BRR != 10; then
echo \| Envelope max ramp time ......................... : $BRR seconds
fi
if test x$USE_EXP_ATTACK == "xyes"; then
echo \| Envelope attack type ........................... : exponential
fi
echo \| author ......................................... : Nick Copeland
echo \| email .......................................... : nickycopeland@hotmail.com
echo \| web ............................................ : http://bristol.sf.net
echo
echo execute \'make install\' then \'startBristol\'
echo
if test -f ${BRISTOL_DIR}/bitmaps/blueprints/mini.xpm ; then
echo To get the benefits of the compressed bitmaps you are advised to run a
echo \'make uninstall\' before building this release.
echo
fi
