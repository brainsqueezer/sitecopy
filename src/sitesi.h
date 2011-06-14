/* 
   sitecopy, for managing remote web sites.
   Copyright (C) 1999-2005, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#ifndef SITESI_H
#define SITESI_H

#include <sys/stat.h>

#include "common.h"
#include "sites.h"
#include "frontend.h" /* for fe_*_abort */

#include "ne_md5.h" /* for ne_md5_* */

/* Internal sites code handling */

void file_uploaded(struct site_file *file, struct site *site);
void file_downloaded(struct site_file *file, struct site *site);

int file_contents_changed(struct site_file *file, struct site *site);
int file_perms_changed(struct site_file *file, struct site *site);

#define DEBUG_GIVE_DIFF(diff)					\
       diff==file_new?"new":(					\
	   diff==file_changed?"changed":(			\
	       diff==file_unchanged?"unchanged":(		\
		   diff==file_deleted?"deleted":"moved")))

/* Macro, used mainly internally to the sites code to dump a file. */
#define DEBUG_DUMP_FILE(ch, file)			\
NE_DEBUG(ch, "%s: local=%s stored=%s, %s%s\n",		\
       (file->type==file_file)?"File":(			\
	   file->type==file_dir?"Dir":"Link"),		\
       file->local.filename?file->local.filename:".",	\
       file->stored.filename?file->stored.filename:".",	\
       DEBUG_GIVE_DIFF(file->diff),			\
       file->ignore?" (ignored)":"")

#define DEBUG_DUMP_FILE_PROPS(ch, file, site)			\
DEBUG_DUMP_FILE(ch, file);					\
if (site->state_method == state_timesize) {			\
    NE_DEBUG(ch, "Time: %ld/%ld Size: %" NE_FMT_OFF_T "/%" NE_FMT_OFF_T, \
       file->local.time, file->stored.time,			\
       file->local.size, file->stored.size);			\
} else {							\
    char l[33], s[33];						\
    ne_md5_to_ascii(file->local.checksum, l);			\
    ne_md5_to_ascii(file->stored.checksum, s);			\
    NE_DEBUG(ch, "Checksum: %32s/%32s\n", l, s);			\
}								\
NE_DEBUG(ch, "ASCII:%c/%c Perms:%03o/%03o\n",			\
       file->local.ascii?'y':'n', file->stored.ascii?'y':'n',	\
       file->local.mode, file->stored.mode   );			\
if (file->server.exists) {					\
    NE_DEBUG(ch, "Server: %ld\n", file->server.time);		\
}

/* Use to iterate over a files list. */
#define for_each_file(file, site) \
for (file = site->files; file != NULL; file = file->next)


/* Remove a file from the files list */
void file_delete(struct site *site, struct site_file *item);

/* Destroys a file state */
void file_state_destroy(struct file_state *state);
/* Copies a file state from src to dest */
void file_state_copy(struct file_state *dest, const struct file_state *src,
		      struct site *site);

/* This *MUST* be called after you change the local or stored state
 * of a file. It sets the ->diff appropriately, and updates the
 * site's num* and total* fields. */
void file_set_diff(struct site_file *file, struct site *site);

struct site_file *
file_find(struct site *site, const char *fname, enum file_type type);

struct site_file *
file_set_stored(enum file_type type, struct file_state *state,
		struct site *site);

struct site_file *
file_set_local(enum file_type type, struct file_state *state,
	       struct site *site);

static inline void
site_stats_increase(const struct site_file *file, struct site *site);

static inline void
site_stats_decrease(const struct site_file *file, struct site *site);

void site_stats_update(struct site *site);

static inline void 
site_stats_increase(const struct site_file *file, struct site *site) {
    switch (file->diff) {
    case file_unchanged:
	site->numunchanged++;
	break;
    case file_changed:
	if (file->ignore) {
	    site->numignored++;
	} else {
	    site->numchanged++;
	    site->totalchanged += file->local.size;
	}
	break;
    case file_new:
	site->numnew++;
	site->totalnew += file->local.size;
	break;
    case file_moved:
	site->nummoved++;
	break;
    case file_deleted:
	site->numdeleted++;
	break;
    default:
	/* Do nothing */
	break;
    }
}

static inline void 
site_stats_decrease(const struct site_file *file, struct site *site) {
    switch (file->diff) {
    case file_unchanged:
	site->numunchanged--;
	break;
    case file_changed:
	if (file->ignore) {
	    site->numignored--;
	} else {
	    site->numchanged--;
	    site->totalchanged -= file->local.size;
	}
	break;
    case file_new:
	site->numnew--;
	site->totalnew -= file->local.size;
	break;
    case file_moved:
	site->nummoved--;
	break;
    case file_deleted:
	site->numdeleted--;
	break;
    default:
	/* Do nothing */
	break;
    }
}

/* Returns the difference between the local and stored state of the
 * given file in the given state.
 * Returns:
 *   file_changed    if changed
 *   file_unchanged  otherwise
 */
enum file_diff inline static
file_compare(const enum file_type type, 
	     const struct file_state *first, const struct file_state *second, 
	     const struct site *site) {

    /* Handle the special cases */
    if (!first->exists)
	return file_deleted;
    if (!second->exists)
	return file_new;

    /* They're both there... compare them properly */
    switch (type) {
    case file_dir:
        if (site->dirperms) {
            return (first->mode != second->mode ?
                    file_changed : file_unchanged);
        }
	break;

    case file_link:
	if (strcmp(first->linktarget, second->linktarget) != 0) {
	    return file_changed;
	}
	break;

    case file_file:
	switch (site->state_method) {
	case state_timesize:
	    if ((first->time != second->time) 
		|| (first->size!=second->size)) {
		return file_changed;
	    }
	    break;
	case state_checksum:
	    if (memcmp(first->checksum, second->checksum, 16) != 0) {
		return file_changed;
	    }
	    break;
	}
	/* Check permissions and ASCIIness.
	 * There's a twist for permissions: if EITHER local or
	 * remote file has an EXEC bit set, then in 'perms exec' mode,
	 * perms are compared. */
	if (first->ascii != second->ascii) {
	    return file_changed;
	} else if (((site->perms == sitep_all) ||
		    (((first->mode & S_IXUSR) || (second->mode & S_IXUSR)) &&
		     (site->perms == sitep_exec)))
		   && (first->mode != second->mode)) {
	    return file_changed;
	}
	if (site->checkmoved && strcmp(first->filename, second->filename)) {
	    return file_moved;
	}
	break;
    }
    return file_unchanged;
}

/* Places the checksum of the given filename in the given file state.
 * The checksum algorithm may in the future be determined from the
 * site. Returns 0 on success or -1 on error.
 */
int file_checksum(const char *fname, struct file_state *state, struct site *s);

/* Returns whether a file of given name is exclude from the given site */
int file_isexcluded(const char *filename, struct site *site);
/* Returns whether a file is ASCII text or binary */
int file_isascii(const char *filename, struct site *site);
/* Returns whether a file is ignored or not */
int file_isignored(const char *filename, struct site *site);

#endif /* SITESI_H */
