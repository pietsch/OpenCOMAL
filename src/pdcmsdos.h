/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */  
    
/* PDComal header file for Msdos, Turbo C */ 
    
#define HOST_OS		"MsDos"
#define HOST_OS_CODE	2	/* Change when adding another OS! */
#define VERSION		"0.1"
#define CLI		""			/* Command Line Interpreter */
    
#define HUGE_POINTER	huge
    
#define MSDOS
#define TURBOC
#define FLEX
#define HAS_STRLWR
    
#ifdef FLEX
#define bcopy(x,y,z) memcpy((x),(y),(z))
#endif	

#define MININT	(-MAXINT-1)

#include <values.h>
