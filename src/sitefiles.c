/* 
   sitecopy, for managing remote web sites. File handling.
   Copyright (C) 1998-2006, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

/* Needed for S_IXUSR */
#include <sys/stat.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <ctype.h>

#include "basename.h"

#include <ne_md5.h>
#include <ne_string.h>
#include <ne_alloc.h>

/* We pick up FNM_LEADING_DIR fnmatch() extension, since we define
 * _GNU_SOURCE in config.h. */
#include <fnmatch.h>

#include "frontend.h"
#include "sitesi.h"
					     
/* fnmatch the filename against list */
inline int fnlist_match(const char *filename, const struct fnlist *list);

/* Deletes the given file from the given site */
void file_delete(struct site *site, struct site_file *item) 
{
    site_stats_decrease(item, site);
    site_stats_update(site);
    if (item->prev) {
	/* Not first in list */
	item->prev->next = item->next;
    } else {
	/* Not last in list */
	site->files = item->next;
    }
    if (item->next) {
	/* Not last in list */
	item->next->prev = item->prev;
    } else {
	/* Last in list */
	site->files_tail = item->prev;
    }

    /* Now really destroy the file */
    file_state_destroy(&item->local);
    file_state_destroy(&item->stored);
    file_state_destroy(&item->server);
    free(item);
}


/* Inserts a file into the files list, position chosen by type.  Must
 * be in critical section on calling. file_set_* ensure this. */
static struct site_file *file_insert(enum file_type type, struct site *site) 
{
    struct site_file *file;
    file = ne_calloc(sizeof(struct site_file));
    if (site->files == NULL) {
        /* Empty list */
        site->files = file;
        site->files_tail = file;
    } else if (type == file_dir) {
        /* Append file */
        site->files_tail->next = file;
        file->prev = site->files_tail;
        site->files_tail = file;
    } else {
        /* Prepend file */
        site->files->prev = file;
        file->next = site->files;
        site->files = file;
    }
    return file;
}

#define FS_ALPHA(f) ((struct file_state *) (((char *)(f)) + alpha_off))
#define FS_BETA(f) ((struct file_state *) (((char *)(f)) + beta_off))

/* file_set implementation, used to update the files list.
 *
 * file_set takes a file type 'type', a file state 'state', the site,
 * two structure offsets, alpha_off and beta_off, and a default diff
 * type, 'default_diff'.  alpha_off represents the offset into a
 * site_file structure for the state which 'state' represents;
 * beta_off represents the offset into the structure for the state
 * against which this state should be compared.
 */
static struct site_file *file_set(enum file_type type, struct file_state *state, 
                                  struct site *site,
                                  size_t alpha_off, size_t beta_off,
                                  enum file_diff default_diff)
{
    struct site_file *file, *direct = NULL, *moved = NULL, *frename = NULL;
    enum file_diff dir_diff;
    char *bname = NULL; /* init to shut up gcc */

    if (site->checkmoved && type == file_file) {
        bname = base_name(state->filename);
    }

    for (file = site->files; file; file = file->next) {
        struct file_state *beta = FS_BETA(file);

        if (beta->exists && direct == NULL
            && file->type == type
            && strcmp(beta->filename, state->filename) == 0) {
            /* Direct match found! */
            NE_DEBUG(DEBUG_FILES, "Direct match found.\n");
            direct = file;
        } 
        /* If this is not a direct match, check for a move/rename candidate,
         * unless the file already has a complete state and diff is unchanged. */
        else if (site->checkmoved 
                 && type == file_file && file->type == file_file
                 && file->diff != file_unchanged
                 && file_compare(file_file, state, 
                                 beta, site) == file_moved) {
            /* TODO: There is a slight fuzz here - if checkrenames is true, 
             * we'll always match the first 'direct move' candidate as a 
             * 'rename move'. This shouldn't matter, since we prefer
             * the move to the rename in the single candidate case,
             * and in the multiple candidate case. */
            if (!moved 
                && strcmp(bname, base_name(beta->filename)) == 0) {
                NE_DEBUG(DEBUG_FILES, "Move candidate: %s\n", 
                         beta->filename);
                moved = file;
            } else if (site->checkrenames && frename == NULL) {
                NE_DEBUG(DEBUG_FILES, "Rename move candidate: %s\n", 
                         beta->filename);
                frename = file;
            }
        }

        /* If all candidates are found, stop looking. */
        if (direct && moved && frename) {
            break;
        }
    }
    NE_DEBUG(DEBUG_FILES, "Found: %s-%s-%s\n", 
          direct?"direct":"", moved?"moved":"", frename?"rename":"");
    /* We prefer a direct move to a rename */
    if (moved == NULL) moved = frename;
    if (direct != NULL) {
        dir_diff = file_compare(type, state, FS_BETA(direct), site);
        NE_DEBUG(DEBUG_FILES, "Direct compare: %s\n", 
              DEBUG_GIVE_DIFF(dir_diff));
    } else {
        dir_diff = default_diff;
    }

    /* We prefer a move to a CHANGED direct match. */
    if ((direct == NULL && moved == NULL)
        || (direct != NULL && direct->diff == file_moved
            && moved == NULL && dir_diff != file_unchanged)) {
        NE_DEBUG(DEBUG_FILES, "Creating new file.\n");
        file = file_insert(type, site);
        file->type = type;
        file->diff = default_diff;
        if (type == file_file) {
            file->ignore = file_isignored(state->filename, site);
        }
    } else {
        /* Overwrite file case...
         * Again, we still prefer a move to a direct match */
        if (moved != NULL && dir_diff != file_unchanged) {
            NE_DEBUG(DEBUG_FILES, "Using moved file.\n");
            file = moved;
            site_stats_decrease(file, site);
            file->diff = file_moved;
        } else {
            NE_DEBUG(DEBUG_FILES, "Using direct match.\n");
            file = direct;
            site_stats_decrease(file, site);
            file->diff = dir_diff;
        }

        if (FS_ALPHA(file)->exists) {
            /* SHOVE! */
            struct site_file *other;
            NE_DEBUG(DEBUG_FILES, "Shoving file:\n");
            other = file_insert(file->type, site);
            other->type = file->type;
            other->diff = default_diff;
            other->ignore = file->ignore;
            /* Copy over the stored state for the moved file. */
            memcpy(FS_ALPHA(other), FS_ALPHA(file), sizeof(struct file_state));
            DEBUG_DUMP_FILE_PROPS(DEBUG_FILES, file, site);
            site_stats_increase(other, site);
        }
    }

    /* Finish up - write over the new state */
    memcpy(FS_ALPHA(file), state, sizeof(struct file_state));

    /* And update the stats */
    site_stats_increase(file, site);
    site_stats_update(site);

    return file;
}

#ifndef offsetof
#define offsetof(t, m) ((size_t) (((char *)&(((t *)NULL)->m)) - (char *)NULL))
#endif

struct site_file *file_set_local(enum file_type type, struct file_state *state, 
                                 struct site *site)
{
    return file_set(type, state, site,
                    offsetof(struct site_file, local),
                    offsetof(struct site_file, stored),
                    file_new);
}

struct site_file *file_set_stored(enum file_type type, struct file_state *state, 
                                  struct site *site)
{
    return file_set(type, state, site,
                    offsetof(struct site_file, stored),
                    offsetof(struct site_file, local),
                    file_deleted);
}

/* Prepends  an item to the fnlist. Returns the item. */
struct fnlist *fnlist_prepend(struct fnlist **list)
{
    struct fnlist *item = ne_malloc(sizeof(struct fnlist));
    item->next = *list;
    item->prev = NULL;
    if (*list != NULL) {
	(*list)->prev = item;
    }
    *list = item;
    return item;
}

/* Returns a deep copy of the given fnlist */
struct fnlist *fnlist_deep_copy(const struct fnlist *src)
{
    const struct fnlist *iter;
    struct fnlist *dest = NULL, *prev = NULL, *item = NULL;
    for (iter = src; iter != NULL; iter = iter->next) {
	item = ne_malloc(sizeof(struct fnlist));
	item->pattern = ne_strdup(iter->pattern);
	item->haspath = iter->haspath;
	if (prev != NULL) {
	    prev->next = item;
	} else {
	    /* First item in list */
	    dest = item;
	}
	item->prev = prev;
	item->next = NULL;
	prev = item;
    }
    return dest;
}

/* Performs fnmatch() of all the strings in the given string list again
 * the given filename. Returns true if a pattern matches, else false. */
inline int fnlist_match(const char *filename, const struct fnlist *list)
{
    const struct fnlist *item;
    const char *bname = base_name(filename);    

    for (item=list; item != NULL; item=item->next) {
	NE_DEBUG(DEBUG_FILES, "%s ", item->pattern);
	if (item->haspath) {
	    if (fnmatch(item->pattern, filename, 
			 FNM_PATHNAME | FNM_LEADING_DIR) == 0)
		break;
	} else {
	    if (fnmatch(item->pattern, bname, 0) == 0)
		break;
	}
    }
	
#ifdef DEBUGGING
    if (item) {
	NE_DEBUG(DEBUG_FILES, "- matched.\n");
    } else if (list) {
	NE_DEBUG(DEBUG_FILES, "\n");
    } else {
	NE_DEBUG(DEBUG_FILES, "(none)\n");
    }
#endif /* DEBUGGING */

    return (item!=NULL);
}

/* Returns whether the given filename is excluded from the
 * given site */
int file_isexcluded(const char *filename, struct site *site)
{
    NE_DEBUG(DEBUG_FILES, "Matching excludes for %s:\n", filename);
    return fnlist_match(filename, site->excludes);
}

int file_isignored(const char *filename, struct site *site)
{
    NE_DEBUG(DEBUG_FILES, "Matching ignores for %s:\n", filename);
    return fnlist_match(filename, site->ignores);
}

int file_isascii(const char *filename, struct site *site)
{
    NE_DEBUG(DEBUG_FILES, "Matching asciis for %s:\n", filename);
    return fnlist_match(filename, site->asciis);
}

void site_stats_update(struct site *site)
{
    NE_DEBUG(DEBUG_FILES, 
	  "Stats: moved=%d new=%d %sdeleted=%d%s changed=%d"
	  " ignored=%d unchanged=%d\n",
	  site->nummoved, site->numnew, site->nodelete?"[":"",
	  site->numdeleted, site->nodelete?"]":"", site->numchanged,
	  site->numignored, site->numunchanged);
    site->remote_is_different = (site->nummoved + site->numnew +
				 (site->nodelete?0:site->numdeleted) + 
				 site->numchanged) > 0;
    site->local_is_different = (site->nummoved + site->numnew +
				site->numdeleted + site->numchanged + 
				site->numignored) > 0;
    NE_DEBUG(DEBUG_FILES, "Remote: %s  Local: %s\n", 
	  site->remote_is_different?"yes":"no", 
	  site->local_is_different?"yes":"no");
}

void file_set_diff(struct site_file *file, struct site *site) 
{
    site_stats_decrease(file, site);
    file->diff = file_compare(file->type, &file->local, &file->stored, site);
    site_stats_increase(file, site);
    site_stats_update(site);
}

void file_state_copy(struct file_state *dest, const struct file_state *src,
                     struct site *site)
{
    file_state_destroy(dest);
    memcpy(dest, src, sizeof(struct file_state));
    if (src->linktarget != NULL) {
	dest->linktarget = ne_strdup(src->linktarget);
    }
    if (src->filename != NULL) {
	dest->filename = ne_strdup(src->filename);
    }
}

void file_state_destroy(struct file_state *state)
{
    if (state->linktarget != NULL) {
	free(state->linktarget);
	state->linktarget = NULL;
    }
    if (state->filename != NULL) {
	free(state->filename);
	state->filename = NULL;
    }
}

/* Checksum the file.
 * We pass the site atm, since it's likely we will add different
 * methods of checksumming later on, with better-faster-happier
 * algorithms.
 * Returns:
 *  0 on success
 *  non-zero on error (e.g., couldn't open file)
 */
int file_checksum(const char *fname, struct file_state *state, struct site *s)
{
    int ret;
    FILE *f;
    f = fopen(fname, "r" FOPEN_BINARY_FLAGS);
    if (f == NULL) {
	return -1;
    }
    ret = ne_md5_stream(f, state->checksum);
    fclose(f); /* worth checking return value? */
#ifdef DEBUGGING
    { 
	char tmp[33] = {0};
	ne_md5_to_ascii(state->checksum, tmp);
	NE_DEBUG(DEBUG_FILES, "Checksum: %s = [%32s]\n", fname, tmp);
    }
#endif /* DEBUGGING */
    return ret;
}    

char *file_full_remote(struct file_state *state, struct site *site)
{
    char *ret;
    ret = ne_malloc(strlen(site->remote_root) + strlen(state->filename) + 1);
    strcpy(ret, site->remote_root);
    if (site->lowercase) {
	int n, off, len;
	/* Write the remote filename in lower case */
	off = strlen(site->remote_root);
	len = strlen(state->filename) + 1; /* +1 for \0 */
	for (n = 0; n < len; n++) {
	    ret[off+n] = tolower(state->filename[n]);
	}
    } else {
	strcat(ret, state->filename);
    }
    return ret;
}

char *file_full_local(struct file_state *state, struct site *site)
{
    return ne_concat(site->local_root, state->filename, NULL);
}

char *file_name(const struct site_file *file)
{
    if (file->diff == file_deleted) {
	return file->stored.filename;
    } else {
	return file->local.filename;
    }
}

/* Returns whether the file "contents" have changed or not.
 * TODO: better name needed. */
int file_contents_changed(struct site_file *file, struct site *site)
{
    int ret = false;
    if (site->state_method == state_checksum) {
	if (memcmp(file->stored.checksum, file->local.checksum, 16))
	    ret = true;
    } else {
	if (file->stored.size != file->local.size ||
	    file->stored.time != file->local.time) 
	    ret = true;
    }
    if (file->stored.ascii != file->local.ascii)
	ret = true;
    return ret;
}

/* Return true if the permission of the given file need changing.  */
int file_perms_changed(struct site_file *file, struct site *site)
{
    /* Slightly obscure boolean here...
     *  If ('permissions all' OR ('permissions exec' and file is chmod u+x)
     *     AND
     *     EITHER (in tempupload mode or nooverwrite mode)
     *         OR the stored file perms are different from the local ones
     *         OR the file doesn't exist locally or remotely,
     *
     * Note that in tempupload and nooverwrite mode, we are
     * creating a new file, so the permissions on the new file
     * will always be "wrong".
     */
    if (((site->perms == sitep_all) || 
	 (((file->local.mode | file->stored.mode) & S_IXUSR) && 
	  (site->perms == sitep_exec))) &&
	(site->tempupload || site->nooverwrite ||
	 file->local.mode != file->stored.mode || 
	 file->local.exists != file->stored.exists)) {
	return true;
    } else {
	return false;
    }
}

void file_uploaded(struct site_file *file, struct site *site)
{
    file->stored.size = file->local.size;
    if (site->state_method == state_checksum) {
	memcpy(file->stored.checksum, file->local.checksum, 16);
    } else {
	file->stored.time = file->local.time;
    }
    /* Now the filename */
    if (file->stored.filename) free(file->stored.filename);
    file->stored.filename = ne_strdup(file->local.filename);
    file->stored.ascii = file->local.ascii;
    file->stored.exists = file->local.exists;
    file->stored.mode = file->local.mode;
    /* Update the diff */
    file_set_diff(file, site);
}
    
void file_downloaded(struct site_file *file, struct site *site)
{
    file->local.size = file->stored.size;
    if (site->state_method == state_checksum) {
	memcpy(file->local.checksum, file->stored.checksum, 16);
    } else {
	file->local.time = file->stored.time;
    }
    /* Now the filename */
    if (file->local.filename) free(file->local.filename);
    file->local.filename = ne_strdup(file->stored.filename);
    file->local.ascii = file->stored.ascii;
    file->local.exists = file->stored.exists;
    file->local.mode = file->stored.mode;
    file_set_diff(file, site);
}
