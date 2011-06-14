/* 
   sitecopy, for managing remote web sites.
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

/* This is the core functionality of sitecopy, performing updates
 * and checking files etc. */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <time.h>
#include <utime.h>

/* neon */
#include <ne_string.h>
#include <ne_alloc.h>
#include <ne_md5.h>
#include <ne_socket.h>

#ifdef HAVE_SNPRINTF_H
#include "snprintf.h"
#endif /* !HAVE_SNPRINTF_H */

#include "basename.h"

#include "i18n.h"
#include "common.h"
#include "frontend.h"
#include "protocol.h"
#include "sitesi.h"

/* Shorthand for protocol driver methods */
#define CALL(a) (*site->driver->a)
#define DRIVER_ERR  ((*site->driver->error)(session))

/* This holds ALL the sites defined in the rcfile */
struct site *all_sites;
 
static int proto_init(struct site *site, void **session);
static void proto_finish(struct site *site, void *session);
static void proto_seterror(struct site *site, void *session);

struct site *site_find(const char *sitename)
{
    struct site *current;

    for (current = all_sites; current!=NULL; current=current->next) {
	if (strcmp(current->name, sitename) == 0) {
	    /* We found it */
	    return current;
	}
    }

    return NULL;
}

static int synch_create_directories(struct site *site)
{
    struct site_file *current;
    char *full_local;
    int ret;
    
    ret = 0;
    
    for_each_file(current, site) {
	if ((current->type==file_dir) && (current->diff==file_deleted)) {
	    full_local = file_full_local(&current->stored, site);
	    fe_synching(current);
	    if (mkdir(full_local, 0755) == 0) {
		fe_synched(current, true, NULL);
	    } else {
		ret = 1;
		fe_synched(current, false, strerror(errno));
		file_downloaded(current, site);
	    }
	    free(full_local);
	}
    }
    return ret;
}

static int synch_files(struct site *site, void *session)
{
    struct site_file *current;
    int ret;

    ret = 0;

    for_each_file(current, site) {
	char *full_local, *full_remote;
	if (current->type != file_file) continue;
	switch (current->diff) {
	case file_changed:
	    if (!file_contents_changed(current, site)) {
		/* Just chmod it */
		full_local = file_full_local(&current->stored, site);
		fe_setting_perms(current);
		if (chmod(full_local, current->stored.mode) < 0) {
		    fe_set_perms(current, false, strerror(errno));
		} else {
		    fe_set_perms(current, true, NULL);
		}
		free(full_local);
		break;
	    }
	    /*** fall-through */
	case file_deleted:
	    full_local = file_full_local(&current->stored, site);
	    full_remote = file_full_remote(&current->stored, site);
	    fe_synching(current);
	    if (CALL(file_download)(session, full_local, full_remote,
				    current->stored.ascii) != SITE_OK) {
		fe_synched(current, false, DRIVER_ERR);
		ret = 1;
	    } else { 
		/* Successfull download */
		fe_synched(current, true, NULL);
		if (site->state_method == state_timesize) {
		    struct utimbuf times;
		    /* Change the modtime of the local file so it doesn't look
		     * like it's changed already */
		    times.actime = current->stored.time;
		    times.modtime = current->stored.time;
		    if (utime(full_local, &times) < 0) {
			fe_warning(_("Could not set modification time of local file."),
				    full_local, strerror(errno));
		    }
		}
		if (file_perms_changed(current, site)) {
		    fe_setting_perms(current);
		    if (chmod(full_local, current->stored.mode) < 0) {
			fe_set_perms(current, false, strerror(errno));
		    } else {
			fe_set_perms(current, true, NULL);
		    }
		}
		/* TODO: not strictly true if the chmod failed. */
		file_downloaded(current, site);
	    }
	    free(full_local);
	    free(full_remote);
	    break;
	case file_new:
	    full_local = file_full_local(&current->local, site);
	    fe_synching(current);
	    if (unlink(full_local) != 0) {
		fe_synched(current, false, strerror(errno));
		ret = 1;
	    } else {
		fe_synched(current, true, NULL);
	    }
	    free(full_local);
	    break;
	case file_moved: {
	    char *old_full_local = file_full_local(&current->stored, site);
	    full_local = file_full_local(&current->local, site);
	    fe_synching(current);
	    if (rename(full_local, old_full_local) == 0) {
		fe_synched(current, true, NULL);
	    } else {
		fe_synched(current, false, strerror(errno));
		ret = 1;
	    }
	    free(old_full_local);
	    free(full_local);
	}
	default:
	    break;	    
	}
    }

    return ret;
}

static int synch_delete_directories(struct site *site)
{
    struct site_file *current, *prev;
    int ret;

    ret = 0;

    for (current=site->files_tail; current!=NULL; current=prev) {
	prev = current->prev;
	if ((current->type==file_dir) && (current->diff==file_new)) {
	    char *full_local = file_full_local(&current->local, site);
	    fe_synching(current);
	    if (rmdir(full_local) == -1) {
		fe_synched(current, false, strerror(errno));
		ret = 3;
	    } else {
		fe_synched(current, true, NULL);
		file_delete(site, current);
	    }
	    free(full_local);
	}
    }

    return ret;
}

/* Resyncs the LOCAL site with the REMOTE site.
 * This is site_update backwards, and is essentially the same in structure,
 * except with the logic reversed.
 */
int site_synch(struct site *site) 
{
    int ret, need_conn;
    void *session;
 
    /* Do we need to connect to the server: note that ignored files
     * are treated as changed files in synch mode. */
    need_conn = (site->numchanged + site->numdeleted + 
		 site->numignored > 0);
   
    if (need_conn) {
	ret = proto_init(site, &session);
	if (ret != SITE_OK) {
	    proto_finish(site, session);
	    return ret;
	}
    }

    ret = synch_create_directories(site);
    if (ret == 0 || site->keep_going) {
	ret = synch_files(site, session);
	if (ret == 0 || site->keep_going) {
	    ret = synch_delete_directories(site);
	}
    }
    
    if (need_conn) {
	proto_finish(site, session);
    }

    if (ret == 0) {
	ret = SITE_OK;
    } else {
	ret = SITE_ERRORS;
    }
    return ret;
}

static int file_chmod(struct site_file *file, struct site *site, void *session)
{
    int ret = 0;
    /* chmod it if necessary */
    if (file_perms_changed(file, site)) {
	char *full_remote = file_full_remote(&file->local, site);
	fe_setting_perms(file);
	if (CALL(file_chmod)(session, full_remote, file->local.mode) != SITE_OK) {
	    fe_set_perms(file, false, DRIVER_ERR);
	    ret = 1;
	} else {
	    file->stored.mode = file->local.mode;
	    fe_set_perms(file, true, NULL);
	    file_set_diff(file, site);
	}
	free(full_remote);
    }
    return ret;
}

static void 
file_retrieve_server(struct site_file *file, struct site *site, void *session)
{
    time_t rtime;
    char *full_remote = file_full_remote(&file->local, site);
    if (CALL(file_get_modtime)(session, full_remote, &rtime) == SITE_OK) {
	file->server.time = rtime;
	file->server.exists = true;
    } else {
	file->server.exists = false;
	fe_warning(_("Upload succeeded, but could not retrieve modification time.\n"
		      "If this message persists, turn off safe mode."),
		    full_remote, DRIVER_ERR);
    }
    free(full_remote);
}

/* Create new directories and change permissions on existing directories. */
static int update_create_directories(struct site *site, void *session)
{
    struct site_file *current;
    int ret = 0;

    for_each_file(current, site) {
	if ((current->type == file_dir) 
            && (current->diff == file_new || current->diff == file_changed)) {
	    /* New or changed directory! */
	    char *full_remote;
            int oret;

	    if (!fe_can_update(current)) continue;

	    full_remote = file_full_remote(&current->local, site);

            if (current->diff == file_new) {
                fe_updating(current);
                oret = CALL(dir_create)(session, full_remote);
                if (oret != SITE_OK) {
                    fe_updated(current, false, DRIVER_ERR);
                } else {
                    fe_updated(current, true, NULL);
                }
            } else {
                oret = SITE_OK;
            }

            if (site->dirperms && oret == SITE_OK) {
                fe_setting_perms(current);
                oret = CALL(file_chmod)(session, full_remote,
                                        current->local.mode);
                if (oret == SITE_OK) {
                    fe_set_perms(current, true, NULL);
                } else {
                    fe_set_perms(current, false, DRIVER_ERR);
                }
            }

            if (oret != SITE_OK) {
                ret = 1;
            } else {
                file_uploaded(current, site);
            }
	    free(full_remote);
	}
    }

    return ret;
}

/* Returns the filename to use for tempupload mode, ne_malloc-allocated.
 * (pass the site since we may have different tempupload modes in the
 * future.)
 * FIXME: implement it efficiently */
static char *temp_upload_filename(const char *filename, struct site *site)
{
    char *pnt, *ret;
    /* Insert a '.in.' prefix into the filename, AFTER
     * any directories */
    ret = ne_malloc(strlen(filename) + 4 + 1);
    strcpy(ret, filename);
    pnt = strrchr(ret, '/');
    if (pnt == NULL) {
	pnt = ret;
    } else {
	pnt++;
    }
    /* Shove the name segment along four bytes so we can insert
     * the '.in.' */
    memmove(pnt+4, pnt, strlen(pnt) + 1);
    memcpy(pnt, ".in.", 4);
    return ret;
}
		
static int update_delete_files(struct site *site, void *session)
{
    struct site_file *current, *next;
    int ret = 0;

    for (current=site->files; current!=NULL; current=next) {
	next = current->next;
	/* Skip directories and links, and only do deleted files on
	 * this pass */
	if (current->diff == file_deleted &&
	    current->type == file_file) {
	    char *full_remote;
	    if (!fe_can_update(current)) continue;
	    full_remote = file_full_remote(&current->stored, site);
	    fe_updating(current);
	    if (CALL(file_delete)(session, full_remote) != SITE_OK) {
		fe_updated(current, false, DRIVER_ERR);
		ret = 1;
	    } else {
		/* Successful update - file was deleted */
		fe_updated(current, true, NULL);
		file_delete(site, current);
	    }
	    free(full_remote);
	}
    }
    
    return ret;
}

static int update_move_files(struct site *site, void *session)
{
    int ret = 0;
    struct site_file *current;
    char *old_full_remote, *full_remote;
    for_each_file(current, site) {
	if (current->diff != file_moved) 
	    continue;
	full_remote = file_full_remote(&current->local, site);
	/* The file has been moved */
	if (!fe_can_update(current)) continue;
	fe_updating(current);
	old_full_remote = file_full_remote(&current->stored, site);
	if (CALL(file_move)(session, old_full_remote, full_remote) != SITE_OK) {
	    ret = 1;
	    fe_updated(current, false, DRIVER_ERR);
	} else {
	    /* Successful update - file was moved */
	    fe_updated(current, true, NULL);
	    file_uploaded(current, site);
	}
	free(old_full_remote);
	free(full_remote);
    }   

    return ret;
}


/* Does everything but file deletes */
static int update_files(struct site *site, void *session)
{
    struct site_file *current;
    char *full_local, *full_remote;
    int ret = 0;

    for_each_file(current, site) {

	/* This loop only handles changed and new files, so
	 * skip everything else. */

	if (current->type != file_file
	    || current->diff == file_deleted
	    || current->diff == file_moved
	    || current->diff == file_unchanged) continue;

	full_local = file_full_local(&current->local, site);
	full_remote = file_full_remote(&current->local, site);

	switch (current->diff) {
	case file_changed: /* File has changed, upload it */
	    if (current->ignore) break;
	    if (!file_contents_changed(current, site)) {
		/* If the file contents haven't changed, then we can
		 * just chmod it */
		if (file_chmod(current, site, session))
		    ret = 1;
		break;
	    }
	    /*** fall-through ***/
	case file_new: /* File is new, upload it */
	    if (!fe_can_update(current)) continue;
	    if ((current->diff == file_changed) && site->nooverwrite) {
		/* Must delete remote file before uploading new copy.
		 * FIXME: Icky hack to convince the FE we are about to
		 * delete the file */
		current->diff = file_deleted;
		fe_updating(current);
		if (CALL(file_delete)(session, full_remote) != SITE_OK) {
		    fe_updated(current, false, DRIVER_ERR);
		    ret = 1;
		    current->diff = file_changed;
		    /* Don't upload it! */
		    break;
		} else {
		    fe_updated(current, true, NULL);
		    current->diff = file_changed;
		}
	    }
	    fe_updating(current);
	    /* Now, upload it */
	    if (site->safemode && current->server.exists) {
		/* Only do this for files we do know the remote modtime for */
		int cret;
		cret = CALL(file_upload_cond)(session,
		    full_local, full_remote, current->local.ascii,
		    current->server.time);
		switch (cret) {
		case SITE_ERRORS:
		    fe_updated(current, false, DRIVER_ERR);
		    ret = 1;
		    break;
		case SITE_FAILED:
		    fe_updated(current, false, 
				_("Remote file has been modified - not overwriting with local changes"));
		    ret = 1;
		    break;
		default:
		    /* Success case */
		    fe_updated(current, true, NULL);
		    file_retrieve_server(current, site, session);
		    if (file_chmod(current, site, session)) ret = 1;
		    file_uploaded(current, site);
		    break;
		}
	    } else if (site->tempupload) {
		/* Do temp file upload followed by a move */
		char *temp_remote = temp_upload_filename(full_remote, site);
		if (CALL(file_upload)(session, full_local, temp_remote,
				       current->local.ascii != SITE_OK)) {
		    fe_updated(current, false, DRIVER_ERR);
		    ret = 1;
		} else {
		    /* Successful upload... now move it */
		    if (CALL(file_move)(session, temp_remote, 
					 full_remote) != SITE_OK) {
			fe_updated(current, false, DRIVER_ERR);
			/* Originally coded to delete the temporary file
			 * here, but, on second thoughts... if something
			 * is broken, let's not try to be too clever, else
			 * we might make it worse. */
			ret = 1;
		    } else {
			/* Successful move */
			fe_updated(current, true, NULL);
			if (site->safemode) {
			    file_retrieve_server(current, site, session);
			}
			if (file_chmod(current, site, session)) ret = 1;
			file_uploaded(current, site);
		    }
		}
		free(temp_remote);
	    } else {
		/* Normal unconditional upload */
		if (CALL(file_upload)(session, full_local, full_remote, 
				       current->local.ascii) != SITE_OK) {
		    fe_updated(current, false, DRIVER_ERR);
		    ret = 1;
		} else {
		    /* Successful upload. */
		    fe_updated(current, true, NULL);
		    if (site->safemode) {
			file_retrieve_server(current, site, session);
		    }
		    if (file_chmod(current, site, session)) ret = 1;
		    file_uploaded(current, site);
		}
	    }
	    break;
		
	default: /* Ignore everything else */
	    break;
	}
	free(full_remote);
	free(full_local);
    }

    return ret;
    
}

static int update_delete_directories(struct site *site, void *session)
{
    struct site_file *current, *prev;
    int ret = 0;

    /* This one must iterate through the list BACKWARDS, so
     * directories are deleted bottom up */
    for (current=site->files_tail; current!=NULL; current=prev) {
	prev = current->prev;
	if ((current->type==file_dir) && (current->diff == file_deleted)) {
	    char *full_remote;
	    if (!fe_can_update(current)) continue;
	    full_remote = file_full_remote(&current->stored, site);
	    fe_updating(current);
	    if (CALL(dir_remove)(session, full_remote) != SITE_OK) {
		ret = 1;
		fe_updated(current, false, DRIVER_ERR);
	    } else {
		/* Successful delete */
		fe_updated(current, true, NULL);
		file_delete(site, current);
	    }
	    free(full_remote);
	}
    }
    return ret;
}


/*
*
*/
static int update_links(struct site *site, void *session)
{
    struct site_file *current, *next;
    int ret = 0;

    for (current=site->files; current!=NULL; current=next) {
	char *full_remote;
	next = current->next;
	if (current->type != file_link) continue;

	full_remote = file_full_remote(&current->local, site);
	switch (current->diff) {
	case file_new:
	    fe_updating(current);
	    if (CALL(link_create)(session, full_remote, 
				  current->local.linktarget) != SITE_OK) {
		fe_updated(current, false, DRIVER_ERR);
		ret = 1;
	    } else {
		fe_updated(current, true, NULL);
		current->diff = file_unchanged;
	    }
	    break;
	case file_changed:
	    fe_updating(current);
	    if (CALL(link_change)(session, full_remote,
				   current->local.linktarget) != SITE_OK) {
		fe_updated(current, false, DRIVER_ERR);
		ret = 1;
	    } else {
		fe_updated(current, true, NULL);
		current->diff = file_unchanged;
	    }
	    break;
	case file_deleted:
	    fe_updating(current);
	    if (CALL(link_delete)(session, full_remote) != SITE_OK) {
		fe_updated(current, false, DRIVER_ERR);
		ret = 1;
	    } else {
		fe_updated(current, true, NULL);
		file_delete(site, current);
	    }
	default:
		break;
	}
	free(full_remote);
    }
    return ret;
}

static void proto_finish(struct site *site, void *session)
{
    proto_seterror(site, session);
    CALL(finish)(session);
}

static void proto_seterror(struct site *site, void *session)
{
    site->last_error = ne_strdup(DRIVER_ERR);
}

const char *site_get_protoname(struct site *site) 
{
    if (site->driver)
	return site->driver->protocol_name;
    else
	return site->proto_string;
}

static int proto_init(struct site *site, void **session)
{
    int ret;
    
    if (site->last_error) {
	free(site->last_error);
	site->last_error = NULL;
    }

    ret = CALL(init)(session, site);
    if (ret != SITE_OK) {
	proto_seterror(site, *session);
	return ret;
    }

    return SITE_OK;
}


/* Updates the remote site.
 * 
 * Executes each of the site_update_* functions in turn (if their
 * guard evaluates to true). 
 */
int site_update(struct site *site)
{
    int ret = 0, num;
    const struct handler {
	int (*func)(struct site *, void *session);
	int guard;
    } handlers[] = {
	{ update_delete_files, !site->nodelete },
	{ update_create_directories, 1 },
	{ update_move_files, site->checkmoved },
	{ update_files, 1 },
	{ update_links, site->symlinks == sitesym_maintain },
	{ update_delete_directories, !site->nodelete },
	{ NULL, 1 }
    };
    void *session;

    ret = proto_init(site, &session);
    if (ret != SITE_OK) {
	proto_finish(site, session);
	return ret;
    }
    
    for (num = 0; handlers[num].func != NULL && (ret == 0 || site->keep_going);
	 num++) {
	if (handlers[num].guard) {
	    int newret;
	    newret = (*handlers[num].func)(site, session);
	    if (newret != 0) {
		ret = newret;
	    }
	}
    }

    if (ret == 0) {
	/* Site updated successfully. */
	ret = SITE_OK;
    } else {
	/* Update not totally successfull */
	ret = SITE_ERRORS;
    }
    
    proto_finish(site, session);

    return ret;
}




/* Read the local site files... 
 * A stack is used for directories within the site - this is not recursive.
 * Each item on the stack is a FULL PATH to the directory, i.e., including
 * the local site root. */
void site_read_local_dirs(struct site *site)
{

/* Initial size of directory stack, and amount it grows
 * each time we fill it. */
#define DIRSTACKSIZE 128

//TODO OS2 aka EMX port
    char **dirstack, *this, *full = NULL;
    int dirtop = 0, /* points to item above top stack item */
    dirmax = DIRSTACKSIZE; /* size of stack */

    dirstack = ne_malloc(sizeof(char *) * DIRSTACKSIZE);
    /* Push the root directory on to the stack */
    dirstack[dirtop++] = ne_strdup(site->local_root);
       printf("Adding %s to dirstack.\n", site->local_root);
  /* Now, for all items in the stack, process all the files, and
     * add the dirs to the stack. Everything we put on the stack is
     * temporary and gets freed eventually. */

   while (dirtop > 0) {
      DIR *curdir;
      struct dirent *ent;
      /* Pop the stack */
      this = dirstack[--dirtop];
      
      NE_DEBUG(DEBUG_FILES, "Scanning: %s\n", this);
      curdir = opendir(this);
      if (curdir == NULL) {
          fe_warning("Could not read directory", this, strerror(errno));
          free(this);
          continue;
      }
      
      /* Now read all the directory entries */
      while ((ent = readdir(curdir)) != NULL) {
         char *fname;
         struct stat item;
         struct site_file *current;
         enum file_type type;
         size_t dnlen = strlen(ent->d_name);
    
         /* Exclude the special directory entries. This test comes
           * high since it kills two stat calls per directory. */
         if (ent->d_name[0] == '.' && 
             (dnlen == 1 || (ent->d_name[1] == '.' && dnlen==2))) {
            continue;
         }
          
         if (full != NULL) free(full);
    
         full = ne_concat(this, ent->d_name, NULL);
    
    #define USE_STAT lstat
    
         if (USE_STAT(full, &item) == -1) {
         fe_warning(_("Could not examine file."), full, strerror(errno));
         continue;
         }
    #undef USE_STAT
    
          
          /* This is the filename of this file - i.e., everything
            * apart from the local root */
         fname = (char *)full+strlen(site->local_root);
          
          /* Check for excludes */
         if (!file_isexcluded(fname, site) && S_ISDIR(item.st_mode)) {
            type = file_dir;
             if (dirtop == dirmax) {
              /* Grow the stack */
              dirmax += DIRSTACKSIZE;
              dirstack = realloc(dirstack, sizeof(char *) * dirmax);
             }
             /* Add it to the search stack */
             printf("Adding %s to dirstack.\n", ne_concat(full, "/", NULL));
             dirstack[dirtop] = ne_concat(full, "/", NULL);
             dirtop++;
         }  //check excludes
    
      }  //while
      /* Close the open directory */
      closedir(curdir);
      /* And we're finished with this */
      free(this);
    }

    free(dirstack);
}




/* Updates the remote site.
 * 
 * Executes each of the site_update_* functions in turn (if their
 * guard evaluates to true). 
 */
int site_autoupdate(struct site *site)
{
    int ret = 0, num;
    const struct handler {
   int (*func)(struct site *, void *session);
   int guard;
    } handlers[] = {
   { update_delete_files, !site->nodelete },
   { update_create_directories, 1 },
   { update_move_files, site->checkmoved },
   { update_files, 1 },
   { update_links, site->symlinks == sitesym_maintain },
   { update_delete_directories, !site->nodelete },
   { NULL, 1 }
    };
    void *session;

    ret = proto_init(site, &session);
     site_read_local_dirs(site);

    return ret;
}




/* This reads off the remote files and the local files. */
int site_readfiles(struct site *site)
{
    int ret;
    site_destroy(site);
    ret = site_read_stored_state(site);
    if (ret == SITE_OK) {
	site_read_local_state(site);
    }
    return ret;
}


/* Initial size of directory stack, and amount it grows
 * each time we fill it. */
#define DIRSTACKSIZE 128

/* Read the local site files... 
 * A stack is used for directories within the site - this is not recursive.
 * Each item on the stack is a FULL PATH to the directory, i.e., including
 * the local site root. */
void site_read_local_state(struct site *site)
{
    char **dirstack, *this, *full = NULL;
    int dirtop = 0, /* points to item above top stack item */
	dirmax = DIRSTACKSIZE; /* size of stack */

    dirstack = ne_malloc(sizeof(char *) * DIRSTACKSIZE);
    /* Push the root directory on to the stack */
    dirstack[dirtop++] = ne_strdup(site->local_root);
    
    /* Now, for all items in the stack, process all the files, and
     * add the dirs to the stack. Everything we put on the stack is
     * temporary and gets freed eventually. */

    while (dirtop > 0) {
	DIR *curdir;
	struct dirent *ent;
	/* Pop the stack */
	this = dirstack[--dirtop];
	
	NE_DEBUG(DEBUG_FILES, "Scanning: %s\n", this);
	curdir = opendir(this);
	if (curdir == NULL) {
	    fe_warning("Could not read directory", this, strerror(errno));
	    free(this);
	    continue;
	}
	
	/* Now read all the directory entries */
	while ((ent = readdir(curdir)) != NULL) {
	    char *fname;
	    struct stat item;
	    struct site_file *current;
	    struct file_state local = {0};
	    enum file_type type;
	    size_t dnlen = strlen(ent->d_name);

	    /* Exclude the special directory entries. This test comes
	     * high since it kills two stat calls per directory. */
	    if (ent->d_name[0] == '.' && 
		(dnlen == 1 || (ent->d_name[1] == '.' && dnlen==2))) {
		continue;
	    }
	    
	    if (full != NULL) free(full);

	    full = ne_concat(this, ent->d_name, NULL);

#ifdef __EMX__
/* There are no symlinks under OS/2, use stat() instead */
#define USE_STAT stat
#else 
#define USE_STAT lstat
#endif
 	    if (USE_STAT(full, &item) == -1) {
		fe_warning(_("Could not examine file."), full, strerror(errno));
		continue;
	    }
#undef USE_STAT

#ifndef __EMX__
	    /* Is this a symlink? */
	    if (S_ISLNK(item.st_mode)) {
		NE_DEBUG(DEBUG_FILES, "symlink - ");
		if (site->symlinks == sitesym_ignore) {
		    /* Just skip it */
		    NE_DEBUG(DEBUG_FILES, "ignoring.\n");
		    continue;
		} else if (site->symlinks == sitesym_follow) {
		    NE_DEBUG(DEBUG_FILES, "followed - ");
		    /* Else, carry on as normal, stat the real file */
		    if (stat(full, &item) == -1) {
			/* It's probably a broken link */
			NE_DEBUG(DEBUG_FILES, "broken.\n");
			continue;
		    }
		} else {
		    NE_DEBUG(DEBUG_FILES, "maintained:\n");
		}
	    }
#endif /* __EMX__ */
	    /* Now process it */
	    
	    /* This is the filename of this file - i.e., everything
	     * apart from the local root */
	    fname = (char *)full+strlen(site->local_root);
	    
	    /* Check for excludes */
	    if (file_isexcluded(fname, site))
		continue;
	    
	    if (S_ISREG(item.st_mode)) {
		switch (site->state_method) {
		case state_timesize:
		    local.time = item.st_mtime;
		    break;
		case state_checksum:
		    if (file_checksum(full, &local, site) != 0) {
			fe_warning(_("Could not checksum file"), full,
				    strerror(errno));
			continue;
		    }
		    break;
		}
		local.size = item.st_size;
		local.ascii = file_isascii(fname, site);
		type = file_file;
	    }
#ifndef __EMX__
	    else if (S_ISLNK(item.st_mode)) {
		char tmp[BUFSIZ] = {0};
		type = file_link;
		NE_DEBUG(DEBUG_FILES, "symlink being maintained.\n");
		if (readlink(full, tmp, BUFSIZ) == -1) {
		    fe_warning(_("The target of the symlink could not be read."), full, strerror(errno));
		    continue;
		}
		local.linktarget = ne_strdup(tmp);
	    }
#endif /* __EMX__ */
	    else if (S_ISDIR(item.st_mode)) {
		type = file_dir;
		if (dirtop == dirmax) {
		    /* Grow the stack */
		    dirmax += DIRSTACKSIZE;
		    dirstack = realloc(dirstack, sizeof(char *) * dirmax);
		}
		/* Add it to the search stack */
		dirstack[dirtop] = ne_concat(full, "/", NULL);
		dirtop++;
	    } else {
		NE_DEBUG(DEBUG_FILES, "something else.\n");
		continue;
	    }
	    
	    /* Set up rest of the local state */
	    local.mode = item.st_mode & 0777;
	    local.exists = true;
	    local.filename = ne_strdup(fname);

	    current = file_set_local(type, &local, site);
	    DEBUG_DUMP_FILE_PROPS(DEBUG_FILES, current, site);

	}
	/* Close the open directory */
	closedir(curdir);
	/* And we're finished with this */
	free(this);
    }

    free(dirstack);
}

/* Pretend the remote site is the same as the local site. */
void site_catchup(struct site *site)
{
    struct site_file *current, *next;
    for (current=site->files; current!=NULL; current=next) {
	next = current->next;
	switch (current->diff) {
	case file_deleted:
	    file_delete(site, current);
	    break;
	case file_changed:
	case file_new:
	case file_moved:
	    file_state_copy(&current->stored, &current->local, site);
	    file_set_diff(current, site);
	    break;
	case file_unchanged:
	    /* noop */
	    break;
	}
    }
}

/* Reinitializes the site - clears any remote files
 * from the list, and marks all other files as 'new locally'.
 */
void site_initialize(struct site *site)
{
    /* So simple. Be sure we have our abstraction layers at least
     * half-decent when things fall out this simple. */
    site_destroy_stored(site);
}

/* Munge modtimes of 'file' accordingly; when modtime of file on
 * server is 'remote_mtime'. */
static void munge_modtime(struct site_file *file, time_t remote_mtime,
			  struct site *site)
{
    /* If this is a file, and we are using timesize mode, and we have
     * a local copy of this file already, we have to cope with the
     * modtimes problem.  The problem is that the modtime locally will
     * ALWAYS be different from the modtime on the SERVER.  */
    if (file->type == file_file && site->state_method == state_timesize) {
	if (file->local.exists) {
	    /* If we are in safe mode, we can actually check whether
	     * the remote file has changed or not when we are using
	     * timesize mode, by comparing what we thought the server
	     * modtime was with what the actual (fetched) server
	     * modtime is. Got that? */
	    NE_DEBUG(DEBUG_FILES, "Fetch: %ld vs %ld\n", 
		     file->server.time, remote_mtime);
	    if (site->safemode && file->server.exists &&
		file->server.time != remote_mtime) {
		NE_DEBUG(DEBUG_FILES, 
			 "Fetch: Marking changed file changed.\n");
		file->stored.time = file->local.time + 1;
	    } else {
		NE_DEBUG(DEBUG_FILES, "Fetch: Marking unchanged files same.\n");
		file->stored.time = file->local.time;
	    }
	} else {
	    /* If the local file doesn't exist, pretend the file was
	     * last uploaded "now" (an arbitrary time is adequate, but
	     * "now" is the least confusing). */
	    file->stored.time = time(NULL);
	}

	/* update the diff. */
	file_set_diff(file, site);
    }
}

/* Return a site_file structure given a proto_file structure fetched
 * by the protocol driver. */
static struct site_file *fetch_add_file(struct site *site,
                                        const struct proto_file *pf)
{
    enum file_type type = file_file; /* init to shut up gcc */
    struct site_file *file;
    struct file_state state = {0};
    
    switch (pf->type) {
    case proto_file:
        type = file_file;
        break;
    case proto_dir:
        type = file_dir;
        break;
    case proto_link:
        type = file_link;
        break;
    }

    state.size = pf->size;
    state.time = pf->modtime;
    state.exists = true;
    state.filename = pf->filename;
    state.mode = pf->mode;
    state.ascii = file_isascii(pf->filename, site);
    memcpy(state.checksum, pf->checksum, 16);
    
    file = file_set_stored(type, &state, site);
    
    munge_modtime(file, pf->modtime, site);
    
    if (site->safemode) {
        /* Store the server modtime. */
        file->server.time = pf->modtime;
        file->server.exists = true;
    }

    return file;
}

static
#if NE_VERSION_MINOR == 24
void
#else
int
#endif
site_fetch_csum_read(void *userdata, const char *s, size_t len)
{
    struct ne_md5_ctx *md5 = userdata;
    ne_md5_process_bytes(s, len, md5);
#if NE_VERSION_MINOR != 24
    return 0;
#endif
}

/* Retrieve the remote checksum for all files */
static int fetch_checksum_file(struct proto_file *file,
                               struct site *site, void *session)
{
#if NE_VERSION_MINOR > 25
    struct ne_md5_ctx *md5;
#define MD5_PTR md5
#else
    struct ne_md5_ctx md5;
#define MD5_PTR &md5
#endif
    char *full_remote = ne_concat(site->remote_root, file->filename, NULL);
    int ret = 0;

#if NE_VERSION_MINOR > 25
    md5 = ne_md5_create_ctx();
#else
    ne_md5_init_ctx(&md5);
#endif

    fe_checksumming(file->filename);
    if (CALL(file_read)(session, full_remote, 
                        site_fetch_csum_read, MD5_PTR) != SITE_OK) {
        ret = 1;
        fe_checksummed(full_remote, false, DRIVER_ERR);
    } else {
        ne_md5_finish_ctx(MD5_PTR, file->checksum);
        fe_checksummed(full_remote, true, NULL);
    }
    free(full_remote);

#if NE_VERSION_MINOR > 25
    ne_md5_destroy_ctx(md5);
#endif

    return ret;
}
    
/* Updates the remote file list... site_fetch_callback is called for
 * every remote file found.
 */
int site_fetch(struct site *site)
{
    int ret, need_modtimes;
    void *session;
    const char *dirstack[DIRSTACKSIZE];
    size_t dirtop;
    struct proto_file *files = NULL;

    ret = proto_init(site, &session);
    if (ret != SITE_OK) {
	proto_finish(site, session);
	return ret;
    }

    if (CALL(fetch_list) == NULL) {
	proto_finish(site, session);
	return SITE_UNSUPPORTED;
    }

    /* The remote modtimes are needed if timesize is used or in safe
     * mode: */
    need_modtimes = site->safemode || site->state_method == state_timesize;

    dirtop = 1;
    dirstack[0] = "";

    do {
        struct proto_file *newfiles = NULL, *f, *lastf = NULL;
        const char *reldir = dirstack[--dirtop];
        const char *slash = reldir[0] == '\0' ? "" : "/";
        char *curdir;

        curdir = ne_concat(site->remote_root, reldir, slash, NULL);

        ret = CALL(fetch_list)(session, curdir, need_modtimes, &newfiles);
        if (ret != SITE_OK) break;

        for (f = newfiles; f; f = f->next) {
            char *relfn;

            relfn = ne_concat(reldir, slash, f->filename, NULL);
            ne_free(f->filename);
            f->filename = relfn;

            if (!file_isexcluded(relfn, site)) {
                if (f->type == proto_dir && dirtop < DIRSTACKSIZE) {
                    dirstack[dirtop++] = relfn;
                } else if (f->type == proto_file 
                           && site->state_method == state_checksum) {
                    fetch_checksum_file(f, site, session);
                }
            }

            lastf = f;
        }

        if (lastf) {
            lastf->next = files;
            files = newfiles;
        }

        ne_free(curdir);
    } while (dirtop > 0);
    
    if (ret == SITE_OK) {
        struct proto_file *f, *nextf;

        /* Remove existing stored state for the site. */
        site_destroy_stored(site);

        /* And replace it with the fetched state. */
        for (f = files; f; f = nextf) {
            if (!file_isexcluded(f->filename, site)) {
                struct site_file *sf = fetch_add_file(site, f);
                fe_fetch_found(sf);
            }
            nextf = f->next;
            ne_free(f);
        }
    } else {
        ret = SITE_FAILED;
    }

    proto_finish(site, session);
    
    return ret;
}

/* Compares files list with files.
 * Returns SITE_OK on match, SITE_ERRORS on no match.
 */
/* Ahhhh, this is crap too.
 * If we had a generic file_set this would be easy and clean and spot
 * moved files too. We need a generic file_set.
 */
static int site_verify_compare(struct site *site, 
			       const struct proto_file *files,
			       int *numremoved)
{
    struct site_file *file;
    const struct proto_file *lfile;
    int numremote = 0;

    /* Clear live state */
    for_each_file(file, site) {
	if (file->stored.exists) {
	    numremote++;
	}
    }

    for (lfile = files; lfile != NULL; lfile = lfile->next) {
	enum file_diff diff = file_new;

	numremote--;
	for_each_file(file, site) {
	    if (file->stored.exists &&
		(strcmp(file->stored.filename, lfile->filename) == 0)) {
		/* Do a mini file_compare job */
		diff = file_unchanged;
		if (site->state_method == state_checksum) {
		    if (memcmp(file->stored.checksum, lfile->checksum, 16))
			diff = file_changed;
		} else {
		    if ((file->stored.size != lfile->size) ||
			(site->safemode && 
			 (file->server.time != lfile->modtime))) {
			diff = file_changed;
		    }
		}
		break;
	    }
	}
	
	/* If new files were added, adjust the count */
	if (diff == file_new)
	    numremote++;

	fe_verified(lfile->filename, diff);	
    }

    *numremoved = numremote;

    if (numremote != 0) {
	return SITE_ERRORS;
    } else {
	return SITE_OK;
    }	   

}

/* Compares what's on the server with what we THINK is on the server.
 * Returns SITE_OK if match, SITE_ERRORS if doesn't match.
 */
int site_verify(struct site *site, int *numremoved)
{
    struct proto_file *files = NULL;
    void *session;
    int ret;

    ret = proto_init(site, &session);
    if (ret != SITE_OK)
	return ret;

    if (CALL(fetch_list) == NULL) {
	return SITE_UNSUPPORTED;
    }

    ret = CALL(fetch_list)(session, site->remote_root, 1, &files);

#if 0
    if (site->state_method == state_checksum) {
	site_fetch_checksum(files, site, session);
    }
#endif

    proto_finish(site, session);
    
    if (ret == SITE_OK) {
	/* Return whether they matched or not */
	return site_verify_compare(site, files, numremoved);
    } else {
	return SITE_FAILED;
    }

}

/* Destroys the stored state of files in the files list for the given
 * site. Removes any files which do not exist locally from the list. */
void site_destroy_stored(struct site *site)
{
    struct site_file *current, *next;
    current = site->files;
    while (current != NULL) {
	next = current->next;
	if (!current->local.exists) {
	    /* It doesn't exist locally... nuke it */
	    file_delete(site, current);
	} else {
	    /* Just nuke the stored state...
	     * TODO-ngm: verify this. */
	    file_state_destroy(&current->stored);
	    /* Could just do .exists = false */
	    memset(&current->stored, 0, sizeof(struct file_state));
	    /* And set the diff */
	    file_set_diff(current, site);
	}
	current = next;
    }
}

/* Called to delete all the files associated with the site */
void site_destroy(struct site *site)
{
    struct site_file *current, *next;

    current = site->files;
    while (current != NULL) {
	next = current->next;
	file_delete(site, current);
	current = next;
    }

}


/* Produces a section of the flat listing output, of all the items
 * with the given diff type in the given site, using the given section
 * name. */
static void site_flatlist_items(FILE *f, struct site *site,
                                enum file_diff diff, const char *name)
{
    struct site_file *current;
    fprintf(f, "sectstart|%s", name);
    putc('\n', f);
    for_each_file(current, site) {
	if (current->diff == diff) {
	    fprintf(f, "item|%s%s", file_name(current),
		    (current->type==file_dir)?"/":"");
	    if (current->diff == file_moved) {
		fprintf(f, "|%s", current->stored.filename);
	    }
            if (current->ignore)
                fputs("|ignored", f);
            putc('\n', f);
	}
    }
    fprintf(f, "sectend|%s\n", name);
}

/* Produce the flat listing output for the given site */
void site_flatlist(FILE *f, struct site *site)
{
    fprintf(f, "sitestart|%s", site->name);
    if (site->url)	fprintf(f, "|%s", site->url);
    putc('\n', f);
    if (site->numnew > 0)
	site_flatlist_items(f, site, file_new, "added");
    if (site->numchanged > 0)
	site_flatlist_items(f, site, file_changed, "changed");
    if (site->numdeleted > 0)
	site_flatlist_items(f, site, file_deleted, "deleted");
    if (site->nummoved > 0)
	site_flatlist_items(f, site, file_moved, "moved");
    fprintf(f, "siteend|%s\n", site->remote_is_different?"changed":"unchanged");
}

void site_sock_progress_cb(void *userdata, off_t progress, off_t total)
{
    fe_transfer_progress(progress, total);
}

void fe_initialize(void)
{
    ne_sock_init();
}
