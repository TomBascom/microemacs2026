/* input.c
 *
 * Various input routines for MicroEMACS
 *	written by Daniel Lawrence
 *	copyright 1986-1993 by Daniel M. Lawrence
 *
 *	Notes:
 *
 *	MicroEMACS's kernel processes two distinct forms of
 *	characters.  One of these is a standard unsigned character
 *	which is used in the edited text.  The other form, called
 *	an EMACS Extended Character is a 2 byte value which contains
 *	both an ascii value, and flags for certain prefixes/events.
 *
 *	Bit	Usage
 *	---	-----
 *	0 -> 7	Standard 8 bit ascii character
 *	8	Control key flag
 *	9	META prefix flag
 *	10	^X prefix flag
 *	11	Function key flag
 *	12	Mouse prefix
 *	13	Shifted flag (not needed on alpha shifted characters)
 *	14	Alterate prefix (ALT key on PCs)
 *
 *	The machine dependent driver is responsible for returning
 *	a byte stream from the various input devices with various
 *	prefixes/events embedded as escape codes.  Zero is used as the
 *	value indicating an escape sequence is next.  The format of
 *	an escape sequence is as follows:
 *
 *	0		Escape indicator
 *	<prefix byte>	upper byte of extended character
 *	{<col><row>}	col, row position if the prefix byte
 *			indicated a mouse event or a menu selection
 *			in which case these form a 16 bit menu ID
 *	<event code>	value of event
 *
 *	A ^<space> sequence (0/1/32) is generated when an actual
 *	null is being input from the control-space key under many
 *	unix systems.  These values are then interpreted by getkey()
 *	to construct the proper extended character sequences to pass
 *	to the MicroEMACS kernel.
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"
#include <pwd.h>

int mlyesno( char *prompt )
{
	int  c;			/* input character */
	char buf[NPAT];		/* prompt to user */

	for (;;) {
		/* build and prompt the user */
		strcpy(buf, prompt);
		strcat(buf, TEXT162);	/* " [y/n]? " */
		mlwrite(buf);

		/* get the response */
	        c = getcmd();   /* getcmd() lets us check for anything that might */
	                        /* generate a 'y' or 'Y' in case use screws up */

		if (c == ectoc(abortc))		/* Bail out! */
			return(ABORT);

	        if  ((c == 'n') || (c == 'N')
        	    || (c & (SPEC|ALTD|CTRL|META|CTLX|MOUS)))
                	return(FALSE);  /* ONLY 'y' or 'Y' allowed!!! */

		if (c=='y' || c=='Y')
			return(TRUE);

		return(FALSE);
	}
}

/*
 * Write a prompt into the message line, then read back a response. Keep
 * track of the physical position of the cursor. If we are in a keyboard
 * macro throw the prompt away, and return the remembered response. This
 * lets macros run at full speed. The reply is always terminated by a carriage
 * return. Handle erase, kill, and abort keys.
 */

int mlreply( char *prompt, char *buf, int nbuf )
{
	return(nextarg(prompt, buf, nbuf, ctoec((int) '\r')));
}

/*	ectoc:	expanded character to character
 *		collapse the CTRL and SPEC flags back into an ascii code   */

int ectoc( int c )
{
	if (c == (CTRL | ' '))
		c = 0;
	if (c & CTRL)
		c = c ^ (CTRL | 0x40);
	if (c & SPEC)
		c = c & 255;
	return(c);
}

/*	ctoec:	character to extended character
 *		pull out the CTRL and SPEC prefixes (if possible)	*/

int ctoec( int c )
{
	if (c == 0)
		c = CTRL | ' ';
	else if ((c >= 0x00 && c <= 0x1F) || c == 0x7F)
                c = CTRL | (c ^ 0x40);
        return(c);
}

/* get a command name from the command line. Command completion means
   that pressing a <SPACE> will attempt to complete an unfinished command
   name if it is unique.
*/

int ( * getname( char *prompt ))(int, int)
{
	char *sp;	/* ptr to the returned string */

	sp = complete(prompt, NULL, CMP_COMMAND, NSTRING);
	if (sp == NULL)
		return(NULL);

	return(fncmatch(sp));
}

/*	getcbuf:	get a completion from the user for a buffer name.

			I was goaded into this by lots of other people's
			completion code.
*/

BUFFER * getcbuf( char *prompt, char *defval, int createflag )
{
	char *sp;	/* ptr to the returned string */

	sp = complete(prompt, defval, CMP_BUFFER, NBUFN);
	if (sp == NULL)
		return(NULL);

	return(bfind(sp, createflag, 0));
}

char * gtfilename( char *prompt )
{
	char *sp;	/* ptr to the returned string */

	/* get a file name, default to current buffer's */
	if (curbp && strcmp(curbp->b_fname, "") != 0)
		sp = complete(prompt, curbp->b_fname, CMP_FILENAME, NFILEN);
	else
		sp = complete(prompt, NULL, CMP_FILENAME, NFILEN);
	return(sp);
}

char * complete( char *prompt, char *defval, int type, int maxlen )
{
	register int c;		/* current input character */
	register int ec;	/* extended input character */
	int cpos;		/* current column on screen output */
	char *home_ptr;		/* pointer to home directory string */
	char *ptr;		/* string pointer */
	char user_name[NSTRING]; /* user name for directory */
	static char buf[NSTRING];/* buffer to hold tentative name */
	struct passwd *pwd;	/* password structure */

	/* if we are executing a command line get the next arg and match it */
	if (clexec) {
		if (macarg(buf) != TRUE)
			return(NULL);
		return(buf);
	}

	/* starting at the beginning of the string buffer */
	cpos = 0;

	/* if it exists, prompt the user for a buffer name */
	if (prompt) {
		if (type == CMP_COMMAND)
			mlwrite("%s", prompt);
		else if (defval)
			mlwrite("%s[%s]: ", prompt, defval);
		else
			mlwrite("%s: ", prompt);
	}

	/* build a name string from the keyboard */
	while (TRUE) {

		/* get the keystroke and decode it */
        if (curbp->b_bname[0] == '?' ) {
           if ((curbp->b_bname[1] == ' ' ) &&
              (curbp->b_bname[strlen(curbp->b_bname)-1] == '/')) {
              strcpy(buf,curbp->b_fname);
              cpos = strlen(curbp->b_fname);
              curbp->b_bname[0] = ':';
              ec = ' ';
	   }
        } else {
		ec = getkey();
	}
		c = ectoc(ec);

		/* if it is from the mouse, or is a function key, blow it off */
		if ((ec & MOUS) || (ec & SPEC))
			continue;

		/* if we are at the end, just match it */
		if (c == '\n'  ||  c == '\r') {
			if (defval && cpos==0)
				return(defval);
			else {
				buf[cpos] = 0;
				return(buf);
			}

		} else if (ec == abortc) {	/* Bell, abort */
			ctrlg(FALSE, 0);
			TTflush();
			return(NULL);

		} else if (c == 0x7F || c == 0x08) {	/* rubout/erase */
			if (cpos != 0) {
				mlout('\b');
				mlout(' ');
				mlout('\b');
				--ttcol;
				--cpos;
				TTflush();
			}

		} else if (c == 0x15) {	/* C-U, kill */
			while (cpos != 0) {
				mlout('\b');
				mlout(' ');
				mlout('\b');
				--cpos;
				--ttcol;
			}
			TTflush();

		} else if ((c == ' ') || (ec == sterm) || (c == '\t')) {	
			/* attempt a completion */
			switch (type) {
				case CMP_BUFFER:
					comp_buffer(buf, &cpos);
					break;
				case CMP_COMMAND:
					comp_command(buf, &cpos);
					break;
				case CMP_FILENAME:
					comp_file(buf, &cpos);
					break;
			}

			TTflush();
			if (buf[cpos - 1] == 0)
				return(buf);
			goto clist;

		} else if ((cpos > 0) &&
			   ((char)c == DIRSEPCHAR) &&
			   (type == CMP_FILENAME) &&
			   (buf[0] == '~') &&
			   ((home_ptr = getenv("HOME")) != (char *)NULL)) {

			/* save the user name! */
			buf[cpos] = 0;
			strcpy(user_name, &buf[1]);

			/* erase the chars on-screen */
			while (cpos > 0) {
				mlout('\b');
				mlout(' ');
				mlout('\b');
				--cpos;
				--ttcol;
			}

			/* lookup someone else's home directory! */
			if (user_name[0] != 0) {
				pwd = getpwnam(user_name);
				if (pwd != (struct passwd *)NULL) {
					ptr = pwd->pw_dir;
					while (*ptr) {
						mlout(*ptr);
						buf[cpos++] = *ptr++;
						++ttcol;
					}
				}
			}
			if (cpos == 0) {

				/* output the home directory */
				ptr = home_ptr;
				while (*ptr) {
					mlout(*ptr);
					buf[cpos++] = *ptr++;
					++ttcol;
				}

				/* is this someone else's home directory */
				if (user_name[0] != 0) {

					/* backup to the last directory sep */
					while ((cpos > 0) &&
					       (buf[cpos-1] != DIRSEPCHAR)) {
						mlout('\b');
						mlout(' ');
						mlout('\b');
						--cpos;
						--ttcol;
					}

					/* and add the user's name */
					ptr = user_name;
					while (*ptr) {
						mlout(*ptr);
						buf[cpos++] = *ptr++;
						++ttcol;
					}
				}

			}

			/* and the last directory seperator */
			if (buf[cpos-1] != DIRSEPCHAR) {
				mlout(DIRSEPCHAR);
				buf[cpos++] = DIRSEPCHAR;
				++ttcol;
			}
			TTflush();

		} else if ((cpos > 1) &&
			   ((char)c == DIRSEPCHAR) &&
			   (type == CMP_FILENAME) &&
			   (buf[0] == '$')) {

			/* expand an environment variable reference */
			/* save the variable name! */
			buf[cpos] = 0;
			strcpy(user_name, &buf[1]);

			/* erase the chars on-screen */
			while (cpos > 0) {
				mlout('\b');
				mlout(' ');
				mlout('\b');
				--cpos;
				--ttcol;
			}

			ptr = getenv(user_name);
			if (ptr != (char *)NULL) {
				while (*ptr) {
					mlout(*ptr);
					buf[cpos++] = *ptr++;
					++ttcol;
				}
			}

			/* and the last directory seperator */
			if (buf[cpos-1] != DIRSEPCHAR) {
				mlout(DIRSEPCHAR);
				buf[cpos++] = DIRSEPCHAR;
				++ttcol;
			}
			TTflush();

		} else if (c == '?') {

clist:			/* make a completion list! */
			switch (type) {
				case CMP_BUFFER:
					clist_buffer(buf, &cpos);
					break;
				case CMP_COMMAND:
					clist_command(buf, &cpos);
					break;
				case CMP_FILENAME:
					clist_file(buf, &cpos);
					break;
			}
			update(TRUE);

			/* if it exists, reprompt the user */
			if (prompt) {
				buf[cpos] = 0;
				if (type == CMP_COMMAND)
					mlwrite("%s%s", prompt, buf);
				else if (defval)
					mlwrite("%s[%s]: %s", prompt, defval, buf);
				else
					mlwrite("%s: %s", prompt, buf);
			}

		} else {
			if (cpos < maxlen && c > ' ') {
				buf[cpos++] = c;
				mlout(c);
				++ttcol;
				TTflush();
			}
		}
	}
}

int comp_command( char *name, int *cpos )	/*	comp_command:	Attempt a completion on a command name	*/
{
	register NBIND *bp;	/* trial command to complete */
	register int index;	/* index into strings to compare */
	register int curbind;	/* index into the names[] array */
	register NBIND *match;	/* last command that matches string */
	register int matchflag;	/* did this command name match? */
	register int comflag;	/* was there a completion at all? */

	/* start attempting completions, one character at a time */
	comflag = FALSE;
	curbind = 0;
	while (*cpos < NSTRING) {

		/* first, we start at the first command and scan the list */
		match = NULL;
		curbind = 0;
		while (curbind <= numfunc) {

			/* is this a match? */
			bp = &names[curbind];
			matchflag = TRUE;
			for (index = 0; index < *cpos; index++)
				if (name[index] != bp->n_name[index]) {
					matchflag = FALSE;
					break;
				}

			/* if it is a match */
			if (matchflag) {

				/* if this is the first match, simply record it */
				if (match == NULL) {
					match = bp;
					name[*cpos] = bp->n_name[*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != bp->n_name[*cpos])
						return 0;
				}
			}

			/* on to the next command */
			curbind++;
		}

		/* with no match, we are done */
		if (match == NULL) {
			/* beep if we never matched */
			if (comflag == FALSE)
				TTbeep();
			return 0;
		}

		/* if we have completed all the way... go back */
		if (name[*cpos] == 0) {
			(*cpos)++;
			return 0;
		}

		/* remember we matched, and complete one character */
		comflag = TRUE;
		TTputc(name[(*cpos)++]);
		TTflush();
	}

	/* don't allow a completion past the end of the max command name length */
	return 0;
}

int clist_command( char *name, int *cpos )	/*	clist_command:	Make a completion list based on a partial name */
{
	register NBIND *bp;	/* trial command to complete */
	register int curbind;	/* index into the names[] array */
	register int name_len;	/* current length of input string */
	register BUFFER *listbuf;/* buffer to put completion list into */

	/* get a buffer for the completion list */
	listbuf = bfind("[Completion list]", TRUE, BFINVS);
	if (listbuf == NULL || bclear(listbuf) == FALSE) {
		ctrlg(FALSE, 0);
		TTflush();
		return 0;
	}

	name_len = *cpos;

	/* first, we start at the first command and scan the list */
	for (curbind = 0; curbind <= numfunc; curbind++) {

		/* is this a match? */
		bp = &names[curbind];
		if (strncmp(name, bp->n_name, name_len) == 0)
			addline(listbuf, bp->n_name);
	}

	wpopup(listbuf);
	return 0;
}

int comp_buffer( char *name, int *cpos )	/*	comp_buffer:	Attempt a completion on a buffer name	*/
{
	register BUFFER *bp;	/* trial buffer to complete */
	register int index;	/* index into strings to compare */
	register BUFFER *match;	/* last buffer that matches string */
	register int matchflag;	/* did this buffer name match? */
	register int comflag;	/* was there a completion at all? */

	/* start attempting completions, one character at a time */
	comflag = FALSE;
	while (*cpos < NBUFN) {

		/* first, we start at the first buffer and scan the list */
		match = NULL;
		bp = bheadp;
		while (bp) {

			/* is this a match? */
			matchflag = TRUE;
			for (index = 0; index < *cpos; index++)
				if (name[index] != bp->b_bname[index]) {
					matchflag = FALSE;
					break;
				}

			/* if it is a match */
			if (matchflag) {

				/* if this is the first match, simply record it */
				if (match == NULL) {
					match = bp;
					name[*cpos] = bp->b_bname[*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != bp->b_bname[*cpos])
						return 0;
				}
			}

			/* on to the next buffer */
			bp = bp->b_bufp;
		}

		/* with no match, we are done */
		if (match == NULL) {
			/* beep if we never matched */
			if (comflag == FALSE)
				TTbeep();
			return 0;
		}

		/* if we have completed all the way... go back */
		if (name[*cpos] == 0) {
			(*cpos)++;
			return 0;
		}

		/* remember we matched, and complete one character */
		comflag = TRUE;
		TTputc(name[(*cpos)++]);
		TTflush();
	}

	/* don't allow a completion past the end of the max buffer name length */
	return 0;
}

int clist_buffer( char *name, int *cpos )	/*	clist_buffer:	Make a completion list based on a partial buffer name */
{
	register int name_len;	/* current length of input string */
	register BUFFER *listbuf;/* buffer to put completion list into */
	register BUFFER *bp;	/* trial buffer to complete */

	/* get a buffer for the completion list */
	listbuf = bfind("[Completion list]", TRUE, BFINVS);
	if (listbuf == NULL || bclear(listbuf) == FALSE) {
		ctrlg(FALSE, 0);
		TTflush();
		return 0;
	}

	/* first, we start at the first buffer and scan the list */
	name_len = *cpos;
	bp = bheadp;

	while (bp) {

		/* is this a match? */
		if (strncmp(name, bp->b_bname, name_len) == 0)
			addline(listbuf, bp->b_bname);

		/* on to the next buffer */
		bp = bp->b_bufp;
	}

	wpopup(listbuf);
	return 0;
}

int comp_file( char *name, int *cpos )	/*	comp_file:	Attempt a completion on a file name	*/
{
	register char *fname;	/* trial file to complete */
	register int index;	/* index into strings to compare */

	register int matches;	/* number of matches for name */
	char longestmatch[NSTRING]; /* temp buffer for longest match */
	int longestlen; 	/* length of longest match (always > *cpos) */

	/* everything (or nothing) matches an empty string */
	if (*cpos == 0)
		return 0;

	/* first, we start at the first file and scan the list */
	matches = 0;
	name[*cpos] = 0;
	fname = getffile(name);
	while (fname) {

		/* is this a match? */
		if (strncmp(name,fname,*cpos) == 0) {

			/* count the number of matches */
			matches++;

			/* if this is the first match, simply record it */
			if (matches == 1) {
				strcpy(longestmatch,fname);
				longestlen = strlen(longestmatch);
			} else {

				/* if there's a difference, stop here */
				if (longestmatch[*cpos] != fname[*cpos])
					return 0;

				for (index = (*cpos) + 1; index < longestlen; index++)
					if (longestmatch[index] != fname[index]) {
						longestlen = index;
						longestmatch[longestlen] = 0;
					}
			}
		}

		/* on to the next file */
		fname = getnfile();
	}

	/* beep if we never matched */
	if (matches == 0) {
		TTbeep();
		return 0;
	}

	/* the longestmatch array contains the longest match so copy and print it */
	for ( ; (*cpos < (NSTRING-1)) && (*cpos < longestlen); (*cpos)++) {
		name[*cpos] = longestmatch[*cpos];
		TTputc(name[*cpos]);
	}

	name[*cpos] = 0;

	/* if only one file matched then increment cpos to signal complete() */
	/* that this was a complete match.  If a directory was matched then */
	/* last character will be the DIRSEPCHAR.  In this case we do NOT */
	/* want to signal a complete match. */
	if ((matches == 1) && (name[(*cpos)-1] != DIRSEPCHAR))
		(*cpos)++;

	TTflush();

	return 0;
}

int clist_file( char *name, int *cpos )	/*	clist_file:	Make a completion list based on a partial file name */
{
	register int name_len;	/* current length of input string */
	register BUFFER *listbuf;/* buffer to put completion list into */
	register char *fname;	/* trial file to complete */

	/* get a buffer for the completion list */
	listbuf = bfind("[Completion list]", TRUE, BFINVS);
	if (listbuf == NULL || bclear(listbuf) == FALSE) {
		ctrlg(FALSE, 0);
		TTflush();
		return 0;
	}

	/* first, we start at the first file and scan the list */
	name_len = *cpos;
	name[*cpos] = 0;
	fname = getffile(name);

	/* first, we start at the first file and scan the list */
	while (fname) {

		/* is this a match? */
		if (strncmp(name, fname, name_len) == 0)
			addline(listbuf, fname);

		/* on to the next file */
		fname = getnfile();
	}

	wpopup(listbuf);
	return 0;
}

int tgetc()	/*	tgetc:	Get a key from the terminal driver, resolve any keyboard macro action	*/
{
	int c;	/* fetched character */

	/* if we are playing a keyboard macro back, */
	if (kbdmode == PLAY) {

		/* if there is some left... */
		if (kbdptr < kbdend)
			return((int)*kbdptr++);

		/* at the end of last repitition? */
		if (--kbdrep < 1) {
			kbdmode = STOP;
			/* force a screen update after all is done */
			update(FALSE);
		} else {

			/* reset the macro to the begining for the next rep */
			kbdptr = &kbdm[0];
			return((int)*kbdptr++);
		}
	}

	/* if no pending character */
	if (cpending == FALSE) {

		/* fetch a character from the terminal driver */
		c = TTgetc();

	} else {
		
		c = charpending;
		cpending = FALSE;
	}

	/* record it for $lastkey */
	lastkey = c;

	/* save it if we need to */
	if (kbdmode == RECORD) {
		*kbdptr++ = c;
		kbdend = kbdptr;

		/* don't overrun the buffer */
		if (kbdptr == &kbdm[NKBDM - 1]) {
			kbdmode = STOP;
			TTbeep();
		}
	}

	/* and finally give the char back */
	return(c);
}

int getkey()	/*	getkey:	Get one keystroke. The legal prefixs here are the SPEC, MOUS and CTRL prefixes.	*/
{
	int c;		/* next input character */
	int upper;	/* upper byte of the extended sequence */
      
	/* get a keystroke */
        c = tgetc();

	/* if it exists, process an escape sequence */
	if (c == 0) {

		/* get the event type */
		upper = tgetc();

		/* mouse events need us to read in the row/col */
		if (upper & (MOUS >> 8)) {
			/* grab the x/y position of the mouse */
			xpos = tgetc();
			ypos = tgetc();
		}

		/* get the event code */
		c = tgetc();

		/* if it is a function key... map it */
		c = (upper << 8) | c;
	}

	/* yank out the control prefix */
        if (((c & 255) >=0x00 && (c & 255) <= 0x1F) || (c & 255) == 0x7F)
                c = CTRL | (c ^ 0x40);

	/* return the character */

        return(c);
}

int getcmd()	/*	GETCMD:	Get a command from the keyboard. Process all applicable prefix keys	*/
{
	int c;		/* fetched keystroke */
	KEYTAB *key;	/* ptr to a key entry */

	/* get initial character */
	c = getkey();
	key = getbind(c);

	/* resolve META and CTLX prefixes */
	if (key) {
		if (key->k_ptr.fp == meta) {
			c = getkey();
			c = upperc(c) | (c & ~255);	/* Force to upper */
			c |= META;
		} else if (key->k_ptr.fp == cex) {
			c = getkey();
			c = upperc(c) | (c & ~255);	/* Force to upper */
			c |= CTLX;
		}
	}

	/* return it */
	return(c);
}

/*	A more generalized prompt/reply function allowing the caller
 *	to specify the proper terminator. If the terminator is not
 *	a return('\r'), return will echo as "<NL>"
 */

int  getstring( char *buf, int nbuf, int eolchar)
{
	register int cpos;	/* current character position in string */
	register int c;		/* current input character */
	register int ec;	/* extended current input character */
	register int quotef;	/* are we quoting the next char? */
	char *kp;		/* pointer into key_name */
	char key_name[10];	/* name of a quoted key */

	cpos = 0;
	quotef = FALSE;

	for (;;) {
		/* get a character from the user */
		ec = getkey();

		/* if they hit the line terminate, wrap it up */
		if (ec == eolchar && quotef == FALSE) {
			buf[cpos++] = 0;

			/* clear the message line */
			mlerase();

			/* if we default the buffer, return FALSE */
			if (buf[0] == 0)
				return(FALSE);

			return(TRUE);
		}

		/* change from command form back to character form */
		c = ectoc(ec);

		if ((ec == abortc) && (quotef == FALSE)) {
			/* Abort the input? */
			ctrlg(FALSE, 0);
			TTflush();
			return(ABORT);
		}

		/* if it is from the mouse, or is a function key, blow it off */
		if ((quotef == FALSE) && ((ec & MOUS) || (ec & SPEC)))
			continue;

		/* rubout/erase */
		if ((c==0x7F || c==0x08) && quotef==FALSE) {
			if (cpos != 0) {
				outstring("\b \b");
				--ttcol;

				if (buf[--cpos] < 0x20) {
					outstring("\b \b");
					--ttcol;
				}

				if (buf[cpos] == '\r') {
					outstring("\b\b  \b\b");
					ttcol -= 2;
				}
				TTflush();
			}
			continue;
 		}

		/* C-K, kill default buffer and return null */
		if (c == 0x0b && quotef == FALSE) {

 			/* clear the buffer */
 			buf[0] = 0;
 
 			/* clear the message line and return */
 			mlwrite("");
 			TTflush();
 			return(TRUE);
		}

		/* C-U, kill */
		if (c == 0x15 && quotef == FALSE) {

			while (cpos != 0) {
				outstring("\b \b");
				--ttcol;

				if (buf[--cpos] < 0x20) {
					outstring("\b \b");
					--ttcol;
				}
				if (buf[cpos] == '\r') {
					outstring("\b\b  \b\b");
					ttcol -= 2;
				}
			}
			TTflush();
			continue;
		}

		/* quoting next character? */
		if ((ec == quotec) && (quotef == FALSE)) {
			quotef = TRUE;
			continue;
		}

		quotef = FALSE;

		/* if it is from the mouse, or is a function key,
		   insert it's name since it was quoted */
		if ((ec & MOUS) || (ec & SPEC)) {
			cmdstr(ec, key_name);
			kp = key_name;
			while (*kp) {
				if (cpos < nbuf - 1) {
					if (disinp)
						mlout(*kp);
					buf[cpos++] = *kp++;
					++ttcol;
				}
			}
			TTflush();
			continue;
		}

		/* insert the character in the string! */
		if (cpos < nbuf-1) {

			buf[cpos++] = c;

			if ((c < ' ') && (c != '\r')) {
				outstring("^");
				++ttcol;
				c ^= 0x40;
			}

			if (c != '\r') {
				if (disinp)
					mlout(c);
			} else {	/* put out <NL> for <ret> */
				outstring("<NL>");
				ttcol += 3;
			}
			++ttcol;
			TTflush();
		}
	}
}

int outstring( char *s )	/* output a string of input characters */
{
	if (disinp)
		while (*s)
			mlout(*s++);
	return(0);
}

int ostring( char *s )		/* output a string of output characters */
{
	if (discmd)
		while (*s)
			mlout(*s++);
	return(0);
}

int mlprompt( char *prompt, char *dflt, int iterm )	/* mlprompt -- Display a prompt [with optional default] and the	input terminator. */
{
	register int tcol;
	char buf[NSTRING];

	/* don't bother displaying if we are't currently */
	if (discmd == FALSE)
		return(0);

	/* show the passed in prompt */
	mlwrite(prompt);
	tcol = strlen(prompt);

	/* If there's a default, put it in brackets and show it. */
	if (dflt != NULL && *dflt != '\0') {
		mlout('[');
		tcol = 1 + echostring(dflt, tcol + 1, NPAT/2);
		mlout(']');
	}

	/* Display the proper current search terminator character. */
	mlout('<');
	switch (iterm) {
		case CTRL | '[':
			mlputs("META"); tcol += 8; break;
		case CTRL | 'M':
			mlputs("NL"); tcol += 6; break;
		default:
			mlputs(cmdstr(iterm, buf));
			tcol += strlen(buf) + 4;
	}
	mlputs(">: ");
	TTflush();
	return(tcol);
}


int echostring( char *str, int tcol, int uptocol )	/* echostring -- Use echochar() to put out a string.  Checks for NULL. */
{
	if (str != NULL) {
		while (*str) {
			tcol = echochar(*str++, tcol);
			if (tcol >= uptocol) {
				mlout('$');
				tcol++;
				break;
			}
		}
	}
	return(tcol);
}

int echochar( unsigned char c, int col )	/* Routine to echo i-search and message-prompting characters. */
{
	movecursor(term.t_nrow, col);	/* Position the cursor	*/
	if (c == '\r') {		/* Newline character?	*/
		mlout('<');
		mlout('N');
		mlout('L');
		mlout('>');
		col += 3;
	}
	else if ((c < ' ') || (c == 0x7F)) {	/* Vanilla control char and Rubout:   */
		mlout('^'); 	/* Yes, output prefix		*/
		mlout(c ^ 0x40);/* Make it "^X"			*/
		col++;		/* Count this char		*/
	}
	else
		mlout(c);		/* Otherwise, output raw char	*/
	TTflush();			/* Flush the output		*/
	return(++col);			/* return the new column number */
}
