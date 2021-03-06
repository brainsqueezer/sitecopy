#!/bin/sh
#
#  sitecopy, for managing remote web sites.
#  Copyright (C) 2008, Ramon Antonio Parada <rap@ramonantonio.net>
#                                                                    
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#




# Some variables and settings used throughout the script
set -e
KOPETE_TMP="/tmp/sitecopy-install";
KOPETE_LOG=$KOPETE_TMP"/sitecopy.log";
KOPETE_SRC=$KOPETE_TMP"/src";
KOPETE_FILES=$KOPETE_TMP"/files";
KOPETE_PRE="[sitecopy]";
AUTHOR_NAME="Ramon Antonio Parada";
AUTHOR_EMAIL="rap@ramonantonio.net";
VERSION="0.17.0";
SUBVERSION="3"
PKGNAME="sitecopy"



kopete_builds() {
   #--prefix=/path/to/your/distribution's/kde
   echo -n "$KOPETE_PRE Configuring... ";
  # ./configure --prefix=/usr/local --quiet &> $KOPETE_LOG;
   ./configure --prefix=/usr --quiet
# --enable-gnomefe
   test_done;
   
   echo -n "$KOPETE_PRE Building sources... "
   make &> $KOPETE_LOG
   test_done;
   
   echo -n "$KOPETE_PRE Creating data dir... ";
  make DESTDIR=$KOPETE_FILES install &> $KOPETE_LOG;
   test_done;
}

test_done () {
   if [ $? -eq 0 ]; then
      echo "done.";
   else
      echo "failed.";
      echo "...";
      tail -n 6 $KOPETE_LOG
      echo "$KOPETE_PRE ERROR: Check $KOPETE_LOG for details."; 
      exit 1;
   fi
}

kopete_dir() {
#rm -r $KOPETE_TMP
   echo -n "$KOPETE_PRE Creating temporal folder structure... ";
   #chmod -R a+w src &&
   rm -rf $KOPETE_TMP &&
   mkdir $KOPETE_TMP &&
   mkdir $KOPETE_FILES &&
   mkdir $KOPETE_FILES/DEBIAN &&
#   mkdir $KOPETE_TMP/files/data
   test_done;

#ln -s $KOPETE_TMP/files/DEBIAN $KOPETE_TMP/files/debian &> $KOPETE_LOG;

}





kopete_permissions() {
   echo -n "$KOPETE_PRE Fixing permissions... ";
   chmod 755 $KOPETE_FILES/DEBIAN/*
   test_done;


   }


kopete_control() {
   # Generate control file
   echo -n "$KOPETE_PRE Generating control file... ";
   cat <<EOF > $KOPETE_FILES/DEBIAN/control
Package: $PKGNAME
Source: $PKGNAME
Version: 1:$VERSION-$SUBVERSION
Architecture: i386
Maintainer: $AUTHOR_NAME <$AUTHOR_EMAIL>
Depends: libc6 (>= 2.7-1), libneon27 (>= 0.27.2)
Conflicts: openssh-client (<< 1:4.2p1-1), xsitecopy (<< 1:0.10.15-1)
Section: web
Priority: extra
Homepage: http://www.manyfish.co.uk/sitecopy/
Description: A program for managing a WWW site via FTP, DAV or HTTP
 Sitecopy is for copying locally stored websites to remote ftp servers. With a
 single command, the program will synchronize a set of local files to a remote
 server by performing uploads and remote deletes as required. The aim is to
 remove the hassle of uploading and deleting individual files using an FTP
 client. Sitecopy will also optionally try to spot files you move locally, and
 move them remotely.
 .
 Sitecopy is designed to not care about what is actually on the remote server -
 it simply keeps a record of what it THINKS is in on the remote server, and
 works from that.
EOF
   test_done;
}
kopete_changelog() {

# Generate a simple changelog template
   echo -n "$KOPETE_PRE Generating changelog... ";
cat <<EOF > $KOPETE_FILES/DEBIAN/changelog
$PKGNAME ($VERSION-1) unstable; urgency=low

  * A standard release

 -- $AUTHOR_NAME  <$AUTHOR_EMAIL>  $(date -R)
EOF
   test_done;

}
kopete_md5() {

   # Generate DEBIAN/md5sums file
   echo -n "$KOPETE_PRE Generating md5sums... ";
   cd $KOPETE_FILES
   find ./ -type f | xargs md5sum >> $KOPETE_FILES/DEBIAN/md5sums 2> $KOPETE_LOG;
   
   test_done;



}

kopete_package() {
cd $KOPETE_FILES
   echo -n "$KOPETE_PRE Creating package control... ";
   echo -n "$KOPETE_PRE Creating package control... " &> $KOPETE_LOG
   #dpkg-gencontrol -isp -p$PKGNAME -P
   dpkg-gencontrol -p$PKGNAME


   test_done;

}
kopete_packag() {

cd $KOPETE_FILES
   echo -n "$KOPETE_PRE Creating package file... ";
   echo -n "$KOPETE_PRE Creating package file... " &> $KOPETE_LOG
   dpkg --build $KOPETE_FILES
   test_done;
}

kopete_remove() {
   echo -n "$KOPETE_PRE Removing temporal folder... ";
   rm -rf $KOPETE_TMP &> $KOPETE_LOG
   test_done;
}


kopete_dir
kopete_builds

#kopete_postinst
#kopete_shlibs
#kopete_postrm
kopete_control
#kopete_changelog
kopete_md5
kopete_permissions
#kopete_package1
kopete_packag
#kopete_remove

exit 0

