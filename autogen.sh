#!/bin/bash

set -xe # be verbose and abort on errors

rm -rf autom4te.cache/ config.cache

autoreconf -vfsi

intltoolize --force --automake # intltool-extract.in intltool-merge.in intltool-update.in po/Makefile.in.in
rm -f po/Makefile.in.in && cp -v po/Makefile.intltool po/Makefile.in.in # fix po/Makefile...

./configure --enable-devel-mode=yes "$@"
