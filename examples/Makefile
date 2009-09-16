#$$LIC$$
#
# $Id: Makefile.in,v 1.3 2006/11/14 15:50:37 luz Exp $
# 
# Makefile
SHELL = /bin/bash

srcdir = .
top_srcdir = ..
prefix = /tmp
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

INSTALL = /usr/bin/install -c -p
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
CPPFLAGS = -DDBSUPPORT 
LDFLAGS = 
LIBS =  -lmysqlclient  -L../lib -L../libmisc -lipfix -lmisc 
CCOPT = -Wall -g
INCLS = -I. -I.. -I../lib -I../libmisc
CFLAGS = $(CCOPT) $(INCLS) $(DEFS)

TARGETS = example_exporter example_collector example_collector_db
OBJS = example_exporter.o example_collector.o example_collector_db.o
CLEANFILES = $(OBJS) $(TARGETS)

all: $(TARGETS)

clean:
	rm -f $(CLEANFILES)

distclean:
	rm -f $(CLEANFILES) Makefile

install:

example_exporter: example_exporter.o Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ example_exporter.o $(LDFLAGS) $(LIBS)

example_collector: example_collector.o Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ example_collector.o $(LDFLAGS) $(LIBS)

example_collector_db: example_collector_db.o Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ example_collector_db.o $(LDFLAGS) $(LIBS)

example_collector.o: example_collector.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c example_collector.c

example_collector_db.o: example_collector_db.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c example_collector_db.c

example_exporter.o: example_exporter.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c example_exporter.c

