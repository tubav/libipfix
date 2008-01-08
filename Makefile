#$$LIC$$
#
# $Id: Makefile.in,v 1.2 2005/01/04 09:25:12 luz Exp $
# 
# Makefile
SHELL = /bin/bash
top_srcdir = .
prefix = /usr/local

SUBDIRS       = lib # collector examples

all: compile

rmtarget clean distclean:
	@ for DIR in $(SUBDIRS) ; \
        do \
                ( \
                cd ./$$DIR; $(MAKE) $@; \
                ); \
        done

compile:
	@ for DIR in $(SUBDIRS) ; \
        do \
                ( \
                cd ./$$DIR; $(MAKE); \
                ); \
        done

install: 
	@ for DIR in $(SUBDIRS) ; \
        do \
                ( \
                cd ./$$DIR; $(MAKE) $@; \
                ); \
        done

