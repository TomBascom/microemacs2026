/* extruct.h
 *
 * common pre-processor definitions for MicroEMACS 5.03
 *
 * (C)Copyright 1993 by Daniel Lawrence based on code by Dave G. Conroy, Steve Wilhite and George Jones
 */

#define PROGNAME	"MicroEMACS"	/* Macros can query this via the $progname variable	*/
#define VERSION 	"2026"

/*	Emacs global flag bit definitions (for gflags)	*/

#define GFREAD	1	/* read first file in at startup */
#define GFSDRAW 2	/* suppress a screen redraw */

/*	internal constants	*/

#define RETCHAR		'\r'
#define DIRSEPSTR	"/"
#define DIRSEPCHAR	'/'
#define DRIVESEPCHAR	':'
#define PATHCHR		':'
#define DUMMYSZ     	/* nothing */

#define NBINDS	300			/* max # of bound keys		*/
#define NFILEN	80			/* # of bytes, file name	*/
#define NBUFN	32			/* # of bytes, buffer name	*/
#define NLINE	256			/* # of bytes, input line	*/
#define NSTRING 500			/* # of bytes, string buffers	*/
#define NKBDM	256			/* # of strokes, keyboard macro */
#define NPAT	128			/* # of bytes, pattern		*/
#define HUGE	1000			/* Huge number			*/
#define NLOCKS	256			/* max # of file locks active	*/
#define NCOLORS 16			/* number of supported colors	*/
#define KBLOCK	250			/* sizeof kill buffer chunks	*/
#define NRING	16			/* # of buffers in kill ring	*/
#define NBLOCK	16			/* line block chunk size	*/
#define NVSIZE	10			/* max #chars in a var name	*/
#define NMARKS	16			/* number of marks		*/

/* Some system headers (e.g. <sys/ttydefaults.h> included via <termios.h>)
 * define CTRL as a function-like macro.  Drop any prior definition so our
 * key-modifier flag below wins regardless of include order.
 */
#ifdef CTRL
#undef CTRL
#endif
#define CTRL	0x0100		/* Control flag, or'ed in		*/
#define META	0x0200		/* Meta flag, or'ed in			*/
#define CTLX	0x0400		/* ^X flag, or'ed in			*/
#define SPEC	0x0800		/* special key (function keys)		*/
#define MOUS	0x1000		/* alternative input device (mouse)	*/
#define SHFT	0x2000		/* shifted (for function keys)		*/
#define ALTD	0x4000		/* ALT key...				*/
#define MENU	MOUS+SPEC	/* menu selection (WINDOW_MSWIN)	*/

#define BINDNUL 0		/* not bound to anything		*/
#define BINDFNC 1		/* key bound to a function		*/
#define BINDBUF 2		/* key bound to a buffer		*/

#define FALSE	0			/* False, no, bad, etc. 	*/
#define TRUE	1			/* True, yes, good, etc.	*/
#define ABORT	2			/* Death, ^G, abort, etc.	*/
#define FAILED	3			/* not-quite fatal false return */

#define STOP	0			/* keyboard macro not in use	*/
#define PLAY	1			/*		  playing	*/
#define RECORD	2			/*		  recording	*/

/*	Completion types 	*/

#define CMP_BUFFER	0
#define CMP_COMMAND	1
#define CMP_FILENAME	2

/*	Directive definitions	*/

#define DIF		0
#define DELSE		1
#define DENDIF		2
#define DGOTO		3
#define DRETURN 	4
#define DENDM		5
#define DWHILE		6
#define DENDWHILE	7
#define DBREAK		8
#define DFORCE		9

#define NUMDIRS 	10

/* PTBEG, PTEND, FORWARD, and REVERSE are all toggle-able values for routines that need directions.	 */

#define PTBEG	0	/* Leave the point at the beginning on search	*/
#define PTEND	1	/* Leave the point at the end on search 	*/
#define FORWARD 0	/* do things in a forward direction		*/
#define REVERSE 1	/* do things in a backwards direction		*/

#define FIOSUC	0			/* File I/O, success.		*/
#define FIOFNF	1			/* File I/O, file not found.	*/
#define FIOEOF	2			/* File I/O, end of file.	*/
#define FIOERR	3			/* File I/O, error.		*/
#define FIOMEM	4			/* File I/O, out of memory	*/
#define FIOFUN	5			/* File I/O, eod of file/bad line*/
#define FIODEL	6			/* Can't delete/rename file	*/

#define CFCPCN	0x0001			/* Last command was C-P, C-N	*/
#define CFKILL	0x0002			/* Last command was a kill	*/
#define CFSRCH	0x0004			/* last command was a search	*/
#define CFYANK	0x0008			/* last command was a yank	*/

#define SRNORM	0			/* end past, begin front	*/
#define SRBEGIN 1			/* always at front		*/
#define SREND	2			/* always one past end		*/

#define BELL	0x07			/* a bell character		*/
#define TAB	0x09			/* a tab character		*/

#define INTWIDTH	sizeof(int) * 3
#define LONGWIDTH	sizeof(long) * 3

/*	Macro argument token types					*/

#define TKNUL	0			/* end-of-string		*/
#define TKARG	1			/* interactive argument 	*/
#define TKBUF	2			/* buffer argument		*/
#define TKVAR	3			/* user variables		*/
#define TKENV	4			/* environment variables	*/
#define TKFUN	5			/* function.... 		*/
#define TKDIR	6			/* directive			*/
#define TKLBL	7			/* line label			*/
#define TKLIT	8			/* numeric literal		*/
#define TKSTR	9			/* quoted string literal	*/
#define TKCMD	10			/* command name 		*/

/*	Internal defined functions					*/

#define nextab(a)	(a - (a % tabsize)) + tabsize

#define FILENAMEREPLY(p,b,nb)   mlreply(p,b,nb)

/* There is a window structure allocated for every active display window. The
 * windows are kept in a big list, in top to bottom screen order, with the
 * listhead at "wheadp". Each window contains its own values of dot and mark.
 * The flag field contains some bits that are set by commands to guide
 * redisplay. Although this is a bit of a compromise in terms of decoupling,
 * the full blown redisplay is just too expensive to run for every input
 * character.
 */

typedef struct	WINDOW {
	struct	WINDOW *w_wndp; 	/* Next window			*/
	struct	BUFFER *w_bufp; 	/* Buffer displayed in window	*/
	struct	LINE *w_linep;		/* Top line in the window	*/
	struct	LINE *w_dotp;		/* Line containing "."		*/
	short	w_doto; 		/* Byte offset for "."		*/
	struct	LINE *w_markp[NMARKS];	/* Line containing "mark"	*/
	short	w_marko[NMARKS];	/* Byte offset for "mark"	*/
	char	w_toprow;		/* Origin 0 top row of window	*/
	char	w_ntrows;		/* # of rows of text in window	*/
	char	w_force;		/* If NZ, forcing row.		*/
	char	w_flag; 		/* Flags.			*/
	char	w_fcolor;		/* current forground color	*/
	char	w_bcolor;		/* current background color	*/
	int	w_fcol; 		/* first column displayed	*/
}	WINDOW;

#define WFFORCE 0x01			/* Window needs forced reframe	*/
#define WFMOVE	0x02			/* Movement from line to line	*/
#define WFEDIT	0x04			/* Editing within a line	*/
#define WFHARD	0x08			/* Better to a full display	*/
#define WFMODE	0x10			/* Update mode line.		*/
#define WFCOLR	0x20			/* Needs a color change 	*/

/* This structure holds the information about each line appearing on the
 * video display. The redisplay module uses an array of virtual display
 * lines. On systems that do not have direct access to display memory,
 * there is also an array of physical display lines used to minimize
 * video updating. In most cases, these two arrays are unique. If
 * WINDOW_MSWIN is 1, there is a pair of such arrays in each SCREEN
 * structure.
 */

typedef struct	VIDEO {
	int	v_flag; 		/* Flags */
	int	v_fcolor;		/* current forground color */
	int	v_bcolor;		/* current background color */
	int	v_rfcolor;		/* requested forground color */
	int	v_rbcolor;		/* requested background color */
	int	v_left;			/* left edge of reverse video */
	int	v_right;		/* right right of reverse video */
	char	v_text[1];		/* Screen data. */
}	VIDEO;

#define VFNEW	0x0001			/* contents not meaningful yet	*/
#define VFCHG	0x0002			/* Changed flag 		*/
#define VFEXT	0x0004			/* extended (beyond column 80)	*/
#define VFCOL	0x0008			/* color change requested	*/

/*	This structure holds the information about each separate "screen"
 * within the current editing session.  On a character based system, these
 * screens overlay each other, and can individually be brought to front.
 * On a windowing system like MicroSoft Windows 3.0, OS/2, the Macintosh,
 * Intuition, Sunview or X-windows, each screen is represented in an OS
 * window.  The terminolgy is wrong in emacs.....
 *
 *	EMACS		The outside World
 *	screen		window
 *	window		pane
 */

typedef struct SCREEN {
	struct SCREEN *s_next_screen;	/* link to next screen in list */
	WINDOW *s_first_window; 	/* head of linked list of windows */
	WINDOW *s_cur_window;		/* current window in this screen */
	char *s_screen_name;		/* name of the current window */
	short s_roworg; 		/* row origin of stored screen */
	short s_colorg; 		/* column origin of stored screen */
	short s_nrow;			/* row width of stored screen */
	short s_ncol;			/* column origin of stored screen */
} SCREEN;

/* Text is kept in buffers. A buffer header, described below, exists for every
 * buffer in the system. The buffers are kept in a big list, so that commands
 * that search for a buffer by name can find the buffer header. There is a
 * safe store for the dot and mark in the header, but this is only valid if
 * the buffer is not being displayed (that is, if "b_nwnd" is 0). The text for
 * the buffer is kept in a circularly linked list of lines, with a pointer to
 * the header line in "b_linep".  Buffers may be "Inactive" which means the
 * files associated with them have not been read in yet.  These get read in
 * at "use buffer" time.
 */

typedef struct	BUFFER {
	struct	BUFFER *b_bufp; 	/* Link to next BUFFER		*/
	struct	LINE *b_dotp;		/* Link to "." LINE structure	*/
	short	b_doto; 		/* Offset of "." in above LINE	*/
	struct	LINE *b_markp[NMARKS];	/* The same as the above two,	*/
	short	b_marko[NMARKS];	/* but for the "mark"		*/
	int	b_fcol; 		/* first col to display 	*/
	struct	LINE *b_linep;		/* Link to the header LINE	*/
	struct	LINE *b_topline;	/* Link to narrowed top text	*/
	struct	LINE *b_botline;	/* Link to narrowed bottom text */
	char	b_active;		/* window activated flag	*/
	char	b_nwnd; 		/* Count of windows on buffer	*/
	char	b_exec; 		/* how many active executions	*/
	char	b_flag; 		/* Flags			*/
	int	b_mode; 		/* editor mode of this buffer	*/
	char	b_fname[NFILEN];	/* File name			*/
	char	b_bname[NBUFN]; 	/* Buffer name			*/
}	BUFFER;

#define BFINVS	0x01			/* Internal invisable buffer	*/
#define BFCHG	0x02			/* Changed since last write	*/
#define BFTRUNC 0x04			/* buffer was truncated when read */
#define BFNAROW 0x08			/* buffer has been narrowed	*/

/*	mode flags	*/

#define NUMMODES	10	       /* # of defined modes	       */

#define MDWRAP	0x0001			/* word wrap			*/
#define MDCMOD	0x0002			/* C indentation and fence match*/
#define MDSPELL 0x0004			/* spell error parsing		*/
#define MDEXACT 0x0008			/* Exact matching for searches	*/
#define MDVIEW	0x0010			/* read-only buffer		*/
#define MDOVER	0x0020			/* overwrite mode		*/
#define MDMAGIC 0x0040			/* regular expresions in search */
#define MDASAVE 0x0100			/* auto-save mode		*/
#define MDREPL	0x0200			/* replace mode 		*/

/* The starting position of a region, and the size of the region in
 * characters, is kept in a region structure.  Used by the region commands.
 */

typedef struct	{
	struct	LINE *r_linep;		/* Origin LINE address. 	*/
	short	r_offset;		/* Origin LINE offset.		*/
	long	r_size; 		/* Length in characters.	*/
}	REGION;

/* All text is kept in circularly linked lists of "LINE" structures. These
 * begin at the header line (which is the blank line beyond the end of the
 * buffer). This line is pointed to by the "BUFFER". Each line contains a the
 * number of bytes in the line (the "used" size), the size of the text array,
 * and the text. The end of line is not stored as a byte; it's implied.
 */

typedef struct	LINE {
	struct	LINE *l_fp;		/* Link to the next line	*/
	struct	LINE *l_bp;		/* Link to the previous line	*/
	short	l_size; 		/* Allocated size		*/
	short	l_used; 		/* Used size			*/
	char	l_text[1];		/* A bunch of characters.	*/
}	LINE;

#define lforw(lp)	((lp)->l_fp)
#define lback(lp)	((lp)->l_bp)
#define lgetc(lp, n)	((lp)->l_text[(n)])
#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define lused(lp)	((lp)->l_used)
#define lsize(lp)	((lp)->l_size)
#define ltext(lp)	((lp)->l_text)

/* The editor communicates with the display using a high level interface. A
 * "TERM" structure holds useful variables, and indirect pointers to routines
 * that do useful operations. The low level get and put routines are here too.
 * This lets a terminal, in addition to having non standard commands, have
 * funny get and put character code too. The calls might get changed to
 * "termp->t_field" style in the future, to make it possible to run more than
 * one terminal type.
 */

typedef struct	{
	short	t_mrow; 		/* max number of rows allowable */
	short	t_nrow; 		/* current number of rows used	*/
	short	t_mcol; 		/* max Number of columns.	*/
	short	t_ncol; 		/* current Number of columns.	*/
	short	t_roworg;		/* origin row (normally zero)	*/
	short	t_colorg;		/* origin column (normally zero)*/
	short	t_margin;		/* min margin for extended lines*/
	short	t_scrsiz;		/* size of scroll region "	*/
	int	t_pause;		/* # times thru update to pause */
	int ( *t_open)(void);    /* Open terminal at the start.*/
	int ( *t_close)(void);   /* Close terminal at end.	*/
	int ( *t_kopen)(void);   /* Open keyboard		*/
	int ( *t_kclose)(void);  /* Close keyboard		*/
	int ( *t_getchar)(void); /* Get character from keyboard. */
	int ( *t_putchar)(int);  /* Put character to display.*/
	int ( *t_flush)(void);   /* Flush output buffers.	*/
	int ( *t_move)(int, int);/* Move the cursor, origin 0.*/
	int ( *t_eeol)(void);    /* Erase to end of line.	*/
	int ( *t_eeop)(void);    /* Erase to end of page.	*/
	int ( *t_clrdesk)(void); /* Clear the page totally	*/
	int ( *t_beep)(void);    /* Beep.			*/
	int ( *t_rev)(int);      /* set reverse video state	*/
	int ( *t_rez)(char *);   /* change screen resolution	*/
	int ( *t_setfor)(int);   /* set forground color	*/
	int ( *t_setback)(int);  /* set background color 	*/
}	TERM;

/*	TEMPORARY macros for terminal I/O  (to be placed in a machine dependant place later) */

#define TTopen		(*term.t_open)
#define TTclose 	(*term.t_close)
#define TTkopen 	(*term.t_kopen)
#define TTkclose	(*term.t_kclose)
#define TTgetc		(*term.t_getchar)
#define TTputc		(*term.t_putchar)
#define TTflush 	(*term.t_flush)
#define TTmove		(*term.t_move)
#define TTeeol		(*term.t_eeol)
#define TTeeop		(*term.t_eeop)
#define TTclrdesk	(*term.t_clrdesk)
#define TTbeep		(*term.t_beep)
#define TTrev		(*term.t_rev)
#define TTrez		(*term.t_rez)
#define TTforg		(*term.t_setfor)
#define TTbacg		(*term.t_setback)

/*	Structure for the table of current key bindings 	*/

union EPOINTER {
	int ( *fp)(int, int);	/* C routine to invoke */
	BUFFER *buf;			/* buffer to execute */
};

typedef struct	{
	short k_code;		/* Key code			*/
	short k_type;		/* binding type (C function or EMACS buffer) */
	union EPOINTER k_ptr;	/* ptr to thing to execute */
}	KEYTAB;

/*	structure for the name binding table		*/

typedef struct {
	char *n_name;			/* name of function key */
	int ( *n_func)(int, int);	/* function name is bound to */
}	NBIND;

/* The editor holds deleted text chunks in the KILL buffer. The
 * kill buffer is logically a stream of ascii characters, however
 * due to its unpredicatable size, it gets implemented as a linked
 * list of chunks. (The d_ prefix is for "deleted" text, as k_
 * was taken up by the keycode structure)
 */

typedef struct KILL {
	struct KILL *d_next;   /* link to next chunk, NULL if last */
	char d_chunk[KBLOCK];	/* deleted text */
} KILL;

/* When emacs's command interpetor needs to get a variable's name,
 * rather than it's value, it is passed back as a VDESC variable
 * description structure. The v_num field is a index into the
 * appropriate variable table.
 */

typedef struct VDESC {
	int v_type;    /* type of variable */
	int v_num;	/* ordinal pointer to variable in list */
} VDESC;

/* The !WHILE directive in the execution language needs to
 * stack references to pending whiles. These are stored linked
 * to each currently open procedure via a linked list of
 * the following structure
*/

typedef struct WHBLOCK {
	LINE *w_begin;		/* ptr to !while statement */
	LINE *w_end;		/* ptr to the !endwhile statement*/
	int w_type;		/* block type */
	struct WHBLOCK *w_next; /* next while */
} WHBLOCK;

#define BTWHILE 	1
#define BTBREAK 	2

/* HICHAR - 1 is the largest character we will deal with. BMAPSIZE represents the number of bytes in the bitmap. */

#define HICHAR		256
#define BMAPSIZE	HICHAR >> 3

/* Defines for the metacharacters in the regular expression
 * search routines.  MCNIL and GROUP are used in both search
 * and replace metachar-arrays.
 */

#define MCNIL		0	/* Like the '\0' for strings.*/
#define LITSTRING	1	/* Literal string.*/
#define LITCHAR 	2	/* Literal character.*/
#define ANY		3	/* Any character but the <NL>.*/
#define CCL		4
#define NCCL		5
#define BOL		6
#define EOL		7
#define GRPBEG		8	/* Signal start of group.*/
#define GRPEND		9	/* Signal end of group.*/
#define GROUP		10	/* String of group match.*/
#define DITTO		11	/* Replacement with match string.*/

#define CLOSURE 	0x0100	/* An or-able value for a closure modifier.*/
#define CLOSURE_1	0x0200	/* An or-able value for a closure modifier.*/
#define ZEROONE 	0x0400	/* An or-able value for a closure modifier.*/

#define ALLCLOS 	(CLOSURE | CLOSURE_1 | ZEROONE)
#define MASKCLO 	(~ALLCLOS)

#define MC_ANY		'.'	/* 'Any' character (except newline).*/
#define MC_CCL		'['	/* Character class.*/
#define MC_NCCL 	'^'	/* Negate character class.*/
#define MC_RCCL 	'-'	/* Range in character class.*/
#define MC_ECCL 	']'	/* End of character class.*/
#define MC_BOL		'^'	/* Beginning of line.*/
#define MC_EOL		'$'	/* End of line.*/
#define MC_CLOSURE	'*'	/* Closure - zero to many characters match.*/
#define MC_CLOSURE_1	'+'	/* Closure - one to many characters match.*/
#define MC_ZEROONE	'?'	/* Closure - zero to one characters match.*/
#define MC_DITTO	'&'	/* Use matched string in replacement.*/
#define MC_GRPBEG	'('	/* Start of group (begun with a backslash).*/
#define MC_GRPEND	')'	/* End of group (begun with a backslash).*/
#define MC_ESC		'\\'	/* Escape - suppress meta-meaning.*/

#define MAXGROUPS	10		/* 1 + maximum # of r. e. groups. */
#define BIT(n)		(1 << (n))	/* An integer with one bit set.*/

/* Typedefs that define the bitmap type for searching (EBITMAP),
 * the meta-character structure for MAGIC mode searching (MC),
 * and the meta-character structure for MAGIC mode replacment (RMC).
 */

typedef char	*EBITMAP;

typedef struct {
	short int	mc_type;
	union {
		int	lchar;
		int	group_no;
		EBITMAP	cclmap;
	} u;
} MC;

typedef struct {
	short int	mc_type;
	union {
		int	group_no;
		char	*rstr;
	} u;
} RMC;
