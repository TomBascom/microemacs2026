/*	basic.c:	Basic movement functions for
 *			MicroEMACS
 *			(C)Copyright 1993 by Daniel Lawrence
 *
 * The routines in this file move the cursor around on the screen. They
 * compute a new value for the cursor, then adjust ".". The display code
 * always updates the cursor location, so only moves between lines, or
 * functions that adjust the top line in the window and invalidate the
 * framing, are hard.
 */
#include	<stdio.h>
#include	<ctype.h>
#include	"estruct.h"
#include	"eproto.h"
#include	"edef.h"
#include	"english.h"

int gotobol( int f, int n )	/* Move the cursor to the beginning of the current line. */
{
        curwp->w_doto = 0;
        return(TRUE);
}

/*
 * Move the cursor backwards by "n" characters. If "n" is less than zero call
 * "forwchar" to actually do the move. Otherwise compute the new cursor
 * location. Error if you try and move out of the buffer. Set the flag if the
 * line pointer for dot changes.
 */

int backchar( int f, int n )
{
        register LINE   *lp;

        if (n < 0)
                return(forwchar(f, -n));
        while (n--) {
                if (curwp->w_doto == 0) {
                        if ((lp=lback(curwp->w_dotp)) == curbp->b_linep)
                                return(FALSE);
                        curwp->w_dotp  = lp;
                        curwp->w_doto  = lused(lp);
                        curwp->w_flag |= WFMOVE;
                } else
                        curwp->w_doto--;
        }
        return(TRUE);
}


int gotoeol( int f, int n )	/* Move the cursor to the end of the current line. Trivial. No errors.	*/
{
        curwp->w_doto  = lused(curwp->w_dotp);
        return(TRUE);
}

/*
 * Move the cursor forwards by "n" characters. If "n" is less than zero call
 * "backchar" to actually do the move. Otherwise compute the new cursor
 * location, and move ".". Error if you try and move off the end of the
 * buffer. Set the flag if the line pointer for dot changes.
 */

int forwchar( int f, int n )
{
        if (n < 0)
                return(backchar(f, -n));
        while (n--) {
                if (curwp->w_doto == lused(curwp->w_dotp)) {
                        if (curwp->w_dotp == curbp->b_linep)
                                return(FALSE);
                        curwp->w_dotp  = lforw(curwp->w_dotp);
                        curwp->w_doto  = 0;
                        curwp->w_flag |= WFMOVE;
                } else
                        curwp->w_doto++;
        }
        return(TRUE);
}

int  gotoline( int f, int n )	/* move to a particular line. argument (n) must be a positive integer for this to actually do anything		*/
{
	register int status;	/* status return */
	char arg[NSTRING];	/* buffer to hold argument */

	/* get an argument if one doesnt exist */
	if (f == FALSE) {
		if ((status = mlreply(TEXT7, arg, NSTRING)) != TRUE) {	/* "Line to GOTO: "	*/
			mlwrite(TEXT8);					/* "[Aborted]"		*/
			return(status);
		}
		n = asc_int(arg);
	}

	if (n < 1) return(FALSE);		/* if a bogus argument...then leave */

        curwp->w_dotp  = lforw(curbp->b_linep);		/* first, we go to the start of the buffer */
        curwp->w_doto  = 0;
	return(forwline(f, n-1));
}

/*
 * Goto the beginning of the buffer. Massive adjustment of dot. This is
 * considered to be hard motion; it really isn't if the original value of dot
 * is the same as the new value of dot. Normally bound to "M-<".
 */

int gotobob( int f, int n )
{
        curwp->w_dotp  = lforw(curbp->b_linep);
        curwp->w_doto  = 0;
        curwp->w_flag |= WFMOVE;
        return(TRUE);
}

/*
 * Move to the end of the buffer. Dot is always put at the end of the file
 * (ZJ). The standard screen code does most of the hard parts of update.
 * Bound to "M->".
 */

int gotoeob( int f, int n )
{
        curwp->w_dotp  = curbp->b_linep;
        curwp->w_doto  = 0;
        curwp->w_flag |= WFMOVE;
        return(TRUE);
}

/*
 * Move forward by full lines. If the number of lines to move is less than
 * zero, call the backward line function to actually do it. The last command
 * controls how the goal column is set. Bound to "C-N". No errors are
 * possible.
 */

int forwline( int f, int n )
{
        register LINE   *dlp;

        if (n < 0)
                return(backline(f, -n));

	/* if we are on the last line as we start....fail the command */
	if (curwp->w_dotp == curbp->b_linep)
		return(FALSE);

	/* if the last command was not note a line move,
	   reset the goal column */
        if ((lastflag&CFCPCN) == 0)
                curgoal = getccol(FALSE);

	/* flag this command as a line move */
        thisflag |= CFCPCN;

	/* and move the point down */
        dlp = curwp->w_dotp;
        while (n-- && dlp!=curbp->b_linep)
                dlp = lforw(dlp);

	/* reseting the current position */
        curwp->w_dotp  = dlp;
        curwp->w_doto  = getgoal(dlp);
        curwp->w_flag |= WFMOVE;
        return(TRUE);
}

/*
 * This function is like "forwline", but goes backwards. The scheme is exactly
 * the same. Check for arguments that are less than zero and call your
 * alternate. Figure out the new line and call "movedot" to perform the
 * motion. No errors are possible. Bound to "C-P".
 */

int backline( int f, int n )
{
        register LINE   *dlp;

        if (n < 0)
                return(forwline(f, -n));


	/* if we are on the last line as we start....fail the command */
	if (lback(curwp->w_dotp) == curbp->b_linep)
		return(FALSE);

	/* if the last command was not note a line move,
	   reset the goal column */
        if ((lastflag&CFCPCN) == 0)
                curgoal = getccol(FALSE);

	/* flag this command as a line move */
        thisflag |= CFCPCN;

	/* and move the point up */
        dlp = curwp->w_dotp;
        while (n-- && lback(dlp)!=curbp->b_linep)
                dlp = lback(dlp);

	/* reseting the current position */
        curwp->w_dotp  = dlp;
        curwp->w_doto  = getgoal(dlp);
        curwp->w_flag |= WFMOVE;
        return(TRUE);
}

/* go back to the beginning of the current paragraph here we look for a <NL><NL> or <NL><TAB> or <NL><SPACE> combination to delimit the beginning of a paragraph	*/

int gotobop( int f, int n )	
{
	register int suc;	/* success of last backchar */
	register int found;	/* found non space character */

	if (n < 0)	/* the other way...*/
		return(gotoeop(f, -n));

	while (n-- > 0) {	/* for each one asked for */

		/* first scan back until we are in a word */
		suc = backchar(FALSE, 1);
		while (!inword() && suc)
			suc = backchar(FALSE, 1);
		curwp->w_doto = 0;	/* and go to the B-O-Line */

		/* and scan back until we hit a <NL><NL> or <NL><TAB>
		   or a <NL><SPACE>					*/
		while (lback(curwp->w_dotp) != curbp->b_linep)
 		        if (lused(curwp->w_dotp) != 0) {
			  found = 0;
                          for (suc=0; suc<lused(curwp->w_dotp); suc++)
          		    if (!isspace(lgetc(curwp->w_dotp,
					       suc+curwp->w_doto))) {
			        found =1;
				break;
			    }
			  if (found == 0)
			      break;
			  else
			      curwp->w_dotp = lback(curwp->w_dotp);
			}
			else
				break;

		/* and then forward until we are in a word */
		suc = forwchar(FALSE, 1);
		while (suc && !inword())
			suc = forwchar(FALSE, 1);
	}
	curwp->w_flag |= WFMOVE;	/* force screen update */
	return(TRUE);
}

int gotoeop( int f, int n )	/* go forword to the end of the current paragraph here we look for a <NL><NL> or <NL><TAB> or <NL><SPACE> combination to delimit the beginning of a paragraph	*/
{
	register int suc;	/* success of last backchar */
	register int found;	/* found non space character */

	if (n < 0)	/* the other way...*/
		return(gotobop(f, -n));

	while (n-- > 0) {	/* for each one asked for */

		/* first scan forward until we are in a word */
		suc = forwchar(FALSE, 1);
		while (!inword() && suc)
			suc = forwchar(FALSE, 1);
		curwp->w_doto = 0;	/* and go to the B-O-Line */
		if (suc)	/* of next line if not at EOF */
			curwp->w_dotp = lforw(curwp->w_dotp);

		/* and scan forword until we hit a <NL><NL> or <NL><TAB>
		   or a <NL><SPACE>					*/
		while (curwp->w_dotp != curbp->b_linep) {
		        if (lused(curwp->w_dotp) != 0) {
			  found = 0;
                          for (suc=0; suc<lused(curwp->w_dotp); suc++)
          		    if (!isspace(lgetc(curwp->w_dotp, 
					       suc+curwp->w_doto))) {
			        found = 1;
				break;
			    }
		          if (found == 0) 
			        break;
			  else
			        curwp->w_dotp = lforw(curwp->w_dotp);
			}
			else
				break;
		}
		/* and then backward until we are in a word */
		suc = backchar(FALSE, 1);
		while (suc && !inword()) {
			suc = backchar(FALSE, 1);
		}
		curwp->w_doto = lused(curwp->w_dotp);	/* and to the EOL */
	}
	curwp->w_flag |= WFMOVE;	/* force screen update */
	return(TRUE);
}

/* This routine, given a pointer to a LINE, and the current cursor goal
 * column, return the best choice for the offset. The offset is returned.
 * Used by "C-N" and "C-P".
 */

int getgoal( LINE *dlp )
/* register LINE   *dlp; */
{
        register int    c;
        register int    col;
        register int    newcol;
        register int    dbo;

        col = 0;
        dbo = 0;
        while (dbo != lused(dlp)) {
                c = lgetc(dlp, dbo);
                newcol = col;
                if (c == '\t')
			newcol += -(newcol % tabsize) + (tabsize - 1);
                else if (c<0x20 || c==0x7F)
                        ++newcol;
                ++newcol;
                if (newcol > curgoal)
                        break;
                col = newcol;
                ++dbo;
        }
        return(dbo);
}

/* Scroll forward by a specified number of lines, or by a full page if no
 * argument. Bound to "C-V". The overlap in the arithmetic on the window size
 * is overlap between screens. This defaults to overlap value in ITS EMACS.
 *  Because this zaps the top line in the window, we have to do a hard update.
 */

int forwpage( int f, int n )
{
        register LINE   *lp;

        if (f == FALSE) {
                n = curwp->w_ntrows - overlap;        /* Default scroll.      */
                if (n <= 0)                     /* Forget the overlap   */
                        n = 1;                  /* if tiny window.      */
        } else if (n < 0)
                return(backpage(f, -n));
        lp = curwp->w_linep;
        while (n-- && lp!=curbp->b_linep)
                lp = lforw(lp);
        curwp->w_linep = lp;
        curwp->w_dotp  = lp;
        curwp->w_doto  = 0;
        curwp->w_flag |= WFHARD;
        return(TRUE);
}

/* This command is like "forwpage", but it goes backwards. overlap, like
 * above, is the overlap between the two windows. The value is from the ITS
 * EMACS manual. Bound to "M-V". We do a hard update for exactly the same
 * reason.
 */

int backpage( int f, int n )
{
        register LINE   *lp;

        if (f == FALSE) {
                n = curwp->w_ntrows - overlap;        /* Default scroll.      */
                if (n <= 0)                     /* Don't blow up if the */
                        n = 1;                  /* window is tiny.      */
        } else if (n < 0)
                return(forwpage(f, -n));
        lp = curwp->w_linep;
        while (n-- && lback(lp)!=curbp->b_linep)
                lp = lback(lp);
        curwp->w_linep = lp;
        curwp->w_dotp  = lp;
        curwp->w_doto  = 0;
        curwp->w_flag |= WFHARD;
        return(TRUE);
}

/* Procedure added to handle Up arrow and Down arrow keys under X/Linux */

int scrollpage( int f, int n )
{
	char c, cp;

        c = tgetc();
        cp = tgetc();

        if (c == '2' && cp == '~' ) {
	        if ((curbp->b_mode & MDOVER)) {
                    curbp->b_mode &= ~MDOVER;
		} else {
                    curbp->b_mode |= MDOVER;
		}
                upmode();
	}
        if (c == '3') delbword( f, n );
        if (c == '6') forwpage( f, n );
        if (c == '5') backpage( f, n );

        if (c == '1' || (c == '2' && cp != '~')) {
	  /* (c,cp) yields value of Fn function or arrow keys... */
                cp = tgetc();
	}
        return(TRUE);
}

int gotoeorbob( int f, int n )
{
	char c;
        c = tgetc();
        if (c == 'H') gotobob(f,n);
        if (c == 'F') gotoeob(f,n);
        return(TRUE);
}

/* Set the mark in the current window to the value of "." in the window. No
 * errors are possible. Bound to "M-.".
 */

int setmark( int f, int n )
{
	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

        curwp->w_markp[n] = curwp->w_dotp;
        curwp->w_marko[n] = curwp->w_doto;
        mlwrite(TEXT9, n);		/* "[Mark %d set]" */
        return(TRUE);
}

/* Remove the mark in the current window.
 * Bound to ^X <space> 
 */

int remmark( int f, int n )
{
	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

        curwp->w_markp[n] = NULL;
        curwp->w_marko[n] = 0;
        mlwrite(TEXT10, n);	/* "[Mark %d removed]" */
        return(TRUE);
}

/* Swap the values of "." and "mark" in the current window. This is pretty
 * easy, bacause all of the hard work gets done by the standard routine
 * that moves the mark about. The only possible error is "no mark". Bound to
 * "C-X C-X".
 */

int swapmark( int f, int n )
{
        register LINE   *odotp;
        register int    odoto;

	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

        if (curwp->w_markp[n] == NULL) {
                mlwrite(TEXT11, n);	/* "No mark %d in this window" */
                return(FALSE);
        }
        odotp = curwp->w_dotp;
        odoto = curwp->w_doto;
        curwp->w_dotp  = curwp->w_markp[n];
        curwp->w_doto  = curwp->w_marko[n];
        curwp->w_markp[n] = odotp;
        curwp->w_marko[n] = odoto;
        curwp->w_flag |= WFMOVE;
        return(TRUE);
}

/* Goto a mark in the current window. This is pretty easy, bacause all of
 * the hard work gets done by the standard routine that moves the mark
 * about. The only possible error is "no mark". Bound to "M-^G".
 */

int gotomark( int f, int n )
{
	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

        if (curwp->w_markp[n] == NULL) {
                mlwrite(TEXT11, n);	/* "No mark %d in this window" */
                return(FALSE);
        }
        curwp->w_dotp  = curwp->w_markp[n];
        curwp->w_doto  = curwp->w_marko[n];
        curwp->w_flag |= WFMOVE;
        return(TRUE);
}
