#!/bin/sh
#
# Update generated sources, created by codegen.
# Last ditch attempt to compromise with cmake ninja generator

LANG=C sort -c src/config-keys-list &&
codegen/srcpp LIST2CODE_CMD src/config-keys.c src/config-keys.h

exit $?
