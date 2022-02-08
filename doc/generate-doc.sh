#!/bin/sh

# Dependencies:
# go-md2man: https://github.com/cpuguy83/go-md2man
# discount: http://www.pell.portland.or.us/~orc/Code/discount/

set -e -x

go-md2man -in=tint2.md > tint2.1
markdown -f githubtags < tint2.md     | cat header.html - footer.html > manual.html
markdown -f githubtags < ../README.md | cat header.html - footer.html > readme.html
