#!/bin/sh

clean ()
{
  make distclean >/dev/null 2>/dev/null

  find . -name "Makefile" -exec rm -f {} \; 2>/dev/null
  find . -name "Makefile.in" -exec rm -f {} \;  2>/dev/null
  find . -name ".deps" -exec rm -rf {} \; 2>/dev/null
  find . -name ".libs" -exec rm -rf {} \; 2>/dev/null

  rm -f config.h config.h.in config.h.in~ stamp-h1
  rm -f aclocal.m4 configure config.log config.status
  rm -rf autom4te.cache m4
  rm -f config.cache config.guess config.sub missing compile depcomp install-sh
  rm -f ltmain.sh libtool ltconfig
}

run()
{
  echo "Running $1 ..."
  $1
}

if test "x$1" = "xclean"; then
  echo "Cleaning Files..."
  clean
  exit 0
fi

AUTORECONF=${AUTORECONF:-autoreconf}

($AUTORECONF --version) >/dev/null 2>/dev/null || (echo "You need GNU autoconf to install from GIT (ftp://ftp.gnu.org/gnu/autoconf/)"; exit 1) || exit 1

run "mkdir -p m4"
run "$AUTORECONF -i"
