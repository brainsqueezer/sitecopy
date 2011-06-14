/* 
   sitecopy, remote web site copy manager.
   Copyright (C) 1998-2002, Joe Orton <joe@manyfish.co.uk>
                                                                     
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef RCFILE_H
#define RCFILE_H

#include "sites.h"

/* You MUST call the initialization functions in this order:
 *
 *    init_env()    - sets up rcfile, copypath etc global vars.
 *    init_paths()  - checks the rcfile permissions etc
 *    init_netrc()  - read the netrc if there is one
 *    rcfile_read() - read the rcfile
 */

/* Filename of rcfile stored here */
extern char *rcfile;
/* Filename of directory used to store info files */
extern char *copypath;

/* rcfile_read: 
 * Read the rcfile, filling the list pointed to by sites.
 * Returns 0 on success, or:
 *    RC_OPENFILE  if the rcfile could not be opened
 *    RC_CORRUPT   if the rcfile is not of valid syntax
 * When RC_CORRUPT is returned, then 
 *    rcfile_linenum is set to the line at which parsing stopped.
 *    rcfile_err is set to the contents of that line.
 * When RC_OPENFILE is returned,
 *    rcfile_err is set to the strerror(errno)
 */
extern int rcfile_linenum; 
extern char *rcfile_err;

int rcfile_read(struct site **sites);

/* Check the permission of the rcfile and storage directory.
 * Returns 0 on success, or:
 *    RC_OPENFILE    if the rcfile could not be stat'ed
 *    RC_DIROPEN     if the direcory could not be stat'ed
 *    RC_PERMS       if the rcfile had invalid permissions
 *    RC_DIRPERMS    if the directory had invalid permissions
 *    RC_NETRCPERMS  if the netrcfile had invalid permissions
 */
int init_paths(void);

/* Initialize the rcfile, copypath, netrcfile variables from the
 * environment.
 * Returns 0 on success or 1 if the copypath 
 */
int init_env(void);

/* Read in the netrc file.
 * Return 0 on success or 1 if the .netrc could not be parsed.
 */
int init_netrc(void);

/* Verify that the given site entry is correct, and fill in
 * missing bits as necessary (e.g. password from netrc, default
 * Returns 0 on success, or:
 *  SITE_UNSUPPORTED  if the protocol is not supported
 *  SITE_NOSERVER     if no server name has been specified
 *  SITE_NOREMOTEDIR  if no remote directory has been specified
 *  SITE_NOLOCALDIR   if no local directory has been specified
 *  SITE_ACCESSLOCALDIR  if the local directory is not readable
 *  SITE_INVALIDPORT  if an invalid port name has been specified
 *  SITE_NOMAINTAIN   if the proto driver doesn't support symlink maintain mode
 *  SITE_NOREMOTEREL  if the proto driver doesn't allow relative remote dirs
 *  SITE_NOPERMS      if the proto driver doesn't support permissions
 *  SITE_NOLOCALREL   if a local relative dir could not be used
 *  SITE_NOSAFEOVER   if they used nooverwrite mode and safe mode
 *  SITE_NOSAFETEMPUP if they used tempupload and safe mode
 *  SITE_NORENAMES    if they want renames + not using state checksum
 */
int rcfile_verify(struct site *any_site);

/* Writes the given sites list into the given rcfile.
 * This overwrites any previous contents of filename.
 * Returns 0 on success, or:
 *    RC_OPENFILE    if the file could not be opened for writing
 *    RC_PERMS       if the file permissions could not be set
 *    RC_CORRUPT     if there were errors while writing to the file
 */
int rcfile_write (char *filename, struct site *list_of_sites);

/* Constants */

#define RC_OPENFILE 900
#define RC_CORRUPT 901
#define RC_PERMS 902
#define RC_DIRPERMS 903
#define RC_DIROPEN 904
#define RC_NETRCOPEN 905
#define RC_NETRCPERMS 906

#define SITE_NOSITE 901
#define SITE_NONAME 920
#define SITE_NOSERVER 921
#define SITE_NOREMOTEDIR 924
#define SITE_NOLOCALDIR 925
#define SITE_ACCESSLOCALDIR 926
#define SITE_INVALIDPORT 927
#define SITE_NOMAINTAIN 928
#define SITE_NOREMOTEREL 929
#define SITE_NOLOCALREL 930
#define SITE_NOPERMS 931
#define SITE_NOSAFEOVER 932
#define SITE_NORENAMES 933
#define SITE_NOSAFETEMPUP 934
#define SITE_NOSERVERCERT 935

#endif /* RCFILE_H */
