/* display.c
 *
 * The functions in this file handle redisplay. There are two halves, the
 * ones that update the virtual display screen, and the ones that make the
 * physical display screen the same as the virtual display screen. These
 * functions use hints that are left in the windows by the commands.
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"
#include <stdarg.h>

#define FARRIGHT	9999		/* column beyond the right edge! */	/* @@@ was 999	*/

static VIDEO   **vscreen;		       /* Virtual screen. */
static VIDEO   **pscreen;		       /* Physical screen. */

/*	some local function declarations	*/

extern void  update_line(int row, struct VIDEO *vp1, struct VIDEO *vp2);
extern void  update_hilite(void);

/*
 * Initialize the data structures used by the display code. The edge vectors
 * used to access the screens are set up. The operating system's terminal I/O
 * channel is set up. All the other things get initialized at compile time.
 * The original window has "WFCHG" set, so that it will get completely
 * redrawn on the first call to "update".
 */

int vtinit()
{
	register int i;
	register VIDEO *vp;

	TTopen();		/* open the screen */
	TTkopen();		/* open the keyboard */
	TTrev(FALSE);

	/* allocate the virtual screen pointer array */
	vscreen = (VIDEO **) malloc(term.t_mrow*sizeof(VIDEO *));
	
	if (vscreen == NULL)
		meexit(1);

	/* allocate the physical shadow screen array */
	pscreen = (VIDEO **) malloc(term.t_mrow*sizeof(VIDEO *));
	if (pscreen == NULL)
		meexit(1);

	/* for every line in the display */
	for (i = 0; i < term.t_mrow; ++i) {

		/* allocate a virtual screen line */
		vp = (VIDEO *) malloc(sizeof(VIDEO)+term.t_mcol);
		if (vp == NULL)
			meexit(1);

		vp->v_flag = 0;		/* init change flags */
		vp->v_left = FARRIGHT;	/* init requested rev video boundries */
		vp->v_right = 0;
		vp->v_rfcolor = 7;	/* init fore/background colors */
		vp->v_rbcolor = 0;
		/* connect virtual line to line array */
		vscreen[i] = vp;

		/* allocate and initialize physical shadow screen line */
		vp = (VIDEO *) malloc(sizeof(VIDEO)+term.t_mcol);
		if (vp == NULL)
			meexit(1);

		vp->v_flag = VFNEW;
		vp->v_left = FARRIGHT;
		vp->v_right = 0;
		pscreen[i] = vp;
	}
	return(TRUE);
}

/*
 * Clean up the virtual terminal system, in anticipation for a return to the
 * operating system. Move down to the last line and clear it out (the next
 * system prompt will be written in the line). Shut down the channel to the
 * terminal.
 */

int vttidy()
{
    mlerase();
    movecursor(term.t_nrow, 0);
    TTflush();
    TTclose();
    TTkclose();
    return(0);
}

/*
 * Set the virtual cursor to the specified row and column on the virtual
 * screen. There is no checking for nonsense values; this might be a good
 * idea during the early stages.
 */

int vtmove( int row, int col )
{
    vtrow = row;
    vtcol = col;
    return(0);
}

/* Write a character to the virtual screen. The virtual row and
   column are updated. If we are not yet on left edge, don't print
   it yet. If the line is too long put a "$" in the last column.
   This routine only puts printing characters into the virtual
   terminal buffers. Only column overflow is checked.
*/

int vtputc( int c )
{
	register VIDEO *vp;	/* ptr to line being updated */

	/* defeate automatic sign extenstion */
	c = c & 0xff;

	/* this is the line to put this character! */
	vp = vscreen[vtrow];

	if (c == '\t') {

		/* output a hardware tab as the right number of spaces */
		do {
			vtputc(' ');
		} while (((vtcol + taboff) % (tabsize)) != 0);

	} else if (vtcol >= term.t_ncol) {

		/* we are at the right edge! */
		++vtcol;
		vp->v_text[term.t_ncol - 1] = '$';

	} else if (disphigh && c > 0x7f) {

		/* char with high bit set is displayed
		   symbolically on 7 bit screens */
		vtputc('^');
		vtputc('!');
		c -= 0x80;
		if (c == '\t') {
			vtputc('^');
			vtputc('I');
		} else
			vtputc(c);

	} else if (c < 0x20 || c == 0x7F) {

		/* control character? */
		vtputc('^');
		vtputc(c ^ 0x40);

	} else {

		/* it's normal, just put it in the screen map */
		if (vtcol >= 0)
			vp->v_text[vtcol] = c;
		++vtcol;
	}
	return(0);
}

int vteeol()		/* Erase from the end of the software cursor to the end of the line on which the software cursor is located.  */
{
    register VIDEO	*vp;

    vp = vscreen[vtrow];
    while (vtcol < term.t_ncol) {
    	if (vtcol >= 0)
		vp->v_text[vtcol] = ' ';
	vtcol++;
    }
    return(0);
}

int  upscreen( int f, int n )	/* upscreen:	user routine to force a screen update always finishes complete update 	*/
{
	update(TRUE);
	return(TRUE);
}

/*
 * Make sure that the display is right. This is a three part process. First,
 * scan through all of the windows looking for dirty ones. Check the framing,
 * and refresh the screen. Second, make sure that "currow" and "curcol" are
 * correct for the current window. Third, make the virtual and physical
 * screens the same.
 */

void update( int force )
{
	register WINDOW *wp;

	/* if we are not forcing the update, and there are keystrokes
	   waiting to be processed, skip the update */
	if (force == FALSE && typahead())
		return;

	/* if we are replaying a keyboard macro, don't bother keeping updated */
	if (force == FALSE && kbdmode == PLAY)
		return;

/*** @@@ indentation seems messed up...  is this doing what someone thinks it is doing? ***/

		/* update any windows that need refreshing */
		wp = wheadp;
		while (wp != NULL) {
			if (wp->w_flag) {
				/* if the window has changed, service it */
				reframe(wp);	/* check the framing */
				if ((wp->w_flag & ~WFMODE) == WFEDIT)
					updone(wp);	/* update EDITed line */
				else if (wp->w_flag & ~WFMOVE)
					updall(wp);	/* update all lines */
				if (wp->w_flag & WFMODE)
					modeline(wp);	/* update modeline */
				wp->w_flag = 0;
				wp->w_force = 0;
			}

			/* on to the next window */
			wp = wp->w_wndp;
		}

		/* recalc the current hardware cursor location */
			updpos();

			/* update the current window if we have to move it around */
			if (curwp->w_flag & WFHARD)
				updall(curwp);
			if (curwp->w_flag & WFMODE)
				modeline(curwp);
			curwp->w_flag = 0;

			/* highlight region in the current window if needed */
			update_hilite();

		/* check for lines to de-extend */
		upddex();

		/* if screen is garbage, re-plot it */
		if (sgarbf != FALSE) {
			if (gflags & GFSDRAW)
				sgarbf = FALSE;
			else
				updgar();
		}
	
		/* update the virtual screen to the physical screen */
		updupd(force);

	/* update the cursor and flush the buffers */
	movecursor(currow, curcol - lbound);
	TTflush();

	return;
}

void reframe( WINDOW *wp )	/*	reframe:	check to see if the cursor is on in the window and re-frame it if needed or wanted		*/
{
	register LINE *lp;	/* search pointer */
	register LINE *rp;	/* reverse search pointer */
	register LINE *hp;	/* ptr to header line in buffer */
	register LINE *tp;	/* temp debugging pointer */
	register int i;		/* general index/# lines to scroll */
	register int nlines;	/* number of lines in current window */

	/* figure out our window size */
	nlines = wp->w_ntrows;
	if (modeflag == FALSE)
		nlines++;

	/* if not a requested reframe, check for a needed one */
	if ((wp->w_flag & WFFORCE) == 0) {
		lp = wp->w_linep;
		for (i = 0; i < nlines; i++) {

			/* if the line is in the window, no reframe */
			if (lp == wp->w_dotp)
				return;

			/* if we are at the end of the file, reframe */
			if (lp == wp->w_bufp->b_linep)
				break;

			/* on to the next line */
			lp = lforw(lp);
		}
	}

	/* reaching here, we need a window refresh */
	i = wp->w_force;

	/* if smooth scrolling is enabled,
		first.. have we gone off the top? */
	if (sscroll && ((wp->w_flag & WFFORCE) == 0)) {
		/* search thru the buffer looking for the point */
		tp = lp = rp = wp->w_linep;
		hp = wp->w_bufp->b_linep;
		while ((lp != hp) || (rp != hp)) {

			/* did we scroll downward? */
			if (lp == wp->w_dotp) {
				i = nlines - 1;
				break;
			}

			/* did we scroll upward? */
			if (rp == wp->w_dotp) {
				i = 0;
				break;
			}

			/* advance forward and back */
			if (lp != hp)
				lp = lforw(lp);
			if (rp != hp)
				rp = lback(rp);

			/* problems????? */
			if (lp == tp || rp == tp) {
				mlforce("BUG IN SMOOTH SCROLL--GET DAN!\n");
				TTgetc();
			}
		}
	/* how far back to reframe? */
	} else if (i > 0) {	/* only one screen worth of lines max */
		if (--i >= nlines)
			i = nlines - 1;
	} else if (i < 0) {	/* negative update???? */
		i += nlines;
		if (i < 0)
			i = 0;
	} else
		i = nlines / 2;

	/* backup to new line at top of window */
	lp = wp->w_dotp;
	while (i != 0 && lback(lp) != wp->w_bufp->b_linep) {
		--i;
		if (i < 0) {
			mlforce("OTHER BUG IN DISPLAY --- GET DAN!!!\n");
			TTgetc();
		}
		lp = lback(lp);
	}

	/* and reset the current line at top of window */
	wp->w_linep = lp;
	wp->w_flag |= WFHARD;
	wp->w_flag &= ~WFFORCE;
}

void update_hilite()	/*	hilite:	in the current window, marks 10 and 11 are set and the area between them is on screen, hilite that area	*/
{
	int first_line;		/* first screen line to highlight */
	short first_pos;	/* position in that line */
	int last_line;		/* last screen line to highlight */
	short last_pos;		/* position in that line */
	LINE *forptr, *bckptr;	/* line pointers searching in current buffer */
	int forline, bckline;	/* screen lines of for/bck ptrs */
	int nlines;		/* number of text lines in current window */
	LINE *first_mark;	/* first mark to highlighted text */
	LINE *last_mark;	/* last mark to highlighted text */
	LINE *b_linep;		/* header line of current buffer */
	int temp_line;		/* temp line # for swap */
	short temp_pos;		/*  more of the same */

	/* $hilight must be set to the first of 2 consecutive marks
	   used to define the region to highlight */
	if (hilite > NMARKS)
		return;

	/* Both marks must be set to define a highlighted region */
	first_mark = curwp->w_markp[hilite];
	last_mark = curwp->w_markp[hilite+1];
	if ((first_mark == (LINE *)NULL) ||
	    (last_mark == (LINE *)NULL))
	    	return;

	/* search for the two marks starting at the top line of this window */
	first_pos = last_pos = -1;
	forptr = curwp->w_linep;
	bckptr = curwp->w_linep;
	forline = bckline = 0;
	b_linep = curwp->w_bufp->b_linep;
	while (((first_pos == -1) || (last_pos == -1)) &&
		((forptr != (LINE *)NULL) || (bckptr != (LINE *)NULL))) {

		/* have we found either mark? */
		if (forptr == first_mark) {
			first_line = forline;
			first_pos = findcol(forptr, curwp->w_marko[hilite]);
		}
		if (forptr == last_mark) {
			last_line = forline;
			last_pos = findcol(forptr, curwp->w_marko[hilite+1]);
		}
		if (bckptr == first_mark) {
			first_line = bckline;
			first_pos = findcol(bckptr, curwp->w_marko[hilite]);
		}
		if (bckptr == last_mark) {
			last_line = bckline;
			last_pos = findcol(bckptr, curwp->w_marko[hilite+1]);
		}

		/* step outward one more line */
		if (forptr != (LINE *)NULL) {
			if (forptr != b_linep)
				forptr = lforw(forptr);
			else
				forptr = (LINE *)NULL;
			forline++;
		}
		if (bckptr != (LINE *)NULL) {
			bckptr = lback(bckptr);
			if (bckptr == b_linep)
				bckptr = (LINE *)NULL;
			bckline--;
		}
	}

	/* if both lines are before the current window */
	if ((first_line < 0) && (last_line < 0))
		return;

	/* if both lines are after the current window */
	nlines = curwp->w_ntrows;
	if (modeflag == FALSE)
		nlines++;
	if ((first_line >= nlines) && (last_line >= nlines))
		return;

	/* if we got them backwards, swap them around */
	if ((first_line > last_line) ||
	    ((first_line == last_line) && (first_pos > last_pos))) {
		temp_line = first_line;
		first_line = last_line;
		last_line = temp_line;
		temp_pos = first_pos;
		first_pos = last_pos;
		last_pos = temp_pos;
	}

	forptr = curwp->w_linep;
	forline = curwp->w_toprow;
	first_line += forline;
	last_line += forline;
	while (forline < curwp->w_toprow + nlines) {
		if ((forline >= first_line) && (forline <= last_line)) {
			vscreen[forline]->v_left = 0;
			vscreen[forline]->v_right = findcol(forptr, lused(forptr));
			if (forline == first_line)
				vscreen[forline]->v_left = first_pos;
			if (forline == last_line)
				vscreen[forline]->v_right = last_pos;

			/* adjust for shifted window */
			vscreen[forline]->v_left -= curwp->w_fcol;
			vscreen[forline]->v_right -= curwp->w_fcol;

			/* adjust for shifted line */
			if (vscreen[forline]->v_flag & VFEXT) {
				vscreen[forline]->v_left -= lbound;
				vscreen[forline]->v_right -= lbound;
			}

                } else {
                        vscreen[forline]->v_left = FARRIGHT;
                        vscreen[forline]->v_right = 0;
                }

                /* step up one more line */
                if (forptr != b_linep)
                        forptr = lforw(forptr);
                forline++;
        }

        /* we need to flag a redraw to update the hilighted region */
        curwp->w_flag |= WFHARD;
        return;
}

void updone( WINDOW *wp )	/*      updone: update the current line to the virtual screen           */
{
        register LINE *lp;      /* line to update */
        register int sline;     /* physical screen line to update */
        register int i;

        /* search down the line we want */
        lp = wp->w_linep;
        sline = wp->w_toprow;
        while (lp != wp->w_dotp) {
                ++sline;
                lp = lforw(lp);
        }

        /* and update the virtual line */
        vscreen[sline]->v_flag |= VFCHG;
        taboff = wp->w_fcol;
        vtmove(sline, -taboff);

        /* move each char of line to virtual screen until at end */
        for (i=0; i < lused(lp); ++i)
                vtputc(lgetc(lp, i));
        vscreen[sline]->v_rfcolor = wp->w_fcolor;
        vscreen[sline]->v_rbcolor = wp->w_bcolor;
        vteeol();
        taboff = 0;
}

void  updall( WINDOW *wp )	/*      updall: update all the lines in a window on the virtual screen */
{
        register LINE *lp;      /* line to update */
        register int sline;     /* physical screen line to update */
        register int i;
        register int nlines;    /* number of lines in the current window */

        /* search down the lines, updating them */
        lp = wp->w_linep;
        sline = wp->w_toprow;
        nlines = wp->w_ntrows;
        if (modeflag == FALSE)
                nlines++;
        taboff = wp->w_fcol;
        while (sline < wp->w_toprow + nlines) {

                /* and update the virtual line */
                vscreen[sline]->v_flag |= VFCHG;
                vscreen[sline]->v_left = FARRIGHT;
                vscreen[sline]->v_right = 0;
                vtmove(sline, -taboff);
                if (lp != wp->w_bufp->b_linep) {
                        /* if we are not at the end */
                        for (i=0; i < lused(lp); ++i)
                                vtputc(lgetc(lp, i));
                        lp = lforw(lp);
                }

                /* make sure we are on screen */
                if (vtcol < 0)
                        vtcol = 0;

                /* on to the next one */
                vscreen[sline]->v_rfcolor = wp->w_fcolor;
                vscreen[sline]->v_rbcolor = wp->w_bcolor;
                vteeol();
                ++sline;
        }
        taboff = 0;
}

void updpos()	/*      updpos: update the position of the hardware cursor and handle extended lines. This is the only update for simple moves.        */
{
        register LINE *lp;
        register int c;
        register int i;

        /* find the current row */
        lp = curwp->w_linep;
        currow = curwp->w_toprow;
        while (lp != curwp->w_dotp) {
                ++currow;
                lp = lforw(lp);
        }

        /* find the current column */
        curcol = 0;
        i = 0;
        while (i < curwp->w_doto) {
                c = lgetc(lp, i++);
                if (c == '\t')
                        curcol += - (curcol % tabsize) + (tabsize - 1);
                else {
                        if (disphigh && c > 0x7f) {
                                curcol += 2;
                                c -= 0x80;
                        }
                        if (c < 0x20 || c == 0x7f)
                                ++curcol;
                }
                if (c == ectoc(c))
                        ++curcol;
        }

        /* adjust by the current first column position */
        curcol -= curwp->w_fcol;

        /* make sure it is not off the left side of the screen */
        while (curcol < 0) {
                if (curwp->w_fcol >= hjump) {
                        curcol += hjump;
                        curwp->w_fcol -= hjump;
                } else {
                        curcol += curwp->w_fcol;
                        curwp->w_fcol = 0;
                }
                curwp->w_flag |= WFHARD | WFMODE;
        }

        /* if horizontall scrolling is enabled, shift if needed */
        if (hscroll) {
                while (curcol >= term.t_ncol - 1) {
                        curcol -= hjump;
                        curwp->w_fcol += hjump;
                        curwp->w_flag |= WFHARD | WFMODE;
                }
        } else {
        /* if extended, flag so and update the virtual line image */
                if (curcol >=  term.t_ncol - 1) {
                        vscreen[currow]->v_flag |= (VFEXT | VFCHG);
                        updext();
                } else
                        lbound = 0;
        }
}

void upddex()	/*      upddex: de-extend any line that derserves it            */
{
        register WINDOW *wp;
        register LINE *lp;
        register int i,j;
        register int nlines;    /* number of lines in the current window */

        wp = wheadp;

        while (wp != NULL) {
                lp = wp->w_linep;
                i = wp->w_toprow;
                nlines = wp->w_ntrows;
                if (modeflag == FALSE)
                        nlines++;

                while (i < wp->w_toprow + nlines) {
                        if (vscreen[i]->v_flag & VFEXT) {
                                if ((wp != curwp) || (lp != wp->w_dotp) ||
                                   (curcol < term.t_ncol - 1)) {
                                        taboff = wp->w_fcol;
                                        vtmove(i, -taboff);
                                        for (j = 0; j < lused(lp); ++j)
                                                vtputc(lgetc(lp, j));
                                        vteeol();
                                        taboff = 0;

                                        /* this line no longer is extended */
                                        vscreen[i]->v_flag &= ~VFEXT;
                                        vscreen[i]->v_flag |= VFCHG;
                                }
                        }
                        lp = lforw(lp);
                        ++i;
                }
                /* and onward to the next window */
                wp = wp->w_wndp;
        }
}

void updgar()	/*      updgar: if the screen is garbage, clear the physical screen and the virtual screen and force a full update              */
{
        register int i;
        register int j;
        register char *txt;

        for (i = 0; i < term.t_nrow; ++i) {
                vscreen[i]->v_flag |= VFCHG;
                vscreen[i]->v_fcolor = gfcolor;
                vscreen[i]->v_bcolor = gbcolor;
                pscreen[i]->v_left = FARRIGHT;
                pscreen[i]->v_right = 0;
                txt = pscreen[i]->v_text;
                for (j = 0; j < term.t_ncol; ++j)
                        txt[j] = ' ';
                pscreen[i]->v_flag &= ~VFNEW;
        }

        movecursor(0, 0);                /* Erase the screen. */
        TTeeop();
        sgarbf = FALSE;                  /* Erase-page clears */
        mpresf = FALSE;                  /* the message area. */
        mlerase();                      /* needs to be cleared if colored */
}

void update_size()	/*      for simple screen size changes (no window re-allocation involved) do the following things	*/
{
        /* if we need the size update */
        if ((first_screen->s_roworg != term.t_roworg) |
            (first_screen->s_colorg != term.t_colorg) |
            (first_screen->s_nrow != term.t_nrow) |
            (first_screen->s_ncol != term.t_ncol)) {

                /* reset the terminal drivers size concept */
                term.t_roworg = first_screen->s_roworg;
                term.t_colorg = first_screen->s_colorg;
                term.t_nrow = first_screen->s_nrow;
                term.t_ncol = first_screen->s_ncol;

                /* make sure the update routines know we need a full update */
                sgarbf = TRUE;
        }
}

/*      Display a pop up window.  Page it for the user.  Any key other
        than a space gets pushed back into the input stream to be interpeted
        later as a command.
*/

int pop( BUFFER *popbuf )
{
        register int index;     /* index into the current output line */
        register int llen;      /* length of the current output line */
        register int cline;     /* current screen line number */
        LINE *lp;       /* ptr to next line to display */
        int numlines;   /* remaining number of lines to display */
        int c;          /* input character */

        /* add the barrier line to the end of the pop up buffer */
        addline(popbuf, "------------------------------------------");

        /* set up to scan pop up buffer */
        lp = lforw(popbuf->b_linep);
        numlines = term.t_nrow-2;
        cline = 0;
	mmove_flag = FALSE;	/* disable mouse move events */

        while (lp != popbuf->b_linep) {

                /* update the virtual screen image for this one line */
                vtmove(cline, 0);
                llen = lused(lp);
                for (index = 0; index < llen; index++)
                        vtputc(lgetc(lp, index));
                vteeol();
                vscreen[cline]->v_rfcolor = gfcolor;
                vscreen[cline]->v_rbcolor = gbcolor;
                vscreen[cline]->v_left = FARRIGHT;
                vscreen[cline]->v_right = 0;
                vscreen[cline++]->v_flag |= VFCHG|VFCOL;

                if (numlines-- < 1) {

                        /* update the virtual screen to the physical screen */
                        updupd(FALSE);

                        /* tell the user there is more */
                        mlwrite("--- more ---");
                        TTflush();

                        /* and see if they want more */
                        if ((c = tgetc()) != ' ') {
                                cpending = TRUE;
                                charpending = c;
                                upwind();

				/* re-enable mouse move events */
				mmove_flag = TRUE;
                                return(TRUE);
                        }

                        /* reset the line counters */
                        numlines = term.t_nrow-2;
                        cline = 0;
                }

                /* on to the next line */
                lp = lforw(lp);
        }
        if (numlines > 0) {

                /* update the virtual screen to the physical screen */
                updupd(FALSE);
                TTflush();

                if ((c = tgetc()) != ' ') {
                        cpending = TRUE;
                        charpending = c;
                }
        }
        upwind();

	/* re-enable mouse move events */
	mmove_flag = TRUE;
        return(TRUE);
}

void updupd( int force )	/*      updupd: update the physical screen from the virtual screen      */
{
        register VIDEO *vp1;
        register int i;

        for (i = 0; i < term.t_nrow; ++i) {
                vp1 = vscreen[i];

                /* for each line that needs to be updated*/
                if (vp1->v_flag & VFCHG) {
                        if (force == FALSE && typahead())
                                return;
                        update_line(i, vp1, pscreen[i]);
                }
        }
        return;
}

/*      updext: update the extended line which the cursor is currently
                on at a column greater than the terminal width. The line
                will be scrolled right or left to let the user see where
                the cursor is
                                                                */
void updext()
{
        register int rcursor;   /* real cursor location */
        register LINE *lp;      /* pointer to current line */
        register int j;         /* index into line */

        /* calculate what column the real cursor will end up in */
        rcursor = ((curcol - term.t_ncol) % term.t_scrsiz)
                        + term.t_margin;
        lbound = curcol - rcursor + 1;
        taboff = lbound + curwp->w_fcol;

        /* scan through the line outputing characters to the virtual screen */
        /* once we reach the left edge                                  */
        vtmove(currow, -taboff); /* start scanning offscreen */
        lp = curwp->w_dotp;             /* line to output */
        for (j=0; j<lused(lp); ++j)     /* until the end-of-line */
                vtputc(lgetc(lp, j));

        /* truncate the virtual line, restore tab offset */
        vteeol();
        taboff = 0;

        /* and put a '$' in column 1 */
        vscreen[currow]->v_text[0] = '$';
}

/*
 * Update a single line. This does not know how to use insert or delete
 * character sequences; we are using VT52 functionality. Update the physical
 * row and column variables. It does try an exploit erase to end of line.
 */

void update_line( int row, struct VIDEO *vp, struct VIDEO *pp )
{

        register char *vir_left;        /* left pointer to virtual line */
        register char *phy_left;        /* left pointer to physical line */
        register char *vir_right;       /* right pointer to virtual line */
        register char *phy_right;       /* right pointer to physical line */
        int rev_left;                   /* leftmost reversed char index */
        int rev_right;                  /* rightmost reversed char index */
        char *left_blank;               /* left-most trailing blank */
        int non_blanks;                 /* non-blanks to the right flag */
        int update_column;              /* update column */
        int old_rev_state = FALSE;      /* reverse video states */
        int new_rev_state;

        /* set up pointers to virtual and physical lines */
        vir_left = &vp->v_text[0];
        vir_right = &vp->v_text[term.t_ncol];
        phy_left = &pp->v_text[0];
        phy_right = &pp->v_text[term.t_ncol];
        update_column = 0;
        rev_left = FARRIGHT;
        rev_right = 0;
	non_blanks = TRUE;

        /* if this is a legitimate line to optimize */
        if (!(pp->v_flag & VFNEW)) {

                /* advance past any common chars at the left */
                while ((vir_left != &vp->v_text[term.t_ncol])
                       && (vir_left[0] == phy_left[0])) {
                        ++vir_left;
                        ++update_column;
                        ++phy_left;
                }

                /* advance past any common chars at the right */
                non_blanks = FALSE;
                while ((vir_right[-1] == phy_right[-1]) &&
                       (vir_right >= vir_left)) {
                        --vir_right;
                        --phy_right;

                        /* Note if any nonblank in right match. */
                        if (vir_right[0] != ' ')
                                non_blanks = TRUE;
                }
        }

        /* new line color? */
        if (((vp->v_rfcolor != vp->v_fcolor) ||
            (vp->v_rbcolor != vp->v_bcolor))) {
                vp->v_fcolor = vp->v_rfcolor;
                vp->v_bcolor = vp->v_rbcolor;
                vp->v_flag &= ~VFCOL;
                vir_left = &vp->v_text[0];
                vir_right = &vp->v_text[term.t_ncol];
                phy_left = &pp->v_text[0];
                phy_right = &pp->v_text[term.t_ncol];
                update_column = 0;
        }

        TTforg(vp->v_fcolor);
        TTbacg(vp->v_bcolor);

        /* reverse video changes? */
        if ((vp->v_left != pp->v_left) || (vp->v_right != pp->v_right)) {

                /* adjust leftmost edge */
                if (vp->v_left < pp->v_left)
                        rev_left = vp->v_left;
                else
                        rev_left = pp->v_left;
                pp->v_left = vp->v_left;
                if (rev_left < update_column) {
                        vir_left = &vp->v_text[rev_left];
                        phy_left = &pp->v_text[rev_left];
                        update_column = rev_left;
                }

                /* adjust rightmost edge */
                if (vp->v_right > pp->v_right)
                        rev_right = vp->v_right;
                else
                        rev_right = pp->v_right;
                pp->v_right = vp->v_right;
                if (&vp->v_text[rev_right] > vir_right) {
                        vir_right = &vp->v_text[rev_right];
                        phy_right = &pp->v_text[rev_right];
                }
        } else {
                rev_left = vp->v_left;
                rev_right = vp->v_right;
        }

        /* if both lines are the same, no update needs to be done */
        if (!(pp->v_flag & VFNEW) && (vir_left > vir_right)) {
                vp->v_flag &= ~VFCHG;   /* flag this line as changed */
                return;
        }

        left_blank = vir_right;

        /* Erase to EOL ? */
        if (non_blanks == FALSE && eolexist == TRUE) {
                while (left_blank!=vir_left && left_blank[-1]==' ')
                        --left_blank;

                if (vir_right-left_blank <= 3)          /* Use only if erase is */
                        left_blank = vir_right;         /* fewer characters. */
        }

        /* move to the beginning of the text to update */
        movecursor(row, update_column);

        while (vir_left != left_blank) {                /* Ordinary. */

                /* are we in a reverse video field? */
                if (pp->v_left <= update_column && update_column < pp->v_right)
                        new_rev_state = TRUE;
                else
                        new_rev_state = FALSE;

                /* if moving in or out of rev video, change it */
                if (new_rev_state != old_rev_state) {
                        TTrev(new_rev_state);
                        old_rev_state = new_rev_state;
		}

		/* output the next character! */
		TTputc(*vir_left);
		++ttcol;
		++update_column;
		*phy_left++ = *vir_left++;
	}

	if (left_blank != vir_right) {		/* Erase. */

                /* are we in a reverse video field? */
                if (pp->v_left <= update_column && update_column < pp->v_right)
                        new_rev_state = TRUE;
                else
                        new_rev_state = FALSE;

                /* if moving in or out of rev video, change it */
                if (new_rev_state != old_rev_state) {
                        TTrev(new_rev_state);
                        old_rev_state = new_rev_state;
                }

	/* TERMCAP does not tell us if the current terminal propagates
	   the current attributes to the end of the line when an erase
	   to end of line sequence is sent. Don't let TERMCAP use EEOL
	   if in a reverse video line!  (ARG...Pain....Agony....)      */
		if (new_rev_state == TRUE)
			while (update_column++ < term.t_ncol)
				TTputc(' ');
		else
			TTeeol();
		while (vir_left != vir_right)
			*phy_left++ = *vir_left++;
	}

	vp->v_flag &= ~VFCHG;		/* flag this line as updated */
	vp->v_flag &= ~VFCOL;

	/* Always leave in the default state */
	if (old_rev_state == TRUE)
		TTrev(FALSE);
	return;
}

/*
 * Redisplay the mode line for the window pointed to by the "wp". This is the
 * only routine that has any idea of how the modeline is formatted. You can
 * change the modeline format by hacking at this routine. Called by "update"
 * any time there is a dirty window.
 */

void modeline( WINDOW *wp )
{
	register char *cp;
	register unsigned char c;
	register int n;		/* cursor position count */
	register BUFFER *bp;
	register int i;		/* loop index */
	register int lchar; 	/* character to draw line in buffer with */
	register int firstm;	/* is this the first mode? */
	char tline[NLINE];	/* buffer for part of mode line */
	char time[6];		/* to hold current time */

	/* don't bother if there is none! */
	if (modeflag == FALSE)
		return;

	n = wp->w_toprow+wp->w_ntrows;		/* Location. */

/*
	Note that we assume that setting REVERSE will cause the terminal
	driver to draw with the inverted relationship of fcolor and
	bcolor, so that when we say to set the foreground color to "white"
	and background color to "black", the fact that "reverse" is
	enabled means that the terminal driver actually draws "black" on a
	background of "white".  Makes sense, no?  This way, devices for
	which the color controls are optional will still get the "reverse"
	signals.
*/

	vscreen[n]->v_flag |= VFCHG | VFCOL;	/* Redraw next time. */
	vscreen[n]->v_left = 0;
	vscreen[n]->v_right = term.t_ncol;
	vscreen[n]->v_rfcolor = 7;			/* black on */
	vscreen[n]->v_rbcolor = 0;			/* white.....*/
	vtmove(n, 0);				/* Seek to right line. */
	if (wp == curwp)			/* mark the current buffer */
		lchar = '=';
	else
	if (revexist)
		lchar = ' ';
	else
		lchar = '-';

	bp = wp->w_bufp;
	if ((bp->b_flag&BFTRUNC) != 0)		/* "#" if truncated */
		vtputc('#');
	else
		vtputc(lchar);

	if ((bp->b_flag&BFCHG) != 0)		/* "*" if changed. */
		vtputc('*');
	else
		vtputc(lchar);

	if ((bp->b_flag&BFNAROW) != 0) {		/* "<>" if narrowed */
		vtputc('<');
		vtputc('>');
	} else {
		vtputc(lchar);
		vtputc(lchar);
	}

	n  = 4;
	strcpy(tline, " "); 			/* Buffer name. */
	strcat(tline, PROGNAME);
	strcat(tline, " ");
	strcat(tline, VERSION);
	strcat(tline, " ");

	/* display the time on the bottom most modeline if active */
	if (timeflag && wp->w_wndp == (WINDOW *)NULL) {

		/* get the current time/date string */
		getdtime(time);
		if (strcmp(time, "") != 0) {

			/* append the hour/min string */
			strcat(tline, "[");
			strcat(tline, time);
			strcat(tline, "] ");
			strcpy(lasttime, time);
		}
	}

	/* are we horizontally scrolled? */
	if (wp->w_fcol > 0) {
		strcat(tline, "[<");
		strcat(tline, int_asc(wp->w_fcol));
		strcat(tline, "]");
	}

	/* display the point position in buffer if on current modeline */
	if (posflag && wp == curwp) {

 		strcat(tline, "L:");
	 	strcat(tline, long_asc(getlinenum(bp, wp->w_dotp)));
 		strcat(tline, " C:");
	 	strcat(tline, int_asc(1+getccol(FALSE)));
 		strcat(tline, " ");
	}

	/* display the modes */
	strcat(tline, "(");
	firstm = TRUE;
	for (i = 0; i < NUMMODES; i++)	/* add in the mode flags */
		if (wp->w_bufp->b_mode & (1 << i)) {
			if (firstm != TRUE)
				strcat(tline, " ");
			firstm = FALSE;
			strcat(tline, modename[i]);
		}
	strcat(tline,") ");

	cp = &tline[0];
	while ((c = *cp++) != 0) {
		vtputc(c);
		++n;
	}

	vtputc(lchar);
	vtputc(lchar);
	vtputc(' ');
	n += 3;
	cp = &bp->b_bname[0];

	while ((c = *cp++) != 0) {
		vtputc(c);
		++n;
	}

	vtputc(' ');
	vtputc(lchar);
	vtputc(lchar);
	n += 3;

	if (bp->b_fname[0] != 0) {	/* File name. */
		vtputc(' ');
		++n;
		cp = TEXT34;
/*                   "File: " */

		while ((c = *cp++) != 0) {
			vtputc(c);
			++n;
		}

		cp = &bp->b_fname[0];

		while ((c = *cp++) != 0) {
			vtputc(c);
			++n;
	        }

		vtputc(' ');
		++n;
	}

	while (n < term.t_ncol) {	/* Pad to full width. */
		vtputc(lchar);
		++n;
	}
}

void  getdtime( char *ts )	/* get the current display time string */
{
	char buf[80];

	strcpy(buf, timeset());
	if (strcmp(buf, errorm) == 0) {
		*ts = 0;
		return;
	}

	buf[16] = 0;
	strcpy(ts, &buf[11]);
	return;
}

void upmode()	/* update all the mode lines */
{
	register WINDOW *wp;

	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}
}

void  upwind()	/* force hard updates on all windows */
{
	register WINDOW *wp;

	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFHARD|WFMODE;
		wp = wp->w_wndp;
	}
}

/*
 * Send a command to the terminal to move the hardware cursor to row "row"
 * and column "col". The row and column arguments are origin 0. Optimize out
 * random calls. Update "ttrow" and "ttcol".
 */

void movecursor( int row, int col )
{
	if (row != ttrow || col != ttcol) {		/* only move it if there is a difference */
		ttrow = row;
		ttcol = col;
		TTmove(row, col);
	}
}

/*
 * Erase the message line. This is a special routine because the message line
 * is not considered to be part of the virtual screen. It always works
 * immediately; the terminal buffer is flushed via a call to the flusher.
 */

void mlferase()
{
	register int save_discmd;

	save_discmd = discmd;
	discmd = TRUE;
	mlerase();
	discmd = save_discmd;;
}

void mlerase()
{
	int i;
    
	movecursor(term.t_nrow, 0);
	if (discmd == FALSE)
		return;

	TTforg(gfcolor);
	TTbacg(gbcolor);

	if (eolexist == TRUE)
		TTeeol();
	else {
		for (i = 0; i < term.t_ncol - 1; i++)
			TTputc(' ');

		/* force the move! */
/*		movecursor(term.t_nrow, 1);*/
		movecursor(term.t_nrow, 0);
	}
	TTflush();
	mpresf = FALSE;
}

/*
 * Write a message into the message line. Keep track of the physical cursor
 * position. A small class of printf like format items is handled. Assumes the
 * stack grows down; this assumption is made by the "+=" in the argument scan
 * loop. If stack grows up use "-=" instead of "+=". Set the "message line"
 * flag TRUE.  Don't write beyond the end of the current terminal width.
 */

void mlout( int c )
{
	if (ttcol + 1 < term.t_ncol)
		TTputc(c);
	if (c != '\b')
		*lastptr++ = c;
	else if (lastptr > &lastmesg[0])
		--lastptr;
}

void mlwrite(char *fmt, ...)
/* char * fmt;*/

		/* variable argument list
			arg1 = format string
			arg2+ = arguments in that string */

{
	register int c; 	/* current char in format string */
	va_list ap;		/* ptr to current data field */
	int arg_int;		/* integer argument */
	long arg_long;		/* long argument */
	char *arg_str;		/* string argument */

	/* if we are not currently echoing on the command line, abort this */
	if (discmd == FALSE)
		return;

	/* set up the proper colors for the command line */
	TTforg(gfcolor);
	TTbacg(gbcolor);

	/* point to the first argument */
	va_start(ap, fmt);

	/* if we can not erase to end-of-line, do it manually */
	if (eolexist == FALSE) {
		mlerase();
		TTflush();
	}

	movecursor(term.t_nrow, 0);
 	lastptr = &lastmesg[0];		/* setup to record message */
	while ((c = *fmt++) != 0) {
		if (c != '%') {
			mlout(c);
			++ttcol;
		} else {
			c = *fmt++;
			switch (c) {
				case 'd':
					arg_int = va_arg(ap, int);
					mlputi(arg_int, 10);
					break;

				case 'o':
					arg_int = va_arg(ap, int);
					mlputi(arg_int, 8);
					break;

				case 'x':
					arg_int = va_arg(ap, int);
					mlputi(arg_int, 16);
					break;

				case 'D':
					arg_long = va_arg(ap, long);
					mlputli(arg_long, 10);
					break;

				case 's':
					arg_str = va_arg(ap, char *);
					mlputs(arg_str);
					break;

				case 'f':
					arg_int = va_arg(ap, int);
					mlputf(arg_int);
					break;

				default:
					mlout(c);
					++ttcol;
			}
		}
	}

	/* if we can, erase to the end of screen */
	if (eolexist == TRUE)
		TTeeol();
	TTflush();
	mpresf = TRUE;
	*lastptr = 0;	/* terminate lastmesg[] */
	va_end(ap);
}

/*	Force a string out to the message line regardless of the
	current $discmd setting. This is needed when $debug is TRUE
	and for the write-message and clear-message-line commands
*/

void mlforce( char *s )
{
	register int oldcmd;	/* original command display flag */

	oldcmd = discmd;	/* save the discmd value */
	discmd = TRUE;		/* and turn display on */
	mlwrite(s);		/* write the string out */
	discmd = oldcmd;	/* and restore the original setting */
}

/* display a serious error message (like "out of memory"). This is
   replaced by a system-specific function when a multitasking system
   that does not like these kind of errors is used, so that the user can
   be offered to abort the application */

void mlabort( char *s )
{
    mlforce(s);
}

/*
 * Write out a string. Update the physical cursor position. This assumes that
 * the characters in the string all have width "1"; if this is not the case
 * things will get screwed up a little.
 */

void mlputs( char *s )
{
	register int c;

	while ((c = *s++) != 0) {
		mlout(c);
		++ttcol;
	}
}

void mlputi( int i, int r )	/* Write out an integer, in the specified radix. Update the physical cursor position. */
{
    register int q;
    static char hexdigits[] = "0123456789ABCDEF";

    if (i < 0)
	{
	i = -i;
	mlout('-');
	}

    q = i/r;

    if (q != 0)
	mlputi(q, r);

    mlout(hexdigits[i%r]);
    ++ttcol;
}

void mlputli( long l, int r )
{
    register long q;

    if (l < 0)
	{
	l = -l;
	mlout('-');
	}

    q = l/r;

    if (q != 0)
	mlputli(q, r);

    mlout((int)(l%r)+'0');
    ++ttcol;
}

void mlputf( int s )	/* write out a scaled integer with two decimal places */
{
	int i;	/* integer portion of number */
	int f;	/* fractional portion of number */

	/* break it up */
	i = s / 100;
	f = s % 100;

	/* send out the integer portion */
	mlputi(i, 10);
	mlout('.');
	mlout((f / 10) + '0');
	mlout((f % 10) + '0');
	ttcol += 3;
}
