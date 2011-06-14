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

/* This is the console front end for sitecopy.
 * TODO: Some of the configuration stuff should be moved out of here. */

#include <config.h>

#include <sys/types.h>

#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif 
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <signal.h>
#include <time.h>
#include <ctype.h>

#include <getopt.h>

#include <ne_string.h> /* for ne_shave */
#include <ne_utils.h> /* for ne_debug_* */
#include <ne_alloc.h>

#include "basename.h"
#include "i18n.h"
#include "frontend.h"
#include "sites.h"
#include "rcfile.h"
#include "common.h"

/* From lib/yesno.c */
int yesno(void);

/* The maximum number of sites which can be specified on the command
 * line */

#define MAXSITES 20

static enum action {
    action_list,
    action_synch,
    action_fetch,
    action_update,
    action_verify,
    action_autoupdate,
    action_catchup,
    action_init,
    action_view,
    action_none
} action;

#define A_LOCAL (1 << 2)
#define A_STORED (1 << 3)
#define A_COND_STORED (A_STORED | (1 << 4))

/* TODO: extend this even further, so it includes the site_whatever
 * handler, whether we need to do a post-operation write_stored_state,
 * any preconditions, ...
 * Changes needed for that are that each site_mode() function have
 * consistent parms and return code usage.
 * Does this structure actually make localization harder or easier?
 * .. Unfortunately harder. :-(<prazak@grisoft.cz>
 */
static struct action_info {
    /* The stem verb: used to form phrases like: "Update the site".
     * Should be capitalized, since it will be used at the beginning of
     * sentences. */
    const char *verb; 
    /* The present participle of the verb (I think?), used to form a
     * phrase like: "I am <doing> the site." */
    const char *doing; 
    /* What the action operates on, used to form a phrase like:
     *  "I am <doing> the <subject> site." */
    const char *subject;
    /* Flags;
     *  A_LOCAL -> must read local state before operation.
     *  A_STORED -> must read stored state before operation.
     *  A_COND_STORED -> read stored state if there is any, otherwise ignore it.
     */
    unsigned int flags;
} actions[] = {
    { N_("Show changes to"), N_("showing changes to"), N_("local"), A_LOCAL | A_STORED },
    { N_("Synchronize"), N_("synchronizing"), N_("local"), A_LOCAL | A_STORED },
    /* TODO: fetch really only needs local state if we're using
     * state_timesize, to fudge the modtimes.
     * And we only need A_STORED if we're using state_timesize AND
     * safe mode. */
    { N_("Fetch"), N_("fetching"), N_("remote"), A_LOCAL | A_COND_STORED },
    { N_("Update"), N_("updating"), N_("remote"), A_LOCAL | A_STORED },
    { N_("Automatic Update"), N_("updating"), N_("remote"), A_LOCAL | A_STORED },
    { N_("Verify"), N_("verifying"), N_("remote"), A_STORED },
    { N_("Catch up"), N_("catching up"), N_("stored"), A_LOCAL },
    { N_("Initialize"), N_("initializing"), N_("stored"), 0 },
    { NULL, NULL, NULL, 0 }
};

static const char *contact_mntr = 
N_("You should never see this message.\n"
   "Please contact the maintainer at sitecopy@lyra.org\n");

/* The short program name, basename(argv[0]) */
static const char *progname;

/* the sites specified on the command line */
static const char *sitenames[MAXSITES]; 
static int numsites; /* the number of sites specified */
static struct site *current_site; /* this is used to save the state if we
				   * get signalled mid-update */

static int upload_total, upload_sofar, in_transfer;

/* User-specified options */
static int quiet; /* How quiet do they want us to be? */
static int allsites, /* Do they want all sites to be operated on? */
    listflat, /* Do they want the 'flat' list style */
    show_progress, /* Do they want the %-complete messages */
    prompting, /* Did they say --prompting? */
    keepgoing, /* Did they say --keep-going? */
    dry_run;   /* Did they say --dry-run? */

/* Functions prototypes */
static void init(int, char **);
static void parse_cmdline(int, char **);
static int act_on_site(struct site *site, enum action act);
static int verify_sites(struct site *sites, enum action act);
int main(int, char **);
static void usage(void);
static void version(void);
static void list_site_changes(struct site *);
static int list_site_definitions(struct site *);
static void init_sites(void);

int main(int argc, char *argv[])
{
    int ret = 0, numgoodsites;
    struct site *current;

#ifdef HAVE_SETLOCALE
    setlocale(LC_ALL, "");
#endif

#ifdef ENABLE_NLS
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    textdomain(PACKAGE_NAME);
#endif /* ENABLE_NLS */

    fe_initialize();
    init(argc, argv);

    if (numsites == 0 && action == action_view)
	allsites = 1;

    if (!allsites) {
	/* Mark all the sites we are interested in */
	int num;
	for (num=0; num < numsites; num++) {
	    current = site_find(sitenames[num]);
	    if (current == NULL) {
		printf(
		    _("%s: Error: No site called `%s' found - skipping.\n"), 
		    progname, sitenames[num]);
	    } else {
		current->use_this = true;
	    }
	}
    }    

    /* Count the number of sites that are okay. */
    numgoodsites = verify_sites(all_sites, action);

    /* Handle the dump sites request */
    if (action == action_view) {
	return list_site_definitions(all_sites);
    }
    
    if (numgoodsites==0) {
	if (numsites > 0) {
	    printf(_("%s: No valid sites specified.\n"), progname);
	} else {
	    printf(_("%s: No sites specified.\n"), progname);
	}
	printf(_("Try `%s --help' for more information.\n"), progname);
	exit(-1);
    }

    for (current=all_sites; current!=NULL; current=current->next) {
	
	if (current->use_this || allsites) {
	    if (!listflat && quiet == 0) {
		/* Display the banner line */
		const char *str_action = _(actions[action].doing);
		printf(_("%s: %c%s site `%s' (on %s in %s)\n"), 
			progname, toupper(*str_action), 
			str_action+1, current->name, 
			current->server.hostname, current->remote_root_user);
	    }
	    ret = act_on_site(current, action);
	}

    }
    
    return ret;
}

/* Produce the normal listing output for the given site.
 */
static void list_site_changes(struct site *the_site)
{
    struct site_file *current;
    int count;

    if (the_site->numnew > 0) {
	printf(_("* These items have been added since the last update:\n"));
	count = 0;
	for (current = the_site->files; current!=NULL; current=current->next) {
	    if (current->diff == file_new) {
		if (count++) printf(", ");
		if (current->type == file_dir) printf(_("dir:"));
		printf("%s", file_name(current));
	    }
	}
	putchar('\n');
    }
    if ((the_site->numchanged > 0) || (the_site->numignored > 0)) {
	printf(_("* These items have been changed since the last update:\n"));
	if (the_site->numignored > 0) {
	    printf(_("  Changes to items in [brackets] are ignored during updates.\n"));
	}    
	count = 0;
	for (current = the_site->files; current!=NULL; current=current->next) {
	    if (current->diff == file_changed) {
		if (count++) printf(", ");
		printf("%s%s%s", current->ignore?"[":"", file_name(current),
			current->ignore?"]":"");
	    }
	}
	putchar('\n');
    }
    if (the_site->numdeleted > 0) {
	if (the_site->nodelete) {
	    printf(_("* These items have been deleted, but will be left on the server:\n"));
	} else {
	    printf(_("* These items have been deleted since the last update:\n"));
	}
	count = 0;
	for (current = the_site->files; current!=NULL; current=current->next) {
	    if (current->diff == file_deleted) {
		if (count++) printf(", ");
		if (current->type == file_dir) printf(_("dir:"));
		printf("%s", file_name(current));
	    }
	}
	putchar('\n');
    }
    if (the_site->nummoved > 0) {
	printf(_("* These items have been moved since the last update:\n"));
	count = 0;
	for (current = the_site->files; current!=NULL; current=current->next) {
	    if (current->diff == file_moved) {
		/* TODO: move to directory */
		if (count++) printf(", ");
		printf("%s->%s", current->stored.filename, 
			current->local.filename);
	    }
	}
	putchar('\n');
    }
}

static const char *get_perms_mode(struct site *site)
{
    switch (site->perms) {
    case sitep_ignore:
	return _("Ignored");
    case sitep_exec:
	return _("Maintained for executables");
    default:
	return _("Always maintained");
    }
}

static const char *get_symlinks_mode(struct site *site)
{
    switch (site->symlinks) {
    case sitesym_ignore:
	return _("Ignored");
    case sitesym_maintain:
	return _("Maintained");
    default:
	return _("Followed");
    }
}

static int list_site_definitions(struct site *sites)
{
    struct site *current;
    for (current=sites; current!=NULL; current=current->next) {
	/* FIXME: Possibly, make sure we have the actual port number first
	 * so we don't have to mess around printing out the port */
	if (!current->use_this && !allsites) continue;
	printf(_("Site: %s\n\tServer: %s"), current->name, 
		current->server.hostname);
	printf(_("  Port: "));
	if (current->server.port == 0) {
	    printf(_("(default)\n"));
	} else {
	    printf(_("%d\n"), current->server.port);
	}
	printf(_("\tProtocol: %s    Username: %s\n"),
		site_get_protoname(current), 
		current->server.username?current->server.username:
		_("(unspecified)"));
	if (! current->ftp_pasv_mode)
	    printf(_("\tPassive mode FTP will not be used.\n"));
	printf(_("\tRemote directory: %s\n\tLocal directory: %s\n"),
		current->remote_root_user, current->local_root_user);
	printf(_("\tPermissions: %s     Symlinks: %s\n"),
		get_perms_mode(current), get_symlinks_mode(current));
	if (current->nodelete) 
	    printf(_("\tRemote files will not be deleted.\n"));
	if (current->checkmoved) 
	    printf(_("\tFiles will be moved remotely if moved locally.\n"));
    }
    return 0;
}

/* Called to ensure only one action is specified at once */
static void set_action(enum action newact)
{
    if (action != action_none) {
	printf(_("%s: Error: Only specify ONE operation mode at a time.\n"),
		progname);
	printf(_("Try `%s --help' for more information.\n"), progname);
	exit(-1);
    } else {
	action = newact;
    }
}

static void parse_cmdline(int argc, char *argv[])
{
    int optc;
    int firstlist = false;
    extern char *optarg;
    extern int optind;
    const static char *shortopts = 
#ifdef NE_DEBUGGING
	"d:g:"
#endif
	"acefhiklonp:qr:suUvVyZ"; /* available: bgjmntwx */
    const static struct option longopts[] = {
	/* Operation modes */
	{ "update", no_argument, NULL, 'u' },
   { "autoupdate", no_argument, NULL, 'U' },
	{ "verify", no_argument, NULL, 'e' },
	{ "initialize", no_argument, NULL, 'i' },
	{ "fetch", no_argument, NULL, 'f' },
	{ "synchronize", no_argument, NULL, 's' },
	{ "list", no_argument, NULL, 'l' },
	{ "flatlist", no_argument, NULL, 'Z' },
	{ "keep-going", no_argument, NULL, 'k' },
        { "dry-run", no_argument, NULL, 'n' },
	{ "show-progress", no_argument, NULL, 'o' },
/*	{ "force-overwrite", no_argument, NULL, 't' }, */
	{ "help", no_argument, NULL, 'h' },
	{ "catchup", no_argument, NULL, 'c' },
	{ "view", no_argument, NULL, 'v' },
	/* Options */
	{ "quiet", no_argument, &quiet, 1 },
	{ "silent", no_argument, &quiet, 2 },
	{ "rcfile", required_argument, NULL, 'r' },
	{ "storepath", required_argument, NULL, 'p' },
#ifdef NE_DEBUGGING
	{ "debug", required_argument, NULL, 'd' },
	{ "logfile", required_argument, NULL, 'g' },
#endif
	{ "prompting", no_argument, NULL, 'y' },
	{ "allsites", no_argument, NULL, 'a' },
	{ "version", no_argument, NULL, 'V' },
	{ 0, 0, 0, 0 }
    };
	
#ifdef NE_DEBUGGING
    /* Debugging defaults to off and stderr */
    int use_debug_mask = 0;
    FILE *use_debug_stream = stderr;
#endif
    
    /* Defaults */
    allsites = prompting = false;
    show_progress = false;
    action = action_none;
    progname = base_name(argv[0]);

    /* Read the cmdline args */
    while ((optc = getopt_long(argc, argv, 
			       shortopts, longopts, NULL)) != -1) {
	switch (optc) {
	case 0:
	    /* Make the action list mode if they gave --flatlist */
	    if (listflat == true)
		action = action_list;
	    break;
	case 'Z':
	    listflat = true;
	    set_action(action_list);
	    break;
	case 'l':
	    if (firstlist == false) {
		set_action(action_list);
		firstlist = true;
	    } else {
		listflat = true;
	    }
	    break;
	case 's': set_action(action_synch); break;
	case 'e': set_action(action_verify); break;
	case 'f': set_action(action_fetch); break;
	case 'i': set_action(action_init); break;
	case 'c': set_action(action_catchup); break;
	case 'u': set_action(action_update); break;
   case 'U': set_action(action_autoupdate); break;
	case 'v': 
	    set_action(action_view);
	    break;
	case 'q': quiet++; break;
	case 'r': 
	    if (strlen(optarg) != 0) {
		rcfile = ne_strdup(optarg);
	    } else {
		usage();
		exit(-1);
	    }
	    break;
	case 'p':
	    if (strlen(optarg) != 0) {
		if (optarg[strlen(optarg)] != '/') {
		    copypath = ne_malloc(strlen(optarg) + 2);
		    strcpy(copypath, optarg);
		    strcat(copypath, "/");
		} else {
		    copypath = ne_strdup(optarg);
		}
	    } else {
		usage();
		exit(-1);
	    }
	    break;
#ifdef NE_DEBUGGING
	case 'd': {
	    char errbuf[20];
	    /* set debugging level */
	    
	    if (map_debug_options(optarg, &use_debug_mask, errbuf)) {
		printf(_("%s: Error: Debug channel %s not known.\n"),
		       progname, errbuf );
		exit(-1);
	    }
	} break;
	case 'g': {
	    FILE *f = fopen(optarg, "a+");
	    if (f == NULL) {
		printf(_("%s: Warning: Could not open `%s' to use as logfile.\n"),
		       progname, optarg);
		f = stderr;
	    } else {
		use_debug_stream = f;
		/* TODO: Close this stream when we exit?... welll...
		 * we *could* do. */
	    }
	} break;
#endif
	case 'y':
	    prompting = true;
	    break;
	case 'k':
	    keepgoing = true;
	    break;
	case 'a':
	    allsites = true;
	    break;
        case 'n':
            dry_run = true;
            break;
	case 'o':
	    show_progress = true;
	    break;
	case 'V': 
	    version();
	    puts(ne_version_string());
	    exit(-1);
	case 'h': usage(); exit(-1);
	case '?': 
	default:
	    printf(_("Try `%s --help' for more information.\n"), progname);
	    exit(1);
	}
    }

    /* Set the default action mode */
    if (action == action_none)
	action = action_list;

    /* Dry-run mode is only currently supported for --update */
    if (dry_run && action != action_update) {
        printf(_("%s: Error: Dry run mode is currently only supported "
                 "for updates.\n"),
               progname);
        exit(1);
    }

#ifdef NE_DEBUGGING
    ne_debug_init(use_debug_stream, use_debug_mask);
#else
    ne_debug_init(stderr, 0);
#endif

    /* Get those site names off the end of the cmdline */
    for (numsites = 0 ; optind < argc; optind++) {
	sitenames[numsites++] = argv[optind];
	if (numsites == MAXSITES) {
	    printf(_("%s: Warning: Only %d sites can be specified on the command line!\nExtra entries are being skipped.\n"), progname, MAXSITES);
	    break;
	}
    }

}

static int get_username(const char *prompt, char *buffer)
{
    printf("%s", prompt);
    if (fgets(buffer, FE_LBUFSIZ, stdin)) {
	ne_shave(buffer, "\r\n ");
	return 0;
    } else {
	return -1;
    }
}

int fe_accept_cert(const ne_ssl_certificate *cert, int failures)
{
    const char *id = ne_ssl_cert_identity(cert);
    char *dn, fprint[61];

    ne_ssl_cert_digest(cert, fprint);

    puts(_("WARNING: Server certificate is not trusted."));
    
    if (id)
        printf(_("Certificate was issued for server `%s'.\n"), id);
    else
        puts(_("WARNING: Certificate does not specify a server"));
    
    printf(_("Fingerprint: %s\n"), fprint);

    dn = ne_ssl_readable_dname(ne_ssl_cert_subject(cert));
    printf(_("Issued to: %s\n"), dn);
    free(dn);

    dn = ne_ssl_readable_dname(ne_ssl_cert_issuer(cert));
    printf(_("Issued by: %s\n"), dn);
    free(dn);

    printf(_("Do you wish to accept this certificate? (y/n) "));

    return !yesno();
}

int fe_login(fe_login_context ctx, const char *realm, const char *hostname,
	     char *username, char *password) 
{
    const char *server = 
	(ctx==fe_login_server)?N_("server"):N_("proxy server");
    char *tmp;
    if (in_transfer) {
	printf("]");
    }
    if (realm) {
	printf(_("Authentication required for %s on %s `%s':\n"), 
	       realm, server, hostname);
    } else {
	printf(_("Authentication required on %s `%s':\n"), server, hostname);
    }
    if (username[0] == '\0') {
	if (get_username(_("Username: "), username)) {
	    printf("\nAuthentication aborted!\n");
	    return -1;
	}
    } else {
	printf(_("Username: %s\n"), username);
    }    
    tmp = getpass(_("Password: "));
    if (tmp == NULL) {
	/* joe: my Linux getpass doesn't say it will ever return NULL, 
	 * but, just to be sure... */
	return -1;
    }

    ne_strnzcpy(password, tmp, FE_LBUFSIZ);

    if (in_transfer) {
	printf(_("Retrying: ["));
	upload_sofar = 0;
    }
    return 0;
}

int fe_decrypt_clicert(const ne_ssl_client_cert *cert, char *password)
{
    const char *name = ne_ssl_clicert_name(cert);
    char *tmp;

    printf(_("%s: Encrypted client certificate configured%s%s.\n"),
           progname, name ? ": " : "", name ? name : "");

    tmp = getpass(_("Password: "));
    if (tmp == NULL) {
        return -1;
    }

    ne_strnzcpy(password, tmp, FE_LBUFSIZ);
    return 0;
}

void fe_warning(const char *descr, const char *reason, const char *err)
{
    printf("%s: Warning", progname);
    if (reason != NULL) {
	printf(" on `%s':\n", reason);
    } else {
	printf(":\n");
    }
    printf("%s", descr);
    if (err != NULL) 
	printf(" - %s\n", err);
    else
	printf("\n");
}

int fe_can_update(const struct site_file *file)
{
    if (!prompting) return true;
    switch (file->type) {
    case file_dir:
	if (file->diff == file_new) {
	    printf(_("Create %s/"), file_name(file));
	} else {
	    printf(_("Delete %s/"), file_name(file));
	}
	break;
    case file_file:
	switch (file->diff) {
	case file_changed:
	case file_new: 
	    printf(_("Upload %s (%" NE_FMT_OFF_T " bytes)"), file_name(file),
		   file->local.size); break;
	case file_deleted: printf(_("Delete %s"), file_name(file)); break;
	case file_moved: printf(_("Move %s->%s"), file->stored.filename,
				 file_name(file)); break;
	default: 
	    /* Shouldn't happen */
	    printf(_("%s: in fe_can_update/file_file\n%s"), progname, 
		    contact_mntr);
	    break;
	}
	break;
    case file_link:
	switch (file->diff) {
	case file_changed: printf(_("Change %s"), file_name(file)); break;
	case file_new: printf(_("Create %s"), file_name(file)); break;
	case file_deleted: printf(_("Remove %s"), file_name(file)); break;
	default:
	    /* Shouldn't happen */
	    printf(_("%s: in fe_can_update/file_link\n%s"), progname,
		    contact_mntr);
	    break;
	}
	break;
    }
    printf(_("? (y/n) "));
    return yesno();
}

void fe_checksumming(const char *filename)
{
    switch (quiet) {
    case 0:
	in_transfer = 0;
	printf(_("Checksumming %s: ["), filename);
	fflush(stdout);
	break;
    case 1:
	printf("%s\n", filename);
    default:
	break;
    }
}

void fe_checksummed(const char *file, int success, const char *err)
{
    switch (quiet) {
    case 0:
	if (success) {
	    printf(_("] done.\n"));
	} else {
	    printf(_("] failed:\n%s\n"), err);
	}
    default:
	break;
    }
}

void fe_setting_perms(const struct site_file *file)
{
    if (quiet == 0) {
	printf(_("Setting permissions on %s%s: "), file_name(file),
               file->type == file_dir ? "/" : "");
	fflush(stdout);
    }
}

void fe_set_perms(const struct site_file *file, int success, const char *error)
{
    if (quiet == 0) {
	if (success) {
	    printf(_("done.\n"));
	} else {
	    printf(_("failed:\n%s\n"), error);
	}
    }
}

/* Called when the given files is about to be updated */
void fe_updating(const struct site_file *file)
{
    if (quiet) {
	if (quiet == 1) {
	    printf("%s\n", file_name(file));
	}
	return;
    }

    in_transfer = 0;
    switch (file->type) {
    case file_dir:
	if (file->diff == file_new) {
	    printf(_("Creating %s/: "), file_name(file));
	} else {
	    printf(_("Deleting %s/: "), file_name(file));
	}
	break;
    case file_file:
	switch (file->diff) {
	case file_changed:
	case file_new: 
	    printf(_("Uploading %s: ["), file_name(file)); 
	    break;
	case file_deleted: 
	    printf(_("Deleting %s: "), file_name(file));
	    break;
	case file_moved: 
	    printf(_("Moving %s->%s: "), file->stored.filename,
		   file->local.filename); 
	    break;
	default: 
	    printf(_("%s: in fe_updating/file_file\n%s"), progname,
		   contact_mntr); 
	    break;
	}
	break;
    case file_link:
	switch (file->diff) {
	case file_changed:
	    printf(_("Changing %s: "), file_name(file)); break;
	case file_new:
	    printf(_("Creating %s: "), file_name(file)); break;
	case file_deleted:
	    printf(_("Deleting %s: "), file_name(file)); break;
	default:
	    printf(_("%s: in fe_updating/file_link\n%s"), progname,
		   contact_mntr); 
	}
	break;
    }

    fflush(stdout);
}

void fe_updated(const struct site_file *file, int success, const char *error)
{
    char wrap = error && strlen(error) < 30 ? ' ' : '\n';

    upload_sofar += file->local.size;

    if (quiet > 0) {
	if (! success) {
	    printf(_("Failed to update %s:%c%s\n"), file_name(file), wrap, error);
	}
	return;
    }
    if ((file->type == file_dir) || 
	(file->diff!=file_changed && file->diff!=file_new)) {
	if (success) {
	    printf(_("done.\n"));
	} else {
	    printf(_("failed:%c%s\n"), wrap, error);
	}
    } else {
	if (success) {
	    if (show_progress) {
		float prog = (100 * (float)upload_sofar) / (float)upload_total;
		if (upload_total == 0) prog = 0;
		printf(("] done. (%.0f%% finished)\n"), prog);
	    } else {
		printf(_("] done.\n"));
	    }
	} else {
	    printf(_("] failed:%c%s\n"), wrap, error);
	}
    }
}

void fe_synching(const struct site_file *file)
{
    if (quiet) {
	printf("%s\n", file_name(file));
	return;
    }

    in_transfer = 0;
    switch (file->type) {
    case file_dir:
	if (file->diff != file_new) {
	    printf(_("Creating %s/: "), file_name(file));
	} else {
	    printf(_("Deleting %s/: "), file_name(file));
	}
	break;
    case file_file:
	switch (file->diff) {
	case file_changed:
	case file_deleted: 
	    printf(_("Downloading %s: ["), file_name(file)); 
	    break;
	case file_new: 
	    printf(_("Deleting %s: "), file_name(file)); 
	    break;
	case file_moved: 
	    printf(_("Moving %s->%s: "), file->local.filename,
		   file->stored.filename); 
	    break;
	default: 
	    break;
	    }
    case file_link:
	/* TODO-ng */
	break;
    }
    fflush(stdout);

}

void fe_synched(const struct site_file *file, int success, const char *error) 
{
    if (quiet)
	return;
    if ((file->type == file_dir) || 
	(file->diff!=file_changed && file->diff!=file_deleted)) {
	if (success) {
	    printf(_("done.\n"));
	} else {
	    printf(_("failed:\n%s\n"), error);
	}
    } else {
	if (success) {
	    printf(_("] done.\n"));
	} else {
	    printf(_("] failed:\n%s\n"), error);
	}
    }
}

void fe_verified(const char *name, enum file_diff match)
{
    const char *state = "huh?";
    switch (match) {
    case file_changed:
	state = _("Changed on server");
	break;
    case file_new:
	state = _("Added on server");
	break;
    case file_unchanged:
    case file_deleted:
    case file_moved:
	return;
	break;
    }
    printf(_("%s: %s\n"), state, name);
}

void fe_transfer_progress(off_t num, off_t total) 
{
    if (quiet == 0) {
	putchar('.');
	fflush(stdout);
	in_transfer = 1;
    }
}

void fe_connection(fe_status status, const char *info) {}

void fe_fetch_found(const struct site_file *file) 
{
    switch (file->type)  {
    case file_dir:
	printf(_("Directory: %s/\n"), file->stored.filename);
	break;
    case file_file:
	/* TODO: could put checksum or modtime in here as appropriate */
	printf(_("File: %s - size %" NE_FMT_OFF_T "%s\n"), 
	       file->stored.filename, file->stored.size, 
	       file->stored.ascii?_(" (ASCII)"):"");
	break;
    case file_link:
	printf(_("Link: %s - target %s\n"), file->stored.filename,
	       file->stored.linktarget);
	break;
    }
}

static void init_sites(void)
{
    int ret;
    
    /* Read the rcfile */
    ret = rcfile_read(&all_sites);
    if (ret == RC_OPENFILE) {
	printf(_("%s: Error: Could not open rcfile: %s.\n"),
		progname, rcfile);
	exit(-1);
    } else if (ret == RC_CORRUPT) {
	printf(_("%s: rcfile corrupt at line %d:\n%s\n"),
		progname, rcfile_linenum, rcfile_err);
	exit(-1);
    }
}

/* Verify sites list. */
static int verify_sites(struct site *sites, enum action act) 
{
    int count = 0, ret;
    struct site *current;
    int isokay;

    for (current = sites; current!=NULL; current=current->next) {
	if (!current->use_this && !allsites) continue;
	/* Check the site rcfile entry is okay */
	ret = rcfile_verify(current);
	switch (ret) {
	case SITE_ACCESSLOCALDIR:
	    printf(_("%s: Could not read directory for `%s':\n\t%s\n"), 
		    progname, current->name, current->local_root);
	    break;
	case SITE_NOSERVER:
	    printf(_("%s: Server not specified in site `%s'.\n"), 
		    progname, current->name);
	    break;
	case SITE_NOREMOTEDIR:
	    printf(_("%s: Remote directory not specified in site `%s'.\n"), 
		    progname, current->name);
	    break;
	case SITE_NOLOCALDIR:
	    printf(_("%s: Local directory not specified in site `%s'.\n"), 
		    progname, current->name);
	    break;
	case SITE_INVALIDPORT:
	    printf(_("%s: Invalid port used in site `%s'.\n"),
		    progname, current->name);
	    break;
	case SITE_NOMAINTAIN:
	    printf(_("%s: %s cannot maintain symbolic links (site `%s').\n"),
		    progname, site_get_protoname(current), current->name);
	    break;
	case SITE_NOREMOTEREL:
	    printf(_("%s: Cannot use a relative remote directory in %s (site `%s').\n"), progname, site_get_protoname(current), current->name);
	    break;
	case SITE_NOPERMS:
	    printf(_("%s: File permissions are not supported in %s (site `%s').\n"), progname, site_get_protoname(current), current->name);
	    break;
	case SITE_NOSAFEOVER:
	    printf(_("%s: Safe mode cannot be used in conjunction with nooverwrite (site `%s').\n"), progname, current->name);
	    break;
	case SITE_NOSAFETEMPUP:
	    printf(_("%s: Safe mode cannot be used in conjunction with tempupload (site `%s').\n"), progname, current->name);
	    break;
	case SITE_NORENAMES:
	    printf(_("%s: Can only check for renamed files when checksumming (site `%s').\n"), progname, current->name);
	    break;
	case SITE_UNSUPPORTED:
	    printf(_("%s: The protocol `%s' is unsupported (site `%s').\n"),
		    progname, current->proto_string, current->name);
	    break;
	case 0:
	    /* Success */
	    break;
	default:
	    printf(_("%s: Unhandled error %d in site `%s' - please contact the maintainer.\n"), progname, ret, current->name);
	    break;
	}

	if (ret != 0) { 
	    isokay = false;
	} else {
	    isokay = true;
	}

	if (isokay && (actions[act].flags & A_LOCAL)) {
	    site_read_local_state(current);
	}

	if (isokay && (actions[act].flags & A_STORED)) {
	    ret = site_read_stored_state(current);
	    switch (ret) {
	    case SITE_ERRORS: 
		printf(_("%s: Error: Corrupt site storage file for `%s':\n%s: %s\n"),
			progname, current->name, progname, current->last_error);
		isokay = false;
		break;
	    case SITE_FAILED:
		if ((actions[act].flags & A_COND_STORED) == A_COND_STORED) {
		    break;
		} else {
		    printf(_(
			"%s: Error: No storage file for `%s'.\n"
			"%s: Use --init, --catchup or --fetch to create a storage file.\n"),
			   progname, current->name, progname);
		}
		isokay = false;
	    default:
		break;
	    }
	}

	if (isokay) {
	    count++;
	} else {
	    printf(_("%s: Skipping site `%s'.\n"), progname, current->name);
	    current->use_this = false;
	}

    }
    return count;
}

static int issue_error(struct site *site, enum action actno, int error) 
{
    struct action_info *act = &actions[actno];
    int ret;
    switch (error) {
    case SITE_OK:
	if (quiet == 0) {
	    printf(_("%s: %s completed successfully.\n"), progname, 
		   _(act->verb));
	}
	ret = 0;
	break;
    case SITE_UNSUPPORTED:
	printf(_("%s: %s unsupported for %s.\n"), progname, _(act->verb), 
		site_get_protoname(site));
	ret = 4;
	break;
    case SITE_ERRORS:
	printf(_("%s: Errors occurred while %s the %s site.\n"), progname,
		_(act->doing), _(act->subject));
	ret = 1;
	break;
    case SITE_LOOKUP:
	printf(_("%s: Error: Could not resolve remote hostname (%s).\n"),
		progname, site->server.hostname);
	ret = 2;
	break;
    case SITE_PROXYLOOKUP:
	printf(
	    _("%s: Error: Could not resolve hostname of proxy server (%s).\n"),
	    progname, site->proxy.hostname);
	ret = 2;
	break;
    case SITE_CONNECT:
	if (site->proxy.hostname) {
	    printf(_("%s: Error: Could not connect to proxy server (%s port %d).\n"),
		   progname, site->proxy.hostname, site->proxy.port);
	} else {
	    printf(_("%s: Error: Could not connect to server (%s port %d).\n"),
		   progname, site->server.hostname, site->server.port);
	}
	ret = 2;
	break;
    case SITE_AUTH:
	printf(_("%s: Error: Could not authorise user on server.\n"),
		progname);
	ret = 2;
	break;
    case SITE_FAILED:
	printf(_("%s: Error: %s\n"), progname, site->last_error);
	ret = 2;
	break;
    case SITE_PROXYAUTH:
	printf(_("%s: Error: Could not authorise user on proxy server (%s).\n"),
		progname, site->proxy.hostname);
	ret = 2;
	break;
    default:
	printf(_("%s: in issue_error\n%s"), progname, contact_mntr);
	ret = 5;
	break;
    }
    return ret;
}

static void init(int argc, char **argv) 
{
    int ret;

    parse_cmdline(argc, argv);

    if (init_env() == 1) {
	printf(_("%s: Error: Environment variable HOME not set.\n"),
		progname);
	exit(-1);
    }

    ret = init_paths();
    switch (ret) {
    case RC_OPENFILE:
	printf(_("%s: Error: Could not open rcfile: %s\n"), progname, rcfile);
	break;
    case RC_PERMS:
	printf(_("%s: Error: rcfile permissions allow other users to read your rcfile.\n"), progname);
	printf(_("%s: Set the permissions to 0600.\n"), progname);
	break;
    case RC_DIROPEN:
	printf(_("%s: Error: Could not open storage directory: %s\n"), progname, copypath);
	printf(_("%s: You need to create this directory and set the permissions to 0700.\n"), progname);
	break;
    case RC_DIRPERMS:
	printf(_("%s: Error: storage directory permissions incorrect.\n"), progname);
	printf(_("%s: Set the permissions to 0700.\n"), progname);
	break;
    case RC_NETRCPERMS:
	printf(_("%s: Error: ~/.netrc permissions incorrect.\n"), progname);
	printf(_("%s: Set the permissions to 0600.\n"), progname);
	break;
    case 0:
	break;
    default:
	printf(_("%s: init_paths gave %d\n%s"), progname, ret,
		contact_mntr);
	break;
    }
    if (ret != 0) exit(-1);

    if (init_netrc() == 1) {
	printf(_("%s: Error: Could not parse ~/.netrc.\n"), progname);
	exit(-1);
    }

    init_sites();
}

static int act_on_site(struct site *site, enum action act) 
{
    int ret = 0, verify_removed;

    /* Set the options */
    site->keep_going = keepgoing;

    switch (act) {
    case action_update:
	if (!site->remote_is_different) {
	    if (quiet == 0) {
		printf(_("%s: Nothing to do - no changes found.\n"), 
		       progname);
	    }
	} else if (dry_run) {
            extern const struct proto_driver null_driver;
            site->driver = &null_driver;
            ret = issue_error(site, act, site_update(site));
        } else {
	    if (site_open_storage_file(site) == NULL) {
		printf(_("%s: Error: Could not open storage file for writing (%s)\n"
			  "%s: Error: %s\n"
			  "%s: Will not update site `%s'.\n"), 
			progname, site->infofile,
			progname, strerror(errno),
			progname, site->name);
		ret = -1;
	    } else {
		current_site = site;
		upload_total = site->totalchanged + site->totalnew;
		upload_sofar = 0;
		ret = issue_error(site, act, site_update(site));
		/* hope we don't get signalled here */
		current_site = NULL;
		site_write_stored_state(site);
	    }
	}
   break;
    case action_autoupdate:
      printf(_("%s: Testing autoupdate.\n"), 
             progname);

            extern const struct proto_driver null_driver;
            site->driver = &null_driver;
            ret = issue_error(site, act, site_autoupdate(site));
        
	break;
    case action_list:
	if (listflat) {
	    site_flatlist(stdout, site);
	} else {
	    list_site_changes(site);
	    if (site->state_method != site->stored_state_method) {
		printf(_("%s: Warning: Current state method differs from stored in site `%s'.\n%s: All existing files will appear changed (use catchup?).\n"), progname, site->name, progname);
	    }
	    if (site->remote_is_different) {
		int count = site->numchanged + site->numdeleted + 
		    site->nummoved + site->numnew;
		if (count > 1) {
		    printf(_("%s: The remote site needs updating (%d items to update).\n"), progname, count);
		} else {
		    printf(_("%s: The remote site needs updating (1 item to update).\n"), progname);
		}
	    } else {
		printf(_("%s: The remote site does not need updating.\n"), progname);
	    }
	}
	ret = site->remote_is_different?1:0;
	break;
    case action_init:
	site_initialize(site);
	site_write_stored_state(site);
	printf(_("%s: All the files and directories are marked as NOT updated remotely.\n"), progname);
	break;
    case action_catchup:
	site_catchup(site);
	site_write_stored_state(site);
	printf(_("%s: All the files and and directories are marked as updated remotely.\n"), progname);
	break;
    case action_fetch:
	ret = site_fetch(site);
	if (ret == SITE_OK) {
	    site_write_stored_state(site);
	}	    
	switch (ret) {
	case SITE_FAILED:
	    printf(_("%s: Failed to fetch file listing for site `%s':\n"
		     "%s: %s\n"), progname, site->name,
		   progname, site->last_error);
	    ret = -1;
	    break;
	default:
	    ret = issue_error(site, act, ret);
	    break;
	} 
	break;
    case action_verify:
	ret = site_verify(site, &verify_removed);
	switch (ret) {
	case SITE_FAILED:
	    printf(_("%s: Failed to fetch file listing to verify site `%s':\n"
		     "%s: %s\n"), progname, site->name, 
		   progname, site->last_error);
	    ret = -1;
	    break;
	case SITE_ERRORS:
	    if (verify_removed > 0) {
		printf(_("%s: Verify found %d files missing from server.\n"),
		       progname, verify_removed);
	    }
	    printf(_("%s: Remote site not synchronized with stored state.\n"), progname);
	    break;
	default:
	    ret = issue_error(site, act, ret);
	    break;
	} 
	break;
    case action_synch:
	if (!site->local_is_different) {
	    printf(_("%s: Nothing to do - no changes found.\n"), progname);
	} else if (site->numunchanged == 0 && site->numdeleted == 0 &&
		   site->nummoved == 0 && site->numchanged == 0 && 
		   site->numignored == 0) {
	    printf(_("%s: Refusing to delete all local files with a synchronize operation.\n"
		      "%s: Use --update to update the remote site.\n"),
		    progname, progname);
	} else {
	    ret = issue_error(site, act, site_synch(site));
	}
	break;
    default:
	printf(_("%s: in act_on_site\n%s"), progname, contact_mntr);
	ret = -1;
	break;
    }
    return ret;
}


static void usage(void) 
{
    version();
    printf(_("Usage: %s [OPTIONS] [MODE] [sitename]...\n"),
	     progname);
    printf(_("Options: \n"));
#ifdef NE_DEBUGGING
    printf(_(
"  -d, --debug=KEY[,KEY] Turn debugging on for each KEY, which may be:\n"
"     socket, files, rcfile, ftp, http, httpbody, rsh, sftp, xml, xmlparse, cleartext\n"
"     Warning: cleartext displays (normally hidden) passwords in plain text\n"
"  -g, --logfile=FILE    Append debugging messages to FILE (else use stderr)\n"
));
#endif
    printf(_(
"  -r, --rcfile=FILE     Use alternate run control file\n"
"  -p, --storepath=PATH  Use alternate site storage directory\n"
"  -y, --prompting       Request confirmation before making each update\n"
"  -a, --allsites        Perform the operation on ALL defined sites\n"
"  -k, --keep-going      Carry on an update regardless of errors\n"
"  -o, --show-progress   Display total percentage file transfer complete\n"
"  -q, --quiet           Be quiet while performing the operation\n"
"  -qq, --silent         Be silent while perforing the operation\n"
"  -n, --dry-run         Display but do not carry out the operation\n"
"Operation modes:\n"
"  -l, --list            List changes between remote and local sites (default)\n"
"  -ll, --flatlist       Flat list of changes between remote and local sites\n"
"  -v, --view            Display a list of the site definitions\n"
"  -i, --initialize      Mark all files and directories as not updated\n"
"  -f, --fetch           Find out what files are on the remote site\n"
"  -e, --verify          Verify stored state of site matches real remote state\n"
"  -c, --catchup         Mark all files and directories as updated\n"
"  -s, --synchronize     Update the local site from the remote site\n"
"  -u, --update          Update the remote site\n"
"  -h, --help            Display this help message\n"
"  -V, --version         Display version information\n"
"Please send feature requests and bug reports to sitecopy@lyra.org\n"));
}


/* Two-liner version information. */
static void version(void) 
{
    printf(PACKAGE_NAME " " PACKAGE_VERSION ":");
#ifdef USE_FTP
    printf(" FTP");
#endif /* FTP */
#ifdef USE_DAV
    printf(" WebDAV");
#endif /* USE_DAV */
#ifdef USE_RSH
    printf(" rsh/rcp");
#endif /* USE_RSH */
#ifdef USE_SFTP
    printf(" sftp/ssh");
#endif /* USE_SFTP */
#ifdef NE_DEBUGGING
    printf(", debugging");
#endif
#ifdef __EMX__
    printf(", EMX/RSX");
#else
#ifdef __CYGWIN__
    printf(", cygwin");
#else
    printf(", Unix");
#endif /* __CYGWIN__ */
#endif /* __EMX__ */
    printf(" platform.\n");
}
