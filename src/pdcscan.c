/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/* OpenComal program management */

#include "pdcglob.h"
#include "pdclexs.h"
#include "pdcmisc.h"
#include "pdcid.h"
#include "pdcsym.h"

#include <string.h>

#define SCAN_STACK_SIZE		(MAX_INDENT)

PRIVATE struct {
	int sym;
	struct comal_line *line;
} scan_stack[SCAN_STACK_SIZE];
PRIVATE int scan_sp;

enum scan_entry_special { NOT_SPECIAL, SHORT_FORM, EXIT, PROCFUNC, EPROCFUNC,
	DATA_STAT, CASE, RETRY
};

struct scan_entry {
	int sym;
	int leavessym;
	int expectsym1;
	int expectsym2;
	enum scan_entry_special special;
};


PRIVATE struct scan_entry scan_tab[] = {
	{caseSYM, whenSYM, 0, 0, CASE},
	{dataSYM, 0, 0, 0, DATA_STAT},
	{elifSYM, elifSYM, elifSYM, 0, NOT_SPECIAL},
	{elseSYM, endifSYM, elifSYM, 0, NOT_SPECIAL},
	{endcaseSYM, 0, whenSYM, endcaseSYM, NOT_SPECIAL},
	{endforSYM, 0, endforSYM, 0, NOT_SPECIAL},
	{endfuncSYM, 0, endfuncSYM, 0, EPROCFUNC},
	{endifSYM, 0, endifSYM, elifSYM, NOT_SPECIAL},
	{endloopSYM, 0, endloopSYM, 0, NOT_SPECIAL},
	{endprocSYM, 0, endprocSYM, 0, EPROCFUNC},
	{endtrapSYM, 0, endtrapSYM, handlerSYM, NOT_SPECIAL},
	{endwhileSYM, 0, endwhileSYM, 0, NOT_SPECIAL},
	{exitSYM, 0, 0, 0, EXIT},
	{forSYM, endforSYM, 0, 0, SHORT_FORM},
	{funcSYM, endfuncSYM, 0, 0, PROCFUNC},
	{handlerSYM, endtrapSYM, handlerSYM, 0, NOT_SPECIAL},
	{ifSYM, elifSYM, 0, 0, SHORT_FORM},
	{loopSYM, endloopSYM, 0, 0, NOT_SPECIAL},
	{otherwiseSYM, endcaseSYM, whenSYM, 0, NOT_SPECIAL},
	{procSYM, endprocSYM, 0, 0, PROCFUNC},
	{retrySYM, 0, 0, 0, RETRY},
	{repeatSYM, untilSYM, 0, 0, SHORT_FORM},
	{trapSYM, handlerSYM, 0, 0, NOT_SPECIAL},
	{untilSYM, 0, untilSYM, 0, NOT_SPECIAL},
	{whenSYM, whenSYM, whenSYM, 0, NOT_SPECIAL},
	{whileSYM, endwhileSYM, 0, 0, SHORT_FORM},
	{0, 0, 0, 0, NOT_SPECIAL}
};


PRIVATE void scan_stack_push(int sym, struct comal_line *line)
{
	if (scan_sp >= SCAN_STACK_SIZE)
		fatal("Internal scan stack overflow");

	scan_stack[scan_sp].sym = sym;
	scan_stack[scan_sp].line = line;
	scan_sp++;
}


PRIVATE void scan_stack_peek(int *sym, struct comal_line **line)
{
	if (!scan_sp) {
		*sym = -1;
		*line = NULL;
	} else {
		*sym = scan_stack[scan_sp - 1].sym;
		*line = scan_stack[scan_sp - 1].line;
	}
}


PRIVATE void scan_stack_pop(int *sym, struct comal_line **line)
{
	scan_stack_peek(sym, line);

	if (*sym != -1)
		--scan_sp;
}


PRIVATE void scan_stack_search(int sym1, int sym2, int *result,
			       struct comal_line **stkline)
{
	int i;

	for (i = scan_sp - 1; i >= 0; i--)
		if (scan_stack[i].sym == sym1 || scan_stack[i].sym == sym2) {
			*result = scan_stack[i].sym;

			if (stkline)
				*stkline = scan_stack[i].line;

			return;
		}

	*result = 0;
	*stkline = NULL;

	return;
}


PRIVATE struct comal_line *routine_search_horse(struct id_rec *id,
						int type,
						struct comal_line *root)
{
	while (root) {
		if (root->cmd == type && root->lc.pfrec.id == id)
			return root;

		root = root->lc.pfrec.proclink;
	}

	return NULL;
}


PRIVATE struct comal_line *routine_search(struct id_rec *id, int type,
					  struct comal_line *curproc)
{
	struct comal_line *father = curproc;

	while (father) {
		struct comal_line *procline;

		procline =
		    routine_search_horse(id, type,
					 father->lc.pfrec.localproc);

		if (procline)
			return procline;

		father = father->lc.pfrec.fatherproc;
	}

	return routine_search_horse(id, type, curenv->globalproc);
}


PRIVATE int scan_pass2(struct seg_des *seg, char *errtxt,
		       struct comal_line **errline)
{
	struct comal_line *curline;
	struct comal_line *theline;
	struct comal_line *procline = NULL;
	struct comal_line *walkline;
	struct comal_line *walk;
	struct parm_list *pwalk;
	struct exp_id *proccall;
	const char *err = NULL;
	int dummy;
	int procfound;
	int sp;

	if (seg)
		curline = seg->lineroot;
	else
		curline = curenv->progroot;

	scan_sp = 0;

	while (curline) {
		theline = line_2line(curline);

		if (!theline)
			theline = curline;

		if (theline->cmd == procSYM || theline->cmd == funcSYM) {
			scan_stack_push(0, procline);
			procline = theline;
		} else if (theline->cmd == endprocSYM
			   || theline->cmd == endfuncSYM)
			scan_stack_pop(&dummy, &procline);
		else if (seg && !procline && theline->cmd != 0) {
			strcpy(errtxt,
			       "Non-// program lines in external segment outside PROC/FUNC def");
			*errline = curline;
			return 0;
		} else if (theline->cmd == importSYM) {
			if (!procline || !procline->lc.pfrec.closed)
				err =
				    "IMPORT only allowed within CLOSED PROCs/FUNCs";
		} else if (theline->cmd == localSYM) {
			if (!procline || procline->lc.pfrec.closed)
				err =
				    "LOCAL only allowed within open PROCs/FUNCs";
		} else if (theline->cmd == returnSYM) {
			if (!procline)
				err =
				    "Can only RETURN from inside a PROC or FUNC";
			else if (procline->cmd == funcSYM) {
				if (!theline->lc.exp)
					err =
					    "This statement (inside a FUNC) must RETURN an expression";
				else if (!type_match1
					 (procline->lc.pfrec.id,
					  theline->lc.exp))
					err =
					    "This statement RETURNs the wrong type of expression";
			} else if (theline->lc.exp)
				err =
				    "This statement (inside a PROC) must not RETURN an expression";
		} else if (theline->cmd == execSYM) {
			proccall = &theline->lc.exp->e.expid;

			if (!routine_search
			    (proccall->id, procSYM, procline)) {
				procfound = 0;

				if (procline) {
					sp = scan_sp;
					walkline = procline;

					while (!procfound && walkline) {
						pwalk =
						    walkline->lc.pfrec.
						    parmroot;

						while (pwalk
						       && pwalk->id !=
						       proccall->id)
							pwalk =
							    pwalk->next;

						procfound = (pwalk
							     && pwalk->
							     ref ==
							     procSYM);

						if (!sp)
							walkline = NULL;
						else {
							sp--;
							walkline =
							    scan_stack[sp].
							    line;
						}
					}
				}

				if (!procfound
				    && !sys_call_scan(proccall->id,
						      proccall->exproot,
						      errtxt)) {
					*errline = curline;
					return 0;
				}
			}
		} else if (theline->cmd == restoreSYM) {
			if (seg) {
				walk = curenv->progroot;

				while (walk && walk->cmd != dataSYM)
					walk = walk->ld->next;

				if (!walk)
					err =
					    "RESTORE in external segment without any DATA line in main program";

				theline->lineptr = walk;
			} else if (!curenv->datalptr)
				err =
				    "RESTORE without any DATA line in program";
			else if (theline->lc.id) {
				walk = curenv->progroot;

				while (walk
				       && !(walk->cmd == idSYM
					    && walk->lc.id ==
					    theline->lc.id))
					walk = walk->ld->next;

				if (!walk)
					err = "Label not found";
				else {
					while (walk
					       && walk->cmd != dataSYM)
						walk = walk->ld->next;

					if (!walk)
						err =
						    "No DATA statements follow the label";
				}

				theline->lineptr = walk;
			} else
				theline->lineptr = curenv->datalptr;

		}

		if (err) {
			strcpy(errtxt, err);
			*errline = curline;
			return 0;
		}

		curline = curline->ld->next;
	}

	return 1;
}


PRIVATE int check_case(struct comal_line *fromline, char *errtxt,
		       struct comal_line **errline)
{
	do
		fromline = fromline->ld->next;
	while (fromline && fromline->cmd == 0);

	if (fromline->cmd != whenSYM) {
		*errline = fromline;
		strcpy(errtxt, "CASE should be followed by WHEN");
		return 0;
	}

	return 1;
}


PRIVATE int do_special1(struct scan_entry *p, struct comal_line *theline,
			struct comal_line *curline,
			struct comal_line **errline, char *errtxt,
			int *level, struct comal_line **procroot,
			struct seg_des *seg)
{
	int sym;
	struct comal_line *lineptr;

	switch (p->special) {
	case CASE:
		return check_case(theline, errtxt, errline);

	case DATA_STAT:
		if (seg) {
			strcpy(errtxt,
			       "DATA not allowed in external segment");
			return 0;
		}

		if (curenv->datalptr)
			break;

		curenv->datalptr = curline;
		curenv->dataeptr = curline->lc.exproot;

		break;

	case EXIT:
		scan_stack_search(endloopSYM, -1, &sym, &lineptr);

		if (sym == 0) {
			strcpy(errtxt, "Can only EXIT from within a loop");
			*errline = curline;
			return 0;
		}

		break;

	case RETRY:
		scan_stack_search(endtrapSYM, -1, &sym, &lineptr);

		if (sym == 0) {
			strcpy(errtxt, "Can only RETRY from within the HANDLER part of a TRAP/ENDTRAP");
			*errline = curline;
			return 0;
		}

		break;


	case PROCFUNC:
		if (routine_search_horse
		    (theline->lc.pfrec.id, theline->cmd, *procroot)) {
			sprintf(errtxt, "%s %s multiply defined",
				lex_sym(theline->cmd),
				theline->lc.pfrec.id->name);
			*errline = curline;
			return 0;
		}

		theline->lc.pfrec.proclink = *procroot;
		theline->lc.pfrec.level = *level;
		scan_stack_search(endprocSYM, endfuncSYM, &sym, &lineptr);

		if (seg && !lineptr)
			theline->lc.pfrec.fatherproc =
			    seg->extdef->lc.pfrec.fatherproc;
		else
			theline->lc.pfrec.fatherproc = lineptr;

		if (!theline->lc.pfrec.external) {
			*procroot = NULL;
			(*level)++;
		} else
			*procroot = theline;

		break;

	case EPROCFUNC:
		scan_stack_peek(&sym, &lineptr);

		if (sym == endprocSYM || sym == endfuncSYM) {
			lineptr->lc.pfrec.localproc = *procroot;
			*procroot = lineptr;
			(*level)--;
		}

		break;

	case NOT_SPECIAL:
	case SHORT_FORM:
		break;
	}

	return 1;
}


PUBLIC int scan_scan(struct seg_des *seg, char *errtxt,
		     struct comal_line **errline)
{
	struct comal_line *curline;
	struct comal_line *theline;
	struct comal_line *lineptr;
	struct scan_entry *p;
	int sym;
	int cmd2;
	struct comal_line *procroot = NULL;
	int level;

	if (seg) {
		curline = seg->lineroot;
		level = seg->extdef->lc.pfrec.level;
	} else {
		curline = curenv->progroot;
		level = 0;
	}

	scan_sp = 0;

	while (curline) {
		int skip_processing;

		theline = line_2line(curline);

		if (theline)
			cmd2 = theline->cmd;
		else {
			theline = curline;
			cmd2 = 0;
		}

		skip_processing = 0;

		if (scan_sp == MAX_INDENT) {
			sprintf(errtxt,
				"Too much control structure nesting (max. %d)",
				MAX_INDENT);
			*errline = curline;
			return 0;
		}

		for (p = scan_tab; p->sym && p->sym != theline->cmd; p++);

		if (p->sym) {
			if ((p->special == SHORT_FORM && cmd2) ||
			    (p->special == PROCFUNC
			     && theline->lc.pfrec.external)
			    || (theline->cmd == trapSYM
				&& theline->lc.traprec.esc))
				skip_processing = 1;	/* Prevent indention */

			if (!do_special1
			    (p, theline, curline, errline, errtxt, &level,
			     &procroot, seg))
				return 0;

			if (!skip_processing) {
				if (p->expectsym1 || p->expectsym2) {
					scan_stack_pop(&sym, &lineptr);

					if (sym != p->expectsym1
					    && sym != p->expectsym2) {
						if (sym != -1)
							sprintf(errtxt,
								"%s expected, not %s",
								lex_sym
								(sym),
								lex_sym
								(theline->
								 cmd));
						else
							sprintf(errtxt,
								"Unexpected %s",
								lex_sym
								(theline->
								 cmd));

						*errline = curline;
						return 0;
					} else {
						theline->lineptr = lineptr;
						lineptr->lineptr = theline;
					}
				}

				if (p->leavessym)
					scan_stack_push(p->leavessym,
							curline);
			}
		}

		curline = curline->ld->next;
	}

	if (scan_sp) {
		sprintf(errtxt, "%d open control structure%s at EOP\n",
			scan_sp, scan_sp == 1 ? "" : "s");
		return 0;
	}

	if (seg) {
		seg->procdef = procroot;

		if (procroot->lc.pfrec.proclink) {
			strcpy(errtxt,
			       "More than 1 PROC or FUNC in external segment");
			return 0;
		}
	} else
		curenv->globalproc = procroot;

	if (!scan_pass2(seg, errtxt, errline))
		return 0;

	return 1;
}


static inline int
INDENT(int x)
{
	return (x >= INDENTION * MAX_INDENT ? INDENTION * MAX_INDENT : x);
}

PUBLIC void prog_structure_scan()
{
	int indent = 0;
	struct comal_line *curline = curenv->progroot;
	struct scan_entry *p;

	if (comal_debug)
		my_printf(MSG_DEBUG, 1, "Structure scanning...");

	while (curline) {
		int skip_processing;
		int cmd2;

		cmd2 = line_2cmd(curline);
		skip_processing = 0;

		curline->ld->indent = INDENT(indent);

		for (p = scan_tab; p->sym && p->sym != curline->cmd; p++);

		if (p->sym) {
			if ((p->special == SHORT_FORM && cmd2) ||
			    (p->special == PROCFUNC
			     && curline->lc.pfrec.external)
			    || (curline->cmd == trapSYM
				&& curline->lc.traprec.esc))
				skip_processing = 1;	/* Prevent indention */

			if (!skip_processing) {
				if (p->expectsym1 || p->expectsym2) {
					indent -= INDENTION;

					if (indent < 0)
						indent = 0;

					curline->ld->indent =
					    INDENT(indent);
				}

				if (p->leavessym)
					indent += INDENTION;
			}
		}

		curline = curline->ld->next;
	}
}


PUBLIC int scan_nescessary(struct comal_line *line)
{
	int i;
	int cmd2 = line_2cmd(line);

	for (i = 0; scan_tab[i].sym; i++)
		if (scan_tab[i].sym == line->cmd
		    || scan_tab[i].sym == cmd2) {
			curenv->scan_ok = 0;

			if (scan_tab[i].leavessym)
				if (line_2cmd(line) != 0
				    ||
				    ((line->cmd == procSYM
				      || line->cmd == funcSYM)
				     && line->lc.pfrec.external)
				    || (line->cmd == trapSYM
					&& line->lc.traprec.esc))
					return NO_STRUCTURE;
				else
					return STRUCTURE_START;
			else if (scan_tab[i].expectsym1
				 || scan_tab[i].expectsym2)
				return STRUCTURE_END;
			else
				return NO_STRUCTURE;
		}

	return NO_STRUCTURE;
}


PUBLIC int assess_scan(struct comal_line *line)
{
	const char *msg = NULL;

	if (entering)
		return 0;

	if (line == curenv->curline)
		msg = "the current execution line";
	else if (line == curenv->datalptr)
		msg = "the current DATA line";
	else if (scan_nescessary(line))
		msg = "a program structure line";

	if (msg) {
		curenv->scan_ok = 0;

		if (curenv->running == HALTED && !curenv->con_inhibited) {
			my_printf(MSG_DIALOG, 1,
				  "Adding/Modifying/Deleting %s has inhibited CONtinuation",
				  msg);
			curenv->con_inhibited = 1;
		}

		return 1;
	}

	return 0;
}
