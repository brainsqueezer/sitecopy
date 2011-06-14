#!/bin/sh -ex
rm -rf config.cache autom4te*.cache
test -f .version || echo -n 0.0.0 > .version
${ACLOCAL:-aclocal} -I m4/neon
${AUTOHEADER:-autoheader}
${AUTOCONF:-autoconf}
cp /usr/share/libtool/config.sub .
cp /usr/share/libtool/config.guess .
rm -rf autom4te*.cache
rm -rf intl
cp -pr /usr/share/gettext/intl intl

