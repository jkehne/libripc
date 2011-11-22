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

AC_HEADER_STDC
AC_HEADER_ASSERT

AC_ARG_ENABLE([debug],AS_HELP_STRING([--enable-debug], [enable debug output (overwrites CFLAGS!)]))
AS_IF([test "x$enable_debug" = "xyes"], [AC_DEFINE([DEBUG], [1], [Define to 1 if debug output should be enabled.])])
AS_IF([test "x$enable_debug" = "xyes"], [CFLAGS="-O0 -g"])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

