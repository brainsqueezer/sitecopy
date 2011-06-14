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

#ifndef LS_PARSER_H
#define LS_PARSER_H 1

#include "config.h"

#include <sys/types.h>

#include "ne_defs.h" /* for off_t */

#include "protocol.h"

struct ls_file {
    mode_t mode;
    off_t size;
    char *name;
};

typedef struct ls_context ls_context_t;

/* Initialize LS parser context. */
ls_context_t *ls_init(const char *dirname);

/* The result of parsing one line of LS output: */
enum ls_result {
    ls_directory, /* a subdirectory */
    ls_file,      /* a file */
    ls_nothing,   /* nothing interesting on this line */
    ls_error      /* a parse error */
};

/* Parse LINE of ls output.  Modifies LINE.  Returns one of the
 * ls_result values; fills in file->mode, file->size, file->name for
 * ls_file; file->mode and file->name for ls_directory.  file->name is
 * ne_malloc-allocated. */
enum ls_result ls_parse(ls_context_t *ctx, char *line, struct ls_file *file);

/* Return the error string from the context, if ls_error was
 * previously called. */
const char *ls_geterror(ls_context_t *ctx);

/* Destroy an LS parser context. */
void ls_destroy(ls_context_t *ctx);

/* Adds a parsed file or directory entry to a proto_file list. */
void ls_pflist_add(struct proto_file **list, struct proto_file **tail,
                   const struct ls_file *lsf, enum ls_result result);

#endif /* LSPARSER_H */


