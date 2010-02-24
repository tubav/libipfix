#!/bin/sh
#
# make new libipfix src distribution
#
# Copyright Fraunhofer FOKUS
#
# 24.02.2010  csc  adaptation to libipfix
# 26-04-2007  luz  subversion adaption
# 02-07-2004  gup  $$LIC$$ support
# 10-07-2002  luz  initial
#
SVNREPOS="https://svnsrv.fokus.fraunhofer.de/svn/cc/ccnet/libipfix/"
SVNTAG="arrabiata-release"
DOTAG=""

# about
#
about () {
    printf "\n libipfix - make source tarball\n\n"
}

# usage
#
usage () {

about

cat <<EOF
  $tskname  [-hT] [-d repository] [-b branch] [-t tag]

  options:
    h :         display this help text
    d url :     set repository
    b dir :     use this branch
    r dir :     use this release
    T :         tag this release
    t tag :     tag this release with 'tag'

EOF
}

tskname=`basename $0`

# parse command line options
#
while getopts hd:b:r:Tt: c ; do
    case $c in
	h) usage ; exit 0 ;;
	b) SVNBRANCH=branches/$OPTARG ;;
	r) SVNBRANCH=releases/$OPTARG ;;
        d) SVNROOT=$OPTARG ;;
        t) DOTAG=1 ; SVNTAG=$OPTARG ;;
        T) DOTAG=1 ;;
	\?) usage ; exit 0 ;;
    esac
done
shift `expr $OPTIND - 1`


# Look for BASEDIR :
# 
if [  -z "$SVNROOT" ] ; then
  SVNROOT=$SVNREPOS
fi

printf "\nusing repository: $SVNROOT/$SVNBRANCH\n"
TMPDIR=/tmp
ROOTDIR=/tmp/_libipfix
OLDDIR=`pwd`
if [  -z "$SVNBRANCH" ] ; then
    SVNURL=$SVNROOT/trunk
    archive_prefix=libipfix
else
    SVNURL=$SVNROOT/$SVNBRANCH
    archive_prefix=libipfix_`echo $SVNBRANCH | sed "s/\//-/g"`
fi
printf "using url: $SVNURL\n"

# check out sources:
#
rm -rf $ROOTDIR
mkdir $ROOTDIR
cd $ROOTDIR
printf "\nchecking out source tree ..."

svn export -q $SVNURL libipfix
if test -f "libipfix/INSTALL" ; then
    printf " done."
else
    printf " failed.\n"
    exit 1
fi

# insert header text
#
LICSCRIPT="./libipfix/scripts/makelicense.sh"
if test -f $LICSCRIPT ; then
    printf "\n insert header/license ...\n"
    chmod u+x $LICSCRIPT # if something ever happens to the permissions of this script
    find . -path "*.h"   -exec $LICSCRIPT {} ./libipfix/scripts/header.txt  \; 2> /dev/null
    find . -path "*.c"   -exec $LICSCRIPT {} ./libipfix/scripts/header.txt  \; 2> /dev/null 
    find . -path "*.cc"  -exec $LICSCRIPT {} ./libipfix/scripts/header.txt \; 2> /dev/null
    find . -path "*.php" -exec $LICSCRIPT {} ./libipfix/scripts/header-php.txt \; 2> /dev/null
    find . -path "*.lua" -exec $LICSCRIPT {} ./libipfix/scripts/header.txt \; 2> /dev/null
    find . -name "Makefile.in" -exec $LICSCRIPT {} ./libipfix/scripts/header-makefile-in.txt \; 2> /dev/null
    sleep 3 ;			# for slow NFS
    printf " done."
else
    printf "\n skipping insertion of header/license (script '"$LICSCRIPT"' not found)";
fi;

# delete some files
#
printf "\ndelete unwanted files ..."
cd libipfix
rm -rf COPYING bootstrap
mv ./scripts ./x
mkdir -m 755 ./scripts
#mv ./x/ntpoffset ./scripts
#mv ./x/mkdb.mysql ./scripts
rm -rf ./x
cd ..
printf " done."

# touch files
#
printf "\nset file time ..."
find . -type f -exec touch -r . {} \; 2> /dev/null
printf " done."

# set TAG
#
TAG=$SVNTAG-`date '+%Y%m%d'`
if [ "x$DOTAG" = "x1" ] ; then
   printf "\nrelase tag: $TAG \n"
   svn copy $SVNURL $SVNROOT/tags/$TAG -m"$TAG"
else
   printf "\nrelase tag: $TAG (not set)"
fi

# change dir name, make tar file, clean up 
#
printf "\ncreate archive ..."
RELNAME=${archive_prefix}_`date '+%y%m%d'`
mv libipfix $RELNAME
tar -cf $RELNAME.tar $RELNAME
gzip -9 $RELNAME.tar
mv $RELNAME.tar.gz $OLDDIR/$RELNAME.tgz
cd $OLDDIR
rm -rf $ROOTDIR

printf "\npkg $RELNAME.tgz created.\n\n"

