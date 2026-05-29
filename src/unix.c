/* unix.c
 *
 * UNIX driver/Microemacs 3.10b/3.10k,
 *	Copyright 1989/1993 D. Lawrence, C. Smith
 */

#include <stdio.h>				/* Standard I/O definitions	*/
#include "estruct.h"				/* Emacs definitions		*/

int scnothing()					/* Do nothing routine 		*/
{
	return(0);
}

#include "eproto.h"				/* Function definitions		*/
#include "edef.h"				/* Global variable definitions	*/
#include "english.h"				/* Language definitions		*/

#undef CTRL					/* Problems with CTRL		*/

#include <sys/types.h>				/* System type definitions	*/
#include <sys/stat.h>				/* File status definitions	*/
#include <sys/ioctl.h>				/* I/O control definitions	*/
#include <signal.h>				/* Signal definitions		*/
#include <time.h>				/* time_t, time(), ctime()	*/
#include <unistd.h>				/* read, write, ioctl, etc.	*/

extern void check_resize(int sig);

#include <termios.h>				/* Terminal I/O definitions	*/

#include <dirent.h>				/* Directory entry definitions	*/
#define DIRENTRY	dirent

#include "termcap.h"				/* termcap definitions		*/

#undef CTRL					/* Restore CTRL			*/
#define CTRL 0x0100

#define NKEYENT		300			/* Number of keymap entries	*/
#define NINCHAR		64			/* Input buffer size		*/
#define NOUTCHAR	256			/* Output buffer size		*/
#define NCAPBUF		8192 /* 1024 @@@ */	/* Termcap storage size		*/
#define MARGIN		8			/* Margin size			*/
#define SCRSIZ		512 /* 64 @@@ */	/* Scroll for margin		*/
#define NPAUSE		10			/* # times thru update to pause */
#define TIMEOUT		255			/* No character available	*/

struct keyent {					/* Key mapping entry		*/
	struct keyent * samlvl;			/* Character on same level	*/
	struct keyent * nxtlvl;			/* Character on next level	*/
	unsigned char ch;			/* Character			*/
	int code;				/* Resulting keycode		*/
};

struct capbind {				/* Capability binding entry	*/
	char * name;				/* Termcap name			*/
	char * store;				/* Storage variable		*/
};

struct keybind {				/* Keybinding entry		*/
	char * name;				/* Termcap name			*/
	int value;				/* Binding value		*/
};

char *reset = (char*) NULL;			/* reset string kjc */

static struct termios curterm;			/* Current modes		*/
static struct termios oldterm;			/* Original modes		*/

static char tcapbuf[NCAPBUF];			/* Termcap character storage	*/
#define CAP_CL		0			/* Clear to end of page		*/
#define CAP_CM		1			/* Cursor motion		*/
#define CAP_CE		2			/* Clear to end of line		*/
#define CAP_SE		3			/* Standout ends		*/
#define CAP_SO		4			/* Standout (reverse video)	*/
#define CAP_IS		5			/* Initialize screen		*/
#define CAP_KS		6			/* Keypad mode starts		*/
#define CAP_KE		7			/* Keypad mode ends		*/
#define CAP_VB		8			/* Visible bell			*/

#define CAP_C0		9			/* Foreground color #0		*/
#define CAP_C1		10			/* Foreground color #1		*/
#define CAP_C2		11			/* Foreground color #2		*/
#define CAP_C3		12			/* Foreground color #3		*/
#define CAP_C4		13			/* Foreground color #4		*/
#define CAP_C5		14			/* Foreground color #5		*/
#define CAP_C6		15			/* Foreground color #6		*/
#define CAP_C7		16			/* Foreground color #7		*/
#define CAP_D0		17			/* Background color #0		*/
#define CAP_D1		18			/* Background color #1		*/
#define CAP_D2		19			/* Background color #2		*/
#define CAP_D3		20			/* Background color #3		*/
#define CAP_D4		21			/* Background color #4		*/
#define CAP_D5		22			/* Background color #5		*/
#define CAP_D6		23			/* Background color #6		*/
#define CAP_D7		24			/* Background color #7		*/

#define CAP_SF		25			/* Set foreground color		*/
#define CAP_SB		26			/* Set background color		*/

static struct capbind capbind[] = {		/* Capability binding list	*/
	{ "cl" },				/* Clear to end of page		*/
	{ "cm" },				/* Cursor motion		*/
	{ "ce" },				/* Clear to end of line		*/
	{ "se" },				/* Standout ends		*/
	{ "so" },				/* Standout (reverse video)	*/
	{ "is" },				/* Initialize screen		*/
	{ "ks" },				/* Keypad mode starts		*/
	{ "ke" },				/* Keypad mode ends		*/
	{ "vb" },				/* Visible bell			*/
	{ "c0" },				/* Foreground color #0		*/
	{ "c1" },				/* Foreground color #1		*/
	{ "c2" },				/* Foreground color #2		*/
	{ "c3" },				/* Foreground color #3		*/
	{ "c4" },				/* Foreground color #4		*/
	{ "c5" },				/* Foreground color #5		*/
	{ "c6" },				/* Foreground color #6		*/
	{ "c7" },				/* Foreground color #7		*/
	{ "d0" },				/* Background color #0		*/
	{ "d1" },				/* Background color #1		*/
	{ "d2" },				/* Background color #2		*/
	{ "d3" },				/* Background color #3		*/
	{ "d4" },				/* Background color #4		*/
	{ "d5" },				/* Background color #5		*/
	{ "d6" },				/* Background color #6		*/
	{ "d7" },				/* Background color #7		*/
	{ "Sf" },				/* Set foreground color		*/
	{ "Sb" },				/* Set background color		*/
};

static int cfcolor = -1;			/* Current forground color	*/
static int cbcolor = -1;			/* Current background color	*/

static struct keybind keybind[] = {		/* Keybinding list		*/
	{ "bt", SHFT|CTRL|'i' },		/* Back-tab key			*/
	{ "k1", SPEC|'1' },			/* F1 key			*/
	{ "k2", SPEC|'2' },			/* F2 key			*/
	{ "k3", SPEC|'3' },			/* F3 key			*/
	{ "k4", SPEC|'4' },			/* F4 key			*/
	{ "k5", SPEC|'5' },			/* F5 key			*/
	{ "k6", SPEC|'6' },			/* F6 key			*/
	{ "k7", SPEC|'7' },			/* F7 key			*/
	{ "k8", SPEC|'8' },			/* F8 key			*/
	{ "k9", SPEC|'9' },			/* F9 key			*/
	{ "kA", SPEC|'A' },			/* F10 key (tom - think hex)	*/
 	{ "kB", SPEC|'B' },			/* F11 key (tom - think hex)	*/
 	{ "kC", SPEC|'C' },			/* F12 key (tom - think hex)	*/
	{ "kb", CTRL|'H' },			/* Backspace key		*/
	{ "kc", CTRL|'L' },			/* Clear screen key		*/
	{ "kD", SPEC|'d' },			/* Delete character key		*/
	{ "kd", SPEC|'N' },			/* Down arrow key		*/
	{ "kE", CTRL|'K' },			/* Clear to end of line key	*/
	{ "kF", CTRL|'V' },			/* Scroll forward key		*/
	{ "kH", SPEC|'>' },			/* Home down key		*/
 	{ "@7", SPEC|'>' },			/* Home down key	(kjc)	*/
	{ "kh", SPEC|'<' },			/* Home key			*/
	{ "kI", SPEC|'I' },			/* Insert character key		*/
	{ "kL", CTRL|'K' },			/* Delete line key		*/
	{ "kl", SPEC|'b' },			/* Left arrow key		*/
	{ "kN", SPEC|'V' },			/* Next page key		*/
	{ "kP", SPEC|'Z' },			/* Previous page key		*/
	{ "kR", CTRL|'Z' },			/* Scroll backward key		*/
	{ "kr", SPEC|'f' },			/* Right arrow key		*/
	{ "ku", SPEC|'p' }			/* Up arrow key			*/
};

static int inbuf[NINCHAR];			/* Input buffer			*/
static int * inbufh = inbuf;			/* Head of input buffer		*/
static int * inbuft = inbuf;			/* Tail of input buffer		*/

static unsigned char outbuf[NOUTCHAR];		/* Output buffer		*/
static unsigned char * outbuft = outbuf; 	/* Output buffer tail		*/

static unsigned char keyseq[256];		/* Prefix escape sequence table	*/
static struct keyent keymap[NKEYENT];		/* Key map			*/
static struct keyent * nxtkey =	keymap;		/* Next free key entry		*/
static DIR *dirptr = NULL;			/* Current directory stream	*/
static char path[NFILEN];			/* Path of file to find		*/
static char rbuf[NFILEN];			/* Return file buffer		*/
static char *nameptr;				/* Ptr past end of path in rbuf	*/

int scopen(void), scclose(void);
int scmove(int row, int col), sceeol(void), sceeop(void), scbeep(void), screv(int state);
int sckopen(void), sckclose(void);
int scfcol(int color), scbcol(int color);

TERM term = {
	128,	/* @@@ */			/* Maximum number of rows	*/	/* 128 works, 129 is too many	*/
	0,					/* Current number of rows	*/
	2048,	/* @@@ */			/* Maximum number of columns	*/
	0,					/* Current number of columns	*/
	0, 0,					/* upper left corner default screen */
	MARGIN,					/* Margin for extending lines	*/
	SCRSIZ,					/* Scroll size for extending	*/
	NPAUSE,					/* # times thru update to pause	*/
	scopen,					/* Open terminal routine	*/
	scclose,				/* Close terminal routine	*/
	sckopen,				/* Open keyboard routine	*/
	sckclose,				/* Close keyboard routine	*/
	ttgetc,					/* Get character routine	*/
	ttputc,					/* Put character routine	*/
	ttflush,				/* Flush output routine		*/
	scmove,					/* Move cursor routine		*/
	sceeol,					/* Erase to end of line routine	*/
	sceeop,					/* Erase to end of page routine	*/
	sceeop,					/* Clear the desktop		*/
	scbeep,					/* Beep! routine		*/
	screv,					/* Set reverse video routine	*/
	scnothing,				/* Set resolution routine	*/
	scfcol,					/* Set forground color routine	*/
	scbcol,					/* Set background color routine	*/
};

int ttopen()
{
	strcpy(os, "UNIX");

	if (tcgetattr(0, &oldterm)) {		/* Get modes */
		perror("Cannot tcgetattr");
		return(-1);
	}
	curterm = oldterm;			/* Save to original mode variable */

	curterm.c_iflag &= ~(INLCR|ICRNL|IGNCR);		/* Set new modes */
/*	curterm.c_iflag &= ~(INLCR|ICRNL|IGNCR|IXON|IXANY|IXOFF); bad idea - don't do this!	*/
	curterm.c_lflag &= ~(ICANON|ISIG|ECHO);
	curterm.c_cc[VMIN] = 1;
	curterm.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &curterm)) {	/* Set tty mode */
		perror("Cannot tcsetattr");
		return(-1);
	}

	return(0);
}

int ttclose()
{
	if (reset != (char*)NULL)		/* Restore original terminal modes */
		write(1, reset, strlen(reset));
	if (tcsetattr(0, TCSANOW, &oldterm))
		return(-1);
	return(0);
}

int ttflush()
{
	int len;

	len = outbuft - outbuf;			/* Compute length of write */
	if (len == 0)
		return(0);
	outbuft = outbuf;			/* Reset buffer position */
	return(write(1, outbuf, len) != len);	/* Perform write to screen */
}

int ttputc( int ch )
{
	if (outbuft == &outbuf[sizeof(outbuf)])	/* Check for buffer full */
		ttflush();
	*outbuft++ = ch;			/* Add to buffer */
	return(0);
}

void addkey( char *seq, int fn )
{
	int first;
	struct keyent * cur, * nxtcur;

	if (!seq)				/* Skip on null sequences */
		return;

	if (strlen(seq) < 2)			/* Skip single character sequences */
		return;

	first = 1;				/* If no keys defined, go directly to insert mode */
	if (nxtkey != keymap) {
		cur = keymap;			/* Start at top of key map */
		while (*seq) {			/* Loop until matches exhast */
			if (*seq == cur->ch) {	/* Do we match current character */
				seq++;		/* Advance to next level */
				cur = cur->nxtlvl;
				first = 0;
			} else {		/* Try next character on same level */
				nxtcur = cur->samlvl;
				if (nxtcur)	/* Stop if no more */
					cur = nxtcur;
				else
					break;
			}
		}
	}

	if ((long)strlen(seq) > NKEYENT - (nxtkey - keymap))	/* Check for room in keymap */
		return;

	if (first)				/* If first character in sequence is inserted, add to prefix table */
		keyseq[(unsigned char)*seq] = 1;

	for (first = 1; *seq; first = 0) {	/* If characters are left over, insert them into list */
		nxtkey->ch = *seq++;		/* Make new entry */
		nxtkey->code = fn;
		if (nxtkey != keymap) {		/* If root, nothing to do */
			if (first)		/* Set first to samlvl, others to nxtlvl */
				cur->samlvl = nxtkey;
			else
				cur->nxtlvl = nxtkey;
		}
		cur = nxtkey++;			/* Advance to next key */
	}
}

unsigned char grabwait()			/** Grab input characters, with wait **/
{
	unsigned char ch;

	if (curterm.c_cc[VTIME]) {		/* Change mode, if necessary */
		curterm.c_cc[VMIN] = 1;
		curterm.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &curterm);
	}
	if (read(0, &ch, 1) != 1) {		/* Perform read */
		puts("** Horrible read error occured **");
		exit(1);
	}
	return(ch);				/* Return new character */
}

unsigned char grabnowait()			/** Grab input characters, short wait **/
{
	int count;
	unsigned char ch;

	if (curterm.c_cc[VTIME] == 0) {		/* Change mode, if necessary */
		curterm.c_cc[VMIN] = 0;
		curterm.c_cc[VTIME] = 5;
		tcsetattr(0, TCSANOW, &curterm);
	}
	count = read(0, &ch, 1);		/* Perform read */
	if (count < 0) {
		puts("** Horrible read error occured **");
		exit(1);
	}
	if (count == 0)
		return(TIMEOUT);

	return(ch);				/* Return new character */
}

void qin( int ch )				/** Queue input character **/
{
	if (inbuft == &inbuf[NINCHAR]) {	/* Check for overflow */
		scbeep();			/* Annoy user */
		return;
	}
	*inbuft++ = ch;				/* Add character */
}

void cook()					/** Cook input characters **/
{
	unsigned char ch;
	struct keyent * cur;

	ch = grabwait();			/* Get first character untimed */
	qin(ch);

	if (keyseq[ch] == 0) {			/* Skip if the key isn't a special leading escape sequence */
		if (ch == 0) {			/* if it is a null, make it a (0/1/32) */
			qin(CTRL >> 8);		/* control */
			qin(32);		/* space */
		}
		return;
	}

	cur = keymap;				/* Start at root of keymap */
	while (cur) {				/* Loop until keymap exhausts */
		if (cur->ch == ch) {		/* Did we find a matching character */
			if (cur->nxtlvl == NULL) {	/* Is this the end */
				inbuft = inbuf;		/* Replace all character with new sequence */
				qin(cur->code);
				return;
			} else {
				cur = cur->nxtlvl;	/* Advance to next level */
				ch = grabnowait();	/* Get next character, timed */
				if (ch == TIMEOUT)
					return;
				qin(ch);		/* Queue character */
			}
		} else				/* Try next character on same level */
			cur = cur->samlvl;
	}
}

int ttgetc()		/** Return cooked characters **/
{
	int ch;


	while (inbufh == inbuft)		/* Loop until character is in input buffer */
		cook();
	ch = *inbufh++;				/* Get input from buffer, now that it is available */

	if (inbufh == inbuft)			/* reset us to the beginning of the buffer if there are no more pending characters */
		inbufh = inbuft = inbuf;

	return(ch);				/* Return next character */
}

int typahead()
{
	int count;

	if (inbufh != inbuft)			/* See if internal buffer is non-empty */
		return(1);


	if (ioctl(0, FIONREAD, &count))		/* Get number of pending characters */
		return(0);
	return(count);
}

void putpad( char *seq )
{
	if (!seq)				/* Check for null */
		return;
	tputs(seq, 1, ttputc);			/* Call on termcap to send sequence */
}

int scopen()
{
	char *cp, termenv[256], tcbuf[8192];	/* @@@ */
	int status;
	struct capbind * cb;
	struct keybind * kp;
	char err_str[NSTRING];

#define TGETSTR(a,b)	tgetstr((a), (b))

	char PC, * UP;
	short ospeed;


	strcpy(termenv, getenv("TERM"));	/* Get terminal type */
	if (!*termenv) {
		puts(TEXT182);			/* "Environment variable \"TERM\" not define!" */
		exit(1);
	}
/*	else */
/* ++++ @@@       if (!strcmp(termenv, "xterm")) strcpy(termenv, "vt100");	*/


	status = tgetent(tcbuf, termenv);	/* Try to load termcap */
	if (status == -1) {
		puts("Cannot open termcap file");
		exit(1);
	}
	if (status == 0) {
		sprintf(err_str, TEXT183, termenv);	/* "No entry for terminal type \"%s\"\n" */
		puts(err_str);
		exit(1);
	}

        signal(SIGWINCH, &check_resize);

	/* initialize max number of rows and cols	@@@	 */

	/* term.t_mrow = 128; */			/* 128 works, 129 is too many	*/
	/* term.t_mcol = 2048; */

	/* Get size from termcap */

/* ++++++ @@@
 *	term.t_nrow = tgetnum("li") - 1;
 *	term.t_ncol = tgetnum("co");
 */

	struct winsize w;
	int rows = 0, cols = 0;
	int sz_fd;

	for (sz_fd = 0; sz_fd <= 2; sz_fd++) {
		if (ioctl(sz_fd, TIOCGWINSZ, &w) == 0 &&
		    w.ws_row > 0 && w.ws_col > 0) {
			rows = w.ws_row;
			cols = w.ws_col;
			break;
		}
	}
	if (rows == 0 || cols == 0) {				/* fall back to termcap */
		int li = tgetnum("li");
		int co = tgetnum("co");
		if (li > 0) rows = li;
		if (co > 0) cols = co;
	}
	if (rows == 0 || cols == 0) {				/* fall back to environment */
		char *e;
		if ((e = getenv("LINES")) != NULL && atoi(e) > 0) rows = atoi(e);
		if ((e = getenv("COLUMNS")) != NULL && atoi(e) > 0) cols = atoi(e);
	}
	if (rows == 0) rows = 24;				/* last-resort defaults */
	if (cols == 0) cols = 80;

	term.t_nrow = rows - 1;
	term.t_ncol = cols;

	if (term.t_nrow > term.t_mrow) {
		puts( "too many screen rows!\n" );
		/* grabwait(); */
		term.t_nrow = term.t_mrow;
	}

	if (term.t_ncol > term.t_mcol) {
		puts( "too many screen columns!\n" );
		term.t_ncol = term.t_mcol;
	}

/* @@@ sprintf( err_str, "window size = %d x %d\n", term.t_nrow, term.t_ncol ); puts( err_str );	*/

	if (term.t_nrow < 3 || term.t_ncol < 3) {
		puts("Screen size is too small!\n");
		exit(1);
	}


	cp = tcapbuf;				/* Start grabbing termcap commands */

	reset = TGETSTR("is", &cp);		/* Get the reset string */

	if (tgetstr("pc", &cp))			/* Get the pad character */
		PC = tcapbuf[0];

	UP = TGETSTR("up", &cp);		/* Get up line capability */

	cb = capbind;				/* Get other capabilities */
	while (cb < &capbind[sizeof(capbind)/sizeof(*capbind)]) {
		cb->store = TGETSTR(cb->name, &cp);
		cb++;
	}

	if (!capbind[CAP_CL].store && (!capbind[CAP_CM].store || !UP)) {			/* Check for minimum */
		puts("This terminal doesn't have enough power to run microEmacs!");
		exit(1);
	}

	if (capbind[CAP_SO].store && capbind[CAP_SE].store)			/* Set reverse video and erase to end of line */
		revexist = TRUE;
	if (!capbind[CAP_CE].store)
		eolexist = FALSE;

	kp = keybind;					/* Get keybindings */
	while (kp < &keybind[sizeof(keybind)/sizeof(*keybind)]) {
		addkey(TGETSTR(kp->name, &cp), kp->value);
		kp++;
	}

	if (ttopen()) {					/* Open terminal device */
		puts("Cannot open terminal");
		exit(1);
	}

	ospeed = cfgetospeed(&curterm);			/* Set speed for padding sequences */
	putpad(capbind[CAP_IS].store);			/* Send out initialization sequences */
	putpad(capbind[CAP_KS].store);
	sckopen();

	/* PC and ospeed exist only to mirror the historical libtermcap globals;
	 * they are written for documentation purposes but never read locally. */
	(void)PC;
	(void)ospeed;

	return(0);
}

int scclose()
{
	putpad(capbind[CAP_KE].store);			/* Turn off keypad mode */
	sckclose();
	ttflush();					/* Close terminal device */
	ttclose();
	return(0);
}

int sckopen()						/* open keyboard -hm */
{
	putpad(capbind[CAP_KS].store);
	ttflush();
	return(0);
}

int sckclose()						/* close keyboard -hm */
{
	putpad(capbind[CAP_KE].store);
	ttflush();
	return(0);
}


int scmove( int row, int col )					/** Move cursor **/
{
	putpad(tgoto(capbind[CAP_CM].store, col, row));		/* Call on termcap to create move sequence */
	return(0);
}

int sceeol()						/** Erase to end of line **/
{
	putpad(capbind[CAP_CE].store);			/* Send erase sequence */
	return(0);
}

int sceeop()						/** Clear screen **/
{
	scfcol(gfcolor);
	scbcol(gbcolor);
	putpad(capbind[CAP_CL].store);			/* Send clear sequence */
	return(0);
}


int screv( int state )		/** Set reverse video state **/
{
	int ftmp, btmp;		/* temporaries for colors */

	putpad(state ? capbind[CAP_SO].store : capbind[CAP_SE].store);	/* Set reverse video state */

	if (state == FALSE) {
		ftmp = cfcolor;
		btmp = cbcolor;
		cfcolor = -1;
		cbcolor = -1;
		scfcol(ftmp);
		scbcol(btmp);
	}
	return(0);
}

int scbeep()
{
	ttputc('\7');	/* The old standby method */
	return(0);
}

int scfcol( int color )	/** Set foreground color **/
{
	if (color == cfcolor)		/* Skip if already the correct color */
		return(0);
	if (capbind[CAP_C0].store) {		/* Send out color sequence */
		putpad(capbind[CAP_C0 + (color & 7)].store);
		cfcolor = color;
	}
	return(0);
}


int scbcol( int color )	/** Set background color **/
{
	if (color == cbcolor)		/* Skip if already the correct color */
		return(0);
	if (capbind[CAP_C0].store) {	/* Send out color sequence */
		putpad(capbind[CAP_D0 + (color & 7)].store);
		cbcolor = color;
	}
	return(0);
}

int spal( char *cmd )	/** Set palette **/
{
	int code, dokeymap;
	char * cp;

	if (strncmp(cmd, "KEYMAP ", 7) == 0)		/* Check for keymapping command */
		dokeymap = 1;
	else
	if (strncmp(cmd, "CLRMAP ", 7) == 0)
		dokeymap = 0;
	else
		return(0);
	cmd += 7;

	for (cp = cmd; *cp != '\0'; cp++)		/* Look for space */
		if (*cp == ' ') {
			*cp++ = '\0';
			break;
		}
	if (*cp == '\0')
		return(1);

	if (dokeymap) {					/* Perform operation */
		code = stock(cmd);			/* Convert to keycode */
		addkey(cp, code);			/* Add to tree */
	}
	else {						/* Convert to color number */
		code = atoi(cmd);
		if (code < 0 || code > 15)
			return(1);
		capbind[CAP_C0 + code].store = malloc(strlen(cp) + 1);		/* Move color code to capability structure */
		if (capbind[CAP_C0 + code].store)
			strcpy(capbind[CAP_C0 + code].store, cp);
	}
	return(0);
}

char * timeset()	/** Get time of day **/
{
	time_t buf;
	char * sp, * cp;

	time(&buf);	/* Get system time */
	sp = ctime(&buf);	/* Pass system time to converter */

	for (cp = sp; *cp; cp++)	/* Eat newline character */
		if (*cp == '\n') {
			*cp = '\0';
			break;
		}
	return(sp);
}


int callout( char *cmd )		/** Callout to system to perform command **/
{
	int status;

	scmove(term.t_nrow, 0);		/* Close down */
	ttflush();
	sckclose();
	ttclose();

	status = system(cmd) == 0;	/* Do command */

        sgarbf = TRUE;			/* Restart system */
	sckopen();
	if (ttopen()) {
		puts("** Error reopening terminal device **");
		exit(1);
	}

        return(status);
}


int spawncli( int f, int n )		/** Create subshell **/
{
	char * sh;

	if (restflag)			/* Don't allow this command if restricted */
		return(resterr());
	sh = getenv("SHELL");		/* Get shell path */
	if (!sh)
		sh = "/bin/sh";
	return(callout(sh));		/* Do shell */
}

/** Spawn a command **/
int spawn( int f, int n )
{
	char line[NLINE];
	int s;

	if (restflag)			/* Don't allow this command if restricted */
		return(resterr());
	s = mlreply("!", line, NLINE);	/* Get command line */
	if (!s)
		return(s);
	s = callout(line);		/* Perform the command */
	if (clexec == FALSE) {		/* if we are interactive, pause here */
	        mlwrite("[End]");
		ttflush();
		ttgetc();
        }
        return(s);
}

int execprg( int f, int n )		/** Execute program **/
{
	return(spawn(f, n));		/* Same as spawn */
}

int pipecmd( int f, int n )			/** Pipe output of program to buffer **/
{
	char line[NLINE];
	int s;
	BUFFER * bp;
	WINDOW * wp;
	static char filnam[] = "command";

	if (restflag)				/* Don't allow this command if restricted */
		return(resterr());
	s = mlreply("@", line, NLINE);		/* Get pipe-in command */
	if (!s)
		return(s);
	bp = bfind(filnam, FALSE, 0);		/* Get rid of the command output buffer if it exists */
	if (bp) {				/* Try to make sure we are off screen */
		wp = wheadp;
		while (wp) {
			if (wp->w_bufp == bp) {
				onlywind(FALSE, 1);
				break;
			}
			wp = wp->w_wndp;
		}
		if (!zotbuf(bp))
			return(0);
	}
	strcat(line, ">");			/* Add output specification */
	strcat(line, filnam);
	s = callout(line);			/* Do command */
	if (!s)
		return(s);
	if (!splitwind(FALSE, 1))		/* Split the current window to make room for the command output */
		return(0);
	if (!getfile(filnam, FALSE))		/* ...and read the stuff in */
		return(0);
	curwp->w_bufp->b_mode |= MDVIEW;	/* Make this window in VIEW mode, update all mode lines */
	wp = wheadp;
	while (wp) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}
	unlink(filnam);				/* ...and get rid of the temporary file */
	return(1);
}

int filter( int f, int n )			/** Filter buffer through command **/
{
	char line[NLINE], tmpnam[NFILEN];
	int s;
	BUFFER * bp;
	static char bname1[] = "fltinp";
	static char filnam1[] = "fltinp";
	static char filnam2[] = "fltout";

	if (restflag)				/* Don't allow this command if restricted */
		return(resterr());
	if (curbp->b_mode & MDVIEW)		/* Don't allow filtering of VIEW mode buffer */
		return(rdonly());
	s = mlreply("#", line, NLINE);		/* Get the filter name and its args */
	if (!s)
		return(s);
	bp = curbp;				/* Setup the proper file names */
	strcpy(tmpnam, bp->b_fname);		/* Save the original name */
	strcpy(bp->b_fname, bname1);		/* Set it to our new one */
	if (!writeout(filnam1, "w")) {		/* Write it out, checking for errors */
		mlwrite("[Cannot write filter file]");
		strcpy(bp->b_fname, tmpnam);
		return(0);
	}
	strcat(line," <fltinp >fltout");	/* Setup input and output */
	s = callout(line);			/* Perform command */
	if (s) {				/* If successful, read in file */
		s = readin(filnam2, FALSE);
		if (s)
			bp->b_flag |= BFCHG;	/* Mark buffer as changed */
	}
	strcpy(bp->b_fname, tmpnam);		/* Reset file name */
	unlink(filnam1);			/* and get rid of the temporary file */
	unlink(filnam2);
	if (!s)					/* Show status */
		mlwrite("[Execution failed]");
	return(s);
}

char *getffile( char *fspec )			/** Get first filename from pattern **/
{
	int index, point;

	strcpy(path, fspec);			/* First parse the file path off the file spec */
	index = strlen(path) - 1;
	while (index >= 0 && (path[index] != '/' &&
		path[index] != '\\' && path[index] != ':'))
		--index;
	path[index+1] = '\0';
	point = strlen(fspec) - 1;		/* Check for an extension */
	while (point >= 0) {
		if (fspec[point] == '.')
			break;
		point--;
	}
	if (dirptr) {				/* Open the directory pointer */
		closedir(dirptr);
		dirptr = NULL;
	}
	dirptr = opendir((path[0] == '\0') ? "./" : path);
	if (!dirptr)
		return(NULL);
	strcpy(rbuf, path);
	nameptr = &rbuf[strlen(rbuf)];
	return(getnfile());			/* ...and call for the first file */
}

char *getnfile()				/** Get next filename from pattern **/
{
	struct DIRENTRY * dp;
	struct stat fstat;

	do {					/* ...and call for the next file */
		dp = readdir(dirptr);
		if (!dp)
			return(NULL);
		strcpy(nameptr, dp->d_name);	/* Check to make sure we skip all weird entries except directories */
	} while (stat(rbuf, &fstat) ||
		((fstat.st_mode & S_IFMT) & (S_IFREG | S_IFDIR)) == 0);

	if ((fstat.st_mode & S_IFMT) == S_IFDIR)	/* if this entry is a directory name, say so */
		strcat(rbuf, DIRSEPSTR);

	return(rbuf);					/* Return the next file name! */
}

/*
 * BSD strlcpy / strlcat shims for systems without them (older glibc).
 * macOS, FreeBSD, NetBSD, OpenBSD, DragonFly provide these in libc.
 * glibc gained them in 2.38.
 */
#if !(defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__DragonFly__) || \
      (defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 38))))

size_t strlcpy(char *dst, const char *src, size_t dsize)
{
	const char *osrc = src;
	size_t nleft = dsize;

	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}
	if (nleft == 0) {
		if (dsize != 0)
			*dst = '\0';
		while (*src++)
			;
	}
	return(src - osrc - 1);
}

size_t strlcat(char *dst, const char *src, size_t dsize)
{
	const char *odst = dst;
	const char *osrc = src;
	size_t n = dsize;
	size_t dlen;

	while (n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));
	while (*src != '\0') {
		if (n != 0) {
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';
	return(dlen + (src - osrc));
}

#endif
