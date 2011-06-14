/* 
   sitecopy WebDAV protocol driver module
   Copyright (C) 2000-2006, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#include <sys/types.h>

#include <sys/stat.h> /* For S_IXUSR */

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#include "common.h" /* for strerror */

#include <ne_request.h>
#include <ne_basic.h>
#include <ne_basic.h>
#include <ne_props.h>
#include <ne_alloc.h>
#include <ne_uri.h>
#include <ne_auth.h>
#include <ne_dates.h>
#include <ne_socket.h>

#include "protocol.h"
#include "frontend.h"
#include "i18n.h"
#include "common.h"

struct fetch_context {
    struct proto_file **files;
    struct proto_file *tail;
    const char *root;
};

#define ENABLE_PROGRESS do { ne_set_progress(sess, site_sock_progress_cb, NULL); } while (0)

#define DISABLE_PROGRESS do { ne_set_progress(sess, NULL, NULL); } while (0)

/* TODO:
 * not really sure whether we should be using an enum here... what
 * should the client do with resourcetypes it doesn't understand?
 * ignore them, or presume they have the same semantics as "normal"
 * resources. I really don't know.
 */
struct private {
    int iscollection;
};

#define ELM_resourcetype (NE_PROPS_STATE_TOP + 1)
#define ELM_collection (NE_PROPS_STATE_TOP + 2)

/* The element definitinos for the complex prop handler. */
static const struct ne_xml_idmap fetch_elms[] = {
    { "DAV:", "resourcetype", ELM_resourcetype },
    { "DAV:", "collection", ELM_collection },
};

static const ne_propname props[] = {
    { "DAV:", "getcontentlength" },
    { "DAV:", "getlastmodified" },
    { "http://apache.org/dav/props/", "executable" },
    { "DAV:", "resourcetype" },
    { NULL }
};

/* Set session error string to 'msg: strerror(errnum)'. */
static void syserr(ne_session *sess, const char *msg, int errnum)
{
    char err[256];
    ne_set_error(sess, "%s: %s", msg, ne_strerror(errnum, err, sizeof err));
}

static int get_server_port(struct site *site)
{
    return site->http_secure ? 443 : 80;
}

static int get_proxy_port(struct site *site)
{
    return 8080;
}

static int auth_common(void *userdata, fe_login_context ctx,
		       const char *realm, int attempt,
		       char *username, char *password)
{
    struct site_host *host = userdata;
    if (host->username && host->password) {
	strcpy(username, host->username);
	strcpy(password, host->password);
	return attempt;
    } else {
	return fe_login(ctx, realm, host->hostname, username, password);
    }
}

static int 
server_auth_cb(void *userdata, const char *realm, int attempt,
	       char *username, char *password)
{
    return auth_common(userdata, fe_login_server, realm, attempt, 
		       username, password);
}

static int 
proxy_auth_cb(void *userdata, const char *realm, int attempt,
	      char *username, char *password)
{
    return auth_common(userdata, fe_login_proxy, realm, attempt, 
		       username, password);
}

static void notify_cb(void *userdata, ne_conn_status status, const char *info)
{

#define MAP(a) case ne_conn_##a: fe_connection(fe_##a, info); break

    switch (status) {
	MAP(namelookup);
	MAP(connecting);
	MAP(connected);
    default:
	break;
    }

#undef MAP
}

static int h2s(ne_session *sess, int errcode)
{
    switch (errcode) {
    case NE_OK:
	return SITE_OK;
    case NE_AUTH:
	return SITE_AUTH;
    case NE_PROXYAUTH:
	return SITE_PROXYAUTH;
    case NE_FAILED:
	return SITE_FAILED;
    case NE_CONNECT:
	return SITE_CONNECT;
    case NE_LOOKUP:
	return SITE_LOOKUP;
    case NE_TIMEOUT:
	ne_set_error(sess, _("The connection timed out."));
	return SITE_ERRORS;
    case NE_ERROR:
    default:
	return SITE_ERRORS;
    }
}

/* Verify callback for when a trusted cert is not known. */
static int verify_untrusted(void *userdata, int failures,
                            const ne_ssl_certificate *cert)
{
    struct site *site = userdata;

    if (fe_accept_cert(cert, failures))
        return -1;
    
    /* TODO: how to handle a write error here? */
    ne_ssl_cert_write(cert, site->certfile);

    return 0;
}

/* Verify callback for when a trusted cert is known. */
static int verify_trusted(void *userdata, int failures,
                          const ne_ssl_certificate *cert)
{
    ne_ssl_certificate *expected = userdata;

    return failures != NE_SSL_UNTRUSTED || ne_ssl_cert_cmp(expected, cert);
}

static int init(void **session, struct site *site)
{
    ne_session *sess;
    ne_server_capabilities caps = {0};
    int ret;
    char *root;

    sess = ne_session_create(site->http_secure?"https":"http",
			     site->server.hostname, site->server.port);

    *session = sess;

    if (site->http_secure && !ne_has_support(NE_FEATURE_SSL)) {
	ne_set_error(sess, _("SSL support has not be compiled in."));
	return SITE_FAILED;
    }

    if (site->http_secure) {
        if (access(site->certfile, R_OK) == 0) {
            ne_ssl_certificate *cert = ne_ssl_cert_read(site->certfile);
            if (cert == NULL) {
                ne_set_error(sess, _("Could not load certificate `%s'."),
                             site->certfile);
                return SITE_FAILED;
            }
            ne_ssl_set_verify(sess, verify_trusted, cert);
        } else {
            ne_ssl_set_verify(sess, verify_untrusted, site);
        }
    }

    ne_set_status(sess, notify_cb, NULL);

    if (site->http_limit) {
#if NE_VERSION_MINOR > 25
        ne_set_session_flag(sess, NE_SESSFLAG_PERSIST, 0);
#else
        ne_set_persist(sess, 0);
#endif
    }

    /* Note, this won't differentiate between xsitecopy and
     * sitecopy... maybe we should put a comment in as well. */
    ne_set_useragent(sess, PACKAGE_NAME "/" PACKAGE_VERSION);

    if (site->proxy.hostname) {
	ne_set_proxy_auth(sess, proxy_auth_cb, &site->proxy);
	ne_session_proxy(sess, site->proxy.hostname, site->proxy.port);
    }

    ne_set_server_auth(sess, server_auth_cb, &site->server);
    
    if (site->http_secure && site->client_cert) {
        ne_ssl_client_cert *cc;

        cc = ne_ssl_clicert_read(site->client_cert);
        if (!cc) {
            ne_set_error(sess, _("Could not read SSL client certificate '%s'."),
                         site->client_cert);
            return SITE_FAILED;
        }

        if (ne_ssl_clicert_encrypted(cc)) {
            char password[FE_LBUFSIZ];

            if (fe_decrypt_clicert(cc, password)) {
                return SITE_FAILED;
            }

            if (ne_ssl_clicert_decrypt(cc, password) != 0) {
                ne_set_error(sess, _("Could not decrypt SSL client "
                                     "certificate '%s'."), site->client_cert);
                return SITE_FAILED;
            }
        }

        ne_ssl_set_clicert(sess, cc);

        ne_ssl_clicert_free(cc);
    }

    if (site->http_tolerant) {
	/* Skip the OPTIONS, since we ignore failure anyway. */
	return SITE_OK;
    }

    root = ne_path_escape(site->remote_root);
    ret = ne_options(sess, root, &caps);
    ne_free(root);
    if (ret == NE_OK) {
	if (!caps.dav_class1) {
	    ne_set_error(sess, 
			    _("The server does not appear to be a WebDAV server."));
	    return SITE_FAILED;
	} else if (site->perms != sitep_ignore && !caps.dav_executable) {
	    /* Need to set permissions, but the server can't do that */
	    ne_set_error(sess, 
			    _("The server does not support the executable live property."));
	    return SITE_FAILED;
	}
    } else {
        ret = h2s(sess, ret);
        if (ret == SITE_ERRORS)
            ret = SITE_FAILED;
        return ret;
    }


    return SITE_OK;
}

static void finish(void *session) 
{
    ne_session *sess = session;
    ne_session_destroy(sess);
}

static int file_move(void *session, const char *from, const char *to) 
{
    ne_session *sess = session;
    char *efrom, *eto;
    int ret;

    efrom = ne_path_escape(from);
    eto = ne_path_escape(to);

    /* Always overwrite destination. */
    ret = ne_move(sess, 1, efrom, eto);

    free(efrom);
    free(eto);
    
    return h2s(sess, ret);
}

static int file_upload(void *session, const char *local, const char *remote, 
		       int ascii)
{
    int ret, fd = open(local, O_RDONLY | OPEN_BINARY_FLAGS);
    ne_session *sess = session;
    char *eremote;

    if (fd < 0) {
	syserr(sess, _("Could not open file"), errno);
	return SITE_ERRORS;
    }
    
    eremote = ne_path_escape(remote);
    ENABLE_PROGRESS;
    ret = ne_put(sess, eremote, fd);
    DISABLE_PROGRESS;
    free(eremote);

    (void) close(fd);

    return h2s(sess, ret);
}

/* conditional PUT using If-Unmodified-Since. */
static int put_if_unmodified(ne_session *sess, const char *uri, int fd,
                             time_t since)
{
    ne_request *req = ne_request_create(sess, "PUT", uri);
    char *date = ne_rfc1123_date(since);
    int ret;

    /* Add in the conditional header */
    ne_add_request_header(req, "If-Unmodified-Since", date);
    ne_free(date);
    
#if NE_VERSION_MINOR == 24
    ne_set_request_body_fd(req, fd);
#else
    {
        struct stat st;

        if (fstat(fd, &st) < 0) {
            int errnum = errno;
            ne_set_error(sess, _("Could not stat file: %s"), strerror(errnum));
            return NE_ERROR;
        }
        
        ne_set_request_body_fd(req, fd, 0, st.st_size);
    }
#endif

    ret = ne_request_dispatch(req);
    
    if (ret == NE_OK) {
	if (ne_get_status(req)->code == 412) {
	    ret = NE_FAILED;
	} else if (ne_get_status(req)->klass != 2) {
	    ret = NE_ERROR;
	}
    }

    ne_request_destroy(req);

    return ret;
}

static int 
file_upload_cond(void *session, const char *local, const char *remote,
		 int ascii, time_t t)
{
    ne_session *sess = session;
    int ret, fd = open(local, O_RDONLY | OPEN_BINARY_FLAGS);
    char *eremote;

    if (fd < 0) {
	syserr(sess, _("Could not open file"), errno);
	return SITE_ERRORS;
    }
    
    eremote = ne_path_escape(remote);
    ENABLE_PROGRESS;
    ret = h2s(sess, put_if_unmodified(sess, eremote, fd, t));
    DISABLE_PROGRESS;
    free(eremote);
    
    (void) close(fd);

    return ret;
}

static int file_get_modtime(void *session, const char *remote, time_t *modtime)
{
    ne_session *sess = session;
    int ret;
    char *eremote;
 
    eremote = ne_path_escape(remote);
    ret = ne_getmodtime(sess,eremote,modtime);
    free(eremote);

    return h2s(sess, ret);
}
    
static int file_download(void *session, const char *local, const char *remote,
			 int ascii) 
{
    ne_session *sess = session;
    int ret, fd = open(local,
		       O_TRUNC | O_CREAT | O_WRONLY | OPEN_BINARY_FLAGS, 0644);
    char *eremote;

    if (fd < 0) {
	syserr(sess, _("Could not open file"), errno);
	return SITE_ERRORS;
    }

    eremote = ne_path_escape(remote);
    ENABLE_PROGRESS;
    ret = h2s(sess, ne_get(sess, eremote, fd));
    DISABLE_PROGRESS;
    free(eremote);
    
    if (close(fd)) {
	ret = SITE_ERRORS;
    }
    
    return ret;
}

static int file_read(void *session, const char *remote, 
		     ne_block_reader reader, void *userdata) 
{
    ne_session *sess = session;
    int ret;
    char *eremote;
    ssize_t bytes;
    ne_request *req;

    eremote = ne_path_escape(remote);    
    req = ne_request_create(sess, "GET", eremote);
    ENABLE_PROGRESS;
    do {
	char buf[BUFSIZ];
	ret = ne_begin_request(req);
	if (ret != NE_OK) break;
	while ((bytes = ne_read_response_block(req, buf, sizeof buf)) > 0)
	    reader(userdata, buf, bytes);
	if (bytes < 0)
	    ret = NE_ERROR;
	else
	    ret = ne_end_request(req);
    } while (ret == NE_RETRY);
    DISABLE_PROGRESS;
    free(eremote);

    return h2s(sess, ret);
}

static int file_delete(void *session, const char *remote) 
{
    ne_session *sess = session;
    char *eremote = ne_path_escape(remote);
    int ret = ne_delete(sess, eremote);

    free(eremote);
    return h2s(sess, ret);
}

static int file_chmod(void *session, const char *remote, mode_t mode) 
{
    ne_session *sess = session;
    static const ne_propname execprop = 
    { "http://apache.org/dav/props/", "executable" };
    /* Use a single operation; set the executable property to... */
    ne_proppatch_operation ops[] = { 
	{ &execprop, ne_propset, NULL }, { NULL } 
    };
    char *eremote = ne_path_escape(remote);
    int ret;
    
    /* True or false, depending... */
    if (mode & S_IXUSR) {
	ops[0].value = "T";
    } else {
	ops[0].value = "F";
    }

    ret = ne_proppatch(sess, eremote, ops);
    free(eremote);

    return h2s(sess, ret);
}

/* Returns escaped path string for a directory. */
static char *coll_escape(const char *dirname)
{
    char *ret = ne_path_escape(dirname);
    if (!ne_path_has_trailing_slash(ret)) {
	ret = ne_realloc(ret, strlen(ret) + 2);
	strcat(ret, "/");
    }
    return ret;
}

static int dir_create(void *session, const char *dirname)
{
    ne_session *sess = session;
    char *edirname = coll_escape(dirname);
    int ret = ne_mkcol(sess, edirname);
    free(edirname);
    return h2s(sess, ret);
}

/* TODO: check whether it is empty first */
static int dir_remove(void *session, const char *dirname)
{
    ne_session *sess = session;
    char *edirname = coll_escape(dirname);
    int ret = ne_delete(sess, edirname);
    free(edirname);
    return h2s(sess, ret);
}

/* Insert the file in the list in the appropriate position (keeping it
 * sorted). */
static void insert_file(struct fetch_context *ctx, struct proto_file *file)
{
    if (ctx->tail) {
        ctx->tail->next = file;
    } else {
        (*ctx->files) = file;
    }
    ctx->tail = file;
    file->next = NULL;
}

#if NE_VERSION_MINOR > 25
static void pfind_results(void *userdata, const ne_uri *uri,
			  const ne_prop_result_set *set)
#else
static void pfind_results(void *userdata, const char *href,
			  const ne_prop_result_set *set)
#endif
{
    struct fetch_context *ctx = userdata;
    struct private *private = ne_propset_private(set);
    const char *clength = NULL, *modtime = NULL, *isexec = NULL;
    struct proto_file *file;
    int iscoll;
    char *uhref;

    iscoll = private->iscollection;

#if NE_VERSION_MINOR < 26
    /* For >= 0.26, this is handled by the destroy_private
     * callback. */
    ne_free(private);
#endif

#if NE_VERSION_MINOR < 26
    /* Strip down to the abspath segment */
    if (strncmp(href, "http://", 7) == 0)
	href = strchr(href+7, '/');
    
    if (strncmp(href, "https://", 8) == 0)
	href = strchr(href+8, '/');

    if (href == NULL) {
	NE_DEBUG(NE_DBG_HTTP, "invalid!\n");
	return;
    }

    uhref = ne_path_unescape(href);
#else
    uhref = ne_path_unescape(uri->path);
    
#endif

    NE_DEBUG(NE_DBG_HTTP, "URI: [%s]: ", uhref);

    if (!ne_path_childof(ctx->root, uhref)) {
	/* URI not a child of the root collection...  ignore this
	 * resource */
	NE_DEBUG(NE_DBG_HTTP, "not child of root collection!\n");
	return;
    }
   
    NE_DEBUG(NE_DBG_HTTP, "okay.\n");

    if (!iscoll) {
	const ne_status *status = NULL;

	clength = ne_propset_value(set, &props[0]);    
	modtime = ne_propset_value(set, &props[1]);
	isexec = ne_propset_value(set, &props[2]);
	
	if (clength == NULL)
	    status = ne_propset_status(set, &props[0]);
	if (modtime == NULL)
	    status = ne_propset_status(set, &props[1]);

	if (clength == NULL || modtime == NULL) {
	    fe_warning(_("Could not access resource"), uhref, 
		       status?status->reason_phrase:NULL);
	    return;
	}

    }

    file = ne_calloc(sizeof(struct proto_file));
    file->filename = ne_strdup(uhref+strlen(ctx->root));
    
    if (iscoll) {
	file->type = proto_dir;

	/* Strip the trailing slash if it has one. */
	if (ne_path_has_trailing_slash(file->filename)) {
	    file->filename[strlen(file->filename) - 1] = '\0';
	}

    } else {
	file->type = proto_file;
	file->size = atoi(clength);
	file->modtime = modtime?ne_httpdate_parse(modtime):0;
	if (isexec && strcasecmp(isexec, "T") == 0) {
	    file->mode = 0755;
	} else {
	    file->mode = 0644;
	}
    }

    /* Insert the file into the files list. */
    insert_file(ctx, file);
}

static int start_element(void *userdata, int parent,
                         const char *nspace, const char *name,
                         const char **atts)
{
    ne_propfind_handler *handler = userdata;
    struct private *priv = ne_propfind_current_private(handler);
    int state;

    state = ne_xml_mapid(fetch_elms, NE_XML_MAPLEN(fetch_elms), nspace, name);

    if (parent == NE_207_STATE_PROP && state == ELM_resourcetype)
        return ELM_resourcetype;
    
    if (parent == ELM_resourcetype && state == ELM_collection)
        priv->iscollection = 1;

    return NE_XML_DECLINE;
}

/* Creates the private structure. */
#if NE_VERSION_MINOR > 25
static void *create_private(void *userdata, const ne_uri *uri)
#else
static void *create_private(void *userdata, const char *uri)
#endif
{
    return ne_calloc(sizeof(struct private));
}

#if NE_VERSION_MINOR > 25
static void destroy_private(void *userdata, void *private)
{
    struct private *priv = private;

    ne_free(priv);
}
#endif

/* TODO: optimize: only ask for lastmod + executable when we really
 * need them: it does waste bandwidth and time to ask for executable
 * when we don't want it, since it forces a 404 propstat for each
 * non-collection resource if it is not defined.  */
static int fetch_list(void *session, const char *dirname, int need_modtimes,
                      struct proto_file **files) 
{
    ne_session *sess = session;
    int ret;
    struct fetch_context ctx;
    ne_propfind_handler *ph;
    char *edirname = ne_path_escape(dirname);

    ctx.root = dirname;
    ctx.files = files;
    ctx.tail = NULL;
    ph = ne_propfind_create(sess, edirname, NE_DEPTH_ONE);

    /* The complex props. */
    ne_propfind_set_private(ph, create_private,
#if NE_VERSION_MINOR > 25
                            destroy_private, 
#endif
                            NULL);

    /* Register the handler for the complex props. */
    ne_xml_push_handler(ne_propfind_get_parser(ph), start_element, NULL, NULL, ph);

    ret = ne_propfind_named(ph, props, pfind_results, &ctx);

    free(edirname);

    return h2s(sess,ret);
}

static int unimp_link2(void *session, const char *l, const char *target)
{
    ne_session *sess = session;
    ne_set_error(sess, "Operation not supported");
    return SITE_UNSUPPORTED;
}
 
static int unimp_link1(void *session, const char *l)
{
    ne_session *sess = session;
    ne_set_error(sess, "Operation not supported");
    return SITE_UNSUPPORTED;
}


static const char *error(void *session) 
{
    ne_session *sess = session;
    return ne_get_error(sess);
}

/* The WebDAV protocol driver */
const struct proto_driver dav_driver = {
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
    unimp_link2, /* create link */
    unimp_link2, /* change link target */
    unimp_link1, /* delete link */
    fetch_list,
    error,
    get_server_port,
    get_proxy_port,
    "WebDAV"
};
