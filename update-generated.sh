#!/bin/sh
#
# Update generated sources.
# For codegen output - last ditch attempt to compromise with cmake ninja generator

selfdir=$( dirname $0 )

LANG=C sort -c "${selfdir}"/src/config-keys-list &&
"${selfdir}"/codegen/srcpp LIST2CODE_CMD "${selfdir}"/src/config-keys.c "${selfdir}"/src/config-keys.h

code=$?
if [ ${code} -ne 0 ]; then
    exit ${code}
fi

command -v tint2 >/dev/null 2>&1 && [ "${selfdir}"/default_icon.png -nt "${selfdir}"/src/default_icon ] &&
cd "${selfdir}"/src && tint2 --dump-image-data ../default_icon.png default_icon

code=$?
if [ ${code} -ne 0 ]; then
    exit ${code}
fi
