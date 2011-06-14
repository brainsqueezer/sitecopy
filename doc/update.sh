#!/bin/sh
# A sample script to create a 'Recent Changes' type file
# for a site before each update.
#   (this file placed in the public domain) - Joe Orton

#### Alter the following accordingly, else it won't work. ####
html=~/sitedir/changes.html
awkf=/usr/share/sitecopy/changes.awk
sitename=mysite


if sitecopy --list ${sitename} > /dev/null; then
	echo No changes to the site.
	exit
fi
sitecopy --flatlist ${sitename} | awk -f ${awkf} > ${html}
sitecopy --update ${sitename}
