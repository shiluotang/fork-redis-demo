#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([embedded-redis], [1.0], [shengquangang2015@163.com])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([tests])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Checks for programs.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [])
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR], [])
LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
PKG_CHECK_MODULES([HIREDIS], [hiredis])

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
