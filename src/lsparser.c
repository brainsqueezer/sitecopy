/* 
   'ls' output parser, for rsh and ftp drivers
   Copyright (C) 1998-2005, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <ne_string.h>
#include <ne_alloc.h>
#include <ne_utils.h>

#include "common.h"
#include "lsparser.h"

struct ls_context {
    int after_blank;
    char *topdir;
    char *curdir;
    const char *error;
};

ls_context_t *ls_init(const char *dir)
{
    ls_context_t *ctx = ne_malloc(sizeof *ctx);
    char *ptr;

    ctx->after_blank = 0;
    ctx->curdir = ne_strdup("");
    ctx->topdir = ne_strdup(dir);

    ptr = ctx->topdir + strlen(ctx->topdir) - 1;
    if (*ptr == '/') *ptr = '\0';

    NE_DEBUG(DEBUG_FTP, "ls: init, topdir: [%s]\n", ctx->topdir);
    
    return ctx;
}

void ls_destroy(ls_context_t *ctx)
{
    ne_free(ctx->curdir);
    ne_free(ctx->topdir);
    ne_free(ctx);
}

/* Parse LINE of length LEN as a directory line. */
static enum ls_result parse_directory(ls_context_t *ctx, char *line, size_t len)
{
    /* A new directory name indicator, which goes:
     *    `directory/name/here:'
     * We want directory names as:
     *    `directory/name/here/' 
     * Hence a bit of messing about. */
    ne_free(ctx->curdir);
    
    /* Skip a leading Windows drive specification */
    if (len > 3 &&
        isalpha(line[0]) && line[1] == ':' && line[2] == '/') {
        line += 2;
    }
    
    if (strncmp(line, ctx->topdir, strlen(ctx->topdir)) == 0) {
        line += strlen(ctx->topdir);
    }
    
    /* Skip a single . if .:  */
    if (strcmp(line,".:") == 0) {
        line++;
    }
    
    /* Skip a leading "./" */
    if (strncmp(line, "./", 2) == 0) {
        line += 2;
    }
    
    /* Skip repeated '/' characters */
    while (*line == '/') line++;
    
    line = ne_strdup(line);
    len = strlen(line);
    
    if (len > 1) {
        line[len-1] = '/';  /* change ':' to '/' */
    } else {
        /* this is just the top-level directory... */
        line[0] = '\0';
    }
    
    ctx->curdir = line;
    NE_DEBUG(DEBUG_FTP, "ls: Now in directory: [%s]\n", ctx->curdir);
    return ls_nothing;
}

static inline char *skip_field(char *line)
{
    while (*line != '\0' && *line != ' ')
        line++;
    
    while (*line != '\0' && *line == ' ')
        line++;
    
    return line;
}

/* Return mode bits for permissions string PERMS */
static mode_t parse_permissions(const char *perms)
{
    mode_t ret = 0;
    const char *p = perms;
    while (*p) {
	ret <<= 1;
	if (*p != '-') {
	    ret |= 1;
        }
        p++;
    }
    return ret & 0777;
}

static enum ls_result fail(ls_context_t *ctx, const char *errstr)
{
    ctx->error = errstr;
    return ls_error;
}

/* Parse LINE of length LEN as a "file" line, placing result in *FILE */
static enum ls_result parse_file(ls_context_t *ctx, char *line, size_t len,
                                 struct ls_file *file)
{
    char *perms, *size;

    perms = ne_token(&line, ' ');
    if (!line) return fail(ctx, "Missing token at beginning of line");
    while (*line++ == ' ') /* nullop */;
 
    /* skip inode, user and group fields */
    line = skip_field(skip_field(skip_field(line)));
    if (*line == '\0') {
        return fail(ctx, "Missing token in inode/user/group fields");
    }

    size = ne_token(&line, ' ');
    if (!line) {
        return fail(ctx, "Missing token after inode/user/group fields");
    }
    while (*line++ == ' ') /* nullop */;

    /* skip Month, day, time fields */
    line = skip_field(skip_field(skip_field(line)));
    if (*line == '\0') {
        return fail(ctx, "Missing token after timestamp field");
    }
 
    /* Bail out if this isn't a file or directory. */
    if (perms[0] != '-' && perms[0] != 'd') {
        NE_DEBUG(DEBUG_FTP, "ls: ignored line\n");
        return ls_nothing;
    }

    /* line now points at the last field, the filename.  Reject any
     * relative filenames. */
    if (strchr(line, '/') != NULL) {
        return fail(ctx, "Relative filename disallowed");
    }

    if (perms[0] == '-') {
        /* Normal file */
        file->mode = parse_permissions(perms);
        file->name = ne_concat(ctx->curdir, line, NULL);
        file->size = strtol(size, NULL, 10);
        NE_DEBUG(DEBUG_FTP, "ls: file (%03o, %" NE_FMT_OFF_T "): [%s]\n",
                 file->mode, file->size, file->name);
        return ls_file;
    } else /* perms[0] == 'd' */ {
        if (strcmp(line, ".") == 0 || strcmp(line, "..") == 0) {
            return ls_nothing;
        }
        file->mode = parse_permissions(perms);
        file->name = ne_concat(ctx->curdir, line, NULL);
        NE_DEBUG(DEBUG_FTP, "ls: directory (%03o): %s\n", file->mode, file->name);
        return ls_directory;
    }
}

enum ls_result ls_parse(ls_context_t *ctx, char *line, struct ls_file *file)
{
    size_t len;

    line = ne_shave(line, "\r\n\t ");
    len = strlen(line);

    NE_DEBUG(DEBUG_FTP, "ls: [%s]\n", line);

    if (len == 0) {
        ctx->after_blank = 1;
        return ls_nothing;
    }

    ctx->after_blank = 0;

    if (strncmp(line, "total ", 6) == 0) {
        /* ignore the line */
        return ls_nothing;
    }

    if (line[len-1] == ':' && (ctx->after_blank || strchr(line, ' ') == NULL)) {
        return parse_directory(ctx, line, len);        
    } else {
        return parse_file(ctx, line, len, file);
    }
}

const char *ls_geterror(ls_context_t *ctx)
{
    return ctx->error;
}

void ls_pflist_add(struct proto_file **list, struct proto_file **tail,
                   const struct ls_file *lsf, enum ls_result result)
{
    struct proto_file *pf = ne_calloc(sizeof *pf);

    switch (result) {
    case ls_file:
        pf->filename = lsf->name;
        pf->mode = lsf->mode;
        pf->type = proto_file;
        pf->size = lsf->size;
        
        pf->next = *list;
        *list = pf;
        if (*tail == NULL) *tail = pf;
        break;

    case ls_directory:
        pf->filename = lsf->name;
        pf->mode = lsf->mode;
        pf->type = proto_dir;
        
        if (*tail == NULL) {
            *list = pf;
        } else {
            (*tail)->next = pf;
        }
        *tail = pf;
        break;

    default:
        ne_free(pf);
        break;
    }
}
