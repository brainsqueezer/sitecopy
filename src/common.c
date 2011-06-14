/* 
   sitecopy, manage remote web sites.
   Copyright (C) 1998-2004, Joe Orton <joe@manyfish.co.uk>.
                                                                     
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

#include "config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif 
#include <stdarg.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#include <ctype.h>

#include <ne_string.h>
#include <ne_utils.h>

#include "i18n.h"
#include "common.h"

#ifdef NE_DEBUGGING

static const struct {
    const char *name;
    int val;
} debug_map[] = {
    { "socket", NE_DBG_SOCKET },
    { "files", DEBUG_FILES },
    { "rcfile", DEBUG_RCFILE },
    { "ftp", DEBUG_FTP },
    { "xml", NE_DBG_XML },
    { "xmlparse", NE_DBG_XMLPARSE },
    { "http", NE_DBG_HTTP },
    { "httpauth", NE_DBG_HTTPAUTH },
    { "httpbody", NE_DBG_HTTPBODY },
    { "cleartext", NE_DBG_HTTPPLAIN },
    { "ssl", NE_DBG_SSL },
    { "rsh", DEBUG_RSH },
    { "sftp", DEBUG_SFTP },
    { NULL, 0 }
};

int map_debug_options(const char *opts, int *mask, char *errbuf)
{
    char *token, *ptr, *orig;
    int n, ret = 0, mapped;

    ptr = orig = ne_strdup(opts);

    do {
        token = ne_token(&ptr, ',');
        token = ne_shave(token, " ");

	mapped = 0;
        
	for (n = 0; debug_map[n].name != NULL; n++) {
	    if (strcasecmp(token, debug_map[n].name) == 0) {
		ret |= debug_map[n].val;
		mapped = 1;
		break;
	    }
	}
	if (!mapped) {
	    memset(errbuf, 0, 20);
	    strncpy(errbuf, token, 19);
            ne_free(orig);
	    return -1;
	}
    } while (ptr);
    
    *mask = ret;
    ne_free(orig);
    return 0;
}
#endif /* DEBUGGING */

/* Snagged from fetchmail */
# if !HAVE_STRERROR && !defined(strerror)
char *strerror (errnum)
     int errnum;
{
  extern char *sys_errlist[];
  extern int sys_nerr;

  if (errnum > 0 && errnum <= sys_nerr)
    return sys_errlist[errnum];
  return _("Unknown system error");
}
# endif /* HAVE_STRERROR */
