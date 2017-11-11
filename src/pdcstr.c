/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/* OpenComal own string routines */

#include <string.h>
#include "pdcglob.h"


PUBLIC char *my_strdup(int pool, const char *s)
{
	char *t = (char *)mem_alloc(pool, strlen(s) + 1);

	return strcpy(t, s);
}


PUBLIC int str_cmp(struct string *s1, struct string *s2)
{
	char HUGE_POINTER *w1 = s1->s;
	char HUGE_POINTER *w2 = s2->s;

	return strcmp(w1, w2);
}


PUBLIC struct string *str_make(int pool, const char *s)
{
	long l = strlen(s);
	struct string *work = STR_ALLOC(pool, l);

	work->len = l;
	strcpy(work->s, s);

	return work;
}

PUBLIC struct string *str_make2(int pool, long len)
{
	struct string *work = STR_ALLOC(pool, len);
	char HUGE_POINTER *s;

	work->len = len;
	
	for (s=work->s; len; --len) {
		*s=' ';
		s++;
	}

	return work;
}

PUBLIC struct string *str_cat(struct string *s1, struct string *s2)
{
	char HUGE_POINTER *w1 = s1->s;

	s1->len += s2->len;
	strcat(w1, s2->s);

	return s1;
}


PUBLIC long str_search(struct string *needle, struct string *haystack)
{
	char HUGE_POINTER *h = haystack->s;
	char HUGE_POINTER *n = needle->s;
	char HUGE_POINTER *wn;
	char HUGE_POINTER *wh;

	while (*h) {
		if (*h == *n) {
			wn = n;
			wh = h;

			while (*wn == *wh && *wn)
				wn++, wh++;

			if (!*wn)
				return h - haystack->s + 1;
		}

		h++;
	}

	return 0L;
}


PUBLIC struct string *str_cpy(struct string *s1, struct string *s2)
{
	s1->len = s2->len;
	strcpy(s1->s, s2->s);

	return s1;
}

PUBLIC struct string *str_ncpy(struct string *s1, struct string *s2, long n)
{
	s1->len = n;
	strncpy(s1->s, s2->s,n);

	return s1;
}

/*
 * This routine copies a substring of string 2 to string 1
 */
PUBLIC struct string *str_partcpy(struct string *s1, struct string *s2,
				  long from, long to)
{
	char HUGE_POINTER *w1 = s1->s;
	char HUGE_POINTER *w2 = s2->s;

	s1->len = to - from + 1;
	w2 = w2 + from - 1;

	while (from <= to && *w2)
		*w1 = *w2, w1++, w2++, from++;

	*w1 = '\0';

	return s1;
}

/*
 * This routine copies string 2 to a substring of string 1
 */
PUBLIC struct string *str_partcpy2(struct string *s1, struct string *s2,
				  long from, long to)
{
	char HUGE_POINTER *w1 = s1->s+from-1; /* Comal strings start at offset 1 */
	char HUGE_POINTER *w2 = s2->s;

	while (from <= to && *w2)
		*w1 = *w2, w1++, w2++, from++;

	return s1;
}

PUBLIC struct string *str_dup(int pool, struct string *s)
{
	struct string *work = STR_ALLOC(pool, s->len);

	str_cpy(work, s);

	return work;
}


PUBLIC struct string *str_maxdup(int pool, struct string *s, long max)
{
	struct string *work;
	long len=(max<s->len)?max:s->len;

	work = STR_ALLOC(pool, len);
	str_ncpy(work, s, len);

	return work;
}

PUBLIC void str_extend(int pool, struct string **s, long newlen)
{
	struct string *work;
	char HUGE_POINTER *t;
	long i;

	if ((*s) && (*s)->len>=newlen) return;

	work=STR_ALLOC(pool,newlen);
	str_cpy(work,*s);
	t=&work->s[(*s)->len];
	
	for (i=newlen-(*s)->len; i; --i) {
		*t=' ';
		t++;
	}
	
	*t=0;
	work->len=newlen;
	mem_free(*s);
	*s=work;
}
