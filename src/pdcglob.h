/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/*
 * GLOB.H
 *
 * This header file contains declarations global to the entire
 * OpenComal interpreter.
 *
 */

#ifndef PDCGLOB_H
#define PDCGLOB_H

#include "pdcconst.h"
#include "pdcsys.h"
#include "pdcdef.h"
#include "pdcmem.h"
#include "pdcerr.h"
#include "pdcmsg.h"
#include "pdcfunc.h"

#ifndef PDCPARS
#ifdef OS_MSDOS
#include "pdcpars.h"
#else
#include "pdcpars.tab.h"
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#define PRIVATE static
#define PUBLIC

#ifndef EXTERN
#define EXTERN extern
#endif

#define NO_STRUCTURE	(0)	/* return values from scan_nescessary */
#define STRUCTURE_START	(1)	/* return values from scan_nescessary */
#define STRUCTURE_END	(2)

#define COMMAND(x)	(32767-x)	/* to distinguish between statements & command from (e.q. RUN, DEL) */

/* RESTART entry codes */

#define JUST_RESTART	1	/* Nothing special, restart interpreter */
#define QUIT		2	/* Restart code = QUIT */
#define RUN		3	/* Restart code = RUN */
#define ERR_FATAL	666	/* fatal error occurred */

EXTERN jmp_buf RESTART;		/* restart entry in the interpreter after error */
EXTERN jmp_buf ERRBUF;		/* Continue point after run_error */

EXTERN struct comal_env *curenv;	/* Current COMAL environment */
EXTERN int entering;		/* ENTER in progress */
EXTERN int comal_debug;		/* Internal debugging switch */

EXTERN FILE *sel_outfile;	/* For select output */
EXTERN FILE *sel_infile;	/* For select input */

EXTERN const char *copyright;
EXTERN char *runfilename;

EXTERN struct env_list *env_root;

#endif
