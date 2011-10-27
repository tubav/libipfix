#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([libipfix],[0.8.1])

AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([lib/ipfix.h])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_TARGET

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PATH_PROG(OPENSSL,openssl,openssl)

#
# os check
#################################################
case "$target_os" in
freebsd*)
	LDFLAGS="$LDFLAGS -L/usr/local/lib "
	CPPFLAGS="$CPPFLAGS -I/usr/local/include "
	;;
esac


#################################################
#################################################

#
# IPv6 support
#################################################
AC_ARG_ENABLE(ipv6,
[
AS_HELP_STRING([--enable-ipv6],[enable ipv6 (with ipv4) support])
AS_HELP_STRING([--disable-ipv6],[disable ipv6 support])],
[ 
   if test $enableval != "no" ; then
      CPPFLAGS="-DINET6 $CPPFLAGS"
   fi
],
[])


#
# SCTP support
#################################################
AC_ARG_ENABLE(sctp,
[
AS_HELP_STRING([--enable-sctp],[enable sctp support])
AS_HELP_STRING([--disable-sctp],[disable sctp support])],
[ 
   if test $enableval != "no" ; then
      CPPFLAGS="-DSCTPSUPPORT $CPPFLAGS"
      SCTPLIBS="-lsctp"
   fi
], 
[])
AC_SUBST(SCTPLIBS)

#
# enable ssl support
#################################################
AC_MSG_CHECKING(enable tls/dtls support)
AC_ARG_WITH(ssl,
[
AS_HELP_STRING([--with-ssl[[=DIR]]],[enable tls/dtls support (from DIR, if supplied)])],
[ 
   if test "$withval" != "no"; then
      AC_MSG_RESULT(yes)
      if test "$withval" != "yes"; then
         AC_MSG_CHECKING(for libssl in $withval/lib)
         if test -f $withval/lib/libssl.a -o -f $withval/lib/libssl.so ; then
            AC_MSG_RESULT(yes)
            LDFLAGS="$LDFLAGS -L$withval/lib"
            CPPFLAGS="$CPPFLAGS -I$withval/include"
         else
            AC_MSG_RESULT(no)
            AC_MSG_CHECKING(for libssl in $withval)
            if test -f $withval/libssl.a -o -f $withval/libssl.so ; then
               AC_MSG_RESULT(yes)
               LDFLAGS="$LDFLAGS -L$withval"
               CPPFLAGS="$CPPFLAGS -I$withval"
            else
               AC_MSG_RESULT(no)
            fi
         fi
      fi

      SAVELIBS=$LIBS
      LIBS=
      AC_CHECK_LIB(ssl, SSL_new,
      [
         SSLLIBS="-lssl -lcrypto"
         CPPFLAGS="$CPPFLAGS -DSSLSUPPORT"
         IPFIX_SSL_OBJ="ipfix_ssl.o dhparams.o"
      ],
      [
         AC_MSG_ERROR([SSL support was requested, but is not available])
      ])
      SSLLIBS="$LIBS $SSLLIBS"
      LIBS=$SAVELIBS
   else
      AC_MSG_RESULT(no)
   fi
], 
[
   AC_MSG_RESULT(no)
]) # no sll
AC_SUBST(SSLLIBS)
AC_SUBST(IPFIX_SSL_OBJ)

#
# specify mysqldir
#################################################
AC_ARG_WITH(mysql,
[AS_HELP_STRING([--with-mysql=DIR],[location of mysql installation}])],
[
   if test "$withval" != "no"; then
      if test "$withval" != "yes"; then
         AC_MSG_CHECKING(for libmysqlclient in $withval/lib)
         if test -f $withval/libmysqlclient.a -o -f $withval/libmysqlclient.so ; then
             LDFLAGS="$LDFLAGS -L$withval "
             AC_MSG_RESULT(yes)
         else 
         if test -f $withval/lib/libmysqlclient.a -o -f $withval/lib/libmysqlclient.so ; then
             LDFLAGS="$LDFLAGS -L$withval/lib "
             CPPFLAGS="$CPPFLAGS -I$withval/include"
             AC_MSG_RESULT(yes)
         else
         if test -f $withval/lib/mysql/libmysqlclient.a -o -f $withval/lib/mysql/libmysqlclient.so ; then
             LDFLAGS="$LDFLAGS -L$withval/lib/mysql "
             CPPFLAGS="$CPPFLAGS -I$withval/include"
             AC_MSG_RESULT(yes)
         else
             AC_MSG_RESULT(no)
         fi
         fi
         fi
      fi

      AC_CHECK_LIB(mysqlclient, mysql_init, 
      [
         MYSQLLIBS="-lmysqlclient"
         IPFIX_DB_OBJ="ipfix_db.o"
         IPFIX_DB_EXAMPLES="example_collector_db"
         CPPFLAGS="$CPPFLAGS -DDBSUPPORT"
      ],
      [
         AC_MSG_WARN([cannot find mysql library])
      ])
   fi
] )
AC_SUBST(MYSQLLIBS)
AC_SUBST(IPFIX_DB_OBJ)
AC_SUBST(IPFIX_DB_EXAMPLES)

#
# libpcap support
#################################################
case "$target_os" in
cygwin)
	CPPFLAGS="-DWINNT $CPPFLAGS"
   PCAP="wpcap"
   ;;
*)
   PCAP="pcap"
   ;;
esac

AC_ARG_WITH(pcap,
[AS_HELP_STRING([--with-pcap=DIR],[location of libpcap installation])],
[ 
   AC_MSG_WARN([pcap path given])
   #AC_MSG_CHECKING(for lib$PCAP in $withval/lib)
   #if test -f $withval/lib/lib$PCAP.a -o -f $withval/lib/lib$PCAP.so ; then
   #   LDFLAGS="$LDFLAGS -L$withval/lib "
   #   PCAPINCLS="-I$withval/include"
   #   AC_MSG_RESULT(yes)
   #else
   #   AC_MSG_RESULT(no)
   #fi
   LDFLAGS="$LDFLAGS -L$withval/lib"
   CPPFLAGS="$CPPFLAGS -I$withval/include"
],
[] )
SAVELIBS=$LIBS
LIBS=
AC_CHECK_LIB([$PCAP], [pcap_open_live],,
             [AC_MSG_WARN([cannot find libpcap])])
AC_CHECK_FUNCS(pcap_breakloop pcap_freecode) 
AC_CHECK_HEADERS([pcap.h])
PCAPLIBS=$LIBS
LIBS=$SAVELIBS
AC_SUBST(PCAPLIBS)


#################################################
#################################################

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h getopt.h libgen.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([gettimeofday basename inet_ntoa memmove memset mkdir select strdup strerror])
AC_CHECK_FUNCS(gethostbyname,,[AC_CHECK_LIB(nsl,gethostbyname,,[AC_CHECK_LIB(socket,gethostbyname)])])
AC_CHECK_FUNCS(setsockopt,,[AC_CHECK_LIB(socket,setsockopt)])
AC_CHECK_FUNCS(hstrerror,,[AC_CHECK_LIB(resolv,hstrerror,,[AC_CHECK_LIB(socket,gethostbyname)])])
AC_CHECK_FUNCS([getopt_long],,
[#FreeBSD has a gnugetopt library for this
  AC_CHECK_LIB([gnugetopt],[getopt_long],
  [LIBS="-lgnugetopt $LIBS";AC_DEFINE([HAVE_GETOPT_LONG])])])

AC_CONFIG_FILES([Makefile \
           lib/Makefile \
           libmisc/Makefile \
           probe/Makefile \
           examples/Makefile \
           collector/Makefile ])
AC_OUTPUT
