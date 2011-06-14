/* 
   sitecopy gnome-vfs driver module
   Copyright (C) 2004, David A Knight <david@screem.org>
                                                   
   the ftp driver module was used as a skeleton for implementation
                  
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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <libgnomevfs/gnome-vfs.h>

#include <ne_alloc.h>

#include "protocol.h"

typedef struct {
    struct site *site;
    const gchar *error;
} vfs_session;

extern void fe_transfer_progress(off_t progress, off_t total);

static gboolean vfs_mkdir( const gchar *path, GnomeVFSFilePermissions perms);

static int init(void **session, struct site *site)
{
    vfs_session *sess = ne_calloc(sizeof *sess);
    int ret = SITE_OK;
    *session = sess;

    sess->site = site;

    if (! vfs_mkdir(site->remote_root_user, 
                     GNOME_VFS_PERM_USER_ALL |
                     GNOME_VFS_PERM_GROUP_READ |
                     GNOME_VFS_PERM_GROUP_EXEC |
                     GNOME_VFS_PERM_OTHER_READ |
                     GNOME_VFS_PERM_OTHER_EXEC)) {
        ret = SITE_FAILED;
    }
	
    return ret;
}

static void finish(void *session)
{
    vfs_session *sess = session;
    free(sess);
}

static int file_upload(void *session, const char *local, const char *remote,
		       int ascii)
{
    GnomeVFSURI *src;
    GnomeVFSURI *dest;
	
    GnomeVFSHandle *shandle;
    GnomeVFSHandle *dhandle;

    GnomeVFSOpenMode mode;
    GnomeVFSResult result;
	
    GnomeVFSFileSize fsize;
    GnomeVFSFileSize rsize;	

    GnomeVFSFileInfo *info;
    GnomeVFSFileInfoOptions options;
	
    gchar buffer[BUFSIZ];
    int ret = SITE_OK;
	
    vfs_session *sess = (vfs_session*)session;

    src = gnome_vfs_uri_new(local);
    dest = gnome_vfs_uri_new(remote);

    /* get perms etc */
    options = GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
    info = gnome_vfs_file_info_new();
    result = gnome_vfs_get_file_info_uri(src, info, options);
    if (result == GNOME_VFS_OK) {
        mode = GNOME_VFS_OPEN_READ;
        result = gnome_vfs_open_uri(&shandle, src, mode);
    }
    if (result == GNOME_VFS_OK) {
        mode = GNOME_VFS_OPEN_WRITE;

        dhandle = NULL;
        fsize = 0;
        result = gnome_vfs_open_uri(&dhandle, dest, mode);
        if (result == GNOME_VFS_ERROR_NOT_FOUND ||
            /* sftp: hack */
            result == GNOME_VFS_ERROR_EOF) {
            result = gnome_vfs_create_uri(&dhandle,
                                           dest, mode, TRUE,
                                           info->permissions);
        }
        if (result != GNOME_VFS_OK) {
            result = GNOME_VFS_ERROR_GENERIC;
        }
		
        while(result == GNOME_VFS_OK) {
            result = gnome_vfs_read(shandle, buffer,
                                     BUFSIZ, &rsize);
            buffer[ rsize ] = '\0';
            if (result == GNOME_VFS_OK) {
                result = gnome_vfs_write(dhandle,
                                          buffer, rsize, &rsize);
                fsize += rsize;
                fe_transfer_progress(fsize, 
                                     info->size);
            }
        }
        if (result == GNOME_VFS_ERROR_EOF) {
            result = GNOME_VFS_OK;
        }
        if (result == GNOME_VFS_OK) {
            gnome_vfs_truncate_handle(dhandle, fsize);
        }
        if (shandle) {
            gnome_vfs_close(shandle);
        }
        if (dhandle) {
            gnome_vfs_close(dhandle);
        }
    } else  {
        ret = SITE_FAILED;
    }
	
    gnome_vfs_file_info_unref(info);

    if (result != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }

    gnome_vfs_uri_unref(src);
    gnome_vfs_uri_unref(dest);

    sess->error = gnome_vfs_result_to_string(result);
	
    return ret;

}

static int file_get_modtime(void *session, const char *remote, time_t *modtime)
{
    GnomeVFSURI *src;
    GnomeVFSFileInfo *info;
    GnomeVFSFileInfoOptions options;
    GnomeVFSResult result;
    int ret = SITE_OK;
    vfs_session *sess = (vfs_session*)session;
	
    src = gnome_vfs_uri_new(remote);

    options = GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
    info = gnome_vfs_file_info_new();
    result = gnome_vfs_get_file_info_uri(src, info, options);
    if (result == GNOME_VFS_OK) {
        *modtime = info->mtime;
    } else {
        ret = SITE_FAILED;
    }
	
    gnome_vfs_uri_unref(src);
    gnome_vfs_file_info_unref(info);
	
    sess->error = gnome_vfs_result_to_string(result);

    return ret;
}

static int file_upload_cond(void *session, const char *local, 
			    const char *remote, int ascii, time_t time)
{
    /* get modtime */
    time_t mtime;
    int ret;

    ret = file_get_modtime(session, remote, &mtime);

    if (ret != SITE_OK) {
        ret = SITE_FAILED;
    } else if (mtime != time) {
        ret = SITE_FAILED;
    } else {
        ret = file_upload(session, local, remote, ascii);
    }

    return ret;
}

static int file_download(void *session, const char *local, 
                         const char *remote, int ascii)
{
    /* reverse local/remote and call file_upload */
    return file_upload(session, remote, local, ascii);
}

static int file_read(void *session, const char *remote, 
		     ne_block_reader reader, void *userdata)
{
    GnomeVFSURI *src;
	
    GnomeVFSHandle *shandle;

    GnomeVFSOpenMode mode;
    GnomeVFSResult result;
	
    GnomeVFSFileSize fsize;
    GnomeVFSFileSize rsize;	

    GnomeVFSFileInfo *info;
    GnomeVFSFileInfoOptions options;
	
    gchar buffer[BUFSIZ];
    int ret = SITE_OK;
	
    vfs_session *sess = (vfs_session*)session;

    src = gnome_vfs_uri_new(remote);

    /* get perms etc */
    options = GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
    info = gnome_vfs_file_info_new();
    result = gnome_vfs_get_file_info_uri(src, info, options);
    if (result == GNOME_VFS_OK) {
        mode = GNOME_VFS_OPEN_READ;
        result = gnome_vfs_open_uri(&shandle, src, mode);
    }
    fsize = 0;
    while(result == GNOME_VFS_OK) {
        result = gnome_vfs_read(shandle, buffer,
                                 BUFSIZ, &rsize);
        buffer[ rsize ] = '\0';
        if (result == GNOME_VFS_OK) {
            fsize += rsize;
            fe_transfer_progress(fsize, info->size);
            (*reader)(userdata, buffer, ret);
        }
    }
    if (result == GNOME_VFS_ERROR_EOF) {
        result = GNOME_VFS_OK;
    }
    gnome_vfs_close(shandle);
    gnome_vfs_file_info_unref(info);

    if (result != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }

    gnome_vfs_uri_unref(src);
	
    sess->error = gnome_vfs_result_to_string(result);

    return ret;
}

static int file_delete(void *session, const char *filename)
{
    vfs_session *sess = (vfs_session*)session;
    int ret = SITE_OK;
	
    if (gnome_vfs_unlink(filename) != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }

    return ret;
}

static int file_move(void *session, const char *from, const char *to)
{
    int ret;
	
    /* move the file: copy then remove */
    ret = file_upload(session, from, to, 0);
    if (ret == SITE_OK) {
        ret = file_delete(session, from);
    }
	
    return ret;
}

static int file_chmod(void *session, const char *filename, mode_t mode)
{
    vfs_session *sess = (vfs_session*)session;
    GnomeVFSFileInfo *info;
    GnomeVFSSetFileInfoMask mask;
    GnomeVFSResult result;
    int ret;

    ret = SITE_OK;
    info = gnome_vfs_file_info_new();
    result = gnome_vfs_get_file_info(filename, info,
                                      GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
    mask = GNOME_VFS_SET_FILE_INFO_PERMISSIONS;
    if (result == GNOME_VFS_OK) {
        info->permissions = mode;
        result = gnome_vfs_set_file_info(filename, info, 
                                          mask);
    }
    if (result != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }
	
    gnome_vfs_file_info_unref(info);
	
    sess->error = gnome_vfs_result_to_string(result);
	
    return ret;
}

/* recusivley makes directories (if needed), starting at current path,
 * this is a copy of mkdir_recursive from fileops.c in screem,
 * copied into here so the driver doesn't depend on being run
 * from screem itself */
static gboolean vfs_mkdir(const gchar *path, GnomeVFSFilePermissions perms)
{
    GnomeVFSURI *uri;
    gboolean ret = FALSE;
    gboolean exists;
	
    uri = gnome_vfs_uri_new(path);
    if (uri) {
        ret = TRUE;

        exists = gnome_vfs_uri_exists(uri);
		
        if ((! exists) && gnome_vfs_uri_has_parent(uri)) {
            GnomeVFSURI *parent;
            gchar *parentname;

            parent = gnome_vfs_uri_get_parent(uri);

            parentname = gnome_vfs_uri_to_string(parent,
                                                  GNOME_VFS_URI_HIDE_NONE);
            gnome_vfs_uri_unref(parent);

            ret = vfs_mkdir(parentname, perms);

            g_free(parentname);
        }
        if (ret && ! exists) {
            GnomeVFSResult result;

            result = gnome_vfs_make_directory_for_uri(uri, 
                                                       perms);
            if (result == GNOME_VFS_ERROR_FILE_EXISTS) {
                /* ensure it is a directory */
                GnomeVFSFileInfo *info;
                GnomeVFSFileInfoOptions options;
				
                info = gnome_vfs_file_info_new();
                options = GNOME_VFS_FILE_INFO_DEFAULT;
                gnome_vfs_get_file_info_uri(uri, info,
                                             options);
                if (info->type ==GNOME_VFS_FILE_TYPE_DIRECTORY){
                    result = GNOME_VFS_OK;
                }
                gnome_vfs_file_info_unref(info);
            } 			
            ret = (result == GNOME_VFS_OK);
        }
        gnome_vfs_uri_unref(uri);
    }
	
    return ret;
}

static int dir_create(void *session, const char *dirname)
{
    vfs_session *sess = (vfs_session*)session;
    int ret;

    ret = SITE_OK;

    if (! vfs_mkdir(dirname, 0x1e4)) {
        ret = SITE_FAILED;
    }
	
    return ret;
}

static int dir_remove(void *session, const char *dirname)
{
    vfs_session *sess = (vfs_session*)session;
    int ret = SITE_OK;

    if (gnome_vfs_remove_directory(dirname) != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }

    return ret;
}

static int fetch_list(void *session, const char *dirname, int need_modtimes,
                      struct proto_file **files)
{
    /* I'm to lazy to do this at the moment, well either that
       or something to do with the following:
       Sun Jan 28 03:07:39 GMT 2001

       :-)
    */

    return SITE_UNSUPPORTED;
}

static int create_link(void *session, const char *l,
                       const char *target)
{
    int ret = SITE_OK;
    GnomeVFSURI *uri;

    uri = gnome_vfs_uri_new(l);
	
    if (gnome_vfs_create_symbolic_link(uri, 
                                        target) != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }
	
    gnome_vfs_uri_unref(uri);
	
    return ret;
}

static int delete_link(void *session, const char *l)
{
    int ret = SITE_OK;

    if (gnome_vfs_unlink(l) != GNOME_VFS_OK) {
        ret = SITE_FAILED;
    }
	
    return ret;
}

static int change_link(void *session, const char *l, 
                       const char *target)
{
    int ret;

    ret = delete_link(session, l);
    if (ret == SITE_OK) {
        ret = create_link(session, l, target);
    }

    return ret;
}

static const char *error(void *session)
{
    return ((vfs_session*)session)->error;
}

static int get_dummy_port(struct site *site)
{
    return 0;
}


/* The protocol drivers */
const struct proto_driver vfs_driver = {
    init, 
    finish,
    file_move,
    file_upload,
    file_upload_cond,
    file_get_modtime,
    file_download,
    file_read,
    file_delete,
    file_chmod,
    dir_create,
    dir_remove,
    create_link, /* create link */
    change_link, /* change link target */
    delete_link, /* delete link */
    fetch_list,
    error,
    get_dummy_port,
    get_dummy_port,
    "vfs"
};

