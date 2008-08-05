#$$LIC$$
#
# $Id: Makefile.in,v 1.8 2007/01/12 11:46:15 luz Exp $
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
top_builddir = .

INSTALL = /usr/bin/install -c -p
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
OPENSSL = /usr/bin/openssl

DEFS = -DHAVE_CONFIG_H 
CPPFLAGS = -DDBSUPPORT 
LDFLAGS = 
LIBS =  -lm
CCOPT = -Wall -g
INCLS = -I. -I..
CFLAGS = $(CCOPT) $(INCLS) $(DEFS)

TARGETS = libmisc.a libipfix.a
OBJS = ipfix.o ipfix_col.o ipfix_print.o \
       ipfix_col_files.o ipfix_col_db.o ipfix_db.o 
DEPHDR = ipfix.h ipfix_def.h ipfix_fields.h ipfix_def_fokus.h ipfix_fields_fokus.h
MISCOBJS = mpoll.o mlog.o
DHPARAMS = dh512.pem dh1024.pem
CLEANFILES = $(OBJS) $(MISCOBJS) $(TARGETS) ipfix_def_fokus.h ipfix_fields_fokus.h

all: $(TARGETS)

clean:
	rm -f $(CLEANFILES)

distclean:
	rm -f $(CLEANFILES) $(DHPARAMS) Makefile

install:
	@[ -d ${libdir} ] || (mkdir -p ${libdir}; chmod 755 ${libdir})
	$(INSTALL_DATA) libipfix.a ${libdir}/
	$(INSTALL_DATA) libmisc.a ${libdir}/
	@[ -d ${includedir} ] || (mkdir -p ${includedir}; chmod 755 ${includedir})
	$(INSTALL_HEADER) ipfix*.h mlog.h mpoll.h ${includedir}/

libmisc.a: $(MISCOBJS) Makefile
	@rm -f $@
	$(AR) rc $@ $(MISCOBJS) 

mlog.o: mlog.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c mlog.c

mpoll.o: mpoll.c Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c mpoll.c

libipfix.a: $(OBJS) Makefile
	@rm -f $@
	$(AR) rc $@ $(OBJS) 

ipfix.o: ipfix.c $(DEPHDR) ipfix_ssl.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix.c

ipfix_col.o: ipfix_col.c $(DEPHDR) ipfix_col.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix_col.c

ipfix_ssl.o: ipfix_ssl.c ipfix_ssl.h $(DEPHDR) ipfix_ssl.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix_ssl.c

ipfix_print.o: ipfix_print.c $(DEPHDR) Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix_print.c

ipfix_col_files.o: ipfix_col_files.c $(DEPHDR) ipfix_col.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix_col_files.c

ipfix_col_db.o: ipfix_col_db.c $(DEPHDR) ipfix_db.h ipfix_col.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix_col_db.c

ipfix_db.o: ipfix_db.c $(DEPHDR) ipfix_db.h Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ipfix_db.c

dh512.pem:
	$(OPENSSL) dhparam -check -text -5 512 -out dh512.pem

dh1024.pem:
	$(OPENSSL) dhparam -check -text -5 1024 -out dh1024.pem

dhparams.c: $(DHPARAMS) Makefile
	echo "#include <openssl/ssl.h>" > dhparams.c
	echo "#include <openssl/x509v3.h>" >> dhparams.c
	$(OPENSSL) dh -noout -C < dh512.pem >> dhparams.c
	$(OPENSSL) dh -noout -C < dh1024.pem >> dhparams.c

dhparam.o: dhparams.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c dhparams.c

ipfix_def_fokus.h: ipfix_FOKUS_IEs.txt make-IPFIX_DEF_FOKUS_H.awk
	awk -f make-IPFIX_DEF_FOKUS_H.awk ipfix_FOKUS_IEs.txt > ipfix_def_fokus.h

ipfix_fields_fokus.h: ipfix_FOKUS_IEs.txt make-IPFIX_FIELDS_FOKUS_H.awk
	awk -f make-IPFIX_FIELDS_FOKUS_H.awk ipfix_FOKUS_IEs.txt > ipfix_fields_fokus.h
