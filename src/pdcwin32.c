/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/* This file contains the system dependent routines for Win32 */

#include <windows.h>
#include <dir.h>
#include <errno.h>

#include "pdcglob.h"
#include "pdcmisc.h"

PRIVATE HANDLE conin;
PRIVATE HANDLE conout;
PRIVATE int nrows, ncols;
PRIVATE int escape = 0;


PRIVATE BOOL handler(DWORD type)
{
	switch (type) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		escape = 1;
		return 1;
	}
	return 0;
}


PUBLIC void sys_init()
{
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	ext_init();
	conin = GetStdHandle(STD_INPUT_HANDLE);
	conout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE) handler, 1);
	GetConsoleScreenBufferInfo(conout, &coninfo);
	nrows = coninfo.dwSize.Y;
	ncols = coninfo.dwSize.X;
	SetConsoleMode(conin, ENABLE_PROCESSED_INPUT);
	SetConsoleMode(conout,
		       ENABLE_PROCESSED_OUTPUT |
		       ENABLE_WRAP_AT_EOL_OUTPUT);
}


PUBLIC void sys_tini()
{
	ext_tini();
}

PUBLIC void sys_setpaged(int flag) 
{
}

PUBLIC int sys_system(char *cmd)
{
	return system(cmd);
}

PUBLIC char *sys_unit_string()
{
	static char buf[3];
	int disk=getdisk();

	buf[0]=disk+'A';
	buf[1]=':';
	buf[2]=0;

	return buf;
}

PUBLIC void sys_unit(char *drive)
{
	if (*drive>='A' && *drive<='Z')
		setdisk(*drive-'A');
	else if (*drive>='a' && *drive<='z')
		setdisk(*drive-'a');
	else
		run_error(SYS_ERR,"Illegal UNIT specification '%s'",drive);
}

PUBLIC char *sys_dir_string() 
{
	static int buf_size=1024;
	static char *buf=0;

	while (1==1) {
		if (!buf) buf=malloc(buf_size);

		if (getcwd(buf,buf_size)!=NULL) return buf;

		if (errno==ERANGE) { /* buffer too small */
			buf_size+=1024;
			free(buf);
			buf=0;
		} else
			run_error(DIRS_ERR,strerror(errno));
	}
}

PUBLIC void sys_dir(char *pattern) {
	FILE *f;
	int l=strlen(pattern);
	char *buf=malloc(8+l);
	char line[256];
	
	strcpy(buf,"dir ");
	strcat(buf,pattern);
	system(buf);
	free(buf);
}

PUBLIC void sys_rand(long *result, long *max)
{
	*result = rand();
	*max = RAND_MAX;
}


PUBLIC int sys_escape()
{
	if (escape) {
		escape = 0;
		return 1;
	}
	return 0;
}


PRIVATE void gotoxy(int x, int y)
{
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(conout, coord);
}

PRIVATE void getxy(int *x, int *y)
{
	CONSOLE_SCREEN_BUFFER_INFO buf;

	GetConsoleScreenBufferInfo(conout, &buf);
	*x = buf.dwCursorPosition.X;
	*y = buf.dwCursorPosition.Y;
}

PRIVATE void page()
{
	COORD home;
	DWORD dummy;
	home.X = home.Y = 0;
	FillConsoleOutputCharacter(conout, ' ', nrows * ncols, home,
				   &dummy);
	gotoxy(0, 0);
}

PRIVATE void conput(char c)
{
	DWORD dummy;
	WriteConsole(conout, &c, 1, &dummy, 0);
}

PUBLIC void sys_put(int stream, char *buf, long len)
{
	ext_put(stream, buf, len);
	for (; len && *buf; len--, buf++)
		conput(*buf);
}


PUBLIC void sys_page(FILE * f)
{
	COORD home;
	DWORD dummy;
	if (f)
		ext_page();

	else
		page();
}

PUBLIC void sys_cursor(FILE * f, long x, long y)
{
	if (f)
		return;

	gotoxy(x + 1, y + 1);
}


PUBLIC void sys_nl(int stream)
{
	ext_nl();
	conput('\n');
}

PUBLIC void sys_screen_readjust()
{
}

PRIVATE int getkey(int *vkey)
{
	INPUT_RECORD in;
	DWORD dummy;
	while (1 == 1) {
		ReadConsoleInput(conin, &in, 1, &dummy);
		if (in.EventType != KEY_EVENT)
			continue;
		if (!in.Event.KeyEvent.bKeyDown)
			continue;
		if (vkey)
			*vkey = in.Event.KeyEvent.wVirtualKeyCode;
		return in.Event.KeyEvent.uChar.AsciiChar;
	}
	return 1;
}

PUBLIC int sys_yn(int stream, char *s)
{
	char c;
	for (;;) {
		my_printf(MSG_DIALOG, 0, s);
		c = getkey(0);
		if (c == 'y' || c == 'Y')
			return 1;

		else if (c == 'n' || c == 'N');
		return 0;
	}
}

PRIVATE int do_get(int stream, char *line, int maxlen, char *prompt)
{
	int i = 0;
	char c;
	int key;
	if (prompt)
		my_printf(stream, 0, prompt);
	while (1 == 1) {
		c = getkey(&key);
		if (sys_escape())
			return 1;
		switch (key) {
		case VK_RETURN:
			line[i] = 0;
			conput('\n');
			return 0;
		case VK_BACK:
			if (i > 0) {
				i--;
				conput(8);
			}
			break;
		default:
			if (c >= 32) {
				line[i] = c;
				conput(c);
				i++;
				if (i == maxlen - 1) {
					line[i] = 0;
					return 0;
				}
			}
			break;
		}
	}
}


PUBLIC int sys_get(int stream, char *line, int maxlen, char *prompt)
{
	if (ext_get(stream, line, maxlen, prompt))
		return 0;

	else
		return do_get(stream, line, maxlen, prompt);
}

/*
 * New sys_edit for Win32 thanks to Gary Lake.
 */
PUBLIC int sys_edit(int stream, char line[], int maxlen, int cursor)
{
    int i;
    char c;
    int key;

    my_printf(stream, 0, "E ");
    for (i = 0; line[i]; i++) {
        if (i == cursor)
            conput('!');
        conput(line[i]);
    }

    while (1 == 1) {
        c = getkey(&key);
        if (sys_escape())
            return 1;
        switch (key) {
        case VK_RETURN:
            line[i] = 0;
            conput('\n');
            return 0;
        case VK_BACK:
            if (i > 0) {
                i--;
                conput(8);
            }
            break;
        default:
            if (c >= 32) {
                line[i] = c;
                conput(c);
                i++;
                if (i == maxlen - 1) {
                    line[i] = 0;
                    return 0;
                }
            }
            break;
        }
    }
}


PUBLIC void *sys_alloc(long size)
{
	if (size > MAXUNSIGNED)
		return NULL;
	return calloc(1, (unsigned) size);
}

PUBLIC void *sys_realloc(void *block, long newsize)
{
	if (newsize > MAXUNSIGNED)
		return NULL;
	return realloc(block, (unsigned) newsize);
}

PUBLIC void sys_free(void *p)
{
	free(p);
}
PUBLIC void sys_chdir(char *dir)
{
	if (chdir(dir)<0)
		run_error(DIR_ERR,strerror(errno));
}

PUBLIC void sys_mkdir(char *dir)
{
	if (!mkdir(dir)<0)
		run_error(DIR_ERR,strerror(errno));
} 

PUBLIC void sys_rmdir(char *dir)
{
	if (rmdir(dir)<0)
		run_error(DIR_ERR,strerror(errno));
}

PUBLIC char *sys_key(long delay)
{
	static char result[2] = {0,0};
	int c=0;

	while (1) {
		/*
	 	* -1 means neverending delay 
	 	*/
		if (delay<0) {
			c=getkey(NULL);
		} else if (WaitForSingleObject(conin,delay*1000)==WAIT_OBJECT_0)
			c=getkey(NULL);

		if (c!=0) break;
	}

	*result=c;
	return result;
}

PUBLIC int sys_call_scan(struct id_rec *id, struct exp_list *exproot,
			 char *errtext)
{
	int rc = ext_call_scan(id, exproot, errtext);
	if (rc == -1) {
		sprintf(errtext, "PROCedure %s not found", id->name);
		return 0;
	}
	return rc;
}

PUBLIC int sys_call(struct id_rec *id, struct exp_list *exproot,
		    int calltype, void **result, enum VAL_TYPE *type)
{
	return ext_call(id, exproot, calltype, result, type);
}

PUBLIC void sys_sys_exp(struct exp_list *exproot, void **result,
			enum VAL_TYPE *type)
{
	if (ext_sys_exp(exproot, result, type) == -1)
		run_error(SYS_ERR, "Unknown SYS() command");
}

PUBLIC void sys_syss_exp(struct exp_list *exproot, struct string **result,
			 enum VAL_TYPE *type)
{
	if (ext_syss_exp(exproot, result, type) == -1)
		run_error(SYS_ERR, "Unknown SYS$() command");
}

PUBLIC int sys_sys_stat(struct exp_list *exproot)
{
	int rc = ext_sys_stat(exproot);
	if (rc == -1)
		run_error(SYS_ERR, "Unknown SYS statement command");
	return rc;
}
