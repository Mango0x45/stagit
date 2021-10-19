#ifndef __UTILS_H_
#define __UTILS_H_

#define SHORT_DATE_FMT "%Y-%m-%d %H:%M"
#define STD_DATE_FMT   "%Y-%m-%dT%H:%M:%SZ"

void joinpath(char *buf, size_t bufsiz, const char *path, const char *path2);
void xmlencode(FILE *fp, const char *s, size_t len);
void xmlencodeline(FILE *fp, const char *s, size_t len);
void printtime(FILE *steam, const git_time *intime);
void printtimez(FILE *stream, const git_time *intime, const char *fmt);

#endif /* !__UTILS_H_ */
