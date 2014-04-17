/* Copyright (C) 1999-2001  bjk <bjk@arbornet.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#include "randtype.h"

static void cleanup(void)
{
    REPLACE *cur, *next;

    if (replace) {
	for (cur = replace; cur; cur = next) {
	    next = cur->next;
	    free(cur->find);
	    free(cur->replace);
	    free(cur);
	}
    }

    return;
}

/* What to do with the caught signals. */
static void catch_signal(int signal)
{
    switch (signal) {
      case SIGALRM:
	raise(SIGTERM);
	break;
      case SIGTERM:
	break;
      case SIGINT:
	break;
      default:
	raise(SIGTERM);
	break;
    }

    cleanup();
    exit(signal);
}

/* Show help text. */
static void usage(const char *me)
{
    fprintf(stderr,
	    "Usage: %s [-hvl] [-d ,|.<string> [-k]] [-n <string>] "
	    "[-t <ms,mult>]"
	    "\n\t\t  [-w <string> [-c ms,mult]] [-r s1,s2[:...]] "
	    "[-m <int>]\n\t\t  [-q <int>] [file ...]\n\n", me);
    fprintf(stderr,
	    "  -t ms,mult\tSpecify randomness where `ms' is "
	    "the maximum microseconds\n\t\tbetween characters and `mult' "
	    "is the multiplier of `ms'.\n\t\tDefault is %i,%i.\n",
	    DEF_MS, DEF_MULT);
    fprintf(stderr,
	    "  -d string\tA string beginning with either ',' (left) or '.' "
	    "(right) which\n\t\tis output immediately.\n");
    fprintf(stderr,
	    "  -k\t\tKill the dump string (-d) when printing the line.\n");
    fprintf(stderr,
	    "  -r s1,s2\tReplace s1 with s2. Separate more than one with "
	    "':'.\n");
    fprintf(stderr,
	    "  -n chars\tPrint these characters immediately.\n");
    fprintf(stderr,
	    "  -w chars\tThe opposite of the -n option, these "
	    "characters are delayed by\n\t\tthe time specified by "
	    "the -c option.\n");
    fprintf(stderr,
	    "  -c ms,mult\tFor use with the -w option, this option "
	    "has the same value\n\t\tformat as the -t option or the "
	    "same default.\n");
    fprintf(stderr,
	    "  -l\t\tThe -t option will apply to outputting "
	    "lines rather than\n\t\tcharacters, all options other than -q are "
	    "ignored.\n");
    fprintf(stderr, "  -m int\tGenerate mistakes on output with a frequency "
	    "of <int>. The\n\t\tfrequency is the comparison of a random "
	    "alphanumeric character\n\t\tand the next character.\n");
    fprintf(stderr,
	    "  -q int\tTerminate program after specified amount of seconds.\n");
    fprintf(stderr, "  -h\t\tYour looking at it.\n");
    fprintf(stderr, "  -v\t\tShow version information.\n");

    exit(-1);
}

/* See if the character matches any character in the --wait/--nowait string. */
static char chk_special(const char *str, char c)
{
    register char i;

    while ((i = *str++)) {
	if (i == '\\' && *str) {
	    switch (i = *str++) {
	      case 'a':
		i = '\007';
		break;
	      case 'b':
		i = '\b';
		break;
	      case 'f':
		i = '\f';
		break;
	      case 'n':
		i = '\n';
		break;
	      case 'r':
		i = '\r';
		break;
	      case 't':
		i = '\t';
		break;
	      case 'v':
		i = (int) 0x0B;
		break;
	      case '\\':
		i = '\\';
		break;
	      default:
		break;
	    }
	}

	if (i == c)
	    break;
    }

    return i;
}

static void randsleep(float ms, unsigned int mult)
{
    static struct timeval tv;
    long i;

#ifdef HAVE_RANDOM
    i = (1 + (int) (ms * random() / (RAND_MAX + 1.0))) * mult;
#else
    i = (1 + (int) (ms * rand() / (RAND_MAX + 1.0))) * mult;
#endif

    /* FreeBSD likes one less */
    if (i > 999999L) {
	tv.tv_sec = i / 1000000;
	tv.tv_usec = i % 1000000;
    }
    else
	tv.tv_usec = i;

    /* select() is unbelievably more efficient than usleep(). */
    select(0, NULL, NULL, NULL, &tv);

    return;
}

static void randchar(int c)
{
    int i;

    do {
#ifdef HAVE_RANDOM
        i = (1 + (int) (94.0 * random() / (RAND_MAX + 1.0))) + 32;
#else
        i = (1 + (int) (94.0 * rand() / (RAND_MAX + 1.0))) + 32;
#endif
    } while (!isalpha(i));

    fprintf(stdout, "%c", (islower(c) ? tolower(i) : i));
    randsleep(tms * 2, tmult);
    fprintf(stdout, "\b \b");
    randsleep(tms * 2, tmult);

    return;
}

static int randint(float max)
{
#ifdef HAVE_RANDOM
    return((1 + (int) (max * random() / (RAND_MAX + 1.0))));
#else
    return((1 + (int) (max * rand() / (RAND_MAX + 1.0))));
#endif
}

static int randtype(char *str)
{
    int n = 0, p;
    register unsigned int i = 0;
    register char c;

    while ((i = *str++)) {
	for (p = 0; p < mistakes; p++) {
	    if (!isalpha(i))
		break;

	    if (randint(strlen(str)) == n) {
		randchar(i);
		break;
	    }
	}

	n++;
	c = chk_special(_nowait, i);

	if (i == c) {
	    fprintf(stdout, "%c", i);
	    continue;
	}

	c = chk_special(_wait, i);

	if (i == c) {
	    randsleep(cms, cmult);
	    fprintf(stdout, "%c", i);
	    continue;
	}

	fprintf(stdout, "%c", i);
	randsleep(tms, tmult);
    }

    return 0;
}

/* Check the -t and -c argument format. */
static int time_split(const char *str, float *ms, unsigned int *mult)
{
    register int n, i;
    char *s, *tmp;

    tmp = strdup(str);

    /* Only 2 values needed so n<=1. Others are ignored. */
    for (n = 0; n <= 1; n++) {
	if (n == 0)
	    s = strtok(tmp, ",");
	else
	    s = strtok(NULL, ",");

	if (s == NULL)
	    return 1;

	for (i = 0; i < strlen(s); i++) {
	    if (isdigit(s[i]) == 0)
		return 1;
	}

	if (n == 0)
	    *ms = atof(s);
	else
	    *mult = atoi(s);
    }

    free(tmp);
    return 0;
}

/* this function was written by Dan Stubbs (dstubbs@garden.csc.calpoly.edu)
 * and leeched from a post on comp.lang.c. it searches a given string for a
 * token and replaces it with another. */ 
static char *strrep(char *str, char *find, char *repl)
{
    char *p0, *p1, *p2;
    size_t find_len = strlen(find);
    size_t repl_len = strlen(repl);
    size_t dist = 0, diff = repl_len - find_len;

    if (!find_len || (p0 = strstr(str ,find)) == NULL)
	return str;

    if (diff > 0) {            /*  If extra space needed--insert it.*/
        for (p1 = p0 + 1; p1 != NULL; p1 = strstr(++p1, find))
            dist += diff;

        memmove (p0 + dist, p0, strlen(str) - (p0 - str) + 1);
    }

    p1 = (diff > 0) ? p0 + dist + find_len : p0 + find_len;
    p2 = strstr(p1, find);

    while (p2) {        /* While there is another string to replace.*/
        memcpy (p0, repl, repl_len);          /* Insert replacement.*/
        p0 += repl_len;
        memmove (p0, p1, p2-p1);     /* Move just the right segment.*/
        p0 += (p2 - p1);
        p1  = p2 + find_len;
        p2  = strstr(p1, find);
    }

    memcpy (p0, repl, repl_len);               /* Final replacement.*/

    if (diff < 0) {          /* If there is a gap at the end of str.*/
        p0 += repl_len;
        p2  = strchr(p1, '\0');
        memmove (p0, p1, p2 - p1 + 1);
    }
    
    return (str);
}

/* Open the file, read lines, check for dump string, pass to other
 * functions then close file. */
int splitter(const char *filename, int kill)
{
#ifdef HAVE_ZLIB
    gzFile *fp = NULL;
#else
    FILE *fp = NULL;
#endif
    char *s, *src, *tmp;
    register unsigned int i;

    if ((s = malloc(LINE_MAX)) == NULL) {
	perror("malloc(LINE_MAX)");
	return errno;
    }

    if ((src = calloc(0, 1)) == NULL) {
	perror("calloc()");
	return errno;
    }

    if (filename[0] == '-' && filename[1] == 0) {
#ifdef HAVE_ZLIB
	if ((fp = gzdopen(0, "rb")) == NULL) {
	    perror(filename);
	    return errno;
	}
#else
	;
#endif
    }
    else {
#ifdef HAVE_ZLIB
	if ((fp = gzopen(filename, "rb")) == NULL) {
#else
	if ((fp = fopen(filename, "r")) == NULL) {
#endif
	    perror(filename);
	    return errno;
	}
    }

#ifdef HAVE_ZLIB
    while ((s = gzgets(fp, s, LINE_MAX)) != NULL) {
#else
    while ((s = fgets(s, LINE_MAX, fp ? fp : stdin)) != NULL) {
#endif
	REPLACE *rnode;
	char *p;

	if (replace) {
	    rnode = replace;

	    while (rnode->next != NULL) {
		char *x;

		x = strrep(s, rnode->find, rnode->replace);
		strncpy(s, x, LINE_MAX - 1);
		rnode = rnode->next;
	    }
	}

	if (dolines) {
	    randsleep(tms, tmult);
	    fprintf(stdout, "%s", s);
	    continue;
	}

	if (dumpstr[0]) {
	    if (dir == ',') {
	        if ((tmp = strstr(s, dumpstr)) != NULL) {
		    for (i = 0; i < (strlen(s) - strlen(tmp)); i++)
		        fprintf(stdout, "%c", s[i]);

	            if ((s = calloc(1, LINE_MAX)) == NULL) {
	                perror("calloc");
	               return errno;
	            }

		    if (kill) {
		        for (i = strlen(dumpstr); i < strlen(tmp); i++) {
			    sprintf(src, "%c", tmp[i]);
			    strncat(s, src, LINE_MAX - 1);
		        }
	 	    }
		    else {
		        while ((i = *tmp++)) {
			    sprintf(src, "%c", i);
			    strncat(s, src, LINE_MAX - 1);
		        }
		    }
	        }
	    }
	    else {
		p = strdup(s);

	        if ((tmp = strtok(s, dumpstr)) != NULL) {
		    randtype(tmp);

		    if (kill)
			i = strlen(tmp) + strlen(dumpstr);
		    else
			i = strlen(tmp);
		    
		    for (i = i; i < strlen(p); i++)
			fprintf(stdout, "%c", p[i]);

		}

		free(p);
	    }
	}

	if (dir != '.')
	    randtype(s);
    }

#ifdef HAVE_ZLIB
    gzclose(fp);
#else
    fclose(fp);
#endif

    if (!dolines)
	free(src);

    free(s);

    return 0;
}

static int parse_replace(const char *str)
{
    REPLACE *rnode;
    char *tmp, *s, *ss;

    if ((replace = (REPLACE *) malloc(sizeof(REPLACE))) == NULL) {
	perror("malloc");
	exit(errno);
    }

    rnode = replace;
    tmp = strdup(str);

    while ((s = strtok_r(tmp, ":", &ss)) != NULL) {
	char *find, *repl;
	int i;

	if ((find = strtok(s, ",")) == NULL)
	    return 1;

	i = strlen(find) + 1;

	if ((rnode->find = (char *) malloc(i)) == NULL) {
	    perror("malloc");
	    exit(errno);
	}

	snprintf(rnode->find, i, "%s", find);

	if ((repl = strtok(NULL, ",")) == NULL)
	    return 1;

	i = strlen(repl) + 1;

	if ((rnode->replace = (char *) malloc(i)) == NULL) {
	    perror("malloc");
	    exit(errno);
	}

	snprintf(rnode->replace, i, "%s", repl);

	if ((rnode->next = (REPLACE *) malloc(sizeof(REPLACE))) == NULL) {
	    perror("malloc");
	    exit(errno);
	}

	rnode = rnode->next;
	tmp = ss;
    }

    rnode->next = NULL;

    tmp = NULL;
    free(tmp);

    return 0;
}

int main(int argc, char *argv[])
{
    static unsigned int i, opt, kill, quit;
    char *time = NULL, *chartime = NULL, *replacestr = NULL;
    char progname[NAME_MAX], tmp[NAME_MAX];
    struct sigaction my_sigaction;

    bzero(&my_sigaction, sizeof(my_sigaction));
    bzero(progname, sizeof(progname));
    snprintf(progname, sizeof(progname), "%s", argv[0]);

    /* Set defaults. */
    tms = cms = DEF_MS;
    tmult = cmult = DEF_MULT;
    _nowait[0] = _wait[0] = '\0';

    while ((opt = getopt(argc, argv, "m:vhr:q:t:n:w:d:c:kl")) != EOF) {
	tmp[0] = 0;

	switch (opt) {
	  case 0:
	    break;
	  case 'm':
	    snprintf(tmp, sizeof(tmp), "%s", optarg);

	    for (i = 0; i < strlen(tmp); i++) {
		if (isdigit(tmp[i]) == 0)
		    usage(progname);
	    }

	    mistakes = atoi(tmp);
	    break;
	  case 'n':
	    snprintf(_nowait, sizeof(_nowait), "%s", optarg);
	    break;
	  case 'w':
	    snprintf(_wait, sizeof(_wait), "%s", optarg);
	    break;
	  case 't':
	    time = optarg;
	    break;
	  case 'r':
	    replacestr = optarg;
	    break;
	  case 'd':
	    if ((optarg[0] != ',' && optarg[0] != '.') || strlen(optarg) < 2)
		usage(progname);

	    dir = optarg[0];

	    while ((*optarg++))
		tmp[i++] = *optarg;

	    tmp[i] = 0;
	    snprintf(dumpstr, sizeof(dumpstr), "%s", tmp);
	    break;
	  case 'c':
	    chartime = optarg;
	    break;
	  case 'k':
	    kill = 1;
	    break;
	  case 'l':
	    dolines = 1;
	    break;
	  case 'q':
	    snprintf(tmp, sizeof(tmp), "%s", optarg);

	    for (i = 0; i < strlen(tmp); i++) {
		if (isdigit(tmp[i]) == 0)
		    usage(progname);
	    }

	    quit = atoi(tmp);
	    break;
	  case 'v':
	    fprintf(stdout, "%s\n%s\n", VERSION, COPYRIGHT);
	    exit(0);
	  case 'h':
	  default:
	    usage(progname);
	    break;
	}
    }

    if (replacestr)
	if (parse_replace(replacestr) != 0)
	    usage(progname);

    if (time)
	if (time_split(time, &tms, &tmult) != 0)
	    usage(progname);

    if (!dolines) {
	if (chartime != NULL && _wait[0] != '\0') {
	    if (time_split(chartime, &cms, &cmult) != 0)
		usage(progname);
	}

	/* STDOUT is line buffered by default, we need to change this to
	 * deal with characters. */
	setvbuf(stdout, 0, _IONBF, BUFSIZ);
    }

    /* Setup the signal handler. */
    sigemptyset(&my_sigaction.sa_mask);
    sigfillset(&my_sigaction.sa_mask);
    my_sigaction.sa_handler = catch_signal;

    /* These are the signals that will be caught. All others are ignored when
     * possible. */
    sigdelset(&my_sigaction.sa_mask, SIGTERM);
    sigdelset(&my_sigaction.sa_mask, SIGALRM);
    sigdelset(&my_sigaction.sa_mask, SIGINT);
    sigdelset(&my_sigaction.sa_mask, SIGTSTP);

    sigprocmask(SIG_BLOCK, &my_sigaction.sa_mask, NULL);

    sigaction(SIGINT, &my_sigaction, NULL);
    sigaction(SIGTERM, &my_sigaction, NULL);
    sigaction(SIGALRM, &my_sigaction, NULL);

#ifdef HAVE_RANDOM
    srandom(getpid());
#else
    srand(getpid());
#endif

    /* Start the quitting timer. */
    alarm(quit);

    /* Keep reading until last non-option argument is read. */
    if (argc == optind)
	err |= splitter("-", kill);
    else {
	for (i = optind; i < argc; i++) {
	    err |= splitter(argv[i], kill);
	}
    }

    cleanup();
    exit(err);
}
