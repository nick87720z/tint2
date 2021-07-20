#!/bin/sh
#
# Processes config-keys.h.in, replacing lines of format
#    LIST2CODE_CMD:cmd
# with 'cmd' output.

cmd_keyword="$1"

preproc() {
    printf '%s\n\n' "/* Do not edit. Auto-generated from $1.in */"                    > "$1"
    sed -e '/^ *'"${cmd_keyword}"'/{ s/^ *'"${cmd_keyword}"'://; e' -e ' }' < "$1.in" >> "$1"
}

if [ -z "${cmd_keyword}" ]; then
    cmd_keyword=LIST2CODE_CMD
fi

preproc config-keys.h
preproc config-keys.c
