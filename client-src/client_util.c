/*
 * Amanda, The Advanced Maryland Automatic Network Disk Archiver
 * Copyright (c) 1991-1998 University of Maryland at College Park
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of U.M. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  U.M. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * U.M. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL U.M.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors: the Amanda Development Team.  Its members are listed in a
 * file named AUTHORS, in the root directory of this distribution.
 */
/* 
 * $Id: client_util.c,v 1.34 2006/05/25 01:47:11 johnfranks Exp $
 *
 */

#include "amanda.h"
#include "conffile.h"
#include "client_util.h"
#include "getfsent.h"
#include "util.h"
#include "timestamp.h"
#include "pipespawn.h"
#include "amxml.h"
#include "glob.h"

#define MAXMAXDUMPS 16

static int add_exclude(FILE *file_exclude, char *aexc, int verbose);
static int add_include(char *disk, char *device, FILE *file_include, char *ainc, int verbose);
static char *build_name(char *disk, char *exin, int verbose);
static char *get_name(char *diskname, char *exin, time_t t, int n);


char *
fixup_relative(
    char *	name,
    char *	device)
{
    char *newname;
    if(*name != '/') {
	char *dirname = amname_to_dirname(device);
	newname = vstralloc(dirname, "/", name , NULL);
	amfree(dirname);
    }
    else {
	newname = stralloc(name);
    }
    return newname;
}


static char *
get_name(
    char *	diskname,
    char *	exin,
    time_t	t,
    int		n)
{
    char number[NUM_STR_SIZE];
    char *filename;
    char *ts;

    ts = get_timestamp_from_time(t);
    if(n == 0)
	number[0] = '\0';
    else
	g_snprintf(number, SIZEOF(number), "%03d", n - 1);
	
    filename = vstralloc(get_pname(), ".", diskname, ".", ts, number, ".",
			 exin, NULL);
    amfree(ts);
    return filename;
}


static char *
build_name(
    char *	disk,
    char *	exin,
    int		verbose)
{
    int n;
    int fd;
    char *filename = NULL;
    char *afilename = NULL;
    char *diskname;
    time_t curtime;
    char *dbgdir;
    char *e = NULL;
    DIR *d;
    struct dirent *entry;
    char *test_name;
    size_t match_len, d_name_len;
    char *quoted;

    time(&curtime);
    diskname = sanitise_filename(disk);

    dbgdir = stralloc2(AMANDA_TMPDIR, "/");
    if((d = opendir(AMANDA_TMPDIR)) == NULL) {
	error(_("open debug directory \"%s\": %s"),
		AMANDA_TMPDIR, strerror(errno));
	/*NOTREACHED*/
    }
    test_name = get_name(diskname, exin,
			 curtime - (AMANDA_DEBUG_DAYS * 24 * 60 * 60), 0);
    match_len = strlen(get_pname()) + strlen(diskname) + 2;
    while((entry = readdir(d)) != NULL) {
	if(is_dot_or_dotdot(entry->d_name)) {
	    continue;
	}
	d_name_len = strlen(entry->d_name);
	if(strncmp(test_name, entry->d_name, match_len) != 0
	   || d_name_len < match_len + 14 + 8
	   || strcmp(entry->d_name+ d_name_len - 7, exin) != 0) {
	    continue;				/* not one of our files */
	}
	if(strcmp(entry->d_name, test_name) < 0) {
	    e = newvstralloc(e, dbgdir, entry->d_name, NULL);
	    (void) unlink(e);                   /* get rid of old file */
	}
    }
    amfree(test_name);
    amfree(e);
    closedir(d);

    n=0;
    do {
	filename = get_name(diskname, exin, curtime, n);
	afilename = newvstralloc(afilename, dbgdir, filename, NULL);
	if((fd=open(afilename, O_WRONLY|O_CREAT|O_APPEND, 0600)) < 0){
	    amfree(afilename);
	    n++;
	}
	else {
	    close(fd);
	}
	amfree(filename);
    } while(!afilename && n < 1000);

    if(afilename == NULL) {
	filename = get_name(diskname, exin, curtime, 0);
	afilename = newvstralloc(afilename, dbgdir, filename, NULL);
	quoted = quote_string(afilename);
	dbprintf(_("Cannot create %s (%s)\n"), quoted, strerror(errno));
	if(verbose) {
	    g_printf(_("ERROR [cannot create %s (%s)]\n"),
			quoted, strerror(errno));
	}
	amfree(quoted);
	amfree(afilename);
	amfree(filename);
    }

    amfree(dbgdir);
    amfree(diskname);

    return afilename;
}


static int
add_exclude(
    FILE *	file_exclude,
    char *	aexc,
    int		verbose)
{
    size_t l;
    char *quoted, *file;

    (void)verbose;	/* Quiet unused parameter warning */

    l = strlen(aexc);
    if(aexc[l-1] == '\n') {
	aexc[l-1] = '\0';
	l--;
    }
    file = quoted = quote_string(aexc);
    if (*file == '"') {
	file[strlen(file) - 1] = '\0';
	file++;
    }
    g_fprintf(file_exclude, "%s\n", file);
    amfree(quoted);
    return 1;
}

static int
add_include(
    char *	disk,
    char *	device,
    FILE *	file_include,
    char *	ainc,
    int		verbose)
{
    size_t l;
    int nb_exp=0;
    char *quoted, *file;

    (void)disk;		/* Quiet unused parameter warning */
    (void)device;	/* Quiet unused parameter warning */

    l = strlen(ainc);
    if(ainc[l-1] == '\n') {
	ainc[l-1] = '\0';
	l--;
    }
    if (strncmp(ainc, "./", 2) != 0) {
        quoted = quote_string(ainc);
        dbprintf(_("include must start with './' (%s)\n"), quoted);
	if(verbose) {
	    g_printf(_("ERROR [include must start with './' (%s)]\n"), quoted);
	}
	amfree(quoted);
    }
    else {
	char *incname = ainc+2;
	int set_root;

        set_root = set_root_privs(1);
	/* Take as is if not root && many '/' */
	if(!set_root && strchr(incname, '/')) {
            file = quoted = quote_string(ainc);
	    if (*file == '"') {
		file[strlen(file) - 1] = '\0';
		file++;
	    }
	    g_fprintf(file_include, "%s\n", file);
	    amfree(quoted);
	    nb_exp++;
	}
	else {
	    int nb;
	    glob_t globbuf;
	    char *cwd;

	    globbuf.gl_offs = 0;

	    cwd = g_get_current_dir();
	    if (chdir(device) != 0) {
		error(_("Failed to chdir(%s): %s\n"), device, strerror(errno));
	    }
	    glob(incname, 0, NULL, &globbuf);
	    if (chdir(cwd) != 0) {
		error(_("Failed to chdir(%s): %s\n"), cwd, strerror(errno));
	    }
	    if (set_root)
		set_root_privs(0);
	    nb_exp = globbuf.gl_pathc;
	    for (nb=0; nb < nb_exp; nb++) {
		file = stralloc2("./", globbuf.gl_pathv[nb]);
		quoted = quote_string(file);
		if (*file == '"') {
		    file[strlen(file) - 1] = '\0';
		    file++;
		}
		g_fprintf(file_include, "%s\n", file);
		amfree(quoted);
		amfree(file);
	    }
	}
    }
    return nb_exp;
}

char *
build_exclude(
    dle_t   *dle,
    int	     verbose)
{
    char *filename;
    FILE *file_exclude;
    FILE *exclude;
    char *aexc;
    sle_t *excl;
    int nb_exclude = 0;
    char *quoted;

    if (dle->exclude_file) nb_exclude += dle->exclude_file->nb_element;
    if (dle->exclude_list) nb_exclude += dle->exclude_list->nb_element;

    if (nb_exclude == 0) return NULL;

    if ((filename = build_name(dle->disk, "exclude", verbose)) != NULL) {
	if ((file_exclude = fopen(filename,"w")) != NULL) {

	    if (dle->exclude_file) {
		for(excl = dle->exclude_file->first; excl != NULL;
		    excl = excl->next) {
		    add_exclude(file_exclude, excl->name,
				verbose && dle->exclude_optional == 0);
		}
	    }

	    if (dle->exclude_list) {
		for(excl = dle->exclude_list->first; excl != NULL;
		    excl = excl->next) {
		    char *exclname = fixup_relative(excl->name, dle->device);
		    if((exclude = fopen(exclname, "r")) != NULL) {
			while ((aexc = agets(exclude)) != NULL) {
			    if (aexc[0] == '\0') {
				amfree(aexc);
				continue;
			    }
			    add_exclude(file_exclude, aexc,
				        verbose && dle->exclude_optional == 0);
			    amfree(aexc);
			}
			fclose(exclude);
		    }
		    else {
			quoted = quote_string(exclname);
			dbprintf(_("Can't open exclude file %s (%s)\n"),
				  quoted, strerror(errno));
			if(verbose && (dle->exclude_optional == 0 ||
				       errno != ENOENT)) {
			    g_printf(_("ERROR [Can't open exclude file %s (%s)]\n"),
				   quoted, strerror(errno));
			}
			amfree(quoted);
		    }
		    amfree(exclname);
		}
	    }
            fclose(file_exclude);
	} else {
	    quoted = quote_string(filename);
	    dbprintf(_("Can't create exclude file %s (%s)\n"),
		      quoted, strerror(errno));
	    if (verbose) {
		g_printf(_("ERROR [Can't create exclude file %s (%s)]\n"),
			quoted, strerror(errno));
	    }
	    amfree(quoted);
	}
    }

    return filename;
}

char *
build_include(
    dle_t   *dle,
    int	     verbose)
{
    char *filename;
    FILE *file_include;
    FILE *include;
    char *ainc = NULL;
    sle_t *incl;
    int nb_include = 0;
    int nb_exp = 0;
    char *quoted;

    if (dle->include_file) nb_include += dle->include_file->nb_element;
    if (dle->include_list) nb_include += dle->include_list->nb_element;

    if (nb_include == 0) return NULL;

    if ((filename = build_name(dle->disk, "include", verbose)) != NULL) {
	if ((file_include = fopen(filename,"w")) != NULL) {

	    if (dle->include_file) {
		for (incl = dle->include_file->first; incl != NULL;
		    incl = incl->next) {
		    nb_exp += add_include(dle->disk, dle->device, file_include,
				  incl->name,
				  verbose && dle->include_optional == 0);
		}
	    }

	    if (dle->include_list) {
		for (incl = dle->include_list->first; incl != NULL;
		    incl = incl->next) {
		    char *inclname = fixup_relative(incl->name, dle->device);
		    if ((include = fopen(inclname, "r")) != NULL) {
			while ((ainc = agets(include)) != NULL) {
			    if (ainc[0] == '\0') {
				amfree(ainc);
				continue;
			    }
			    nb_exp += add_include(dle->disk, dle->device,
						  file_include, ainc,
						  verbose && dle->include_optional == 0);
			    amfree(ainc);
			}
			fclose(include);
		    }
		    else {
			quoted = quote_string(inclname);
			dbprintf(_("Can't open include file %s (%s)\n"),
				  quoted, strerror(errno));
			if (verbose && (dle->include_optional == 0 ||
				       errno != ENOENT)) {
			    g_printf(_("ERROR [Can't open include file %s (%s)]\n"),
				   quoted, strerror(errno));
			}
			amfree(quoted);
		   }
		   amfree(inclname);
		}
	    }
            fclose(file_include);
	} else {
	    quoted = quote_string(filename);
	    dbprintf(_("Can't create include file %s (%s)\n"),
		      quoted, strerror(errno));
	    if (verbose) {
		g_printf(_("ERROR [Can't create include file %s (%s)]\n"),
			quoted, strerror(errno));
	    }
	    amfree(quoted);
	}
    }
	
    if (nb_exp == 0) {
	quoted = quote_string(dle->disk);
	dbprintf(_("No include for %s\n"), quoted);
	if (verbose && dle->include_optional == 0) {
	    g_printf(_("ERROR [No include for %s]\n"), quoted);
	}
	amfree(quoted);
    }

    return filename;
}


void
parse_options(
    char         *str,
    dle_t        *dle,
    am_feature_t *fs,
    int           verbose)
{
    char *exc;
    char *inc;
    char *p, *tok;
    char *quoted;

    p = stralloc(str);
    tok = strtok(p,";");

    while (tok != NULL) {
	if(am_has_feature(fs, fe_options_auth)
	   && BSTRNCMP(tok,"auth=") == 0) {
	    if (dle->auth != NULL) {
		quoted = quote_string(tok + 5);
		dbprintf(_("multiple auth option %s\n"), quoted);
		if(verbose) {
		    g_printf(_("ERROR [multiple auth option %s]\n"), quoted);
		}
		amfree(quoted);
	    }
	    dle->auth = stralloc(&tok[5]);
	}
	else if(am_has_feature(fs, fe_options_bsd_auth)
	   && BSTRNCMP(tok, "bsd-auth") == 0) {
	    if (dle->auth != NULL) {
		dbprintf(_("multiple auth option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple auth option]\n"));
		}
	    }
	    dle->auth = stralloc("bsd");
	}
	else if (am_has_feature(fs, fe_options_krb4_auth)
	   && BSTRNCMP(tok, "krb4-auth") == 0) {
	    if (dle->auth != NULL) {
		dbprintf(_("multiple auth option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple auth option]\n"));
		}
	    }
	    dle->auth = stralloc("krb4");
	}
	else if (BSTRNCMP(tok, "compress-fast") == 0) {
	    if (dle->compress != COMP_NONE) {
		dbprintf(_("multiple compress option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple compress option]\n"));
		}
	    }
	    dle->compress = COMP_FAST;
	}
	else if (BSTRNCMP(tok, "compress-best") == 0) {
	    if (dle->compress != COMP_NONE) {
		dbprintf(_("multiple compress option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple compress option]\n"));
		}
	    }
	    dle->compress = COMP_BEST;
	}
	else if (BSTRNCMP(tok, "srvcomp-fast") == 0) {
	    if (dle->compress != COMP_NONE) {
		dbprintf(_("multiple compress option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple compress option]\n"));
		}
	    }
	    dle->compress = COMP_SERVER_FAST;
	}
	else if (BSTRNCMP(tok, "srvcomp-best") == 0) {
	    if (dle->compress != COMP_NONE) {
		dbprintf(_("multiple compress option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple compress option]\n"));
		}
	    }
	    dle->compress = COMP_SERVER_BEST;
	}
	else if (BSTRNCMP(tok, "srvcomp-cust=") == 0) {
	    if (dle->compress != COMP_NONE) {
		dbprintf(_("multiple compress option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple compress option]\n"));
		}
	    }
	    dle->compprog = stralloc(tok + SIZEOF("srvcomp-cust=") -1);
	    dle->compress = COMP_SERVER_CUST;
	}
	else if (BSTRNCMP(tok, "comp-cust=") == 0) {
	    if (dle->compress != COMP_NONE) {
		dbprintf(_("multiple compress option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple compress option]\n"));
		}
	    }
	    dle->compprog = stralloc(tok + SIZEOF("comp-cust=") -1);
	    dle->compress = COMP_CUST;
	    /* parse encryption options */
	} 
	else if (BSTRNCMP(tok, "encrypt-serv-cust=") == 0) {
	    if (dle->encrypt != ENCRYPT_NONE) {
		dbprintf(_("multiple encrypt option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple encrypt option]\n"));
		}
	    }
	    dle->srv_encrypt = stralloc(tok + SIZEOF("encrypt-serv-cust=") -1);
	    dle->encrypt = ENCRYPT_SERV_CUST;
	} 
	else if (BSTRNCMP(tok, "encrypt-cust=") == 0) {
	    if (dle->encrypt != ENCRYPT_NONE) {
		dbprintf(_("multiple encrypt option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple encrypt option]\n"));
		}
	    }
	    dle->clnt_encrypt= stralloc(tok + SIZEOF("encrypt-cust=") -1);
	    dle->encrypt = ENCRYPT_CUST;
	} 
	else if (BSTRNCMP(tok, "server-decrypt-option=") == 0) {
	  dle->srv_decrypt_opt = stralloc(tok + SIZEOF("server-decrypt-option=") -1);
	}
	else if (BSTRNCMP(tok, "client-decrypt-option=") == 0) {
	  dle->clnt_decrypt_opt = stralloc(tok + SIZEOF("client-decrypt-option=") -1);
	}
	else if (BSTRNCMP(tok, "no-record") == 0) {
	    if (dle->record != 1) {
		dbprintf(_("multiple no-record option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple no-record option]\n"));
		}
	    }
	    dle->record = 0;
	}
	else if (BSTRNCMP(tok, "index") == 0) {
	    if (dle->create_index != 0) {
		dbprintf(_("multiple index option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple index option]\n"));
		}
	    }
	    dle->create_index = 1;
	}
	else if (BSTRNCMP(tok, "exclude-optional") == 0) {
	    if (dle->exclude_optional != 0) {
		dbprintf(_("multiple exclude-optional option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple exclude-optional option]\n"));
		}
	    }
	    dle->exclude_optional = 1;
	}
	else if (strcmp(tok, "include-optional") == 0) {
	    if (dle->include_optional != 0) {
		dbprintf(_("multiple include-optional option\n"));
		if (verbose) {
		    g_printf(_("ERROR [multiple include-optional option]\n"));
		}
	    }
	    dle->include_optional = 1;
	}
	else if (BSTRNCMP(tok,"exclude-file=") == 0) {
	    exc = unquote_string(&tok[13]);
	    dle->exclude_file = append_sl(dle->exclude_file, exc);
	    amfree(exc);
	}
	else if (BSTRNCMP(tok,"exclude-list=") == 0) {
	    exc = unquote_string(&tok[13]);
	    dle->exclude_list = append_sl(dle->exclude_list, exc);
	    amfree(exc);
	}
	else if (BSTRNCMP(tok,"include-file=") == 0) {
	    inc = unquote_string(&tok[13]);
	    dle->include_file = append_sl(dle->include_file, inc);
	    amfree(inc);
	}
	else if (BSTRNCMP(tok,"include-list=") == 0) {
	    inc = unquote_string(&tok[13]);
	    dle->include_list = append_sl(dle->include_list, inc);
	    amfree(inc);
	}
	else if (BSTRNCMP(tok,"kencrypt") == 0) {
	    dle->kencrypt = 1;
	}
	else if (strcmp(tok,"|") != 0) {
	    quoted = quote_string(tok);
	    dbprintf(_("unknown option %s\n"), quoted);
	    if (verbose) {
		g_printf(_("ERROR [unknown option: %s]\n"), quoted);
	    }
	    amfree(quoted);
	}
	tok = strtok(NULL, ";");
    }
    amfree(p);
}

void
output_tool_property(
    FILE  *tool,
    dle_t *dle)
{
    sle_t *sle;
    char *q;

    if (!is_empty_sl(dle->exclude_file)) {
	for(sle = dle->exclude_file->first ; sle != NULL; sle=sle->next) {
	    q = quote_string(sle->name);
	    g_fprintf(tool, "EXCLUDE-FILE %s\n", q);
	    amfree(q);
	}
    }

    if (!is_empty_sl(dle->exclude_list)) {
	for(sle = dle->exclude_list->first ; sle != NULL; sle=sle->next) {
	    q = quote_string(sle->name);
	    g_fprintf(tool, "EXCLUDE-LIST %s\n", q);
	    amfree(q);
	}
    }

    if (!is_empty_sl(dle->include_file)) {
	for(sle = dle->include_file->first ; sle != NULL; sle=sle->next) {
	    q = quote_string(sle->name);
	    g_fprintf(tool, "INCLUDE-FILE %s\n", q);
	    amfree(q);
	}
    }

    if (!is_empty_sl(dle->include_list)) {
	for(sle = dle->include_list->first ; sle != NULL; sle=sle->next) {
	    q = quote_string(sle->name);
	    g_fprintf(tool, "INCLUDE-LIST %s\n", q);
	    amfree(q);
	}
    }

    if (!is_empty_sl(dle->exclude_file) ||
	!is_empty_sl(dle->exclude_list)) {
	if (dle->exclude_optional)
	    g_fprintf(tool, "EXCLUDE-OPTIONAL YES\n");
	else
	    g_fprintf(tool, "EXCLUDE-OPTIONAL NO\n");
    }

    if (!is_empty_sl(dle->include_file) ||
	!is_empty_sl(dle->include_list)) {
	if (dle->include_optional)
	    g_fprintf(tool, "INCLUDE-OPTIONAL YES\n");
	else
	    g_fprintf(tool, "INCLUDE-OPTIONAL NO\n");
    }

    g_hash_table_foreach(dle->application_property,
			 &output_tool_proplist,
			 tool);
}

int
application_property_argv_size(dle_t *dle) {
    int nb;

    nb = 0;
    if (dle->include_list)
	nb += dle->include_list->nb_element;
    if (dle->include_file)
	nb += dle->include_file->nb_element;
    nb++;
    if (dle->exclude_list)
	nb += dle->exclude_list->nb_element;
    if (dle->exclude_file)
	nb += dle->exclude_file->nb_element;
    nb++;
    nb *= 2;  /*name + value */
    nb += property_argv_size(dle->application_property);

    return nb;
}

int
application_property_add_to_argv(
    char **argvchild,
    dle_t *dle)
{
    char **argv = argvchild;

    g_hash_table_foreach(dle->application_property,
			&proplist_add_to_argv, &argv);
    return (argv - argvchild);
}

backup_support_option_t *
backup_support_option(
    char       *program,
    g_option_t *g_options,
    char       *disk,
    char       *amdevice)
{
    pid_t   supportpid;
    int     supportin, supportout, supporterr;
    char   *cmd;
    char  **argvchild;
    int     i;
    FILE   *streamout;
    char   *line;
    backup_support_option_t *bsu;

    cmd = vstralloc(APPLICATION_DIR, "/", program, NULL);
    argvchild = malloc(12 * SIZEOF(char *));
    i = 0;
    argvchild[i++] = program;
    argvchild[i++] = "support";
    if (g_options->config) {
	argvchild[i++] = "--config";
	argvchild[i++] = g_options->config;
    }
    if (g_options->hostname) {
	argvchild[i++] = "--host";
	argvchild[i++] = g_options->hostname;
    }
    if (disk) {
	argvchild[i++] = "--disk";
	argvchild[i++] = disk;
    }
    if (amdevice) {
	argvchild[i++] = "--device";
	argvchild[i++] = stralloc(amdevice);
    }
    argvchild[i++] = NULL;

    supporterr = fileno(stderr);
    supportpid = pipespawnv(cmd, STDIN_PIPE|STDOUT_PIPE, 0, &supportin,
			    &supportout, &supporterr, argvchild);

    aclose(supportin);

    bsu = malloc(SIZEOF(*bsu));
    memset(bsu, '\0', SIZEOF(*bsu));
    bsu->config = 1;
    bsu->host = 1;
    bsu->disk = 1;
    streamout = fdopen(supportout, "r");
    while((line = agets(streamout)) != NULL) {
	dbprintf(_("support line: %s\n"), line);
	if (strncmp(line,"CONFIG ", 7) == 0) {
	    if (strcmp(line+7, "YES") == 0)
		bsu->config = 1;
	} else if (strncmp(line,"HOST ", 5) == 0) {
	    if (strcmp(line+5, "YES") == 0)
	    bsu->host = 1;
	} else if (strncmp(line,"DISK ", 5) == 0) {
	    if (strcmp(line+5, "YES") == 0)
		bsu->disk = 1;
	} else if (strncmp(line,"INDEX-LINE ", 11) == 0) {
	    if (strcmp(line+11, "YES") == 0)
		bsu->index_line = 1;
	} else if (strncmp(line,"INDEX-XML ", 10) == 0) {
	    if (strcmp(line+10, "YES") == 0)
		bsu->index_xml = 1;
	} else if (strncmp(line,"MESSAGE-LINE ", 13) == 0) {
	    if (strcmp(line+13, "YES") == 0)
		bsu->message_line = 1;
	} else if (strncmp(line,"MESSAGE-XML ", 12) == 0) {
	    if (strcmp(line+12, "YES") == 0)
		bsu->message_xml = 1;
	} else if (strncmp(line,"RECORD ", 7) == 0) {
	    if (strcmp(line+7, "YES") == 0)
		bsu->record = 1;
	} else if (strncmp(line,"INCLUDE-FILE ", 13) == 0) {
	    if (strcmp(line+13, "YES") == 0)
		bsu->include_file = 1;
	} else if (strncmp(line,"INCLUDE-LIST ", 13) == 0) {
	    if (strcmp(line+13, "YES") == 0)
		bsu->include_list = 1;
	} else if (strncmp(line,"EXCLUDE-FILE ", 13) == 0) {
	    if (strcmp(line+13, "YES") == 0)
		bsu->exclude_file = 1;
	} else if (strncmp(line,"EXCLUDE-LIST ", 13) == 0) {
	    if (strcmp(line+13, "YES") == 0)
		bsu->exclude_list = 1;
	} else if (strncmp(line,"COLLECTION ", 11) == 0) {
	    if (strcmp(line+11, "YES") == 0)
		bsu->collection = 1;
	} else if (strncmp(line,"MAX-LEVEL ", 10) == 0) {
	    bsu->max_level  = atoi(line+10);
	} else {
	    dbprintf(_("Invalid support line: %s\n"), line);
	}
	amfree(line);
    }
    aclose(supportout);

    return bsu;
}

void
run_client_script(
    script_t     *script,
    execute_on_t  execute_on,
    g_option_t   *g_options,
    dle_t	 *dle)
{
    pid_t   scriptpid;
    int     scriptin, scriptout, scripterr;
    char   *cmd;
    char  **argvchild;
    int     i, k;
    FILE   *streamout;
    char   *line;

    if ((script->execute_on & execute_on) == 0)
	return;
    if (script->execute_where != ES_CLIENT)
	return;

    cmd = vstralloc(APPLICATION_DIR, "/", script->plugin, NULL);
    k = property_argv_size(script->property);
    argvchild = malloc((12+k) * SIZEOF(char *));
    i = 0;
    argvchild[i++] = script->plugin;

    switch (execute_on) {
    case EXECUTE_ON_PRE_DLE_AMCHECK:
	argvchild[i++] = "PRE-DLE-AMCHECK"; break;
    case EXECUTE_ON_PRE_HOST_AMCHECK:
	argvchild[i++] = "PRE-HOST-AMCHECK"; break;
    case EXECUTE_ON_POST_DLE_AMCHECK:
	argvchild[i++] = "POST-DLE-AMCHECK"; break;
    case EXECUTE_ON_POST_HOST_AMCHECK:
	argvchild[i++] = "POST-HOST-AMCHECK"; break;
    case EXECUTE_ON_PRE_DLE_ESTIMATE:
	argvchild[i++] = "PRE-DLE-ESTIMATE"; break;
    case EXECUTE_ON_PRE_HOST_ESTIMATE:
	argvchild[i++] = "PRE-HOST-ESTIMATE"; break;
    case EXECUTE_ON_POST_DLE_ESTIMATE:
	argvchild[i++] = "POST-DLE-ESTIMATE"; break;
    case EXECUTE_ON_POST_HOST_ESTIMATE:
	argvchild[i++] = "POST-HOST-ESTIMATE"; break;
    case EXECUTE_ON_PRE_DLE_BACKUP:
	argvchild[i++] = "PRE-DLE-BACKUP"; break;
    case EXECUTE_ON_PRE_HOST_BACKUP:
	argvchild[i++] = "PRE-HOST-BACKUP"; break;
    case EXECUTE_ON_POST_DLE_BACKUP:
	argvchild[i++] = "POST-DLE-BACKUP"; break;
    case EXECUTE_ON_POST_HOST_BACKUP:
	argvchild[i++] = "POST-HOST-BACKUP"; break;
    }

    if (g_options->config) {
	argvchild[i++] = "--config";
	argvchild[i++] = g_options->config;
    }
    if (g_options->hostname) {
	argvchild[i++] = "--host";
	argvchild[i++] = g_options->hostname;
    }
    if (dle->disk) {
	argvchild[i++] = "--disk";
	argvchild[i++] = dle->disk;
    }
    if (dle->device) {
	argvchild[i++] = "--device";
	argvchild[i++] = stralloc(dle->device);
    }
    i += property_add_to_argv(&argvchild[i], script->property);
    argvchild[i++] = NULL;

    scripterr = fileno(stderr);
    scriptpid = pipespawnv(cmd, STDIN_PIPE|STDOUT_PIPE, 0, &scriptin,
			   &scriptout, &scripterr, argvchild);

    close(scriptin);

    streamout = fdopen(scriptout, "r");
    if (streamout) {
	while((line = agets(streamout)) != NULL) {
	    dbprintf("script: %s\n", line);
	}
    }
    fclose(streamout);
    waitpid(scriptpid, NULL, 0);
}

void
run_client_scripts(
    execute_on_t  execute_on,
    g_option_t   *g_options,
    dle_t	 *dle)
{
    GSList   *scriptlist;

    for (scriptlist = dle->scriptlist; scriptlist != NULL;
	 scriptlist = scriptlist->next) {
	run_client_script(scriptlist->data, execute_on, g_options, dle);
    }
}

void
check_access(
    char *	filename,
    int		mode)
{
    char *noun, *adjective;
    char *quoted = quote_string(filename);

    if(mode == F_OK)
        noun = "find", adjective = "exists";
    else if((mode & X_OK) == X_OK)
	noun = "execute", adjective = "executable";
    else if((mode & (W_OK|R_OK)) == (W_OK|R_OK))
	noun = "read/write", adjective = "read/writable";
    else 
	noun = "access", adjective = "accessible";

    if(access(filename, mode) == -1)
	g_printf(_("ERROR [can not %s %s: %s]\n"), noun, quoted, strerror(errno));
    else
	g_printf(_("OK %s %s\n"), quoted, adjective);
    amfree(quoted);
}

void
check_file(
    char *	filename,
    int		mode)
{
    struct stat stat_buf;
    char *quoted;

    if(!stat(filename, &stat_buf)) {
	if(!S_ISREG(stat_buf.st_mode)) {
	    quoted = quote_string(filename);
	    g_printf(_("ERROR [%s is not a file]\n"), quoted);
	    amfree(quoted);
	}
    }
    check_access(filename, mode);
}

void
check_dir(
    char *	dirname,
    int		mode)
{
    struct stat stat_buf;
    char *quoted;
    char *dir;

    if(!stat(dirname, &stat_buf)) {
	if(!S_ISDIR(stat_buf.st_mode)) {
	    quoted = quote_string(dirname);
	    g_printf(_("ERROR [%s is not a directory]\n"), quoted);
	    amfree(quoted);
	}
    }
    dir = stralloc2(dirname, "/.");
    check_access(dir, mode);
    amfree(dir);
}

void
check_suid(
    char *	filename)
{
#ifndef SINGLE_USERID
    struct stat stat_buf;
    char *quoted = quote_string(filename);

    if(!stat(filename, &stat_buf)) {
	if(stat_buf.st_uid != 0 ) {
	    g_printf(_("ERROR [%s is not owned by root]\n"), quoted);
	}
	if((stat_buf.st_mode & S_ISUID) != S_ISUID) {
	    g_printf(_("ERROR [%s is not SUID root]\n"), quoted);
	}
    }
    else {
	g_printf(_("ERROR [can not stat %s]\n"), quoted);
    }
    amfree(quoted);
#else
    (void)filename;	/* Quiet unused parameter warning */
#endif
}

/*
 * Returns the value of the first integer in a string.
 */

double
the_num(
    char *      str,
    int         pos)
{
    char *num;
    int ch;
    double d;

    do {
	ch = *str++;
	while(ch && !isdigit(ch)) ch = *str++;
	if (pos == 1) break;
	pos--;
	while(ch && (isdigit(ch) || ch == '.')) ch = *str++;
    } while (ch);
    num = str - 1;
    while(isdigit(ch) || ch == '.') ch = *str++;
    str[-1] = '\0';
    d = atof(num);
    str[-1] = (char)ch;
    return d;
}


char *
config_errors_to_error_string(
    GSList *errlist)
{
    char *errmsg;
    gboolean multiple_errors = FALSE;

    if (errlist) {
	errmsg = (char *)errlist->data;
	if (errlist->next)
	    multiple_errors = TRUE;
    } else {
	errmsg = _("(no error message)");
    }

    return vstrallocf("ERROR %s%s", errmsg,
	multiple_errors? _(" (additional errors not displayed)"):"");
}
