#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([libipfix],[1.0])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_SRCDIR([lib/ipfix.h])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_TARGET

VERSION=0.8.1
PACKAGE=libipfix

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PATH_PROG(OPENSSL,openssl,openssl)

#
# IPv6 support
#
AC_ARG_ENABLE(ipv6,
[  --enable-ipv6           enable ipv6 (with ipv4) support
  --disable-ipv6          disable ipv6 support],
[ case "$enableval" in
  yes) ipv6=yes;;
  *)   ipv6=no ;;
  esac ], [ipv6=no])

if test $ipv6 = yes; then
	CPPFLAGS="-DINET6 $CPPFLAGS"
fi

#
# SCTP support
#
AC_ARG_ENABLE(sctp,
[  --enable-sctp           enable sctp support
  --disable-sctp          disable sctp support],
[ case "$enableval" in
  yes) sctp=yes;;
  *)   sctp=no ;;
  esac ], [sctp=no])

if test $sctp = yes; then
	CPPFLAGS="-DSCTPSUPPORT $CPPFLAGS"
        SCTPLIBS="-lsctp"
fi

#
# specify mysqldir
#
withval=""
AC_ARG_WITH(mysql,[  --with-mysql=DIR        location of mysql installation],
[
AC_MSG_CHECKING(for libmysqlclient in $withval/lib)
if test -f $withval/libmysqlclient.a -o -f $withval/libmysqlclient.so ; then
    LDFLAGS="$LDFLAGS -L$withval "
    AC_MSG_RESULT(yes)
else 
if test -f $withval/lib/libmysqlclient.a -o -f $withval/lib/libmysqlclient.so ; then
    LDFLAGS="$LDFLAGS -L$withval/lib "
    CPPFLAGS="-I$withval/include $CPPFLAGS"
    AC_MSG_RESULT(yes)
else
if test -f $withval/lib/mysql/libmysqlclient.a -o -f $withval/lib/mysql/libmysqlclient.so ; then
    LDFLAGS="$LDFLAGS -L$withval/lib/mysql "
    CPPFLAGS="-I$withval/include $CPPFLAGS"
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi
fi
fi
] )

# specify libpcap
#
case "$target_os" in
cygwin)
	CPPFLAGS="-DWINNT $CPPFLAGS"
        PCAP="wpcap"
        PCAPLIBS="-lwpcap"
        ;;
*)
        PCAP="pcap"
        PCAPLIBS="-lpcap"
        ;;
esac

withval=""
AC_ARG_WITH(pcap,
[  --with-pcap=DIR         location of libpcap installation],
[ 
   if test $withval != no
   then
      AC_MSG_CHECKING(for lib$PCAP in $withval/lib)
      if test -f $withval/lib/lib$PCAP.a -o -f $withval/lib/lib$PCAP.so ; then
         LDFLAGS="$LDFLAGS -L$withval/lib "
         PCAPINCLS="-I$withval/include"
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
      fi
   fi
] )

# enable ssl support
#
SSLLIBS=''
AC_MSG_CHECKING(enable tls/dtls support)
AC_ARG_WITH(ssl,
[  --with-ssl[[=DIR]]        enable tls/dtls support (from DIR, if supplied)],
[ case "x$withval" in
  xyes) ssl=yes;ssldir="";;
  xno)  ssl=yes;;
  *)    ssl=yes;ssldir=$withval ;;
  esac ], [ssl=no])
if test "$ssl" = "yes";  then
   AC_MSG_RESULT(yes)
   if test "x$ssldir" != "x"; then
      AC_MSG_CHECKING(for libssl in $ssldir/lib)
      if test -f $ssldir/lib/libssl.a -o -f $ssldir/lib/libssl.so ; then
         SSLLIBS="-L$ssldir/lib "
         CPPFLAGS="-I$ssldir/include $CPPFLAGS"
         AC_MSG_RESULT(yes)
      elif test -f $ssldir/libssl.a -o -f $ssldir/libssl.so ; then
         SSLLIBS="-L$ssldir "
         CPPFLAGS="-I$ssldir/include $CPPFLAGS"
         AC_MSG_RESULT(yes)
      else
         AC_MSG_RESULT(no)
      fi
   fi

   AC_CHECK_LIB(ssl, SSL_new,
   [
      SSLLIBS="$SSLLIBS -lssl -lcrypto"
      CPPFLAGS="-DSSLSUPPORT $CPPFLAGS"
      IPFIX_SSL_OBJ="ipfix_ssl.o dhparams.o"
   ],[
      AC_MSG_ERROR([SSL support was requested, but is not available])
   ])
else
   AC_MSG_RESULT(no)
fi
AC_SUBST(SSLLIBS)
AC_SUBST(IPFIX_SSL_OBJ)

# os check
#
case "$target_os" in
freebsd*)
	LDFLAGS="$LDFLAGS -L/usr/local/lib "
	CPPFLAGS="$CPPFLAGS -I/usr/local/include "
	;;
esac

# Checks for libraries.
#
AC_CHECK_LIB(mysqlclient, mysql_init, 
             [dbsupport="yes"],
             [dbsupport="no"; AC_MSG_WARN([cannot find mysql library])])
if test $dbsupport = yes; then
        MYSQLLIBS="-lmysqlclient"
        IPFIX_DB_OBJ="ipfix_db.o"
        IPFIX_DB_EXAMPLES="example_collector_db"
        CPPFLAGS="-DDBSUPPORT $CPPFLAGS"
fi
AC_SUBST(MYSQLLIBS)
AC_SUBST(SCTPLIBS)
AC_SUBST(IPFIX_DB_OBJ)
AC_SUBST(IPFIX_DB_EXAMPLES)

# libpcap support
#
AC_CHECK_LIB([$PCAP], pcap_open_live,[pcapsupport="yes"], 
             [pcapsupport="no"; AC_MSG_WARN([cannot find libpcap])])
if test $pcapsupport = yes; then
    SAVELIBS=$LIBS
    LIBS="$LIBS $PCAPLIBS"
    AC_CHECK_FUNCS(pcap_breakloop pcap_freecode)
    LIBS=$SAVELIBS
else
    PCAPLIBS=""
fi
AC_SUBST(PCAPLIBS)
AC_SUBST(PCAPINCLS)

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
