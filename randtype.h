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

/* Set some defaults. */
#ifndef DEF_MS
#define DEF_MS 18
#endif

#ifndef DEF_MULT
#define DEF_MULT 20000
#endif

#ifndef LINE_MAX
#define LINE_MAX 2048
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define VERSION "randtype 1.13"
#define COPYRIGHT "Copyright (C) 1999-2001 bjk <bjk@arbornet.org>"

typedef struct replaces {
    char *find;
    char *replace;
    struct replaces *next;
} REPLACE;

/* External variables. */
REPLACE *replace;
float tms, cms;
int err;
unsigned int tmult, cmult;
unsigned int dolines, mistakes, dir;
char nowait[137], wait[137];
char dumpstr[64];

/* Function prototypes. */
static void usage(const char *me);
static int randtype(char *str);
static int splitter(const char *filename, int kill);
static char chk_special(const char *str, const char c);
static int time_split(const char *str, float *ms, unsigned int *mult);
static void randsleep(float ms, unsigned int mult);
static void catch_signal(int signal);
