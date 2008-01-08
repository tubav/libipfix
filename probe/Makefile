#$$LIC$$
#
# $Id: Makefile.in,v 1.2 2006/12/01 18:25:56 luz Exp $
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
OBJEXT = o
PACKAGE = libipfix
VERSION = 1.0
install_sh = @install_sh@

DEFS = -DHAVE_CONFIG_H 
CPPFLAGS = -DDBSUPPORT 
LDFLAGS = 
LIBS =  -lpcap   -L../lib -lipfix -lmisc 
CCOPT = -Wall -g
INCLS = -I. -I.. -I../lib 
CFLAGS = $(CCOPT) $(INCLS) $(DEFS)

TARGETS = ipfix_probe
DEPS = Makefile 
OBJS = probe.o ipflow.o
CLEANFILES = $(OBJS) $(TARGETS)

all: $(TARGETS)

clean:
	rm -f $(CLEANFILES)

distclean:
	rm -f $(CLEANFILES)

install: $(TARGETS)
	[ -d $(bindir) ] || \
	    (mkdir -p $(bindir); chmod 755 $(bindir))
	$(INSTALL_PROGRAM) $(TARGETS) $(bindir)

ipfix_probe: $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

probe.o: probe.c $(DEPS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c probe.c

ipflow.o: ipflow.c $(DEPS) ipflow.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipflow.c

