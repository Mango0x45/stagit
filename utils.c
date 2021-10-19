#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <git2.h>

#include "utils.h"

/* This function takes the two directory paths given in `path` and `path2` and concatinates them
 * together into the buffer `buf` which has a size of `bufsiz`.
 */
void
joinpath(char *buf, size_t bufsiz, const char *path, const char *path2)
{
	int r;
	bool slash;

	slash = path[0] && path[strlen(path) - 1] != '/';
	r = snprintf(buf, bufsiz, "%s%s%s", path, slash ? "/" : "", path2);
	if (r < 0 || (size_t) r >= bufsiz)
		errx(EXIT_FAILURE, "path truncated: '%s%s%s'", path, slash ? "/" : "", path2);
}

/* This function escapes the characters below in the string `s` of length `len` as
 * HTML 2.0 / XML 1.0. and writes the result to standard output.
 */
void
xmlencode(FILE *fp, const char *s, size_t len)
{
	size_t i;

	for (i = 0; *s && i < len; s++, i++) {
		switch(*s) {
		case '<':  fputs("&lt;",   fp); break;
		case '>':  fputs("&gt;",   fp); break;
		case '\'': fputs("&#39;",  fp); break;
		case '&':  fputs("&amp;",  fp); break;
		case '"':  fputs("&quot;", fp); break;
		default:   putc(*s, fp);
		}
	}
}

/* Same as above but do not write \r or \n */
void
xmlencodeline(FILE *fp, const char *s, size_t len)
{
	size_t i;

	for (i = 0; *s && i < len; s++, i++) {
		switch(*s) {
		case '<':  fputs("&lt;",   fp); break;
		case '>':  fputs("&gt;",   fp); break;
		case '\'': fputs("&#39;",  fp); break;
		case '&':  fputs("&amp;",  fp); break;
		case '"':  fputs("&quot;", fp); break;
		case '\r': break; /* ignore CR */
		case '\n': break; /* ignore LF */
		default:   putc(*s, fp);
		}
	}
}

/* This function writes the date and time of the commit which is given by `intime` to the stream
 * `stream` with the date and time format given by `fmt`.
 */
void
printtimez(FILE *stream, const git_time *intime, const char *fmt)
{
	struct tm *intm;
	time_t t;
	char out[32];

	t = (time_t) intime->time;
	if (!(intm = gmtime(&t)))
		return;
	strftime(out, sizeof(out), fmt, intm);
	fputs(out, stream);
}

/* This function writes the date and time of the commit which is given by `intime` to the stream
 * `stream` with the date and time format "%a, %e %b %y %H:%M:%S" while also writing the time
 * offset.
 */
void
printtime(FILE *steam, const git_time *intime)
{
	struct tm *intm;
	time_t t;
	char out[32];

	t = (time_t)intime->time + (intime->offset * 60);
	if (!(intm = gmtime(&t)))
		return;
	strftime(out, sizeof(out), "%a, %e %b %Y %H:%M:%S", intm);
	if (intime->offset < 0)
		fprintf(steam, "%s -%02d%02d", out,
		            -(intime->offset) / 60, -(intime->offset) % 60);
	else
		fprintf(steam, "%s +%02d%02d", out,
		            intime->offset / 60, intime->offset % 60);
}
