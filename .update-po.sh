#!/bin/sh -ex
# Script to run update-po and update-gmo before generating
# a release tarball.  Run from .release.sh, *before*
# autogen.sh. 

inmk=/usr/share/gettext/po/Makefile.in.in
tmpmk=`mktemp /tmp/sitecopy.XXXXXX`
pot=`mktemp /tmp/sitecopy.XXXXXX`
trap 'rm -f $tmpmk $pot' TERM INT 0

cd po

lngs=`echo *.po | sed "s/.po//g"`
for l in $lngs; do
    CATALOGS="$CATALOGS $l.gmo"
    POFILES="$POFILES $l.po"
done
GMOFILES="$CATALOGS"

sed -e "/^#/d" -e "/^[ 	]*\$/d" -e "s,.*,     ../& \\\\," \
    -e "\$s/\(.*\) \\\\/\1/" < POTFILES.in > $pot

sed -e "
/POTFILES =/r $pot;
s/@SET_MAKE@//g;
s/@PACKAGE@/sitecopy/g;
s/@VERSION@/$1/g;
/^.*VPATH.*$/d;
1i\
DOMAIN = sitecopy
s/@srcdir@/./g;
s/@top_srcdir@/../g;
s/@CATALOGS@/$CATALOGS/g;
s/@POFILES@/$POFILES/g;
s/@UPDATEPOFILES@/$POFILES/g;
s/@GMOFILES@/$GMOFILES/g;
s/@GMSGFMT@/msgfmt/g;
s/@MSGFMT@/msgfmt/g;
s/@XGETTEXT@/xgettext/g;
s/@MSGMERGE@/msgmerge/g;
s/: Makefile.*/:/g;
s/\$(MAKE) update-gmo/echo Done/g;" $inmk > $tmpmk

make -f $tmpmk sitecopy.pot-update ${GMOFILES}
