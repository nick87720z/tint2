#!/bin/sh

# Dependencies:
# go-md2man: https://github.com/cpuguy83/go-md2man
# discount: http://www.pell.portland.or.us/~orc/Code/discount/

set -e -x

go-md2man -in='tint2.md' > 'tint2.1'

markdown -f githubtags < 'tint2.md' |
sed 's/^# TINT2 .*$/# TINT2/g'      |
cat 'html_templ/header.html' - 'html_templ/footer.html' > 'manual.html'

markdown -f githubtags < '../README.md' |
sed 's|doc/tint2.md|manual.html|g'      |
cat 'html_templ/header.html' - 'html_templ/footer.html' > 'readme.html'
