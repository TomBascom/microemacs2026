/* word.c
 *
 * The routines in this file implement commands that work word or a
 * paragraph at a time.  There are all sorts of word mode commands.  If I
 * do any sentence mode commands, they are likely to be put in this file. 
 */

#include <stdio.h>
#include <ctype.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"

/* Word wrap on n-spaces. Back-over whatever precedes the point on the current
 * line and stop on the first word-break or the beginning of the line. If we
 * reach the beginning of the line, jump back to the end of the word and start
 * a new line.	Otherwise, break the line at the word-break, eat it, and jump
 * back to the end of the word. Make sure we force the display back to the
 * left edge of the current window
 * Returns TRUE on success, FALSE on errors.
 */

int wrapword( int f, int n)
{
	register int cnt;	/* size of word wrapped to next line */
	register int c;		/* charector temporary */

	/* backup from the <NL> 1 char */
	if (!backchar(FALSE, 1))
		return(FALSE);

	/* back up until we aren't in a word,
	   make sure there is a break in the line */
	cnt = 0;
	while (((c = lgetc(curwp->w_dotp, curwp->w_doto)) != ' ')
				&& (c != '\t')) {
		cnt++;
		if (!backchar(FALSE, 1))
			return(FALSE);
		/* if we make it to the beginning, start a new line */
		if (curwp->w_doto == 0) {
			gotoeol(FALSE, 0);
			return(lnewline());
		}
	}

	/* delete the forward white space */
	if (!forwdel(0, 1))
		return(FALSE);

	/* put in a end of line */
	if (!lnewline())
		return(FALSE);

	/* and past the first word */
	while (cnt-- > 0) {
		if (forwchar(FALSE, 1) == FALSE)
			return(FALSE);
	}

	/* make sure the display is not horizontally scrolled */
	if (curwp->w_fcol != 0) {
		curwp->w_fcol = 0;
		curwp->w_flag |= WFHARD | WFMOVE | WFMODE;
	}

	return(TRUE);
}

int backword( int f, int n )
{
	if (n < 0)
		return(forwword(f, -n));
	if (backchar(FALSE, 1) == FALSE)
		return(FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (backchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
		while (inword() != FALSE) {
			if (backchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
	}
	return(forwchar(FALSE, 1));
}

int forwword( int f, int n )
{
	if (n < 0)
		return(backword(f, -n));
	while (n--) {
		/* scan through the current word */
		while (inword() == TRUE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}

		/* scan through the intervening white space */
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
	}
	return(TRUE);
}

int endword( int f, int n )
{
	if (n < 0)
		return(backword(f, -n));
	while (n--) {
		/* scan through the intervening white space */
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}

		/* scan through the current word */
		while (inword() == TRUE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
	}
	return(TRUE);
}

int upperword( int f, int n )
{
	int c;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return(FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
		while (inword() != FALSE) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (is_lower(c)) {
				c = upperc(c);
				lputc(curwp->w_dotp, curwp->w_doto, c);
				lchange(WFHARD);
			}
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
	}
	return(TRUE);
}

int lowerword( int f, int n )
{
	int c;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return(FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
		while (inword() != FALSE) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (is_upper(c)) {
				c = lowerc(c);
				lputc(curwp->w_dotp, curwp->w_doto, c);
				lchange(WFHARD);
			}
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
	}
	return(TRUE);
}

int capword( int f, int n )
{
	int c;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (n < 0)
		return(FALSE);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
		}
		if (inword() != FALSE) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (is_lower(c)) {
				c = upperc(c);
				lputc(curwp->w_dotp, curwp->w_doto, c);
				lchange(WFHARD);
			}
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
			while (inword() != FALSE) {
				c = lgetc(curwp->w_dotp, curwp->w_doto);
				if (is_upper(c)) {
					c = lowerc(c);
					lputc(curwp->w_dotp, curwp->w_doto, c);
					lchange(WFHARD);
				}
				if (forwchar(FALSE, 1) == FALSE)
					return(FALSE);
			}
		}
	}
	return(TRUE);
}

int delfword( int f, int n )
{
	register LINE	*dotp;	/* original cursor line */
	register int	doto;	/*	and row */
	register int c;		/* temp char */
	long size;		/* # of chars to delete */

	/* don't allow this command if we are in read only mode */
	if (curbp->b_mode&MDVIEW)
		return(rdonly());

	/* ignore the command if there is a negative argument */
	if (n < 0)
		return(FALSE);

	/* Clear the kill buffer if last command wasn't a kill */
	if ((lastflag&CFKILL) == 0)
		next_kill();
	thisflag |= CFKILL;	/* this command is a kill */

	/* save the current cursor position */
	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	/* figure out how many characters to give the axe */
	size = 0;

	/* get us into a word.... */
	while (inword() == FALSE) {
		if (forwchar(FALSE, 1) == FALSE)
			return(FALSE);
		++size;
	}

	if (n == 0) {
		/* skip one word, no whitespace! */
		while (inword() == TRUE) {
			if (forwchar(FALSE, 1) == FALSE)
				return(FALSE);
			++size;
		}
	} else {
		/* skip n words.... */
		while (n--) {
	
			/* if we are at EOL; skip to the beginning of the next */
			while (curwp->w_doto == lused(curwp->w_dotp)) {
				if (forwchar(FALSE, 1) == FALSE)
					return(FALSE);
				++size;
			}
	
			/* move forward till we are at the end of the word */
			while (inword() == TRUE) {
				if (forwchar(FALSE, 1) == FALSE)
					return(FALSE);
				++size;
			}
	
			/* if there are more words, skip the interword stuff */
			if (n != 0)
				while (inword() == FALSE) {
					if (forwchar(FALSE, 1) == FALSE)
						return(FALSE);
					++size;
				}
		}
	
		/* skip whitespace and newlines */
		while ((curwp->w_doto == lused(curwp->w_dotp)) ||
			((c = lgetc(curwp->w_dotp, curwp->w_doto)) == ' ') ||
			(c == '\t')) {
				if (forwchar(FALSE, 1) == FALSE)
					break;
				++size;
		}
	}

	/* restore the original position and delete the words */
	curwp->w_dotp = dotp;
	curwp->w_doto = doto;
	return(ldelete(size, TRUE));
}

int delbword( int f, int n )
{
	long size;

	/* don't allow this command if we are in read only mode */
	if (curbp->b_mode&MDVIEW)
		return(rdonly());

	/* ignore the command if there is a nonpositive argument */
	if (n <= 0)
		return(FALSE);

	/* Clear the kill buffer if last command wasn't a kill */
	if ((lastflag&CFKILL) == 0)
		next_kill();
	thisflag |= CFKILL;	/* this command is a kill */

	if (backchar(FALSE, 1) == FALSE)
		return(FALSE);
	size = 0;
	while (n--) {
		while (inword() == FALSE) {
			if (backchar(FALSE, 1) == FALSE)
				return(FALSE);
			++size;
		}
		while (inword() != FALSE) {
			++size;
			if (backchar(FALSE, 1) == FALSE)
				goto bckdel;
		}
	}
	if (forwchar(FALSE, 1) == FALSE)
		return(FALSE);
bckdel:	if (forwchar(FALSE, size) == FALSE)
		return(FALSE);
	return(ldelete(-size, TRUE));
}

int inword()
{
	register int c;

	/* the end of a line is never in a word */
	if (curwp->w_doto == lused(curwp->w_dotp))
		return(FALSE);

	/* grab the word to check */
	c = lgetc(curwp->w_dotp, curwp->w_doto);

	/* if we are using the table.... */
	if (wlflag)
		return(wordlist[c]);

	/* else use the default hard coded check */
	/* if (isletter(c))
		return(TRUE);
	if (c>='0' && c<='9')
		return(TRUE);
	if (c == '_')
	        return(TRUE); */

        if (!isspace(c))
	        return(TRUE);
	return(FALSE);
}

int justifyline(char *str, int *clength, int leftmarg)
{
	char newline[NSTRING];
	int  added, i, j, k, l, num, npunct, quot, rem, suppl;

        if (*clength >= fillcol) return(0);
        added = fillcol - *clength;

        num = 0;
        npunct = 0;

        for (i=leftmarg; i < *clength; i++) {
                if (isspace(str[i])) {
                     num++;
                     if (i>leftmarg && ispunct(str[i-1])) ++npunct;
		}
	}

        if (num==0) return(0);

        quot = added / num;
        rem = added % num;
        if (rem >= npunct) {
	  rem = rem - npunct;
          suppl = 1;
	} else
	  suppl = 0;

        j = 0;

        for (i=leftmarg; i < *clength; i++) {
	        newline[j] = str[i]; 
                j++;
                k = 0;
                if (isspace(str[i])) {
                   k = quot;
                   if ( i > leftmarg && ispunct(str[i-1])) {
                      if (suppl || rem>0) {
                         ++k;
                         if (!suppl) --rem;
		      }
		   }
		   else
                      if (suppl && rem>0) {
		         ++k;
                         --rem;
		      }
		}
                for (l=0; l<k; l++) {
                  newline[j] = ' ';
                  j++;
		}
	}

        *clength = j + leftmarg;
        for (i=0; i<j; i++) str[i + leftmarg] = newline[i];
        return(0);
}

int reformat( int f, int n )
{
	register int c;			/* current char durring scan	*/
	register int wordlen;		/* length of current word	*/
	register int i;			/* index during word copy	*/
	register int first;		/* where to start copy		*/
	register int eopflag;		/* Are we at the End-Of-Paragraph? */
	register int spaceflag;		/* need space? (0 for 1st word)	*/
	register int longword;		/* long word encountered	*/
	register LINE *eopline;		/* pointer to line just past EOP */
	char wbuf[NSTRING];		/* buffer for current word	*/
	char lbuf[2*NSTRING];		/* buffer for current line	*/
	int clength;			/* position on line during fill	*/
	int leftmarg;			/* left marginal */

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if (fillcol == 0) {	/* no fill column set */
		mlwrite(TEXT98);
		return(FALSE);
	}

        /* calculate leftmarg to be used in case of indentation */
        leftmarg = getccol(FALSE);
        clength = curwp->w_doto;
       	if (indentflag && leftmarg+10 > fillcol) {
	 	        mlwrite(TEXT59, fillcol);
		        return(FALSE);
	}

	/* record the pointer to the line just past the EOP */
	gotoeop(FALSE, 1);
	eopline = lforw(curwp->w_dotp);

        /* add blank space to avoid problems at end of file! */
        linsert(1, ' ');

	/* and back top the beginning of the paragraph */
	gotobop(FALSE, 1);

        first = leftmarg;

	/* initialize various info */
        curwp->w_doto = clength;
	clength = leftmarg;

	wordlen = 0;
        longword = 0;
        spaceflag = (indentflag && leftmarg>0 &&
                        isspace(curwp->w_dotp->l_text[leftmarg]));

	/* scan through lines, filling words */
	eopflag = FALSE;
	while (!eopflag) {
		/* get the next character in the paragraph */
		if (curwp->w_doto == lused(curwp->w_dotp)) {
			c = ' ';
			if (lforw(curwp->w_dotp) == eopline)
				eopflag = TRUE;
		} else
			c = lgetc(curwp->w_dotp, curwp->w_doto);

		/* and then delete it */
		ldelete(1L, FALSE);

		/* if not a separator, just add it in */
                if (!isspace(c) && !longword) {
			wbuf[wordlen++] = c;
			if (wordlen >= NSTRING - 1) {
                               longword = 1;
                               goto toolong;
			}
		} else if (wordlen) {
			/* at a word break with a word waiting */
			/* check tentative new length with word added */
			if (clength + spaceflag + wordlen <= fillcol) {
				if (spaceflag) {
					lbuf[clength] = ' '; /* the space */
					++clength;
				}
			} else  {
            		     toolong:
                                /* justify the line with blanks */
			        if (justflag)
                                        justifyline(lbuf, &clength, leftmarg);
 		                /* add the line to buffer */
			        for (i=first; i<clength; i++)
				        linsert(1, lbuf[i]);
                                first = 0;
 			        /* start a new line */
				lnewline();
                                if (!indentflag) 
                                       leftmarg = 0;
                                else
				       for (i=0; i<leftmarg; i++)
				                lbuf[i] = ' ';
				clength = leftmarg;
			}
			/* add word to line in both cases */
                        if (longword) {
                               for (i=0; i<wordlen; i++)
			                  linsert(1, wbuf[i]);
                               wordlen = 0;
                               clength = 0;
                               spaceflag = 0;
			} else {
                               for (i=0; i<wordlen; i++)
			                  lbuf[clength++] = wbuf[i];
			       spaceflag = 1;
			       wordlen = 0;
			}
		}
	}

	/* and add the last line and a newline for the end of new paragraph */
        for (i=first; i<clength; i++)
	   linsert(1, lbuf[i]);
	lnewline();

	forwword(FALSE, 1);
	if (lused(curwp->w_dotp) > leftmarg)
		curwp->w_doto = leftmarg;
	else
		curwp->w_doto = lused(curwp->w_dotp);

	return(TRUE);
}

int formatpara( int f, int n )		/* Reformat the current paragraph according to the current fill column		*/
{
        justflag = FALSE;
	indentflag = FALSE;
        return(reformat(f, n));
}

int rightjustifypara( int f, int n )	/* Justify the current paragraph according to the current fill column		*/
{
        int status;
        justflag = TRUE;
	indentflag = FALSE;
        status = reformat(f, n);
        return(status);
}

int leftjustifypara( int f, int n )	/* Indent the current paragraph according to the current cursor column		*/
{
        int status;
        justflag = FALSE;
        indentflag = TRUE;
        status = reformat(f, n);
        indentflag = FALSE;
        return(status);
}

int leftrightjustifypara( int f, int n )		/* Adjust the current paragraph according to the current cursor and fill columns	*/
{
        int status;
        justflag = TRUE;
        indentflag = TRUE;
        status = reformat(f, n);
        indentflag = FALSE;
        return(status);
}

int killpara( int f, int n )		/* delete n paragraphs starting with the current one */
{
	register int status;	/* returned status of functions */

	while (n--) {		/* for each paragraph to delete */

		/* mark out the end and beginning of the para to delete */
		gotoeop(FALSE, 1);

		/* set the mark here */
		curwp->w_markp[0] = curwp->w_dotp;
		curwp->w_marko[0] = curwp->w_doto;

		/* go to the beginning of the paragraph */
		gotobop(FALSE, 1);
		curwp->w_doto = 0;	/* force us to the beginning of line */
	
		/* and delete it */
		if ((status = killregion(FALSE, 1)) != TRUE)
			return(status);

		/* and clean up the 2 extra lines */
		ldelete(2L, TRUE);
	}
	return(TRUE);
}

/*	wordcount:	count the # of words in the marked region,
			along with average word sizes, # of chars, etc,
			and report on them.			*/

int wordcount( int f, int n )
{
	register LINE *lp;	/* current line to scan */
	register int offset;	/* current char to scan */
	long size;		/* size of region left to count */
	register int ch;	/* current character to scan */
	register int wordflag;	/* are we in a word now? */
	register int lastword;	/* were we just in a word? */
	long nwords;		/* total # of words */
	long nchars;		/* total number of chars */
	int nlines;		/* total number of lines in region */
	int avgch;		/* average number of chars/word */
	int status;		/* status return code */
	REGION region;		/* region to look at */

	/* make sure we have a region to count */
	if ((status = getregion(&region)) != TRUE)
		return(status);
	lp = region.r_linep;
	offset = region.r_offset;
	size = region.r_size;

	/* count up things */
	lastword = FALSE;
	nchars = 0L;
	nwords = 0L;
	nlines = 0;
	while (size--) {

		/* get the current character */
		if (offset == lused(lp)) {	/* end of line */
			ch = '\r';
			lp = lforw(lp);
			offset = 0;
			++nlines;
		} else {
			ch = lgetc(lp, offset);
			++offset;
		}

		/* and tabulate it */
		wordflag = ((ch >= 'a' && ch <= 'z') ||
			    (ch >= 'A' && ch <= 'Z') ||
			    (ch >= '0' && ch <= '9'));
		if (wordflag == TRUE && lastword == FALSE)
			++nwords;
		lastword = wordflag;
		++nchars;
	}

	/* and report on the info */
	if (nwords > 0L)
		avgch = (int)((100L * nchars) / nwords);
	else
		avgch = 0;

	mlwrite(TEXT100,		/*              "Words %D Chars %D Lines %d Avg chars/word %f" */
		nwords, nchars, nlines + 1, avgch);
	return(TRUE);
}
