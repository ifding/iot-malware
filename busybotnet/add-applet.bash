#!/bin/bash
#########################################
##         Applenater			##
## Script to add an applet to busybox	###
## Author: Kod				##
#########################################
#


echo -n "enter basename: ";read i; 

echo "Config"
cat miscutils/Config.src | sed "s/INSERT/INSERT\nconfig\ $i\n\tbool\ \"$i\"\n\tdefault\ y\n\thelp\n\t\tx/" > tmp.config.src; cat tmp.config.src > miscutils/Config.src
rm -v tmp.config.src
echo "Kbuild"

cat miscutils/Kbuild.src | sed "s/INSERT/INSERT\nlib-\$\(CONFIG_`awk '{ print toupper($0) }' <<< $i`\)\ \ \ \ += $i.o/" > tmp.kbuild.src; cat tmp.kbuild.src > miscutils/Kbuild.src
rm -v tmp.kbuild.src

echo "applets.src"
cat include/applets.src.h | sed "s/INSERT/INSERT\nIF_`awk '{ print toupper($0) }' <<< $i`\(APPLET\($i, BB_DIR_BIN, BB_SUID_DROP\)\)/" > tmp.applets.src; cat tmp.applets.src > include/applets.src.h
rm -v tmp.applets.src

echo "usage"
cat include/usage.src.h | sed "s/INSERT/\#define\ $i\_full\_usage\ \ \"None\"\n\#define\ $i\_trivial\_usage\ \ \"None\"\n\nINSERT/" > tmp.usage.src; cat tmp.usage.src > include/usage.src.h

rm -v tmp.usage.src

echo "done"
