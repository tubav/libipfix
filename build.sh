#! /bin/bash

USAGE="Usage: `basename $0` [-tsb] [-d <dist>]\n\t -t tag git tree\n\t -s source package"


BUILD_CMD="git-buildpackage --git-ignore-new"
BUILDER="--git-builder=debuild -I -i"
BRANCH=
EXPORTDIR=
DIST=

# Parse command line options.
while getopts tsbd: OPT; do
   case "$OPT" in
     h)
       echo $USAGE
       exit 0
       ;;
     t)
       TAG="--git-tag --git-retag"
       ;;
     s)
       BUILDER="--git-builder=debuild -I -i -S"
       ;;
     b)
       BUILDER="--git-builder=debuild -I -i -b"
       ;;
     d)
       DIST=$OPTARG
       BRANCH="--git-debian-branch=$DIST"
       EXPORTDIR="--git-export-dir=../build-area/$DIST/"
       ;;
     \?)
       # getopts issues an error message
       echo $USAGE >&2
       exit 1
       ;;
   esac
done

# Remove the switches we parsed above.
shift `expr $OPTIND - 1`

# check if there are any other parameter
if [ $# -ne 0 ]; then
  echo unkown parameter: $@
  echo $USAGE >&2
  exit 1
fi

# switch to $DIST branch; store current branch
CURRENT_BRANCH=$(git branch | grep \* | sed 's/\* \(.*\)/\1/g')
if [ "$DIST" != "$CURRENT_BRANCH" ]; then
  echo switch to branch \'$DIST\'
  git checkout $DIST
fi

#if [ "$DIST" == $(git branch | grep \* | sed 's/\* \(.*\)/\1/g') ]; then
  # build the package
  echo $BUILD_CMD $TAG $BUILDER $BRANCH $EXPORTDIR
  $BUILD_CMD $TAG $BUILDER $BRANCH $EXPORTDIR
#else
  #echo could not switch branch
#fi

# switch back branch
if [ -n $CURRENT_BRANCH ]; then
  echo switch back to branch \'$CURRENT_BRANCH\'
  git checkout $CURRENT_BRANCH
fi



