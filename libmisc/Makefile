#$$LIC$$
#
# $Id: Makefile.in 1026 2009-03-30 15:15:30Z csc $
# 
# Makefile
SHELL = /bin/bash

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${datarootdir}
datarootdir = ${prefix}/share
libdir = ${exec_prefix}/lib
mandir = ${datarootdir}/man
includedir = ${prefix}/include
oldincludedir = /usr/include
top_builddir = .

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL}
INSTALL_HEADER = $(INSTALL_DATA)
CC = gcc
EXEEXT = 
INSTALL_STRIP_PROGRAM = @INSTALL_STRIP_PROGRAM@
OBJEXT = o
PACKAGE = @PACKAGE@
VERSION = @VERSION@
install_sh = @install_sh@

DEFS = -DHAVE_CONFIG_H 
CPPFLAGS = 
LDFLAGS = 
LIBS =  -lm -lpcap
CCOPT = -Wall -g
INCLS = -I. -I.. -I../.. 
CFLAGS = $(CCOPT) $(INCLS) $(DEFS)

TARGETS = libmisc.a
OBJS = misc.o mpoll.o mlog.o mlist.o mhash.o hashtable.o getopt.o getopt1.o
CLEANFILES = $(OBJS) $(TARGETS) testmhash

all: $(TARGETS)

clean:
	rm -f $(CLEANFILES)

distclean:
	rm -f $(CLEANFILES) Makefile

install:
	$(INSTALL_HEADER) mlog.h mpoll.h ${includedir}/

libmisc.a: $(OBJS) Makefile
	@rm -f $@
	$(AR) rc $@ $(OBJS) 

misc.o: misc.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c misc.c

mpoll.o: mpoll.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c mpoll.c

mhash.o: mhash.c hashtable.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c mhash.c

hashtable.o: hashtable.c hashtable.h hashtable_private.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c hashtable.c

testmhash: testmhash.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) testmhash.c libmisc.a hashtable.c -lm -o testmhash

mlog.o: mlog.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c mlog.c

mlist.o: mlist.c misc.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c mlist.c

getopt.o: getopt.c getopt.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c getopt.c

getopt1.o: getopt1.c getopt.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c getopt1.c

