ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -g -Wall
AM_CFLAGS 	=
AM_CXXFLAGS =
AM_LDFLAGS 	=

lib_LTLIBRARIES = libprocs.la
libprocs_la_SOURCES = src/processbuilder.c

check_PROGRAMS =

check_PROGRAMS += test1
test1_SOURCES = tests/test1.c
test1_CPPFLAGS = $(AM_CPPFLAGS) $(HIREDIS_CFLAGS)
test1_LDFLAGS = $(AM_LDFLAGS) $(HIREDIS_LIBS)
test1_LDADD = libprocs.la

TESTS = $(check_PROGRAMS)
