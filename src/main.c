/* main.c
 *
 *	MicroEMACS 5.03
 *		written by Daniel M. Lawrence
 *		based on code by Dave G. Conroy.
 *
 *	(C)Copyright 1988,1989,1990,1991,1992,1993 by Daniel M. Lawrence
 *	MicroEMACS 5.03 can be copied and distributed freely for any
 *	non-commercial purposes. MicroEMACS 5.03 can only be incorporated
 *	into commercial software with the permission of the current author.
 *
 * This file contains the main driving routine, and some keyboard processing
 * code, for the MicroEMACS screen editor.
 *
 */

#include <stdio.h>

/* make global definitions not external */

#define maindef

#include "estruct.h"	/* global structures and defines */
#include "eproto.h"	/* variable prototype definitions */
#include "efunc.h"	/* function declarations and name table */
#include "edef.h"	/* global definitions */
#include "english.h"	/* human language definitions */
#include "ebind.h"	/* default key bindings */

int main( int argc, char *argv[] )
{
	register int status = 0;

	eexitflag = FALSE;		/* Initialize the editor */
	vtinit();			/* Terminal */

	if (eexitflag)
		goto abortrun;
	edinit(mainbuf); 		/* Buffers, windows, screens */

	varinit();			/* user variables */
	initchars();			/* character set definitions */

	dcline(argc, argv, TRUE);	/* Process the command line and let the user edit */
	status = editloop();		/* @@@ more than 128 rows blows up here	*/

abortrun:
	vttidy();
	exit(status);
	return(status);
}

int dcline( int argc, char *argv[], int firstflag )	/* Process a command line.   May be called any time.	*/
{
	register BUFFER *bp;		/* temp buffer pointer */
	register int	firstfile;	/* first file flag */
	register int	carg;		/* current arg to scan */
	register int	startflag;	/* startup executed flag */
	BUFFER *firstbp = NULL; 	/* ptr to first buffer in cmd line */
	int viewflag;			/* are we starting in view mode? */
	int gotoflag;			/* do we need to goto a line at start? */
	int gline;			/* if so, what line? */
	int gchar;			/* and what character? */
	int searchflag; 		/* Do we need to search at start? */
	int errflag;			/* C error processing? */
	VDESC vd;			/* variable num/type */
	char bname[NBUFN];		/* buffer name of file to read */

	viewflag = FALSE;	/* view mode defaults off in command line */
	gotoflag = FALSE;	/* set to off to begin with */
	gline = 1; gchar = 1;	/* line and character to go to */
	searchflag = FALSE;	/* set to off to begin with */
	firstfile = TRUE;	/* no file to edit yet */
	startflag = FALSE;	/* startup file not executed yet */
	errflag = FALSE;	/* not doing C error parsing */
	exec_error = FALSE;	/* no macro error pending */
	disphigh = FALSE;	/* don't escape high bit characters */
	lterm[0] = 0;		/* standard line terminators */

	for (carg = 1; carg < argc; ++carg) {		/* Parse a command line */
		if (argv[carg][0] == '-') {		/* Process Switches */
			switch (argv[carg][1]) {
				case 'c':	/* -c for changable file */
				case 'C':
					viewflag = FALSE;
					break;
				case 'e':	/* -e process error file */
				case 'E':
					errflag = TRUE;
					break;
				case 'g':	/* -g for initial goto line */
				case 'G':
					gotoflag = TRUE;
					gline = asc_int(&argv[carg][2]);
					break;
				case 'i':	/* -i<var> <value> set an initial */
				case 'I':	/* value for a variable */
					bytecopy(bname, &argv[carg][2], NVSIZE);
					findvar(bname, &vd, NVSIZE + 1);
					if (vd.v_type == -1) {
						mlwrite(TEXT52, bname);	/* "%%No such variable as '%s'" */
						break;
					}
					svar(&vd, argv[++carg]);
					break;
				case 'p':	/* -p for initial goto char position */
				case 'P':
					gotoflag = TRUE;
					gchar = asc_int(&argv[carg][2]);
					break;
				case 'r':	/* -r restrictive use */
				case 'R':
					restflag = TRUE;
					break;
				case 's':	/* -s for initial search string */
				case 'S':
					searchflag = TRUE;
					bytecopy(pat,&argv[carg][2],NPAT);
					setjtable();
					break;
				case 'v':	/* -v for View File */
				case 'V':
					viewflag = TRUE;
					break;
				default:	/* unknown switch */
					/* ignore this for now */
					break;
			}
		} else if (argv[carg][0] == '+') {		/* +<line num> */
			gotoflag = TRUE;
			gline = asc_int(&argv[carg][1]);
		} else if (argv[carg][0]== '@') {
			if (startup(&argv[carg][1]) == TRUE)	/* Process Startup macroes */
				startflag = TRUE;		/* don't execute emacs.rc */
		} else {
			makename(bname, argv[carg]);		/* Process an input file */
			unqname(bname);				/* set up a buffer for this file */
			bp = bfind(bname, TRUE, 0);		/* set this to inactive */
			strlcpy(bp->b_fname, argv[carg], NFILEN);
			bp->b_active = FALSE;
			if (firstfile) {
				firstbp = bp;
				firstfile = FALSE;
			}
			if (viewflag)				/* set the modes appropriatly */
				bp->b_mode |= MDVIEW;
		}
	}
	if (errflag) {						/* if we are C error parsing... run it! */
		if (startup("error.cmd") == TRUE)
			startflag = TRUE;
	}
	if (firstflag && startflag == FALSE)			/* if invoked with no other startup files, run the system startup file here */
		startup("");
	if (firstflag) {					/* if there are any files to read, read the first one! */
		bp = bfind(mainbuf, FALSE, 0);
		if (firstfile == FALSE && (gflags & GFREAD)) {
			swbuffer(firstbp);
			curbp->b_mode |= gmode;
			update(TRUE);
			mlwrite(lastmesg);
			zotbuf(bp);
		} else
			bp->b_mode |= gmode;
	} else {
		swbuffer(firstbp);
		curbp->b_mode |= gmode;
		update(TRUE);
		mlwrite(lastmesg);
	}
	if (gotoflag && searchflag) {				/* Deal with startup gotos and searches */
		update(FALSE);
		mlwrite(TEXT101);				/* "[Can not search and goto at the same time!]" */
	}
	else if (gotoflag) {
		if ((gotoline(TRUE, gline) == FALSE) ||
		    (forwchar(TRUE, gchar - 1) == FALSE)) {
			update(FALSE);
			mlwrite(TEXT102);			/* "[Bogus goto argument]" */
		}
	} else if (searchflag) {
		if (forwhunt(FALSE, 0) == FALSE)
			update(FALSE);
	}
	return(TRUE);
}

#define GETBASEKEY getkey

int editloop()
{
	register int c;		/* command character */
	register int f; 	/* default flag */
	register int n; 	/* numeric repeat count */
	register int mflag;	/* negative flag on repeat */
	register int basec;	/* c stripped of meta character */
	register int oldflag;	/* old last flag value */
	char time[6];		/* current display time */

        if ((curbp->b_bname[0] == '?' ) && (curbp->b_bname[1] == ' ' )
           && (curbp->b_bname[strlen(curbp->b_bname)-1] == '/')) {
           for(c=2; (size_t)c<= strlen(curbp->b_bname); ++c)
              curbp->b_fname[c-2] = curbp->b_bname[c];
           discmd = TRUE;
           filefind(0,0);
           if ((curbp->b_bname[0] == ':' ) && (curbp->b_bname[1] == ' ' )
	    && (curbp->b_bname[strlen(curbp->b_bname)-1] == '/')) {
              strcpy(curbp->b_bname, "main");
              strcpy(curbp->b_fname, "");
              upmode();
	   }	 
	}

	lastflag = 0;				/* Fake last flags.	*/

loop:
	if (exec_error) {			/* if a macro error is pending, wait for a character */
		mlforce(TEXT227);		/* "\n--- Press any key to Continue ---" */
		tgetc();
		sgarbf = TRUE;
		update(FALSE);
		mlferase();
		exec_error = FALSE;
	}

	if (eexitflag)				/* if we were called as a subroutine and want to leave, do so */
		return(eexitval);

	oldflag = lastflag;			/* preserve lastflag through this */
	execkey(&cmdhook, FALSE, 1);		/* execute the "command" macro...normally null */
	lastflag = oldflag;

	if (timeflag)				/* update time on the bottom modeline? */
	    if (!typahead())
		{
			getdtime(time);
			if (strcmp(lasttime, time) != 0)
				upmode();
		}

	if (posflag)				/* update position on current modeline? */
	    if (!typahead())
		upmode();

	update(FALSE);				/* Fix up the screen	*/

	discmd = TRUE;				/* get the next command from the keyboard */
	disinp = TRUE;
	c = GETBASEKEY();

	if (mpresf != FALSE) {			/* if there is something on the command line, clear it */
		mlerase();
		update(FALSE);
	}

	if (prefix) {				/* override the arguments if prefixed */
		if (is_lower(c & 255))
			c = (c & ~255) | upperc(c & 255);
		c |= prefix;
		f = predef;
		n = prenum;
		prefix = 0;
	} else {
		f = FALSE;
		n = 1;
	}

	/* do META-# processing if needed */

	basec = c & ~META;		/* strip meta char off if there */
	if ((c & META) && ((basec >= '0' && basec <= '9') || basec == '-') &&
	    (getbind(c) == NULL)) {
		f = TRUE;		/* there is a # arg */
		n = 0;			/* start with a zero default */
		mflag = 1;		/* current minus flag */
		c = basec;		/* strip the META */
		while ((c >= '0' && c <= '9') || (c == '-')) {
			if (c == '-') {
				/* already hit a minus or digit? */
				if ((mflag == -1) || (n != 0))
					break;
				mflag = -1;
			} else {
				n = n * 10 + (c - '0');
			}
			if ((n == 0) && (mflag == -1))	/* lonely - */
				mlwrite("Arg:");
			else
				mlwrite("Arg: %d",n * mflag);

			c = GETBASEKEY();	/* get the next key */
		}
		n = n * mflag;	/* figure in the sign */
	}

	/* do ^U repeat argument processing */

	if (c == reptc) {		   /* ^U, start argument   */
		f = TRUE;
		n = 4;				/* with argument of 4 */
		mflag = 0;			/* that can be discarded. */
		mlwrite("Arg: 4");
		while (((c=GETBASEKEY()) >='0' && c<='9') || c==reptc || c=='-') {
			if (c == reptc)
				if ((n > 0) == ((n*4) > 0))
					n = n*4;
				else
					n = 1;
			/*
			 * If dash, and start of argument string, set arg.
			 * to -1.  Otherwise, insert it.
			 */
			else if (c == '-') {
				if (mflag)
					break;
				n = 0;
				mflag = -1;
			}
			/*
			 * If first digit entered, replace previous argument
			 * with digit and set sign.  Otherwise, append to arg.
			 */
			else {
				if (!mflag) {
					n = 0;
					mflag = 1;
				}
				n = 10*n + c - '0';
			}
			mlwrite("Arg: %d", (mflag >=0) ? n : (n ? -n : -1));
		}
		/*
		 * Make arguments preceded by a minus sign negative and change
		 * the special argument "^U -" to an effective "^U -1".
		 */
		if (mflag == -1) {
			if (n == 0)
				n++;
			n = -n;
		}
	}
	execute(c, f, n);			/* and execute the command */
	goto loop;
}

/*
 * Initialize all of the buffers, windows and screens. The buffer name is
 * passed down as an argument, because the main routine may have been told
 * to read in a file by default, and we want the buffer name to be right.
 */

int edinit( char bname[] )

{
	register BUFFER *bp;
	register int index;

	for (index = 0; index < NRING; index++) {		/* init the kill ring */
		kbufp[index] = (KILL *)NULL;
		kbufh[index] = (KILL *)NULL;
		kskip[index] = 0;
		kused[index] = KBLOCK;
	}
	kill_index = 0;

	readhook.k_ptr.fp = nullproc;				/* initialize some important globals */
	readhook.k_type = BINDFNC;
	wraphook.k_ptr.fp = wrapword;
	wraphook.k_type = BINDFNC;
	cmdhook.k_ptr.fp = nullproc;
	cmdhook.k_type = BINDFNC;
	writehook.k_ptr.fp = nullproc;
	writehook.k_type = BINDFNC;
	bufhook.k_ptr.fp = nullproc;
	bufhook.k_type = BINDFNC;
	exbhook.k_ptr.fp = nullproc;
	exbhook.k_type = BINDFNC;

	bp = bfind(bname, TRUE, 0);				/* allocate the first buffer */
	blistp = bfind("[Buffers]", TRUE, BFINVS);		/* Buffer list buffer	*/
	slistp = bfind("[Screens]", TRUE, BFINVS);		/* Buffer list buffer	*/
	if (bp==NULL || blistp==NULL)
		meexit(1);

	first_screen = (SCREEN *)NULL;				/* and allocate the default screen */
	init_screen("MAIN", bp);
	if (first_screen == (SCREEN *)NULL)
		meexit(1);

	curbp = bp;						/* set the current default screen/buffer/window */
	curwp = wheadp = first_screen->s_cur_window = first_screen->s_first_window;
	return(TRUE);
}

/*
 * This is the general command execution routine. It handles the fake binding
 * of all the keys to "self-insert". It also clears out the "thisflag" word,
 * and arranges to move it to the "lastflag", so that the next command can
 * look at it. Return the status of command.
 */

int execute( int c, int f, int n )	/* key to execute, prefix argument flag, prefix value */
{
	register int status;
	KEYTAB *key;		/* key entry to execute */

	/* if the keystroke is a bound function...do it */
	key = getbind(c);
	if (key != NULL) {
		if ((key->k_type == BINDFNC) &&			/* Don't reset the function type flags on a prefix */
		    ((key->k_ptr.fp == meta) || (key->k_ptr.fp == cex)))
			status = execkey(key, f, n);
		else {
			thisflag = 0;
			status = execkey(key, f, n);
			lastflag = thisflag;
		}
		return(status);
	}

	/*
	 * If a space was typed, fill column is defined, the argument is non-
	 * negative, wrap mode is enabled, and we are now past fill column,
	 * and we are not read-only, perform word wrap.
	 */
	if (c == ' ' && (curwp->w_bufp->b_mode & MDWRAP) && fillcol > 0 &&
	    n >= 0 && getccol(FALSE) > fillcol &&
	    (curwp->w_bufp->b_mode & MDVIEW) == FALSE)
		execkey(&wraphook, FALSE, 1);

	if ((c>=0x20 && c<=0xFF)) {	/* Self inserting.	*/
		if (n <= 0) {			/* Fenceposts.		*/
			lastflag = 0;
			return(n<0 ? FALSE : TRUE);
		}
		thisflag = 0;			/* For the future.	*/

		/* replace or overwrite mode, not at the end of a string */
		if (curwp->w_bufp->b_mode & (MDREPL | MDOVER) &&
		    curwp->w_doto < lused(curwp->w_dotp)) {

			/* if we are in replace mode, or
			   (next char is not a tab or we are at a tab stop) */
			if (curwp->w_bufp->b_mode & MDREPL ||
			    (lgetc(curwp->w_dotp, curwp->w_doto) != '\t' ||
			    getccol(FALSE) % tabsize == (tabsize - 1)))
						ldelete(1L, FALSE);
		}

		/* do the appropriate insertion */
		if (c == '}' && (curbp->b_mode & MDCMOD) != 0)
			status = insbrace(n, c);
		else if (c == '#' && (curbp->b_mode & MDCMOD) != 0)
			status = inspound();
		else
			status = linsert(n, c);

		if ((c == '}' || c == ')' || c == ']') &&		/* check for CMODE fence matching */
				(curbp->b_mode & MDCMOD) != 0)
			fmatch(c);

		if (curbp->b_mode & MDASAVE)				/* check auto-save mode */
			if (--gacount == 0) {
				/* and save the file if needed */
				upscreen(FALSE, 0);
				filesave(FALSE, 0);
				gacount = gasave;
			}

		lastflag = thisflag;
		return(status);
	}
	TTbeep();
	mlwrite(TEXT19);						/* "[Key not bound]" */
	lastflag = 0;							/* Fake last flags.	*/
	return(FALSE);
}

/* Fancy quit command, as implemented by Norm. If the any buffer has changed,
 * do a write on that buffer and exit emacs, otherwise simply exit.
 */

int quickexit( int f, int n )
{
	register BUFFER *bp;	/* scanning pointer to buffers */
	register BUFFER *oldcb; /* original current buffer */
	register int status;

	oldcb = curbp;							/* save in case we fail */

	bp = bheadp;
	while (bp != NULL) {
		if ((bp->b_flag&BFCHG) != 0				/* Changed.		*/
		&& (bp->b_flag&BFINVS) == 0) {				/* Real.		*/
			curbp = bp;					/* make that buffer cur */
			mlwrite(TEXT103,bp->b_fname);			/* "[Saving %s]" */
			mlwrite("\n");
			if ((status = filesave(f, n)) != TRUE) {
				curbp = oldcb;				/* restore curbp */
				return(status);
			}
		}
	bp = bp->b_bufp;						/* on to the next buffer */
	}
	quit(f, n);							/* conditionally quit	*/
	return(TRUE);
}

/* Quit command. If an argument, always quit. Otherwise confirm if a buffer
 * has been changed and not written out. Normally bound to "C-X C-C".
 */

int quit( int f, int n )
{
	register int status;						/* return status */

	if (f != FALSE							/* Argument forces it.	*/
	|| anycb() == FALSE						/* All buffers clean or user says it's OK. */
	|| (status = mlyesno(TEXT104)) == TRUE) {			/* "Modified buffers exist. Leave anyway" */
		if (f)
			status = meexit(n);
		else
			status = meexit(0);
	}
	mlerase();
	return(status);
}

int meexit( int status )
{
	eexitflag = TRUE;						/* flag a program exit */
	eexitval = status;
	return(TRUE);							/* and now.. we leave and let the main loop kill us */
}

/* Begin a keyboard macro.
 * Error if not at the top level in keyboard processing. Set up variables and return.
 */

int ctlxlp( int f, int n )
{
	if (kbdmode != STOP) {
		mlwrite(TEXT105);					/* "%%Macro already active" */
		return(FALSE);
	}
	mlwrite(TEXT106);						/* "[Start macro]" */
	kbdptr = &kbdm[0];
	kbdend = kbdptr;
	kbdmode = RECORD;
	return(TRUE);
}

/* End keyboard macro. Check for the same limit conditions as the above
 * routine. Set up the variables and return to the caller.
 */

int ctlxrp( int f, int n )
{
	if (kbdmode == STOP) {
		mlwrite(TEXT107);					/* "%%Macro not active" */
		return(FALSE);
	}
	if (kbdmode == RECORD) {
		mlwrite(TEXT108);					/* "[End macro]" */
		kbdmode = STOP;
	}
	return(TRUE);
}

/* Execute a macro.
 * The command argument is the number of times to loop. Quit as soon as a
 * command gets an error. Return TRUE if all ok, else FALSE.
 */

int ctlxe( int f, int n )
{
	if (kbdmode != STOP) {
		mlwrite(TEXT105);					/* "%%Macro already active" */
		return(FALSE);
	}
	if (n <= 0)
		return(TRUE);
	kbdrep = n;							/* remember how many times to execute */
	kbdmode = PLAY; 						/* start us in play mode */
	kbdptr = &kbdm[0];						/*    at the beginning */
	return(TRUE);
}

/* Abort.
 * Beep the beeper. Kill off any keyboard macro, etc., that is in progress.
 * Sometimes called as a routine, to do general aborting of stuff.
 */

int ctrlg( int f, int n )
{
	TTbeep();
	kbdmode = STOP;
	mlwrite(TEXT8);							/* "[Aborted]" */
	return(ABORT);
}



int rdonly()
{
	TTbeep();
	mlwrite(TEXT109);						/* "[Key illegal in VIEW mode]" */
	return(FALSE);
}

int resterr()
{
	TTbeep();
	mlwrite(TEXT110);						/* "[That command is RESTRICTED]" */
	return(FALSE);
}

int nullproc( int f, int n)						/* user function that does NOTHING */
{
	return(TRUE);
}

int meta( int f, int n )						/* set META prefixing pending */
{
	prefix |= META;
	prenum = n;
	predef = f;
	return(TRUE);
}

int cex( int f, int n )							/* set ^X prefixing pending */
{
	prefix |= CTLX;
	prenum = n;
	predef = f;
	return(TRUE);
}

int unarg( int f, int n )						/* dummy function for binding to universal-argument */
{
	return(TRUE);
}

char * bytecopy( char *dst, char *src, int maxlen )			/* copy a string with length restrictions ALWAYS null terminate */
{
	char *dptr;	/* ptr into dst */

	dptr = dst;
	while ((maxlen-- > 0) && *src)
		*dptr++ = *src++;
	*dptr = 0;
	return(dst);
}

char * copystr( char *sp )
{
	char *dp;	/* copy of string */

	/* make room! */
	dp = malloc(strlen(sp)+1);
	if (dp == NULL)
		return(NULL);
	strcpy(dp, sp);
	return(dp);
}
