/* screen.c
 *
 * Screen manipulation commands
 *
 * written by Daniel Lawrence
 *
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"

void refresh_screen( SCREEN *sp )		/* Redraw given screen and all screens behind it */
{
	if (gflags & GFSDRAW)			/* if we are suppressing redraws */
		return;

	if (sp == (SCREEN *)NULL)		/* at end of list, do nothing */
		return;

	if (sp == first_screen) {		/* if first refresh, erase the page */
		(*term.t_clrdesk)();
		if (sp->s_next_screen == (SCREEN *)NULL)
			sgarbf = TRUE;
	}

	if (sp->s_next_screen)			/* if there are others below, defer to them first */
		refresh_screen(sp->s_next_screen);

	select_screen(sp, FALSE);
	update(TRUE);
}

int cycle_screens( int f, int n )	/* This command takes the last window in the linked window list, which is visibly rearmost, and brings it to front.	*/
{
	SCREEN *sp;					/* ptr to screen to switch to */

	sp = first_screen;				/* find the last screen */
	while (sp->s_next_screen != (SCREEN *)NULL)
		sp = sp->s_next_screen;

	return(select_screen(sp, TRUE));		/* and make this screen current */
}

int find_screen( int f, int n )
{
	char scr_name[NSTRING];	/* buffer to hold screen name */
	SCREEN *sp;		/* ptr to screen to switch to */
	int result;

	if ((result = mlreply(TEXT242, scr_name, NSTRING)) != TRUE) {	    /* "Find Screen: " */
            return result;
        }
	sp = lookup_screen(scr_name);

	if (sp == (SCREEN *)NULL) {		/* save the current dot position in the buffer info so the new screen will start there! */
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		sp = init_screen(scr_name, curbp);	/* screen does not exist, create it */
	}

	return(select_screen(sp, TRUE));		/* and make this screen current */
}

int free_screen( SCREEN *sp )	/* free all resouces associated with a screen */
{
	register int cmark;	/* mark ordinal index */
	register WINDOW *wp;	/* ptr to window to free */
	register WINDOW *tp;	/* temp window pointer */

	wp = sp->s_first_window;			/* first, free the screen's windows */
	while (wp) {
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp  = wp->w_dotp;
			wp->w_bufp->b_doto  = wp->w_doto;
			for (cmark = 0; cmark < NMARKS; cmark++) {
				wp->w_bufp->b_markp[cmark] = wp->w_markp[cmark];
				wp->w_bufp->b_marko[cmark] = wp->w_marko[cmark];
			}
			wp->w_bufp->b_fcol  = wp->w_fcol;
		}
		tp = wp->w_wndp;			/* on to the next window, free this one */
		free((char *) wp);
		wp = tp;
	}
	free(sp->s_screen_name);			/* and now, free the screen struct itself */
	free((char *) sp);
	return(0);
}

void unlist_screen( SCREEN *sp )
{
	SCREEN *last_scr;	/* screen previous to one to delete */

	last_scr = first_screen;
	while (last_scr) {
		if (last_scr->s_next_screen == sp)
			break;
		last_scr = last_scr->s_next_screen;
	}
	last_scr->s_next_screen = sp->s_next_screen;
}

int delete_screen( int f, int n )
{
	char scr_name[NSTRING];	/* buffer to hold screen name */
	SCREEN *sp;		/* ptr to screen to switch to */
	int result;

	if ((result = mlreply(TEXT243, scr_name, NSTRING)) != TRUE) {		/* "Delete Screen: " */	/* get the name of the screen to delete */
            return result;
	}
	sp = lookup_screen(scr_name);

	if (sp == (SCREEN *)NULL) {						/* make sure it exists... */
		mlwrite(TEXT240);   						/* "[No such screen]" */
		return(FALSE);
	}

	if (sp == first_screen) {						/* it can't be current... */
		mlwrite(TEXT241);						/* "%%Can't delete current screen" */
		return(FALSE);
	}

	unlist_screen(sp);
	free_screen(sp);
	refresh_screen(first_screen);
	return(TRUE);
}

/* this function initializes a new screen.... */

SCREEN * init_screen( char *scr_name, BUFFER *scr_buf)
{
	int cmark;		/* current mark to initialize */
	SCREEN *sp;		/* pointer to allocated screen */
	SCREEN *last_sp;	/* pointer to last screen */
	WINDOW *wp;		/* ptr to first window of new screen */

	sp = (SCREEN *)malloc(sizeof(SCREEN));		/* allocate memory for this screen */
	if (sp == (SCREEN *)NULL)
		return(sp);

	sp->s_next_screen = (SCREEN *)NULL;		/* set up this new screens fields! */
	sp->s_screen_name = copystr(scr_name);
	sp->s_roworg = term.t_roworg;
	sp->s_colorg = term.t_colorg;
	sp->s_nrow = term.t_nrow;
	sp->s_ncol = term.t_ncol;

	wp = (WINDOW *)malloc(sizeof(WINDOW));		/* allocate its first window */
	if (wp == (WINDOW *)NULL) {
		free((char *)sp);
		return((SCREEN *)NULL);
	}
	sp->s_first_window = sp->s_cur_window = wp;

	wp->w_wndp = NULL;				/* and setup the windows info */
	wp->w_bufp = scr_buf;
	scr_buf->b_nwnd += 1;	
	wp->w_linep = scr_buf->b_linep;

	wp->w_dotp  = scr_buf->b_dotp;			/* position us at the buffers dot */
	wp->w_doto  = scr_buf->b_doto;

	for (cmark = 0; cmark < NMARKS; cmark++) {	/* set all the marks to UNSET */
		wp->w_markp[cmark] = NULL;
		wp->w_marko[cmark] = 0;
	}
	wp->w_toprow = 0;
	wp->w_fcolor = gfcolor;				/* initalize colors to global defaults */
	wp->w_bcolor = gbcolor;
	wp->w_fcol = 0;
	wp->w_ntrows = term.t_nrow-1;			/* "-1" for mode line.	*/
	wp->w_force = 0;
	wp->w_flag  = WFMODE|WFHARD;			/* Full.		*/

	if (first_screen == (SCREEN *)NULL) {		/* first screen? */
		first_screen = sp;
		return(sp);
	}

	last_sp = first_screen;				/* insert it at the tail of the screen list */
	while (last_sp->s_next_screen != (SCREEN *)NULL)
		last_sp = last_sp->s_next_screen;
	last_sp->s_next_screen = sp;

	return(sp);					/* and return the new screen pointer */
}

SCREEN * lookup_screen( char *scr_name )
{
	SCREEN *result;

	result = first_screen;				/* scan the screen list */
	while (result) {
		if (strcmp(scr_name, result->s_screen_name) == 0)	/* if this is it, return its handle! */
			return(result);
		result = result->s_next_screen;		/* on to the next screen */
	}
	return((SCREEN *)NULL);				/* we didn't find it..... */
}

int  select_screen( SCREEN *sp, int announce )
{
	WINDOW *temp_wp;	/* backup of current window ptr (curwp) */
	SCREEN *temp_screen;	/* temp ptr into screen list */

	if (sp == (SCREEN *)NULL)			/* make sure there is something here to set to! */
		return(FALSE);

	if (sp == first_screen)				/* nothing to do, it is already current */
		return(TRUE);

	temp_wp = curwp;				/* deselect the current window */
	curwp = (WINDOW *)NULL;
	modeline(temp_wp);
	updupd(TRUE);
	curwp = temp_wp;

	first_screen->s_cur_window = curwp;		/* save the current screens concept of current window */
	first_screen->s_roworg = term.t_roworg;
	first_screen->s_colorg = term.t_colorg;
	first_screen->s_nrow = term.t_nrow;
	first_screen->s_ncol = term.t_ncol;

	temp_screen = first_screen;			/* re-order the screen list */
	while (temp_screen->s_next_screen != (SCREEN *)NULL) {
		if (temp_screen->s_next_screen == sp) {
			temp_screen->s_next_screen = sp->s_next_screen;
			break;
		}
		temp_screen = temp_screen->s_next_screen;
	}
	sp->s_next_screen = first_screen;
	first_screen = sp;
	wheadp = first_screen->s_first_window;		/* reset the current screen, window and buffer */
	curwp = first_screen->s_cur_window;
	curbp = curwp->w_bufp;
	update_size();					/* let the display driver know we need a full screen update */
	upwind();
	if (announce) {
		mlwrite(TEXT225, first_screen->s_screen_name);		/* "[Switched to screen %s]" */
	}
	return(TRUE);
}

int list_screens( int f, int n )		/* Build and popup a buffer containing the list of all screens. */
{
	register int status;	/* stutus return */

	if ((status = screenlist(f)) != TRUE)
		return(status);
	return(wpopup(slistp));
}

/*
 * This routine rebuilds the
 * text in the special secret buffer
 * that holds the screen list. It is called
 * by the list screens command. Return TRUE
 * if everything works. Return FALSE if there
 * is an error (if there is no memory). Iflag
 * indecates weather to list hidden screens.
 */

int screenlist( int iflag )
{
	SCREEN *sp;		/* ptr to current screen to list */
	WINDOW *wp;		/* ptr into current screens window list */
	int status;		/* return status from functions */
	char line[NSTRING];	/* buffer to construct list lines */
	char bname[NSTRING];	/* name of next buffer */

	slistp->b_flag &= ~BFCHG;		/* mark this buffer as unchanged so... */

	if ((status = bclear(slistp)) != TRUE)		/* we can dump it's old contents without complaint */
		return(status);

	strcpy(slistp->b_fname, "");		/* there is no file connected with this buffer */

	if (addline(slistp, "Screen         Buffers") == FALSE		/* construct the header of this list */
	 || addline(slistp, "------         -------") == FALSE)
		return(FALSE);

	sp = first_screen;						/* starting from the first screen */
	while (sp) {							/* scan all the screens */
		strcpy(line, sp->s_screen_name);			/* construct the screen name */
		strcat(line, "                ");
		line[15] = 0;
		wp = sp->s_first_window;				/* list this screens windows's buffer names */
		while (wp) {
			strcpy(bname, wp->w_bufp->b_bname);		/* grab this window's buffer name */
			if (strlen(line) + strlen(bname) + 1 > 78) {	/* handle full lines */
				if (addline(slistp, line) == FALSE)
					return(FALSE);
				strcpy(line, "               ");
			}
			if (strlen(line) > 15)				/* append this buffer name */
				strcat(line, " ");
			strcat(line, bname);
			wp = wp->w_wndp;				/* on to the next window */
		}
		if (addline(slistp, line) == FALSE)			/* and add the line to the buffer */
			return(FALSE);
		sp = sp->s_next_screen;					/* on to the next screen */
	}
	return(TRUE);							/* all constructed! */
}

int  rename_screen( int f, int n )		/* rename_screen:    change the current screen's name	*/
{
	char scr_name[NSTRING];  /* buffer to hold screen name */
	int result;

	if ((result = mlreply(TEXT335, scr_name, NSTRING)) != TRUE) {	/* "Change screen name to: " */
		return(result);
	} 

	if (lookup_screen(scr_name) != (SCREEN*)NULL) {
		mlwrite(TEXT336);					/* "[Screen name already in use]" */
		return(FALSE);
	}

	free(first_screen->s_screen_name);				/* replace the old screen name with the new */
	first_screen->s_screen_name = copystr(scr_name);
	return(TRUE);
}
