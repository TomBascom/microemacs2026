/*	EDEF:		Global variable definitions for
			MicroEMACS 3.9

                        written by Daniel Lawrence
                        based on code by Dave G. Conroy,
                        	Steve Wilhite and George Jones
*/

#ifdef	maindef

/* for MAIN.C */

/* initialized global definitions */

 int  fillcol = 72; 	/* Current fill column		*/
 short kbdm[NKBDM];		/* Macro			*/
 char *execstr = NULL;		/* pointer to string to execute */
 char golabel[NPAT] = "";	/* current line to go to	*/
 char paralead[NPAT] = " \t";	/* paragraph leadin chars	*/
 char fmtlead[NPAT] = "";	/* format command leadin chars	*/
 char mainbuf[] = "main";	/* name of main buffer		*/
 char lterm[NSTRING];		/* line terminators on file write */
 unsigned char wordlist[256];	/* characters considered "in words" */
 int  wlflag = FALSE;	/* word list enabled flag	*/
 int  clearflag = TRUE;	/* clear screen on screen change? */
 int  execlevel = 0;	/* execution IF level		*/
 int  eolexist = TRUE;	/* does clear to EOL exist?	*/
 int  revexist = FALSE;	/* does reverse video exist?	*/
 int  exec_error = FALSE;	/* macro execution error pending? */
 int  flickcode = TRUE;	/* do flicker supression?	*/
 int  mouse_move = 1;	/* user allow tracking mouse moves? */
 int  mmove_flag = TRUE;	/* code currently allowing mmoves? */
 int  overlap = 2;		/* overlap when paging screens */
 const char *modename[] = { "WRAP", "CMODE", "SPELL", "EXACT", "VIEW", "OVER", "MAGIC", "ASAVE", "REP"};		/* name of modes		*/
 const char modecode[] = "WCSEVOMAR";	/* letters to represent modes	*/
 int  numfunc = NFUNCS;	/* number of bindable functions */
 int  gmode = 0;		/* global editor mode		*/
 int  gflags = GFREAD;	/* global control flag		*/
 int  gfcolor = 7;		/* global forgrnd color (white) */
 int  gbcolor = 0;		/* global backgrnd color (black)*/
 int  deskcolor = 0;	/* desktop background color	*/
 int  gasave = 256; 	/* global ASAVE size		*/
 int  gacount = 256;	/* count until next ASAVE	*/
 int  sgarbf = TRUE; 	/* TRUE if screen is garbage	*/
 int  mpresf = FALSE;	/* TRUE if message in last line */
 int  clexec = FALSE;	/* command line execution flag	*/
 int  mstore = FALSE;	/* storing text to macro flag	*/
 int  discmd = TRUE; 	/* display command flag 	*/
 int  disinp = TRUE; 	/* display input characters	*/
 int  modeflag = TRUE;	/* display modelines flag	*/
 int  timeflag = FALSE;	/* display time			*/
 char  lasttime[6] = "";	/* last time string displayed	*/
 int  popflag = TRUE;	/* pop-up windows enabled?	*/
 int  posflag = FALSE;	/* display point position	*/
 int  indentflag = FALSE;	/* indenting flag		*/
 int  justflag = FALSE;	/* justification flag		*/
 int cpending = FALSE;		/* input character pending?	*/
 int charpending;		/* character pushed back	*/
 int  sscroll = FALSE;	/* smooth scrolling enabled flag*/
 int  hscroll = TRUE;	/* horizontal scrolling flag	*/
 int  hscrollbar = TRUE;	/* horizontal scroll bar flag	*/
 int  vscrollbar = TRUE;	/* vertical scroll bar flag */
 int  hjump = 1;		/* horizontal jump size 	*/
 int  ssave = TRUE; 	/* safe save flag		*/
 struct BUFFER *bstore = NULL;	/* buffer to store macro text to*/
 int  vtrow = 0;		/* Row location of SW cursor	*/
 int  vtcol = 0;		/* Column location of SW cursor */
 int  ttrow = HUGE; 	/* Row location of HW cursor	*/
 int  ttcol = HUGE; 	/* Column location of HW cursor */
 int  lbound = 0;		/* leftmost column of current line
					   being displayed		*/
 int  taboff = 0;		/* tab offset for display	*/
 int  tabsize = 8;		/* current hard tab size	*/
 int  stabsize = 0;		/* current soft tab size (0: use hard tabs)  */
 int  reptc = CTRL | 'U';	/* current universal repeat char*/
 int  abortc = CTRL | 'G';	/* current abort command char	*/
 int  sterm = CTRL | '[';	/* search terminating character */
 int  isterm = CTRL | '[';	/* incremental-search terminating char */
 int  searchtype = SRNORM;	/* current search style		*/
 int  yankflag = FALSE;	/* current yank style		*/

 int  prefix = 0;		/* currently pending prefix bits */
 int  prenum = 0;		/*     "       "     numeric arg */
 int  predef = TRUE;	/*     "       "     default flag */

 int  quotec = CTRL | 'Q';	/* quote char during mlreply() */
 const char *cname[] = {		/* names of colors		*/
	"BLACK", "RED", "GREEN", "YELLOW", "BLUE",
	"MAGENTA", "CYAN", "GREY",
	"GRAY", "LRED", "LGREEN", "LYELLOW", "LBLUE",
	"LMAGENTA", "LCYAN", "WHITE"
 };
  
 int kill_index;			/* current index into kill ring */
 KILL *kbufp[NRING];		/* current kill buffer chunk pointer*/
 KILL *kbufh[NRING];		/* kill buffer header pointer	*/
	int kskip[NRING];		/* # of bytes to skip in 1st kill chunk */
 int kused[NRING];		/* # of bytes used in last kill chunk*/
 WINDOW *swindow = NULL; 	/* saved window pointer 	*/
 int cryptflag = FALSE;		/* currently encrypting?	*/
 int oldcrypt = FALSE;		/* using old(broken) encryption? */
 short *kbdptr;			/* current position in keyboard buf */
 short *kbdend = &kbdm[0];	/* ptr to end of the keyboard */
 int  kbdmode = STOP;	/* current keyboard macro mode	*/
 int  kbdrep = 0;		/* number of repetitions	*/
 int  restflag = FALSE;	/* restricted use?		*/
 int  lastkey = 0;		/* last keystoke		*/
 long  seed = 1L;		/* random number seed		*/
 long envram = 0l;		/* # of bytes current in use by malloc */
 int  macbug = FALSE;	/* macro debugging flag		*/
 int  mouseflag = TRUE;	/* use the mouse?		*/
 int  diagflag = FALSE;	/* diagonal mouse movements?	*/
 const char errorm[] = "ERROR";		/* error literal		*/
 const char truem[] = "TRUE";		/* true literal 		*/
 const char falsem[] = "FALSE";		/* false litereal		*/
 int  cmdstatus = TRUE;	/* last command status		*/
 char palstr[49] = "";		/* palette string		*/
 char lastmesg[NSTRING] = ""; 	/* last message posted		*/
 char *lastptr = NULL;		/* ptr to lastmesg[]		*/
 int  saveflag = 0; 	/* Flags, saved with the $target var */
 char *fline = NULL;		/* dynamic return line		*/
 int  flen = 0;		/* current length of fline	*/
 int  rval = 0;		/* return value of a subprocess */
 int  eexitflag = FALSE;	/* EMACS exit flag		*/
 int  eexitval = 0; 	/* and the exit return value	*/
 int xpos = 0;		/* current column mouse is positioned to*/
 int ypos = 0;		/* current screen row	     "		*/
 int nclicks = 0;	/* cleared on any non-mouse event	*/
 int disphigh = FALSE;	/* display high bit chars escaped	*/
 int defferupdate = FALSE;	/* if TRUE, update(TRUE) should be
					called before yielding to another
					Windows application */
 int notquiescent = 1;		/* <=0 only when getkey called
					directly by editloop () */
 int fbusy = FALSE;		/* indicates file activity if FREADING
					or FWRITING. Used by abort mechanism */
 int hilite = 10;		/* current region to highlight (255 if none) */

/* uninitialized global definitions */

 int  currow;	/* Cursor row			*/
 int  curcol;	/* Cursor column		*/
 int  thisflag;	/* Flags, this command		*/
 int  lastflag;	/* Flags, last command		*/
 int  curgoal;	/* Goal for C-P, C-N		*/
 WINDOW *curwp; 		/* Current window		*/
 BUFFER *curbp; 		/* Current buffer		*/
 WINDOW *wheadp;		/* Head of list of windows	*/
 BUFFER *bheadp;		/* Head of list of buffers 	*/
 SCREEN *first_screen;	/* Head and current screen in list */
 BUFFER *blistp;		/* Buffer for C-X C-B		*/
 BUFFER *slistp;		/* Buffer for A-B		*/

 char sres[NBUFN];	/* current screen resolution	*/
 char os[NBUFN];		/* what OS are we running under */

	char lowcase[HICHAR];	/* lower casing map		*/
	char upcase[HICHAR];	/* upper casing map		*/

 char pat[NPAT];		/* Search pattern		*/
 char tap[NPAT];		/* Reversed pattern array.	*/
 char rpat[NPAT];	/* replacement pattern		*/

/*	Various "Hook" execution variables	*/

 KEYTAB readhook;	/* executed on all file reads */
 KEYTAB wraphook;	/* executed when wrapping text */
 KEYTAB cmdhook;		/* executed before looking for a command */
 KEYTAB writehook;	/* executed on all file writes */
 KEYTAB exbhook;		/* executed when exiting a buffer */
 KEYTAB bufhook;		/* executed when entering a buffer */

/* The variables matchline and matchoff hold the line
 * and offset position of the *start* of the match.
 * The variable patmatch holds the string that satisfies
 * the search command.
 */
 int	matchlen;
 int	matchoff;
 LINE	*matchline;
 char *patmatch = NULL;

/*
 * The variables magical and rmagical determine if there
 * were actual metacharacters in the search and replace strings -
 * if not, then we don't have to use the slower MAGIC mode
 * search functions.
 */

 short int  magical = FALSE;
 short int  rmagical = FALSE;
 MC mcpat[NPAT]; 	/* the magic pattern		*/
 MC tapcm[NPAT]; 	/* the reversed magic pattern	*/
 RMC rmcpat[NPAT];	/* the replacement magic array	*/
 char *grpmatch[MAXGROUPS];	/* holds groups found in search */ 

/* directive name table: This holds the names of all the directives....	*/

const char *dname[] = {
	"if", "else", "endif",
	"goto", "return", "endm",
	"while", "endwhile", "break",
	"force"
};
/* directive lengths		*/
const short int dname_len[NUMDIRS] = {2, 4, 5, 4, 6, 4, 5, 7, 5, 5};

#else

/* for all the other .C files */

/* initialized global external declarations */

 extern int  fillcol;	/* Current fill column		*/
 extern short kbdm[DUMMYSZ];	/* Holds keyboard macro data	*/
 extern char *execstr;		/* pointer to string to execute */
 extern char golabel[DUMMYSZ];	/* current line to go to	*/
 extern char paralead[DUMMYSZ];	/* paragraph leadin chars	*/
 extern char fmtlead[DUMMYSZ];	/* format command leadin chars	*/
 extern char mainbuf[DUMMYSZ];	/* name of main buffer		*/
 extern char lterm[DUMMYSZ];	/* line terminators on file write */
 extern unsigned char wordlist[DUMMYSZ];/* characters considered "in words" */
 extern int  wlflag;	/* word list enabled flag	*/
 extern int  clearflag;	/* clear screen on screen change? */
 extern int  execlevel;	/* execution IF level		*/
 extern int  eolexist;	/* does clear to EOL exist?	*/
 extern int  revexist;	/* does reverse video exist?	*/
 extern int  exec_error;	/* macro execution error pending? */
 extern int  flickcode;	/* do flicker supression?	*/
 extern int  mouse_move;	/* user allow tracking mouse moves? */
 extern int  mmove_flag;	/* code currently allowing mmoves? */
 extern int  overlap;	/* overlap when paging screens */
 extern const char *modename[DUMMYSZ];	/* text names of modes		*/
 extern const char modecode[DUMMYSZ];	/* letters to represent modes	*/
 extern int  numfunc;	/* number of bindable functions */
 extern KEYTAB keytab[DUMMYSZ];	/* key bind to functions table	*/
 extern NBIND names[DUMMYSZ];	/* name to function table	*/
 extern int  gmode;		/* global editor mode		*/
 extern int  gflags;	/* global control flag		*/
 extern int  gfcolor;	/* global forgrnd color (white) */
 extern int  gbcolor;	/* global backgrnd color (black)*/
 extern int  deskcolor;	/* desktop background color	*/
 extern int  gasave;	/* global ASAVE size		*/
 extern int  gacount;	/* count until next ASAVE	*/
 extern int  sgarbf;	/* State of screen unknown	*/
 extern int  mpresf;	/* Stuff in message line	*/
 extern int  clexec;	/* command line execution flag	*/
 extern int  mstore;	/* storing text to macro flag	*/
 extern int  discmd;	/* display command flag 	*/
 extern int  disinp;	/* display input characters	*/
 extern int  modeflag;	/* display modelines flag	*/
 extern int  timeflag;	/* display time			*/
 extern char  lasttime[DUMMYSZ];/* last time string displayed	*/
 extern int  popflag;	/* pop-up windows enabled?	*/
 extern int  posflag;	/* display point position	*/
 extern int  indentflag;	/* indenting flag		*/
 extern int  justflag;	/* justification flag		*/
 extern int cpending;		/* input character pending?	*/
 extern int charpending;		/* character pushed back	*/
 extern int  sscroll;	/* smooth scrolling enabled flag*/
 extern int  hscroll;	/* horizontal scrolling flag	*/
 extern int  hscrollbar;	/* horizontal scroll bar flag	*/
 extern int  vscrollbar;	/* vertical scroll bar flag */
 extern int  hjump;		/* horizontal jump size 	*/
 extern int  ssave;		/* safe save flag		*/
 extern struct BUFFER *bstore;	/* buffer to store macro text to*/
 extern int  vtrow;		/* Row location of SW cursor	*/
 extern int  vtcol;		/* Column location of SW cursor */
 extern int  ttrow;		/* Row location of HW cursor	*/
 extern int  ttcol;		/* Column location of HW cursor */
 extern int  lbound;	/* leftmost column of current line
					   being displayed		*/
 extern int  taboff;	/* tab offset for display	*/
 extern int  tabsize;	/* current hard tab size	*/
 extern int  stabsize;	/* current soft tab size (0: use hard tabs)  */
 extern int  reptc;		/* current universal repeat char*/
 extern int  abortc;	/* current abort command char	*/
 extern int  sterm;		/* search terminating character */
 extern int  isterm;	/* incremental-search terminating char */
 extern int  searchtype;	/* current search style		*/
 extern int  yankflag;	/* current yank style		*/

 extern int  prefix;	/* currently pending prefix bits */
 extern int  prenum;	/*     "       "     numeric arg */
 extern int  predef;	/*     "       "     default flag */

 extern int  quotec;	/* quote char during mlreply() */
 extern const char *cname[DUMMYSZ];/* names of colors		*/
  
 extern int kill_index;		/* current index into kill ring */
 extern KILL *kbufp[DUMMYSZ];	/* current kill buffer chunk pointer */
 extern KILL *kbufh[DUMMYSZ];	/* kill buffer header pointer	*/
	extern int kskip[DUMMYSZ];	/* # of bytes to skip in 1st kill chunk */
 extern int kused[DUMMYSZ];	/* # of bytes used in kill buffer*/
 extern WINDOW *swindow; 	/* saved window pointer 	*/
 extern int cryptflag;		/* currently encrypting?	*/
 extern int oldcrypt;		/* using old(broken) encryption? */
 extern short *kbdptr;		/* current position in keyboard buf */
 extern short *kbdend;		/* ptr to end of the keyboard */
 extern int kbdmode;		/* current keyboard macro mode	*/
 extern int kbdrep;		/* number of repetitions	*/
 extern int restflag;		/* restricted use?		*/
 extern int lastkey;		/* last keystoke		*/
 extern long seed;		/* random number seed		*/
 extern long envram;		/* # of bytes current in use by malloc */
 extern int  macbug;	/* macro debugging flag		*/
 extern int  mouseflag;	/* use the mouse?		*/
 extern int  diagflag;	/* diagonal mouse movements?	*/
 extern const char errorm[DUMMYSZ];	/* error literal		*/
 extern const char truem[DUMMYSZ];	/* true literal 		*/
 extern const char falsem[DUMMYSZ];	/* false litereal		*/
 extern int  cmdstatus;	/* last command status		*/
 extern char palstr[DUMMYSZ];	/* palette string		*/
 extern char lastmesg[DUMMYSZ];	/* last message posted		*/
 extern char *lastptr;		/* ptr to lastmesg[]		*/
 extern int  saveflag;	/* Flags, saved with the $target var */
 extern char *fline; 		/* dynamic return line */
 extern int  flen;		/* current length of fline */
 extern int  rval; 		/* return value of a subprocess */
 extern int  eexitflag;	/* EMACS exit flag */
 extern int  eexitval;	/* and the exit return value */
 extern int xpos;		/* current column mouse is positioned to */
 extern int ypos;		/* current screen row	     "	 */
 extern int nclicks;		/* cleared on any non-mouse event*/
 extern int disphigh;		/* display high bit chars escaped*/
 extern int defferupdate;	/* if TRUE, update(TRUE) should be
					called before yielding to another
					Windows application */
 extern int notquiescent;	/* <=0 only when getkey called
					directly by editloop () */
 extern int fbusy;		/* indicates file activity if FREADING
					or FWRITING. Used by abort mechanism */
 extern int hilite;		/* current region to highlight (255 if none) */

/* uninitialized global external declarations */

 extern int  currow;	/* Cursor row			*/
 extern int  curcol;	/* Cursor column		*/
 extern int  thisflag;	/* Flags, this command		*/
 extern int  lastflag;	/* Flags, last command		*/
 extern int  curgoal;	/* Goal for C-P, C-N		*/
 extern WINDOW *curwp; 		/* Current window		*/
 extern BUFFER *curbp; 		/* Current buffer		*/
 extern WINDOW *wheadp;		/* Head of list of windows	*/
 extern BUFFER *bheadp;		/* Head of list of buffers	*/
 extern SCREEN *first_screen;	/* Head and current screen in list */
 extern BUFFER *blistp;		/* Buffer for C-X C-B		*/
 extern BUFFER *slistp;		/* Buffer for A-B		*/

 extern char sres[NBUFN];	/* current screen resolution	*/
 extern char os[NBUFN];		/* what OS are we running under */

	extern char lowcase[HICHAR];	/* lower casing map		*/
	extern char upcase[HICHAR];	/* upper casing map		*/

 extern char pat[DUMMYSZ];	/* Search pattern		*/
 extern char tap[DUMMYSZ];	/* Reversed pattern array.	*/
 extern char rpat[DUMMYSZ];	/* replacement pattern		*/

/*	Various "Hook" execution variables	*/

 extern KEYTAB readhook;		/* executed on all file reads */
 extern KEYTAB wraphook;		/* executed when wrapping text */
 extern KEYTAB cmdhook;		/* executed before looking for a cmd */
 extern KEYTAB writehook;	/* executed on all file writes */
 extern KEYTAB exbhook;		/* executed when exiting a buffer */
 extern KEYTAB bufhook;		/* executed when entering a buffer */

 extern int matchlen;
 extern int matchoff;
 extern LINE *matchline;
 extern char *patmatch;

 extern short int magical;
 extern short int rmagical;
 extern MC mcpat[NPAT];		/* the magic pattern		*/
 extern MC tapcm[NPAT];		/* the reversed magic pattern	*/
 extern RMC rmcpat[NPAT];	/* the replacement magic array	*/
 extern char *grpmatch[MAXGROUPS];	/* holds groups found in search */ 

 extern const char *dname[DUMMYSZ];	/* directive name table 	*/
 extern const short int dname_len[NUMDIRS]; /* directive lengths		*/

#endif

extern TERM     term;           /* Terminal information.        */

