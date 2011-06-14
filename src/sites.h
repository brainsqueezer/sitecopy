/* 
   sitecopy, for managing remote web sites.
   Copyright (C) 1999-2006, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#ifndef SITES_H
#define SITES_H

#include "config.h"

/* Need this for off_t, mode_t etc */
#include <sys/types.h>

#include <time.h>

#include <stdio.h> /* for FILE * for the storage_file... unfortuntely */

#include "common.h"
#include "protocol.h"


/* 
   Description of the sitecopy "Data Model"
   ----------------------------------------
   
   SITES are made up of lists of FILES.  A file can be a directory, a
   link, or an actual file, but, we call it a FILE, whichever of these
   it actually is.  The "type" member of site_file indicates the file
   type.

   Several STATES are associated with each file.  A state records all
   the relevant properties of a file *at a given point in time*: its
   filename, size, last modification time, checksum, file permissions,
   etc etc.  Note that some properties are redundant for some files;
   e.g., a directory has no checksum, or link target.
   
   It is important to realize that a "filename" is a part of a file
   STATE, note merely another property of the file.  This is because
   of the "at a given point in time" note... when a file is moved, its
   filename will change, but it is conceptually the same "file".
   
   The first state is the LOCAL STATE.  This state is a direct mapping
   from the file on disk, to the file in memory.  The second is the
   STORED STATE.  This state is a copy of the local state of the file
   - as at the last update.  By comparing the stored and local states
   of a file, we tell whether it needs to be updated or not.  The
   third state is the SERVER STATE, which is only used for sites in
   'safe mode'.  This is a copy of the the state of the file *on the
   server*, as at the last update.
 
   It might help to think of the stored state as a snapshot of the
   file taken at the time of the last update.
 
   The server state IS different from the stored state, since, e.g.
   the last-modification time of an uploaded file on the server is
   different to that locally.  Try it with an FTP client.
 
   pre-0.9.0, we used to call the 'stored state' remotetime and
   remotesize.  But they were misnomers, because they were nothing to
   do with the real remote modtime and the real remote size.

   A slightly confusing flag is "file->ignored". EXCLUDED files (which
   match a regex in site->excludes) are never added to the files list.
   IGNORED files *are* added to the list, hence this flag. In update
   mode, a changed file which is "ignored" is NOT uploaded to the
   remote site.  This is the ONLY effect of the ignored flag.

*/

/* 
   We don't currently use the 'server state' to its full potential...
  only storing the server modification time. It would be possible to
  do more clever things with this, such as use HTTP Etags, or
  the HTTP Content-MD5 etc.

   There is a fourth state:

   The 'live state', or 'remote state', which is the actual state 
  of the file as held on the server (the complement of the local
  state, as the server state is the complement of the stored state).
  This can be used to do a 'verify' mode for sites with safety 
  turned on:
    -> run across the entire remote site, grabbing the file state
    into 'live state', like fetch mode except fetch mode writes it
    into 'stored state'.
    -> if serverstate and livestate differ, scream blue murder.
 */

/*   
  Within a given site, the site roots are the same for all files locally
  and remotely. The site roots may be 0-length, eg. for FTP sites where
  the home (login) directory is the site root directory.

  The root directories are stored as three members of site.
      ->foo_root, ->foo_root_user, ->foo_root_isrel.
   (where foo is remote or local)

  ->foo_root_user is what the user enters as the root in the
  rcfile. This may have a ~/ prefix to indicate the root is to be
  taken relative to the login directory. This is translated into a
  usable version, in ->foo_root. foo_roo_isrel is true if this is a
  relative directory (i.e., ->foo_root_user has a ~/ prefix).

  Example:
	->local_root_user = "~/html/mysite/"
	->local_root = "/home/ego/html/mysite/"
	->local_root_isrel = true;

	->remote_root_user = "/mydir/"
	->remote_root = "/mydir/"
	->remote_root_isrel = false;
*/

/* The different methods of defining the state of a file at a given
   moment in time are:
 
    - modification time and size
    - checksum of contents
    - link target 
 
  The method chosen dictates when we need to update the remote copy of
  the site. For a given file, exactly ONE method is used to define
  state.  The same method is used for all files of the same type in
  any given site.  For link files, the linktarget is always used. For
  normal files, the user chooses between using modification time and
  file size, or checksumming - on a per-site basis.
  
  For 'link' files, the 'link target' determines the state - only when
  the link target changes, does the remote site need updating.
 
  Checksumming allows you to do random things to the modification
  time, which is what RCS users want. But, it's a muuuch slower than
  time/size. Also, moved files can be spotted more accurately using
  checksums.
 
*/

/*  Filename handling
    -----------------
 
  The filename of a state is relative to the site root. It has no
  leading slash, and directories do not have a trailing slash.  If a
  state "does not exist" (i.e. state.exists == false), then the
  filename is undefined.  If it does exist (i.e. exists == true), then
  the filename is guaranteed to be defined.
 
  This makes filename handling in the frontend slightly awkward, since
  for any given file, determining its filename entails checking it's
  diff.  Consequently, the "file_name" function is provided, which,
  given a file, returns the stored filename of a deleted file (since
  file->local.filename is undefined), and otherwise the local
  filename.
 
  To operate on the local filesystem and on the remote site via the
  protocol driver, the file_full_remote and file_full_local functions
  are used. Given a file state, these functions return the filename
  that should be used to manipulate that file remotely and locally.
 
  These functions must only be used for states which exist (i.e., have
  a filename); otherwise they will dereference NULL pointers. For this
  reason, the use of these functions in the frontend is not
  encouraged.
  
 */

/* Return codes for site_update/fetch/synch */
/* updated okay */
#define SITE_OK 0 
/* could not resolve hostname */
#define SITE_LOOKUP -1 
/* Could not resolve hostname of proxy server */
#define SITE_PROXYLOOKUP -2
/* could not connect to remote host */
#define SITE_CONNECT -3
/* there were some errors when updating */
#define SITE_ERRORS -4
/* Could not authenticate user on server */
#define SITE_AUTH -5
/* Could not authenticate user on proxy server */
#define SITE_PROXYAUTH -6
/* Operation failed */
#define SITE_FAILED -7
/* Unsupported operation / protocol */
#define SITE_UNSUPPORTED -9

/* For use by the frontend ONLY - never returned by site_* */
#define SITE_ABORTED -101

struct site_file;
struct site;

/* Which state method is in use over the site */
enum state_method {
    state_timesize,
    state_checksum
};

enum file_diff {
    file_unchanged, /* Remote file is same as local file */
    file_changed, /* File has changed locally, and should be uploaded */
    file_new, /* File is new locally, and should be uploaded */
    file_deleted,  /* File deleted locally, and should be deleted remotely */
    file_moved /* File has been moved locally, should be moved remotely */
};

enum file_type {
    file_file,
    file_dir,
    file_link
};

struct file_state {
    char *filename; /* the file name */
    time_t time; /* the last-modification time of the file */
    off_t size; /* the size of the file */
    unsigned char checksum[16]; /* the MD5 checksum of the file */
    char *linktarget; /* the target of the link */
    unsigned int exists; /* whether the file exists in this state or not */
    unsigned int ascii; /* whether the file is 'ASCII' or not */
    mode_t mode; /* the protection modes & 0777 of the file */
};

/* To Consider: 
 *
 * - The directory is identical among many files - make a site_dir
 * structure, sharing the char *. This could include a depth, which
 * could enable 'forcecd' mode for relative remote directories more
 * easily. This could also pave the way for checking whether a whole
 * directory has moved.
 * */

/* File representation */
struct site_file {
    /* The diff between the local and stored states. */
    enum file_diff diff;

    /* The diff between the server and live states. */
    enum file_diff live_diff;

    enum file_type type;
    
    unsigned int ignore; /* whether to ignore any changes to this file */

    /* Probably want to make the states into an array, so they can be
     * indexed and used more generically than this. e.g.:
     *    struct file_state states[4];
     *    struct file_state *local, *stored, *server, *live;
     * In file_create, set ->local = ->states[0],
     *                     ->stored = ->states[1] etc etc.
     * This allows file_set_local and file_set_stored to be
     * abstracted out. Should also allow the abstract file_set to be
     * used for site_verify.
     */
    struct file_state local, stored, server, live;

    /* Linked list nodes */
    struct site_file *next;
    struct site_file *prev;
};

/* Valid file permissions mirroring values */
enum site_perm_modes {
    sitep_ignore, /* Ignore file permissions */
    sitep_exec, /* Maintain execute permissions */
    sitep_all /* Maintain all permissions */
};

/* Valid symlink handling modes */
enum site_symlink_modes {
    sitesym_ignore,
    sitesym_follow,
    sitesym_maintain
};

/* Protocol modes */
enum site_protocol_modes {
    siteproto_ftp,
    siteproto_dav,
    siteproto_rsh,
    siteproto_sftp,
    siteproto_unknown
};

/*

 fnlist - lists of fnmatch() patterns
 ------------------------------------
 
 There are two types of pattern - patterns with paths, and patterns
 without paths. The rcfile entry
     exclude "/backup/back*"
 excludes files matching back* in the asda/ directory of the site. Whereas,
 the entry
     exclude *~
 excludes ALL files matching *~ throughout the site.

 Internally, the leading slash of with-path patterns must be stripped,
 since they are used match against filenames, which don't have a
 leading slash.  If the pattern *did* have a leading slash, then the
 'haspath' field must be set to 'true'.

 e.g.
    exclude *.txt
    exclude /asda/back*

 ->  fnlist list:
	{ "*.txt", false, ... } ,
	{ "asda/back*", true, ... }  
	
*/
	   
struct fnlist {
    char *pattern;
    unsigned int haspath;
    struct fnlist *next;
    struct fnlist *prev;
};


struct site_host {
    char *hostname;
    int port;
    char *username;
    char *password;
};

/* This represents a site */
struct site {

    char *name; /* symbolic name for site */
    char *url; /* URL for site - used by flatlist mode */
    
    struct site_host server;
    struct site_host proxy;

    enum site_protocol_modes protocol;
    char *proto_string; /* protocol name used in rcfile. */
    const struct proto_driver *driver; /* the protocol driver routines */

    char *remote_root; /* root directory of site on server */
    char *remote_root_user; /* what the user gave/sees as the remote root */
    unsigned int remote_isrel; /* is the remote root dir relative to login dir? (~/) */
    char *local_root; /* root directory of site locally */
    char *local_root_user; /* what the user gave/sees as the remote root */
    unsigned int local_isrel; /* is the local root directory relative to home dir */

    char *infofile;  /* local storage file in ~/.sitecopy/  */
    char *certfile;  /* file in which cached SSL certificate is stored. */
    FILE *storage_file;  /* The file opened for the storage file */

    char *client_cert; /* client certificate */

    /* Options for the site */
    enum site_perm_modes perms; /* permissions maintenance mode */
    int dirperms; /* directory permissions maintenance mode */
    enum site_symlink_modes symlinks; /* symlink handline mode */

    /* Protocol-driver specific options here */
    unsigned int ftp_pasv_mode;
    unsigned int ftp_echo_quit;
    unsigned int ftp_forcecd;
    unsigned int ftp_use_cwd;
    unsigned int http_use_expect;
    unsigned int http_limit;
    unsigned int http_secure;
    unsigned int http_tolerant;
    char *rsh_cmd;
    char *rcp_cmd;

    unsigned int nodelete; /* whether to delete any files remotely */
    unsigned int checkmoved; /* whether to check for moved files */
    unsigned int checkrenames; /* whether to check for renamed files */
    unsigned int nooverwrite; /* whether to delete changed files before overwriting */ 
    unsigned int safemode;  /* whether we are in safe mode or not */
    unsigned int lowercase; /* whether to use all-lowercase filenames remotely */
    unsigned int tempupload; /* whether to use temporary files when uploading */

    /* These are parameters to site_update really. */
    unsigned int keep_going; /* if true, keep going past errors in updates */

    unsigned int use_this; /* whether the site is being operated on - handy
			      * for the console FE */

    /* We have two 'is_different' fields. This is unintuitive, since
     * if the local site is different from the remote site, the
     * reverse must also be true, right? Wrong, because of 'ignores'
     * and 'nodelete': using these, a change can be made to the local
     * site which will NOT be mirrored by update mode, but WILL be
     * affected by synch mode. */
    unsigned int local_is_different; /* use this if you want to know whether
					* site_synch will do anything */
    unsigned int remote_is_different; /* use this if you want to know whether
					 * site_update will do anything */
    
    enum state_method state_method; /* as dictated by rcfile */
    enum state_method stored_state_method; /* as used in info file */

    /* Files which are excluded */
    struct fnlist *excludes;
    /* Files which are ignored */
    struct fnlist *ignores;
    /* Files which are ASCII */
    struct fnlist *asciis;

    struct site_file *files; /* list of files */
    struct site_file *files_tail; /* end of the list */

    /* Some useful counts for the files */
    int numnew; /* number of new files */
    int numchanged; /* number of changed files */
    int numignored; /* number of changed files which are being ignored */
    int numdeleted; /* number of deleted files */
    int nummoved; /* number of moved files */
    int numunchanged; /* number of unchanged files */
    
    off_t totalnew; /* total file size of new files */
    off_t totalchanged; /* total file size of changed files */

    char *last_error;

    /* "Critical section" handling: do NOT modify */
    int critical;

    struct site *next;
    struct site *prev;
};

/* The list of all sites as read from the rcfile */
extern struct site *all_sites;

/* Open the storage file for writing, pre-update.
 * Returns site->storage_file or NULL on error. */
FILE *site_open_storage_file(struct site *site);
int site_close_storage_file(struct site *site);

void fe_initialize(void);

/* This reads the files information for the given site - both the
 * local and remote ones. Returns:
 *   SITE_OK      on success
 *   SITE_ERRORS  on corrupt info file
 *   SITE_FAILED  on non-existent info file
 */
int site_readfiles(struct site *);

/* This makes out like we've just done a successful site_update. */

/* This writes the stored files list back to disk.
 * Returns 0 on success or -1 on failure. */
int site_write_stored_state(struct site *);

/* This merges the stored files list in the storage file with the
 * in-memory files list of the site. Returns:
 *   SITE_OK      on success
 *   SITE_ERRORS  on corrupt info file
 *   SITE_FAILED  on non-existent info file
 */
int site_read_stored_state(struct site *site);

/* This merges the local files on disk with the in-memory files list
 * of the site. */
void site_read_local_state(struct site *site);

/* Initialize the site - pretend there are NO files held remotely */
void site_initialize(struct site *);

/* Catch up the site - mark all files as updated remotely */
void site_catchup(struct site *site);

/* Verify that that the stored state of the remote site matches the
 * actual make up of the remote site. Returns:
 *    SITE_OK       if states match up
 *    SITE_ERRORS   if states do not match
 *    SITE_FAILED   if the comparison could not begin (e.g. auth failure).
 *
 * If SITE_ERRORS is returned, then *numremoved is set to the number
 * of files missing from the server, and fe_verified() will have been
 * called for any changed or added to the remote site.  */
int site_verify(struct site *site, int *numremoved);

/* Update the remote site.
 * fe_updating, fe_updated, fe_setting_perms, fe_set_perms may be
 * called during the update. fe_can_update may be called during the
 * update if site->prompting is set.
 *
 * Returns:
 *   SITE_ERRORS if an error occurred which was reported using
 *     the fe_update_* functions. site->last_error is undefined.
 *   SITE_FAILED if the update never began, and you should
 *     look at site->last_error for the error message.
 *   SITE_* for other errors. site->last_error is undefined.
 */
int site_update(struct site *site);

/* Finds a site with the given name, and returns a pointer to it.
 * If no site of given name is found, returns NULL
 */
struct site *site_find(const char *sitename);

/* Syncronizes the local site with the remote copy.
 * fe_synch_* will be called during the synchronize.
 *
 * Returns:
 *   SITE_ERRORS if an error occurred which was reported using
 *     the fe_* functions.
 *   SITE_FAILED if the update never began, and you should
 *     look at site->last_error for the error message.
 *   SITE_* for other errors.
 *
 */
int site_synch(struct site *site);

/* Updates the files listing from the remote site.
 *
 * fe_fetch_found() will be called for each file that is found
 * in the fetch.  If the site is using checksumming, after the fe_fetch_found
 * calls are made, fe_checksumming/fe_checksummed call pairs will be made
 * for each file on the remote site.
 * Returns:
 *   SITE_ERRORS if an error occurred which was reported using
 *     the fe_* functions.
 *   SITE_FAILED if the update never began, and you should
 *     look at site->last_error for the error message.
 *   SITE_* for other errors.
 */
int site_fetch(struct site *site);

/* Destroys all the files... use before doing a second
 * site_readfiles on a site. */
void site_destroy(struct site *the_site);

/* Destroys the stored state of the site. Use before calling
 * site_fetch, or site_read_stored_state. */
void site_destroy_stored(struct site *site);

/* Outputs the flat listing style output for the given site
 * to the given stream
 */
void site_flatlist(FILE *f, struct site *the_site);

/* Returns a pseudo-URL for the given site, in a statically allocated
 * memory location which will be overwritten by subsequent calls to
 * the function. (-> NOT thread-safe) */
const char *site_pseudourl(struct site *the_site);

char *file_full_remote(struct file_state *state, struct site *site);
char *file_full_local(struct file_state *state, struct site *site);
char *file_name(const struct site_file *file);

struct fnlist *fnlist_prepend(struct fnlist **list);
struct fnlist *fnlist_deep_copy(const struct fnlist *src);

const char *site_get_protoname(struct site *site);

#endif /* SITES_H */
