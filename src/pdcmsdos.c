/*
 * OpenComal -- a free Comal implementation
 *
 * This file is part of the OpenComal package.
 * (c) Copyright 1992-2002 Jos Visser <josv@osp.nl>
 *
 * The OpenComal package is covered by the GNU General Public
 * License. See doc/LICENSE for more information.
 */

/* PDCOMAL SYS routines for MSDOS, Turbo C */

#include "pdcglob.h"
#include "pdcmisc.h"

#include <alloc.h>
#include <dos.h>
#include <bios.h>
#include <mem.h>
#include <ctype.h>
#include <time.h>		/* For randomize() */
#include <math.h>		/* For matherr() */


#define NCOLS			80
#define NROWS			25

#define MONO			7
#define COLOUR			3

#define NORMAL			7
#define UNDERLINE		1
#define HI_UNDERLINE		9
#define HI			15
#define	REVERSE			0x70
#define BLINKING		0x87
#define BLINKING_HI		0x8f
#define BLINKING_REVERSE	0xf0

#define CTRL_C			3
#define ESCAPE			27
#define RETURN			13
#define BACKSPACE		8
#define INS			0x8052
#define DEL			0x8053
#define CSR_LEFT		0x804b
#define CSR_RIGHT		0x804d
#define END			0x804f
#define HOME			0x8047
#define CTRL_LEFT		0x8073
#define CTRL_RIGHT		0x8074

typedef struct {
	char kar;
	char attr;
} CELL;

typedef CELL SCREEN[NROWS][NCOLS];

PRIVATE SCREEN *video;
PRIVATE void interrupt(*old_cbreak_handler) ();
PRIVATE int monitor;
PRIVATE int escape = 0;
PRIVATE int curx, cury;

/* Disable stack overflow check & register allocation for interrupt func */

#pragma option -N- -r-

PRIVATE void interrupt cbreak()
{
	escape = 1;
}

#pragma option -N+ -r+


PRIVATE void set_textmode(int mode)
{
	union REGS r;

	r.h.al = mode;
	r.h.ah = 0;
	int86(0x10, &r, &r);

	r.x.cx = 0x001f;
	r.h.ah = 1;
	int86(0x10, &r, &r);	/* Set full block cursor */

	curx = cury = 0;
}


PRIVATE void scroll_up(int no)
{
	int x, y;

	memmove(video, &(*video)[no][0],
		sizeof(SCREEN) - no * NCOLS * sizeof(CELL));

	for (y = NROWS - no; y < NROWS; y++)
		for (x = 0; x < NCOLS; x++) {
			(*video)[y][x].attr = 7;
			(*video)[y][x].kar = ' ';
		}
}


PRIVATE void gotoxy(int x, int y)
{
	union REGS r;

	r.h.ah = 2;
	r.h.dh = y;
	r.h.dl = x;
	r.h.bh = 0;
	int86(0x10, &r, &r);
	curx = x;
	cury = y;
}


PRIVATE unsigned getkey()
{
	int c=bioskey(0);

	if (!(c&0xff)) 
		c=(c>>8)+0x8000;
	else
		c=c&0xff;

	return c;
}

PRIVATE unsigned scankey()
{
	int c=bioskey(1);

	if (c==-1) {
		c=0;
		escape=1;
	} else if (c>0)
		c=getkey();

	return c;
}

PRIVATE void read_cursorxy()
{
	union REGS r;

	r.h.ah=3;
	r.h.bh=0;
	int86(0x10,&r,&r);
	curx=r.h.dl;
	cury=r.h.dh;
}

PRIVATE void adjust(int *x, int *y)
{
	if (*x >= NCOLS) {
		*y = *y + *x / NCOLS;
		*x = *x % NCOLS;
	} else if (*x < 0) {
		*y = *y + *x / NCOLS - 1;
		*x = NCOLS - (-*x) % NCOLS;
	}
}


PRIVATE void beep()
{
	union REGS r;

	r.h.ah = 0x0e;
	r.h.al = 7;
	int86(0x10, &r, &r);
}

PUBLIC void sys_setpaged(int n)
{
}

PUBLIC int sys_system(char *cmd)
{
	int rc=system(cmd);

	read_cursorxy();
	return rc;
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
	sys_system(buf);
	free(buf);
}

PUBLIC void sys_chdir(char *dir)
{
	if (chdir(dir)<0)
		run_error(DIR_ERR,strerror(errno));
}

PUBLIC void sys_mkdir(char *dir)
{
	if (!mkdir(dir,0777)<0)
		run_error(DIR_ERR,strerror(errno));
} 

PUBLIC void sys_rmdir(char *dir)
{
	if (rmdir(dir)<0)
		run_error(DIR_ERR,strerror(errno));
}

PUBLIC char *sys_key(long _delay)
{
	static char result[2] = {0,0};
	int c=0;
	int n;

	/*
	 * -1 means neverending delay 
	 */
	if (_delay<0) {
		c=getkey();
	} else if (_delay==0) {
		c=scankey();
	} else {
		/*
		 * This code sucks
		 */
		while (!c && _delay>0) {
			n=200;
			for (n=200; !c && n>0; n--) {
				c=scankey();

				if (c==0) delay(5);
			}

			_delay--;
		}
	}

	*result=c;
	return result;
}



PUBLIC void sys_init()
{
	union REGS r;

	randomize();

	old_cbreak_handler = getvect(0x1b);
	setvect(0x1b, cbreak);

	int86(0x11, &r, &r);

	if (((r.x.ax & 0x30) >> 4) == 3)
		monitor = MONO;
	else
		monitor = COLOUR;

	if (monitor == COLOUR)
		video = MK_FP(0xb800, 0);
	else
		video = MK_FP(0xb000, 0);

	set_textmode(monitor);
	ext_init();
}


PUBLIC void sys_tini()
{
	setvect(0x1b, old_cbreak_handler);
	ext_tini();
}

#pragma exit sys_tini


PUBLIC void  sys_rand(long *result, long *max)
{
	*result = rand();
	*max = RAND_MAX;
}


PRIVATE void flush_kbd()
{
	int *biosdata = MK_FP(0x40, 0);

	*(biosdata + 0xd) = 0x1e;
	*(biosdata + 0xe) = 0x1e;
}


PUBLIC int sys_escape()
{
	if (escape) {
		escape = 0;
		flush_kbd();
		return 1;
	}

	return 0;
}


PUBLIC void sys_nl(int stream)
{
	curx = 0;
	cury++;

	if (cury == NROWS) {
		scroll_up(1);
		cury = NROWS - 1;
	}

	gotoxy(curx, cury);
}


PUBLIC void sys_put(int stream, char *buf, long len)
{
	char attr = (stream == MSG_ERROR) ? REVERSE : NORMAL;
	CELL *p = &(*video)[cury][curx];

	while (*buf) {
		p->kar = *buf;
		p->attr = attr;
		buf++;
		p++;
		curx++;

		if (curx == NCOLS) {
			curx = 0;
			cury++;

			if (cury == NROWS) {
				scroll_up(1);
				cury = NROWS - 1;
				p = &(*video)[NROWS - 1][0];
			}
		}
	}

	gotoxy(curx, cury);
}


PUBLIC void sys_page(FILE * f)
{
	if (!f)
		set_textmode(monitor);
	else
		fputc(12, f);
}


PUBLIC void sys_cursor(FILE * f, long x, long y)
{
	if (f)
		return;

	y--;
	x--;

	if (y >= NROWS || y < 0 || x >= NCOLS || x < 0)
		run_error(CURSOR_ERR, "CURSOR x,y out of bounds");

	gotoxy((int) x, (int) y);
}


PUBLIC void sys_screen_readjust()
{
	union REGS r;

	r.h.ah = 3;
	r.h.bh = 0;		/* Video page */
	int86(0x10, &r, &r);
	curx = r.h.dl;
	cury = r.h.dh;
}


PUBLIC int sys_yn(int stream, char *title)
{
	char s[2];

	sys_put(stream, title, strlen(title));
	s[1] = '\0';

	do {
		s[0] = getkey();

		if (sys_escape())
			s[0] = 'N';

		if (islower(s[0]))
			s[0] = toupper(s[0]);
	}
	while (s[0] != 'Y' && s[0] != 'N');

	sys_put(stream, s, -1L);
	sys_nl(stream);

	return s[0] == 'Y';
}


PRIVATE int do_get(int stream, char line[], int maxlen, char *prompt,
		   int cursor)
{
	int startx, starty;
	int endx, endy;
	int x, y;
	int len = strlen(line);
	unsigned c;
	int i;
	int insert = 0;
	CELL *cell;

	cursor--;		/* Computer scientists count from 0 onwards */
	sys_put(MSG_DIALOG, prompt, -1L);
	sys_put(MSG_DIALOG, line, (long) len);
	sys_put(MSG_DIALOG, " ", 1L);
	startx = curx - len - 1;
	starty = cury;
	adjust(&startx, &starty);

	if (cursor > len)
		cursor = len;

	y = starty;
	x = startx + cursor;

	while (1 == 1) {
		adjust(&x, &y);
		gotoxy(x, y);
		c = getkey();

		if (sys_escape())
			c = ESCAPE;

		switch (c) {
		case RETURN:
			sys_nl(stream);
			return 0;

		case CTRL_C:
		case ESCAPE:
			sys_nl(stream);
			line[0] = '\0';
			return 1;

		case HOME:
			cursor = 0;
			x = startx;
			y = starty;
			break;

		case END:
			cursor = len;
			x = startx + len;
			y = starty;
			break;

		case CSR_LEFT:
			if (cursor > 0) {
				cursor--;
				x--;
			} else
				beep();

			break;

		case CSR_RIGHT:
			if (cursor < len) {
				cursor++;
				x++;
			} else
				beep();

			break;

		case INS:
			insert = !insert;
			break;

		case BACKSPACE:
			if (cursor <= 0) {
				beep();
				break;
			}

			cursor--;
			x--;
			/* Fall through */

		case DEL:
			if (cursor < len) {
				cell = &(*video)[y][x];

				for (i = cursor; i < len; i++, cell++) {
					cell->kar =
					    line[i + 1] ? line[i +
							       1] : ' ';
					line[i] = line[i + 1];
				}

				len--;
				line[len] = '\0';
			} else
				beep();

			break;

		case CTRL_RIGHT:
			i = cursor;

			while (line[i] != ' ' && i < len)
				i++, x++;

			while (line[i] == ' ' && i < len)
				i++, x++;

			cursor = i;
			break;

		case CTRL_LEFT:
			i = cursor;

			if (i > 0 && line[i] != ' ' && line[i - 1] == ' ')
				i--, x--;

			while (line[i] == ' ' && i)
				i--, x--;

			while (line[i] != ' ' && i)
				i--, x--;

			if (i)
				i++, x++;

			cursor = i;
			break;

		default:
			if (insert && len == maxlen)
				beep();
			else if (c >= ' ' && c < 256) {
				cell = &(*video)[y][x];

				if (insert) {
					endy = starty;
					endx = startx + len;
					adjust(&endx, &endy);

					if (endy == NROWS - 1
					    && endx == NCOLS - 1) {
						scroll_up(1);
						starty--;
						y--;
						cell -= NCOLS;
					}

					for (i = len - 1; i >= cursor; i--)
						line[i + 1] = line[i];

					line[cursor] = c;
					len++;
					line[len] = '\0';

					cell->kar = c;
					cell++;

					for (i = cursor + 1; i < len;
					     i++, cell++)
						cell->kar = line[i];

					cursor++;
					x++;
				} else if (cursor == len && len == maxlen)
					beep();
				else {
					line[cursor] = c;

					if (cursor == len) {
						len++;
						line[len] = '\0';
					}

					cell->kar = c;
					cursor++;
					x++;

					if (x == NCOLS && y == NROWS - 1) {
						scroll_up(1);
						starty--;
						y--;
					}
				}
			} else
				beep();
		}
	}
}


PUBLIC int sys_get(int stream, char *line, int maxlen, char *prompt)
{
	if (ext_get(stream, line, maxlen, prompt))
		return 0;
	else
		return do_get(stream, line, maxlen, prompt, 1);
}


PUBLIC int sys_edit(int stream, char line[], int maxlen, int cursor)
{
	return do_get(stream, line, maxlen, "E ", cursor);
}


PUBLIC void *sys_alloc(long size)
{
	return farcalloc(1, size);
}


PUBLIC void *sys_realloc(void *block, long newsize)
{
	return farrealloc(block, newsize);
}


PUBLIC void sys_free(void *p)
{
	farfree(p);
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


PUBLIC int sys_call(struct id_rec *id, struct exp_list *exproot, int
		    calltype, void **result, enum VAL_TYPE *type)
{
	return ext_call(id, exproot, calltype, result, type);
}


PUBLIC void sys_sys_exp(struct exp_list *exproot, void **result, enum
			VAL_TYPE *type)
{
	if (ext_sys_exp(exproot, result, type) == -1)
		run_error(SYS_ERR, "Unknown SYS() command");
}


PUBLIC void sys_syss_exp(struct exp_list *exproot, struct string **result, enum
			 VAL_TYPE *type)
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


PUBLIC int matherr(struct exception *e)
{
	char *err;

	switch (e->type) {
	case DOMAIN:
		err = "Domain";
		break;
	case SING:
		err = "Singularity";
		break;
	case OVERFLOW:
		err = "Overflow";
		break;
	case UNDERFLOW:
		err = "Underflow";
		break;
	case PLOSS:
		err = "Partial loss of significance";
		break;
	case TLOSS:
		err = "Total loss of significance";
		break;
	default:
		err = "Unknown";
	}

	run_error(MATH_ERR, "Math err: %s error when executing %s", err,
		  e->name);

	return 0;
}
