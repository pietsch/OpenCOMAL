/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

#define HOST_OS		"Win32"
#define HOST_OS_CODE	3	/* Change when adding another OS! */
#define VERSION		"0.1"
#define CLI		""	/* Command Line Interpreter */

#include <values.h>
#define MININT	(-MAXINT-1)

#define HUGE_POINTER
#define EVIL32
#define HAS_STRLWR

#define FLEX

#ifdef FLEX
#define bcopy(x,y,z) memcpy((x),(y),(z))
#endif				/*  */
