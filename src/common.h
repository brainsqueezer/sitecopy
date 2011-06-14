/* 
   sitecopy, manage remote web sites.
   Copyright (C) 1998-2005, Joe Orton <joe@manyfish.co.uk>.
                                                                     
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

#ifndef COMMON_H
#define COMMON_H

#include <config.h>

#include <sys/types.h>

#include <stdio.h>

#include <ne_utils.h>

/* our own DEBUG_ channels */
#define DEBUG_FILES (1<<10)
#define DEBUG_RCFILE (1<<11)
#define DEBUG_FTP (1<<12)
#define DEBUG_GNOME (1<<13)
#define DEBUG_FILESEXTRA (1<<13)
#define DEBUG_RSH (1<<14)
#define DEBUG_SFTP (1<<15)

/* A signal hander */
typedef void (*sig_handler)(int);

#ifdef __EMX__
/* siebert: strcasecmp is stricmp */
#define strcasecmp stricmp
#endif

/* boolean */
#define true 1
#define false 0

#if defined (__EMX__) || defined(__CYGWIN__)
#define FOPEN_BINARY_FLAGS "b"
#define OPEN_BINARY_FLAGS O_BINARY
#else
#define FOPEN_BINARY_FLAGS ""
#define OPEN_BINARY_FLAGS 0
#endif

#if !HAVE_STRERROR && !defined(strerror)
char *strerror (int errnum);
#endif

extern const char *default_charset;
void init_charset(void);

/* Map debug options onto a debug mask. Returns non-zero on error, 
 * zero on success. err must be at least 20 bytes.
 * On non-zero return, err will contain the token which was not
 * recognized.
 * example use:
 
   char errbuf[20];
   int mask;

   if (map_debug_options(argv[n], &mask, errbuf)) {
       printf("did not understand channel %s\n", errbuf);
       exit(-1);
   }
 
*/ 

int map_debug_options(const char *opts, int *mask, char *err);

/* neon 0.24 compatibility */
#if NE_VERSION_MINOR == 24
#define ne_xml_failed(p) (!ne_xml_valid(p))
#define NE_FEATURE_SSL 1
#define ne_has_support(x) ne_supports_ssl()
#endif

#endif

