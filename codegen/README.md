Code generation utils
=====================

srcpp
------

Processes files, searching for lines of format `KEYWORD command`, where `command` is shell command and `KEYWORD` is unique string, used to prefix these commands. When such like is found, entire line is replaced with `command` output.


Helper commands support
-----------------------

`srcpp` has some ready to use helper programs, placed in the same directory as srcpp. Any executables, placed in this directory, may be specified just by name, without their file path. There are few ready to use helpers.


List helpers
------------

These helpers are intended for easy strings indexing.

- `list2csarray` *(list to C string array)* - Coverts input lines into static strings array, where each string holds line from input file.

- `list2skenum` *(list to string key enum)* - Converts input lines into enum constants with names made from lines contents. Characters, which can't be in C identifier, are replaced with underscore.

When used both for same list, this resutts to two related objects - enum constants, referring to respective string in the array. For sorted input - resulting code may be used to implement binary strings search, to find index, useful in switch operator.

When used both for same list, element from resulting strings array may be indexed by constant, whose name is formed from this string. If input file is sorted, resulting code may be used to implement binary strings search, to find index, useful in switch operator.


Text helpers
------------

- `text2cstr` *(text to C string)* - Embeds text content into C static string, effectively making it a resource.
