# a silly hack that generates autoregen.sh but it's handy
echo "#!/bin/sh" > autoregen.sh
echo "./autogen.sh $@ \$@" >> autoregen.sh
chmod +x autoregen.sh

# helper functions for autogen.sh

debug ()
# print out a debug message if DEBUG is a defined variable
{
  if test ! -z "$DEBUG"
  then
    echo "DEBUG: $1"
  fi
}

version_check ()
# check the version of a package
# first argument : package name (executable)
# second argument : optional path where to look for it instead
# third argument : source download url
# rest of arguments : major, minor, micro version
{
  PACKAGE=$1
  PKG_PATH=$2
  URL=$3
  MAJOR=$4
  MINOR=$5
  MICRO=$6

  WRONG=

  if test ! -z "$PKG_PATH"
  then
    COMMAND="$PKG_PATH"
  else
    COMMAND="$PACKAGE"  
  fi
  debug "major $MAJOR minor $MINOR micro $MICRO"
  VERSION=$MAJOR
  if test ! -z "$MINOR"; then VERSION=$VERSION.$MINOR; else MINOR=0; fi
  if test ! -z "$MICRO"; then VERSION=$VERSION.$MICRO; else MICRO=0; fi

  debug "major $MAJOR minor $MINOR micro $MICRO"
  
  test -z "$NOCHECK" && {
      echo -n "  checking for $1 >= $VERSION"
      if test ! -z "$PKG_PATH"; then echo -n " (in $PKG_PATH)"; fi
      echo -n "... "
  } || {
    # we set a var with the same name as the package, but stripped of
    # unwanted chars
    VAR=`echo $PACKAGE | sed 's/-//g'`
    debug "setting $VAR"
    eval $VAR="$COMMAND"
      return 0
  }

  debug "checking version with $COMMAND"
  ($COMMAND --version) < /dev/null > /dev/null 2>&1 || 
  {
	echo "not found !"
	echo "You must have $PACKAGE installed to compile $package."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at $URL"
	return 1
  }
  # the following line is carefully crafted sed magic
  #pkg_version=`$COMMAND --version|head -n 1|sed 's/^[a-zA-z\.\ ()]*//;s/ .*$//'`
  pkg_version=`$COMMAND --version|head -n 1|sed 's/^.*) //'|sed 's/ (.*)//'`
  debug "pkg_version $pkg_version"
  # remove any non-digit characters from the version numbers to permit numeric
  # comparison
  pkg_major=`echo $pkg_version | cut -d. -f1 | sed s/[a-zA-Z\-].*//g`
  pkg_minor=`echo $pkg_version | cut -d. -f2 | sed s/[a-zA-Z\-].*//g`
  pkg_micro=`echo $pkg_version | cut -d. -f3 | sed s/[a-zA-Z\-].*//g`
  test -z "$pkg_minor" && pkg_minor=0
  test -z "$pkg_micro" && pkg_micro=0

  debug "found major $pkg_major minor $pkg_minor micro $pkg_micro"

  #start checking the version
  debug "version check"

  if [ ! "$pkg_major" -gt "$MAJOR" ]; then
    debug "$pkg_major <= $MAJOR"
    if [ "$pkg_major" -lt "$MAJOR" ]; then
      WRONG=1
    elif [ ! "$pkg_minor" -gt "$MINOR" ]; then
      if [ "$pkg_minor" -lt "$MINOR" ]; then
        WRONG=1
      elif [ "$pkg_micro" -lt "$MICRO" ]; then
	WRONG=1
      fi
    fi
  fi

  if test ! -z "$WRONG"; then
    echo "found $pkg_version, not ok !"
    echo
    echo "You must have $PACKAGE $VERSION or greater to compile $package."
    echo "Get the latest version from $URL"
    echo
    return 1
  else
    echo "found $pkg_version, ok."
    # we set a var with the same name as the package, but stripped of
    # unwanted chars
    VAR=`echo $PACKAGE | sed 's/-//g'`
    debug "setting $VAR"
    eval $VAR="$COMMAND"
  fi
}

aclocal_check ()
{
  # normally aclocal is part of automake
  # so we expect it to be in the same place as automake
  # so if a different automake is supplied, we need to adapt as well
  # so how's about replacing automake with aclocal in the set var,
  # and saving that in $aclocal ?
  # note, this will fail if the actual automake isn't called automake*
  # or if part of the path before it contains it
  if [ -z "$automake" ]; then
    echo "Error: no automake variable set !"
    return 1
  else
    aclocal=`echo $automake | sed s/automake/aclocal/`
    debug "aclocal: $aclocal"
  fi
}

autoconf_2_52d_check ()
{
  # autoconf 2.52d has a weird issue involving a yes:no error
  # so don't allow it's use
  test -z "$NOCHECK" && {
    ac_version=`$autoconf --version|head -n 1|sed 's/^[a-zA-z\.\ ()]*//;s/ .*$//'`
    if test "$ac_version" = "2.52d"; then
      echo "autoconf 2.52d has an issue with our current build."
      echo "We don't know who's to blame however.  So until we do, get a"
      echo "regular version.  RPM's of a working version are on the gstreamer site."
      exit 1
    fi
  }
  return 0
}

die_check ()
{
  # call with $DIE
  # if set to 1, we need to print something helpful then die
  DIE=$1
  if test "x$DIE" = "x1";
  then
    echo
    echo "- Please get the right tools before proceeding."
    echo "- Alternatively, if you're sure we're wrong, run with --nocheck."
    exit 1
  fi
}

autogen_options ()
{
  if test `getopt --version | cut -d' ' -f2` != "(enhanced)"; then
    echo "- non-gnu getopt(1) detected, not running getopt on autogen command-line options"
    return 0
  fi

  # we use getopt stuff here, copied things from the example example.bash
  TEMP=`getopt -o h --long noconfigure,nocheck,debug,help,with-automake:,with-autoconf:,prefix:\
       -- "$@"`

  eval set -- "$TEMP"

  while true ; do
    case "$1" in
      --noconfigure)
          NOCONFIGURE=defined
	  AUTOGEN_EXT_OPT="$AUTOGEN_EXT_OPT --noconfigure"
          echo "+ configure run disabled"
          shift
          ;;
      --nocheck)
	  AUTOGEN_EXT_OPT="$AUTOGEN_EXT_OPT --nocheck"
          NOCHECK=defined
          echo "+ autotools version check disabled"
          shift
          ;;
      --debug)
          DEBUG=defined
	  AUTOGEN_EXT_OPT="$AUTOGEN_EXT_OPT --debug"
          echo "+ debug output enabled"
          shift
          ;;
      --prefix)
	  CONFIGURE_EXT_OPT="$CONFIGURE_EXT_OPT --prefix=$2"
	  echo "+ passing --prefix=$2 to configure"
          shift 2
          ;;
      -h|--help)
          echo "autogen.sh (autogen options) -- (configure options)"
          echo "autogen.sh help options: "
          echo " --noconfigure            don't run the configure script"
          echo " --nocheck                don't do version checks"
          echo " --debug                  debug the autogen process"
	  echo " --prefix		  will be passed on to configure"
          echo
          echo " --with-autoconf PATH     use autoconf in PATH"
          echo " --with-automake PATH     use automake in PATH"
          echo
          echo "to pass options to configure, put them as arguments after -- "
	  exit 1
          ;;
      --with-automake)
          AUTOMAKE=$2
          echo "+ using alternate automake in $2"
          shift 2
          ;;
      --with-autoconf)
          AUTOCONF=$2
          echo "+ using alternate autoconf in $2"
          shift 2
          ;;
       --) shift ; break ;;
      *) echo "Internal error !" ; exit 1 ;;
    esac
  done

  for arg do CONFIGURE_EXT_OPT="$CONFIGURE_EXT_OPT $arg"; done
  if test ! -z "$CONFIGURE_EXT_OPT"
  then
    echo "+ options passed to configure: $CONFIGURE_EXT_OPT"
  fi
}

toplevel_check ()
{
  srcfile=$1
  test -f $srcfile || {
        echo "You must run this script in the top-level $package directory"
        exit 1
  }
}


tool_run ()
{
  tool=$1
  options=$2
  echo "+ running $tool $options..."
  $tool $options || {
    echo
    echo $tool failed
    exit 1
  }
}
