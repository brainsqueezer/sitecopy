/* 
   sitecopy noop protocol driver module
   Copyright (C) 2004, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#include <config.h>

#include <sys/types.h>

#include "protocol.h"
#include "frontend.h"
#include "i18n.h"


static int init(void **session, struct site *site)
{
    return SITE_OK;
}

static void finish(void *session) 
{
}

static int null_move(void *session, const char *from, const char *to) 
{
    return SITE_OK;
}

static int null_upload_cond(void *session, const char *local, 
                            const char *remote, int ascii, time_t t)
{
    return SITE_OK;
}

static int null_get_modtime(void *session, const char *remote, time_t *modtime)
{
    time(modtime);
    return SITE_OK;
}
    
static int null_updownload(void *session, const char *local, const char *remote,
                           int ascii) 
{
    return SITE_OK;
}

static int null_onearg(void *session, const char *dirname)
{
    return SITE_OK;
}

static int null_read(void *session, const char *remote, 
		     ne_block_reader reader, void *userdata) 
{
    return SITE_OK;
}

static int null_chmod(void *session, const char *remote, mode_t mode) 
{
    return SITE_OK;
}


static int fetch_list(void *session, const char *dirname, int need_modtimes,
		       struct proto_file **files) 
{
    return SITE_OK;
}

static int unimp_link2(void *session, const char *l, const char *target)
{
    return SITE_UNSUPPORTED;
}
 
static int unimp_link1(void *session, const char *l)
{
    return SITE_UNSUPPORTED;
}

static int null_get_port(struct site *site)
{
    return 0;
}


static const char *null_error(void *session) 
{
    return "No error occurred";
}

/* The noop protocol driver */
const struct proto_driver null_driver = {
    init,
    finish,
    null_move,
    null_updownload,
    null_upload_cond,
    null_get_modtime,
    null_updownload,
    null_read,
    null_onearg,
    null_chmod,
    null_onearg,
    null_onearg,
    unimp_link2, /* create link */
    unimp_link2, /* change link target */
    unimp_link1, /* delete link */
    fetch_list,
    null_error,
    null_get_port,
    null_get_port,
    "NULL"
};
