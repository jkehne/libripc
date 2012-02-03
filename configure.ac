dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libripc, 0.1)

AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE(libripc,0.1)
AM_PROG_LIBTOOL

AC_PROG_INSTALL

AC_LANG_C
AC_PROG_CC


if test "x$host_alias" != x; then
  # We do not necessarily need crt0 and friends when cross-compiling.
  LDFLAGS="-nostdlib $LDFLAGS"
  HOST=$host_alias
else
  HOST=`(uname -m) 2>/dev/null || echo unknown`
fi

case $HOST in
  ia32|x86*|i386*|i486*|i586*|i686*|amd64*)
        AC_CHECK_LIB([ibverbs],[ibv_get_device_list],[],[
                echo "Error: Libibverbs not found!"
                exit -1
        ])

        AC_CHECK_LIB([ibumad],[umad_init],[],[
                echo "Error: Libibumad not found!"
                exit -1
        ])
	;;
  powerpc*)
	;;
  *)
	AC_MSG_ERROR([Unknown hardware architecture: $HOST.
	Please specify hardware architecture using --host=ARCH.])
	;;
esac



AC_CHECK_LIB([pthread],[pthread_create],[],[
	echo "Error: Libpthread not found!"
	exit -1
])

AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_ASSERT

AC_CHECK_HEADER([pthread.h],[],[
	echo "Error: Libpthread development headers not found!"
	exit -1
])

AC_FUNC_MALLOC

AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug], [enable debug output (overwrites CFLAGS!)]))
AS_IF([test "x$enable_debug" = "xyes"], [AC_DEFINE([HAVE_DEBUG], [1], [Define to 1 if debug output should be enabled.])])
AS_IF([test "x$enable_debug" = "xyes"], [CFLAGS="-O0 -ggdb3"])

AC_CONFIG_HEADERS([src/config.h clients/config.h])
AC_CONFIG_FILES(Makefile src/Makefile clients/Makefile)
AC_OUTPUT

