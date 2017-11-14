/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/* OpenComal symbol table and related stuff */

#include "pdcglob.h"
#include "pdcid.h"
#include "pdcstr.h"
#include "pdcmisc.h"

#include <string.h>
#include <stdbool.h>

PUBLIC struct sym_env *sym_newenv(int closed, struct sym_env *prev,
				  struct comal_line *curproc, const char *name)
{
	struct sym_env *work = GETCORE(RUN_POOL, struct sym_env);

	if (comal_debug) {
		my_printf(MSG_DEBUG, 0,
			  "Handing out new env %s at %p, prev=%p for: ",
			  name, work, prev);
		puts_line(MSG_DEBUG, curproc);
	}

	work->prev = prev;
	work->closed = closed;
	work->itemroot = NULL;
	work->name = my_strdup(RUN_POOL, name);
	work->curproc = curproc;

	return work;
}


PUBLIC struct sym_env *search_env(char *name, struct sym_env *start)
{
	struct sym_env *work = start;

	while (work && strcmp(name, work->name) != 0)
		work = work->prev;

	return work;
}


PUBLIC struct sym_env *search_env_level(int level, struct sym_env *start)
{
	struct sym_env *work = start;

	while (work && proclevel(work->curproc) > level)
		work = work->prev;

	return work;
}


PUBLIC struct sym_env *sym_newvarenv(struct sym_env *env)
{
	int level = proclevel(env->curproc);

	while (!env->closed) {
		level--;

		while (proclevel(env->curproc) != level)
			env = env->prev;
	}

	return env;
}


PUBLIC struct sym_item *sym_enter(struct sym_env *env, struct id_rec *id,
				  enum SYM_TYPE type, void *ptr)
{
	struct sym_item *work = env->itemroot;

	if (comal_debug)
		my_printf(MSG_DEBUG, 1,
			  "Entering Symbol %s (type %d) in table %p",
			  id->name, type, env);

	while (work && !(work->id == id && work->symtype == type))
		work = work->next;

	if (work) {
		if (comal_debug)
			my_printf(MSG_DEBUG, 1,
				  "Illegally found SYM at %p, %s", work,
				  work->id->name);

		return NULL;
	}

	work = GETCORE(RUN_POOL, struct sym_item);
	work->id = id;
	work->symtype = type;
	work->data.ptr = ptr;

	work->next = env->itemroot;
	env->itemroot = work;

	return work;
}


PRIVATE struct sym_item *search_horse(struct sym_env *env,
				      struct id_rec *id,
				      enum SYM_TYPE type)
{
	struct sym_item *work = env->itemroot;

	if (comal_debug)
		my_printf(MSG_DEBUG, 1,
			  "Searching Symbol %s (type %d) in table %p",
			  id->name, type, env);

	while (work && !(work->id == id && work->symtype == type)) {
		if (comal_debug)
			my_printf(MSG_DEBUG, 1, "  Examining symbol %s",
				  work->id->name);

		work = work->next;
	}

	if (comal_debug) {
		if (work)
			my_printf(MSG_DEBUG, 1,
				  "Found SYM at %p, %s, type %d", work,
				  work->id->name, work->symtype);
		else
			my_printf(MSG_DEBUG, 1, "Returning NULL");
	}

	return work;
}


PUBLIC struct sym_item *sym_search(struct sym_env *env, struct id_rec *id,
				   enum SYM_TYPE type)
{
	int level = proclevel(env->curproc);
	struct sym_item *ret;

	while (true) {
		ret = search_horse(env, id, type);

		if (ret || env->closed)
			break;

		level--;

		while (proclevel(env->curproc) != level)
			env = env->prev;
	}

	if (comal_debug)
		my_printf(MSG_DEBUG, 1, "Returning %p", ret);

	return ret;
}


PRIVATE void free_var(struct var_item *var)
{
	if (!var->ref) {
		long nritems;

		if (var->array) {
			nritems = var->array->nritems;
			free_list((struct my_list *) var->array);
		} else
			nritems = 1;

		switch (var->type) {
		case V_INT:
		case V_FLOAT:
			break;

		case V_STRING:
			for (--nritems; nritems >= 0; nritems--)
				if (var->data.str[nritems])
					mem_free(var->data.str[nritems]);

			break;

		default:
			fatal("free_var default action");
		}
	}

	mem_free(var);
}


PRIVATE struct sym_item *free_symitem(struct sym_item *item)
{
	struct sym_item *next = item->next;

	if (comal_debug)
		my_printf(MSG_DEBUG, 1, "Free item %s", item->id->name);

	switch (item->symtype) {
	case S_PROCVAR:
	case S_FUNCVAR:
		break;

	case S_VAR:
		free_var(item->data.var);
		break;

	case S_NAME:
		mem_free(item->data.name);
		break;

	default:
		fatal("free_symitem default action");
	}

	mem_free(item);

	return next;
}



PUBLIC struct sym_env *sym_freeenv(struct sym_env *env, int recur)
{
	do {
		struct sym_item *work;

		if (comal_debug)
			my_printf(MSG_DEBUG, 1, "Free env %p", env);

		work = env->itemroot;

		while (work)
			work = free_symitem(work);

		mem_free(env->name);
		env = (struct sym_env *)mem_free(env);
	}
	while (env && recur);

	return env;
}


PRIVATE struct arr_des *make_arrdes(struct arr_dim *arrdim)
{
	struct arr_des *arrdes = GETCORE(RUN_POOL, struct arr_des);
	struct arr_dim *walk = arrdim;
	int nrdims = 0;
	long nritems = 1;

	while (walk) {
		nrdims++;
		nritems = nritems * (walk->top - walk->bottom + 1);

		walk = walk->next;
	}

	arrdes->dimroot = arrdim;
	arrdes->nrdims = nrdims;
	arrdes->nritems = nritems;

	return arrdes;
}


PUBLIC struct var_item *var_newvar(enum VAL_TYPE type,
				   struct arr_dim *arrdim, long strlen)
{
	struct arr_des *arrdes;
	struct var_item *work;
	long nritems;

	if (arrdim) {
		arrdes = make_arrdes(arrdim);
		nritems = arrdes->nritems;
	} else {
		arrdes = NULL;
		nritems = 1;
	}

	work =
	    (struct var_item *)mem_alloc(RUN_POOL,
		      sizeof(struct var_item) - sizeof(union var_data) +
		      type_size(type) * nritems);

	work->type = type;
	work->ref = 0;
	work->strlen = strlen;
	work->array = arrdes;

	return work;
}


/*
 * Create a new var entry for a REF variable
 */
PUBLIC struct var_item *var_refvar(struct var_item *lvar, enum VAL_TYPE type, long strlen, void *vref)
{
	struct var_item *work;

	if (comal_debug)
		my_printf(MSG_DEBUG, 1, "NEW RefVar pointing at %p", vref);

	work =
	    (struct var_item *)mem_alloc(RUN_POOL,
		      sizeof(struct var_item) - sizeof(union var_data) +
		      sizeof(void *));

	work->type = lvar->type;
	work->ref = 1;
	work->strlen = strlen;
	work->data.vref = vref;

	if (type == V_ARRAY)
		work->array = lvar->array;
	else
		work->array = 0;

	return work;
}


PUBLIC struct name_rec *name_new(struct sym_env *env,
				 struct expression *exp)
{
	struct name_rec *work =
	    (struct name_rec *)mem_alloc(RUN_POOL, sizeof(struct name_rec));

	work->env = env;
	work->exp = exp;

	return work;
}

/*
 * Return a pointer to the data araa of a variable. If this
 * variable is a REFerence variable, this function takes that
 * into account...
 */
PUBLIC void *var_data(struct var_item *var)
{
	if (!var->ref)
		return &var->data;

	return var->data.vref;
}
