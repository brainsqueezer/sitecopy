#!/bin/bash -ex
# Release script run before generating a release tarball.
# Usage: ./.release.sh VERSION

echo $1 > .version

# Generate docs for Xsitecopy
pushd gnome/doc
db2html -o `pwd` xsitecopy.sgml
# Remove generated images dir
rm xsitecopy/stylesheet-images/*.gif
rmdir -p xsitecopy/stylesheet-images
popd

sed s/@VERSION@/$1/ < sitecopy.spec.in > sitecopy.spec

exec ./.update-po.sh
