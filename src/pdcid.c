/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/* OpenComal's ID routines */

#include "pdcglob.h"
#include "pdcmisc.h"
#include <string.h>

PRIVATE struct id_rec *id_root = NULL;

/* The private function install builds a new record in memory for the
 * specified id name. Memory is allocated from the free pool.
 */

PRIVATE struct id_rec *install(char *idname)
{
	struct id_rec *work;
	int l = strlen(idname);

	work = (struct id_rec *)mem_alloc(MISC_POOL, sizeof(struct id_rec) + l);
	work->left = work->right = NULL;
	strncpy(work->name, idname, l);
	work->name[l] = '\0';

	switch (work->name[l - 1]) {
	case '#':
		work->type = V_INT;
		break;
	case '$':
		work->type = V_STRING;
		break;
	default:
		work->type = V_FLOAT;
		break;
	}

	return work;
}

/* The next routine does the horse work of searching and installing an
 * identifier.
 */

PRIVATE struct id_rec *id_horse(char *idname)
{
	struct id_rec *walk = id_root;
	struct id_rec *lastone = NULL;
	struct id_rec *installed;
	enum { atroot, overleft, overright } lastchoice = atroot;
	int found = 0;

	while (walk != NULL && !found) {
		int cmp = strcmp(idname, walk->name);

		lastone = walk;

		if (cmp < 0) {
			lastchoice = overleft;
			walk = walk->left;
		} else if (cmp > 0) {
			lastchoice = overright;
			walk = walk->right;
		} else
			found = 1;
	}

	if (found)
		return walk;

	installed = install(idname);

	if (lastchoice == atroot)
		return (id_root = installed);
	else if (lastchoice == overleft)
		return (lastone->left = installed);
	else
		return (lastone->right = installed);
}


/* id_search searches for an identifier and if it is not present installs it.
 * A handle for the identifier is returned.
 */

PUBLIC struct id_rec *id_search(char *id)
{
	char idname[MAX_IDLEN];
	int i;

	for (i = 0; i < MAX_IDLEN; idname[i++] = '\0');

	strncpy(idname, id, MAX_IDLEN - 1);
	strlwr(idname);
	return (struct id_rec *) id_horse(idname);
}
