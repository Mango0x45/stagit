#include <err.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <git2.h>

#include "utils.h"

static git_repository *repo;

static const char *relpath = "";

static bool print_author;
static char description[255];
static char *page_description; /* The description that appears to the right of the logo */
static char *title;            /* The page title (the text in the <title> tags) */
static char *name = "";
static char owner[255];

/* This function writes the index page up to the repository list */
static void
writeheader(void)
{
	fputs("<!DOCTYPE html>\n<html>\n<head>\n"
	      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n"
	      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/>\n"
	      "<title>", stdout);
	if (title)
		xmlencode(stdout, title, strlen(title));
	printf("</title>\n<link rel=\"icon\" type=\"image/svg+xml\" href=\"%slogo.svg\"/>\n", relpath);
	printf("<link rel=\"alternate icon\" href=\"%sfavicon.ico\"/>\n", relpath);
	printf("<link rel=\"stylesheet\" type=\"text/css\" href=\"%sstyle.css\"/>\n", relpath);
	puts("</head>\n<body id=\"home\">\n<table><tbody>\n<tr><td>"
	     "<img src=\"logo.svg\" alt=\"Site logo\" width=32 height=32></td>");
	fputs("<td><span class=\"desc\">", stdout);
	if (page_description)
		xmlencode(stdout, page_description, strlen(page_description));
	printf("</span></td></tr>\n</tbody></table>\n<hr/>\n<div id=\"content\">\n"
	       "<h2 id=\"repositories\">Repositories</h2>\n<hr/><div class=\"table-container\">\n"
	       "<table id=\"index\"><thead>\n<tr><td><b>Name</b></td><td><b>Description</b></td>"
	       "%s<td><b>Last commit</b></td></tr></thead><tbody>\n",
	       print_author ? "<td><b>Author</b></td>" : "");
}

/* This function writes the remainder of the index page from after the repository list */
static void
writefooter(void)
{
	puts("</tbody>\n</table>\n</div>\n<hr/>\n</body>\n</html>");
}

/* This function lists the given repository in the repostitory list */
static int
writelog(void)
{
	git_commit *commit = NULL;
	const git_signature *author;
	git_revwalk *w = NULL;
	git_oid id;
	char *stripped_name = NULL, *p;
	int ret = 0;

	git_revwalk_new(&w, repo);
	git_revwalk_push_head(w);

	if (git_revwalk_next(&id, w) || git_commit_lookup(&commit, repo, &id)) {
		ret = -1;
		goto err;
	}

	author = git_commit_author(commit);

	/* Strip .git suffix */
	if (!(stripped_name = strdup(name)))
		err(1, "strdup");
	if ((p = strrchr(stripped_name, '.')))
		if (!strcmp(p, ".git"))
			*p = '\0';

	fputs("<tr class=\"repo\"><td><a href=\"", stdout);
	xmlencode(stdout, stripped_name, strlen(stripped_name));
	fputs("/\">", stdout);
	xmlencode(stdout, stripped_name, strlen(stripped_name));
	fputs("</a></td><td>", stdout);
	xmlencode(stdout, description, strlen(description));
	fputs("</td>", stdout);

	if (print_author) {
		fputs("<td>", stdout);
		xmlencode(stdout, owner, strlen(owner));
		fputs("</td>", stdout);
	}

	fputs("<td>", stdout);
	if (author)
		printtimez(stdout, &(author->when), SHORT_DATE_FMT);
	puts("</td></tr>");

	git_commit_free(commit);
err:
	git_revwalk_free(w);
	free(stripped_name);

	return ret;
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	char path[PATH_MAX], repodirabs[PATH_MAX + 1];
	const char *repodir;
	int i, ret = 0, tmp, opt;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s [-a] [-d description] [-t title] "
				"[[-c category] repodir...]\n", argv[0]);
		return EXIT_FAILURE;
	}

#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");
#endif

	while ((opt = getopt(argc, argv, "acd:t:")) != -1) {
		switch (opt) {
		case 'a':
			print_author = true;
			break;
		case 'c':
			optind--;
			goto opt_exit;
		case 'd':
			page_description = optarg;
			break;
		case 't':
			title = optarg;
			break;
		default:
			return EXIT_FAILURE;
		}
	}

opt_exit:
	argv += optind;
	argc -= optind;

	git_libgit2_init();

	writeheader();

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-c")) {
			i++;
			if (i == argc)
				err(EXIT_FAILURE, "missing argument");
			repodir = argv[i];
			fputs("<tr class=\"cat\"><td>", stdout);
			xmlencode(stdout, repodir, strlen(repodir));
			printf("</td><td></td><td></td>%s\n", print_author ? "<td></td>" : "");
			continue;
		}

		repodir = argv[i];
		if (!realpath(repodir, repodirabs))
			err(EXIT_FAILURE, "realpath");

		if (git_repository_open_ext(&repo, repodir, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL)) {
			fprintf(stderr, "%s: cannot open repository '%s'\n", argv[0], repodirabs);
			ret = EXIT_FAILURE;
			continue;
		}

		/* Use directory name as name */
		if ((name = strrchr(repodirabs, '/')))
			name++;
		else
			name = "";

		/* Read description or .git/description */
		joinpath(path, sizeof(path), repodir, "description");
		if (!(fp = fopen(path, "r"))) {
			joinpath(path, sizeof(path), repodir, ".git/description");
			fp = fopen(path, "r");
		}
		description[0] = '\0';
		if (fp) {
			if (!fgets(description, sizeof(description), fp))
				description[0] = '\0';
			tmp = strlen(description);
			if (tmp > 0 && description[tmp-1] == '\n')
				description[tmp-1] = '\0';
			fclose(fp);
		}

		/* Read owner or .git/owner */
		joinpath(path, sizeof(path), repodir, "owner");
		if (!(fp = fopen(path, "r"))) {
			joinpath(path, sizeof(path), repodir, ".git/owner");
			fp = fopen(path, "r");
		}
		owner[0] = '\0';
		if (fp) {
			if (!fgets(owner, sizeof(owner), fp))
				owner[0] = '\0';
			owner[strcspn(owner, "\n")] = '\0';
			fclose(fp);
		}
		writelog();
	}
	writefooter();

	/* Cleanup */
	git_repository_free(repo);
	git_libgit2_shutdown();

	return ret;
}
