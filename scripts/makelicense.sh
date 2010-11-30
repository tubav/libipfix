#!/bin/sh

#
#  DON'T CALL THIS FILE DIRECTLY
#
#  Uses perl
#
#  Inserts a header (e.g. license) in place of $$LIC$$ placeholder for
#  a file specified on the command line

if [ $# != 2 ]; then
 echo "Usage: "
 echo "  makelicense file header"
 exit 1
fi

license=`cat $2`

perl -e "s|\\\$\\\$LIC\\\$\\\$|$license|m; print;" -n $1 > temp

# Note: Inefficient: file is copied even if it wasn't changed !

mv temp $1

#echo "Adding header to $1"
