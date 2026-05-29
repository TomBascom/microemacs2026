/* exec.c
 *
 * This file is for functions dealing with execution of
 * commands, command lines, buffers, files and startup files
 *
 * written 1993 by Daniel Lawrence
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"

int namedcmd( int f, int n )	/* namedcmd:	execute a named command even if it is not bound */
{
	int (*kfunc)(int, int); 	/* ptr to the function to execute */
	char buffer[NSTRING];		/* buffer to store function name */
	int status;

	/* if we are non-interactive.... force the command interactivly */
	if (clexec == TRUE) {
		/* grab token and advance past */
		execstr = token(execstr, buffer, NPAT);

		/* evaluate it */
		{
			char *sp = fixnull(getval(buffer));
			memmove(buffer, sp, strlen(sp) + 1);	/* sp may overlap */
		}
		if (strcmp(buffer, errorm) == 0)
			return(FALSE);

		/* and look it up */
		if ((kfunc = fncmatch(buffer)) == NULL) {
			mlwrite(TEXT16);	/* "[No such Function]" */
			return(FALSE);
		}
		
		/* and execute it  INTERACTIVE */
		clexec = FALSE;
		status = (*kfunc)(f, n);	/* call the function */
		clexec = TRUE;
		return(status);
	}

	/* prompt the user to type a named command */
	/* and get the function name to execute */
	kfunc = getname(": ");
	if (kfunc == NULL) {
		mlwrite(TEXT16);	/* "[No such function]" */
		return(FALSE);
	}

	/* and then execute the command */
	return((*kfunc)(f, n));
}

int execcmd( int f, int n )	/* execcmd:	Execute a command line command to be typed in by the user */
{
	register int status;		/* status return */
	char cmdstr[NSTRING];		/* string holding command to execute */

	/* get the line wanted */
	if ((status = mlreply(": ", cmdstr, NSTRING)) != TRUE)
		return(status);

	execlevel = 0;
	return(docmd(cmdstr));
}

/*	docmd:	take a passed string as a command line and translate
		it to be executed as a command. This function will be
		used by execute-command-line and by all source and
		startup files. Lastflag/thisflag is also updated.

	format of the command line is:

		{# arg} <command-name> {<argument string(s)>}

*/

int docmd( char *cline )
{
	register int f;		/* default argument flag */
	register int n;		/* numeric repeat value */
	int (*fnc)(int, int);/* function to execute */
	BUFFER *bp;		/* buffer to execute */
	int status;		/* return status of function */
	int oldcle;		/* old contents of clexec flag */
	char *oldestr;		/* original exec string */
	char tkn[NSTRING];	/* next token off of command line */
        char bufn[NBUFN+2];	/* name of buffer to execute */

	/* if we are scanning and not executing..go back here */
	if (execlevel)
		return(TRUE);

	oldestr = execstr;	/* save last ptr to string to execute */
	execstr = cline;	/* and set this one as current */

	/* first set up the default command values */
	f = FALSE;
	n = 1;
	lastflag = thisflag;
	thisflag = 0;

	if ((status = macarg(tkn)) != TRUE) {	/* and grab the first token */
		execstr = oldestr;
		return(status);
	}

	/* process leadin argument */
	if (gettyp(tkn) != TKCMD) {
		f = TRUE;
		{
			char *sp = fixnull(getval(tkn));
			memmove(tkn, sp, strlen(sp) + 1);	/* sp may overlap */
		}
		n = asc_int(tkn);

		/* and now get the command to execute */
		if ((status = macarg(tkn)) != TRUE) {
			execstr = oldestr;
			return(status);
		}
	}

	/* and match the token to see if it exists */
	if ((fnc = fncmatch(tkn)) == NULL) {

		/* construct the buffer name */
		strcpy(bufn, "[");
		strcat(bufn, tkn);
		strcat(bufn, "]");

		/* find the pointer to that buffer */
        	if ((bp=bfind(bufn, FALSE, 0)) == NULL) {
			mlwrite(TEXT16);	/* "[No such Function]" */
			execstr = oldestr;
			return(FALSE);
		}

		/* execute the buffer */
		oldcle = clexec;	/* save old clexec flag */
		clexec = TRUE;		/* in cline execution */
		while (n-- > 0)
			if ((status = dobuf(bp)) != TRUE)
				break;
		cmdstatus = status;	/* save the status */
		clexec = oldcle;	/* restore clexec flag */
		execstr = oldestr;
		return(status);
	}
	
	/* save the arguments and go execute the command */
	oldcle = clexec;		/* save old clexec flag */
	clexec = TRUE;			/* in cline execution */
	status = (*fnc)(f, n);		/* call the function */
	cmdstatus = status;		/* save the status */
	clexec = oldcle;		/* restore clexec flag */
	execstr = oldestr;
	return(status);
}

char *token( char *src, char *tok, int size )	/* token:	chop a token off a string return a pointer past the token */
{
	register int quotef;	/* is the current string quoted? */
	register char c;	/* temporary character */

	/* first scan past any whitespace in the source string */
	while (*src == ' ' || *src == '\t')
		++src;

	/* scan through the source string */
	quotef = FALSE;
	while (*src) {
		/* process special characters */
		if (*src == '~') {
			++src;
			if (*src == 0)
				break;
			switch (*src++) {
				case 'r':	c = 13; break;
				case 'n':	c = 13; break;
				case 'l':	c = 10; break;
				case 't':	c = 9;  break;
				case 'b':	c = 8;  break;
				case 'f':	c = 12; break;
				default:	c = *(src-1);
			}
			if (--size > 0) {
				*tok++ = c;
			}
		} else {
			/* check for the end of the token */
			if (quotef) {
				if (*src == '"')
					break;
			} else {
				if (*src == ' ' || *src == '\t')
					break;
			}

			/* set quote mode if quote found */
			if (*src == '"')
				quotef = TRUE;

			/* record the character */
			c = *src++;
			if (--size > 0)
				*tok++ = c;
		}
	}

	/* terminate the token and exit */
	if (*src)
		++src;
	*tok = 0;
	return(src);
}

int macarg( char *tok )	/* get a macro line argument */
{
	int savcle;	/* buffer to store original clexec */
	int status;

	savcle = clexec;	/* save execution mode */
	clexec = TRUE;		/* get the argument */
	status = nextarg("", tok, NSTRING, ctoec('\r'));
	clexec = savcle;	/* restore execution mode */
	return(status);
}

int nextarg( char *prompt, char *buffer, int size, int terminator )	/*	nextarg:	get the next argument	*/
{
	register char *sp;	/* return pointer from getval() */

	/* if we are interactive, go get it! */
	if (clexec == FALSE) {
		/* prompt the user for the input string */
		if (discmd) {
			if (prompt) mlwrite(prompt);
		}
		else
			movecursor(term.t_nrow, 0);

		return(getstring(buffer, size, terminator));
	}

	/* grab token and advance past */
	execstr = token(execstr, buffer, size);

	/* evaluate it */
	if ((sp = getval(buffer)) == NULL)
		return(FALSE);
	memmove(buffer, sp, strlen(sp) + 1);	/* sp may overlap buffer */
	return(TRUE);
}

int storemac( int f, int n )	/*	storemac:	Set up a macro buffer and flag to store all executed command lines there			*/
{
	register struct BUFFER *bp;	/* pointer to macro buffer */
	char bname[NBUFN];		/* name of buffer to use */

	/* must have a numeric argument to this function */
	if (f == FALSE) {
		mlwrite(TEXT111);	/* "No macro specified" */
		return(FALSE);
	}

	/* range check the macro number */
	if (n < 1 || n > 40) {
		mlwrite(TEXT112);	/* "Macro number out of range" */
		return(FALSE);
	}

	/* construct the macro buffer name */
	strcpy(bname, "[Macro xx]");
	bname[7] = '0' + (n / 10);
	bname[8] = '0' + (n % 10);

	/* set up the new macro buffer */
	if ((bp = bfind(bname, TRUE, BFINVS)) == NULL) {
		mlwrite(TEXT113);	/* "Can not create macro" */
		return(FALSE);
	}

	/* and make sure it is empty */
	bclear(bp);

	/* and set the macro store pointers to it */
	mstore = TRUE;
	bstore = bp;
	return(TRUE);
}

int storeproc( int f, int n )		/*	storeproc:	Set up a procedure buffer and flag to store all executed command lines there			*/
{
	register struct BUFFER *bp;	/* pointer to macro buffer */
	register int status;		/* return status */
	char bname[NBUFN];		/* name of buffer to use */

	/* a numeric argument means its a numbered macro */
	if (f == TRUE)
		return(storemac(f, n));

	/* get the name of the procedure */
        if ((status = mlreply(TEXT114, &bname[1], NBUFN-2)) != TRUE)	/* "Procedure name: " */
                return(status);

	/* construct the macro buffer name */
	bname[0] = '[';
	strcat(bname, "]");

	/* set up the new macro buffer */
	if ((bp = bfind(bname, TRUE, BFINVS)) == NULL) {
		mlwrite(TEXT113);	/* "Can not create macro" */
		return(FALSE);
	}

	/* and make sure it is empty */
	bclear(bp);

	/* and set the macro store pointers to it */
	mstore = TRUE;
	bstore = bp;
	return(TRUE);
}

int execproc( int f, int n )		/*	execproc:	Execute a procedure				*/
{
        register BUFFER *bp;		/* ptr to buffer to execute */
        register int status;		/* status return */
        char bufn[NBUFN+2];		/* name of buffer to execute */

	/* find out what buffer the user wants to execute */
        if ((status = mlreply(TEXT115, &bufn[1], NBUFN)) != TRUE)	/* "Execute procedure: " */
                return(status);

	/* construct the buffer name */
	bufn[0] = '[';
	strcat(bufn, "]");

	/* find the pointer to that buffer */
        if ((bp=bfind(bufn, FALSE, 0)) == NULL) {
		mlwrite(TEXT116);	/* "No such procedure" */
                return(FALSE);
        }

	/* and now execute it as asked */
	while (n-- > 0)
		if ((status = dobuf(bp)) != TRUE)
			return(status);
	return(TRUE);
}

int execbuf( int f, int n )	/*	execbuf:	Execute the contents of a buffer of commands	*/
{
        register BUFFER *bp;		/* ptr to buffer to execute */
        register int status;		/* status return */

	/* find out what buffer the user wants to execute */
	if ((bp = getcbuf(TEXT117, curbp->b_bname, FALSE)) == NULL)	/* "Execute buffer: " */
		return(ABORT);

	/* and now execute it as asked */
	while (n-- > 0)
		if ((status = dobuf(bp)) != TRUE)
			return(status);
	return(TRUE);
}

/*	dobuf:	execute the contents of the buffer pointed to
		by the passed BP

	Directives start with a "!" and include:

	!endm		End a macro
	!if (cond)	conditional execution
	!else
	!endif
	!return		Return (terminating current macro)
	!goto <label>	Jump to a label in the current macro
	!force		Force macro to continue...even if command fails
	!while (cond)	Execute a loop if the condition is true
	!endwhile
	
	Line Labels begin with a "*" as the first nonblank char, like:

	*LBL01
*/

int dobuf( BUFFER *bp )
{
        register int status;	/* status return */
	register LINE *lp;	/* pointer to line to execute */
	register LINE *hlp;	/* pointer to line header */
	register LINE *glp;	/* line to goto */
	LINE *mp;		/* Macro line storage temp */
	int dirnum;		/* directive index */
	int linlen;		/* length of line to execute */
	int i;			/* index */
	int force;		/* force TRUE result? */
	WINDOW *wp;		/* ptr to windows to scan */
	WHBLOCK *whlist;	/* ptr to !WHILE list */
	WHBLOCK *scanner;	/* ptr during scan */
	WHBLOCK *whtemp;	/* temporary ptr to a WHBLOCK */
	char *einit = NULL;	/* initial value of eline */
	char *eline;		/* text of line to execute */
	char tkn[NSTRING];	/* buffer to evaluate an expresion in */

	/* clear IF level flags/while ptr */
	execlevel = 0;
	whlist = NULL;
	scanner = NULL;

	/* flag we are executing the buffer */
	bp->b_exec += 1;

	/* scan the buffer to execute, building WHILE header blocks */
	hlp = bp->b_linep;
	lp = lforw(hlp);
	while (lp != hlp) {

		/* scan the current line */
		eline = ltext(lp);
		i = lused(lp);

		/* trim leading whitespace */
		while (i-- > 0 && (*eline == ' ' || *eline == '\t'))
			++eline;

		/* if theres nothing here, don't bother */
		if (i <= 0)
			goto nxtscan;

		/* if is a while directive, make a block... */
		if (eline[0] == '!' && eline[1] == 'w' && eline[2] == 'h') {
			whtemp = (WHBLOCK *)malloc(sizeof(WHBLOCK));
			if (whtemp == NULL) {
noram:				errormesg(TEXT119, bp, lp);	/* "%%Out of memory during while scan" */

failexit:			freewhile(scanner);
				goto eabort;
			}
			whtemp->w_begin = lp;
			whtemp->w_type = BTWHILE;
			whtemp->w_next = scanner;
			scanner = whtemp;
		}

		/* if is a BREAK directive, make a block... */
		if (eline[0] == '!' && eline[1] == 'b' && eline[2] == 'r') {
			if (scanner == NULL) {
				errormesg(TEXT120, bp, lp);	/* "%%!BREAK outside of any !WHILE loop" */
				goto failexit;
			}
			whtemp = (WHBLOCK *)malloc(sizeof(WHBLOCK));
			if (whtemp == NULL)
				goto noram;
			whtemp->w_begin = lp;
			whtemp->w_type = BTBREAK;
			whtemp->w_next = scanner;
			scanner = whtemp;
		}

		/* if it is an endwhile directive, record the spot... */
		if (eline[0] == '!' && strncmp(&eline[1], "endw", 4) == 0) {
			if (scanner == NULL) {
				errormesg(TEXT121, bp, lp);	/* "%%!ENDWHILE with no preceding !WHILE" */
				goto failexit;
			}
			/* move top records from the scanner list to the
			   whlist until we have moved all BREAK records
			   and one WHILE record */
			do {
				scanner->w_end = lp;
				whtemp = whlist;
				whlist = scanner;
				scanner = scanner->w_next;
				whlist->w_next = whtemp;
			} while (whlist->w_type == BTBREAK);
		}

nxtscan:	/* on to the next line */
		lp = lforw(lp);
	}

	/* while and endwhile should match! */
	if (scanner != NULL) {
		errormesg(TEXT122, bp, lp);			/* "%%!WHILE with no matching !ENDWHILE" */
		goto failexit;
	}

	/* let the first command inherit the flags from the last one..*/
	thisflag = lastflag;

	/* starting at the beginning of the buffer */
	hlp = bp->b_linep;
	lp = lforw(hlp);
	while (lp != hlp && eexitflag == FALSE) {

		/* allocate eline and copy macro line to it */
		linlen = lused(lp);
		if ((einit = eline = malloc(linlen+1)) == NULL) {
			errormesg(TEXT123, bp, lp);		/* "%%Out of Memory during macro execution" */
			freewhile(whlist);
			bp->b_exec -= 1;
			return(FALSE);
		}
		bytecopy(eline, ltext(lp), linlen);
		eline[linlen] = 0;	/* make sure it ends */

		/* trim leading whitespace */
		while (*eline == ' ' || *eline == '\t')
			++eline;

		/* dump comments and blank lines */
		if (*eline == ';' || *eline == 0)
			goto onward;

		/* Parse directives here.... */
		dirnum = -1;
		if (*eline == '!') {
			/* Find out which directive this is */
			++eline;
			for (dirnum = 0; dirnum < NUMDIRS; dirnum++)
				if (strncmp(eline, dname[dirnum],
				            dname_len[dirnum]) == 0)
					break;

			/* and bitch if it's illegal */
			if (dirnum == NUMDIRS) {
				errormesg(TEXT124, bp, lp);	/* "%%Unknown Directive" */
				goto eabort;
			}

			/* service only the !ENDM macro here */
			if (dirnum == DENDM) {
				mstore = FALSE;
				bstore = NULL;
				goto onward;
			}

			/* restore the original eline....*/
			--eline;
		}

		/* if macro store is on, just salt this away */
		if (mstore) {
			/* allocate the space for the line */
			linlen = strlen(eline);
			if ((mp=lalloc(linlen)) == NULL) {
				errormesg(TEXT125, bp, lp);	/* "Out of memory while storing macro" */
				goto eabort;
			}
	
			/* copy the text into the new line */
			for (i=0; i<linlen; ++i)
				lputc(mp, i, eline[i]);
	
			/* attach the line to the end of the buffer */
	       		bstore->b_linep->l_bp->l_fp = mp;
			mp->l_bp = bstore->b_linep->l_bp;
			bstore->b_linep->l_bp = mp;
			mp->l_fp = bstore->b_linep;
			goto onward;
		}
	
		force = FALSE;

		/* dump comments */
		if (*eline == '*')
			goto onward;

		/* now, execute directives */
		if (dirnum != -1) {
			/* skip past the directive */
			while (*eline && *eline != ' ' && *eline != '\t')
				++eline;
			execstr = eline;

			switch (dirnum) {
			case DIF:	/* IF directive */
				/* grab the value of the logical exp */
				if (execlevel == 0) {
					if (macarg(tkn) != TRUE) {
						free(einit);
						goto eexec;
					}
					if (stol(tkn) == FALSE)
						++execlevel;
				} else
					++execlevel;
				goto onward;

			case DWHILE:	/* WHILE directive */
				/* grab the value of the logical exp */
				if (execlevel == 0) {
					if (macarg(tkn) != TRUE) {
						free(einit);
						goto eexec;
					}
					if (stol(tkn) == TRUE)
						goto onward;
				}
				/* drop down and act just like !BREAK */
				/* fall through */

			case DBREAK:	/* BREAK directive */
				if (dirnum == DBREAK && execlevel)
					goto onward;

				/* jump down to the endwhile */
				/* find the right while loop */
				whtemp = whlist;
				while (whtemp) {
					if (whtemp->w_begin == lp)
						break;
					whtemp = whtemp->w_next;
				}
			
				if (whtemp == NULL) {
					errormesg(TEXT126, bp, lp);	/* "%%Internal While loop error" */
					goto eabort;
				}
			
				/* reset the line pointer back.. */
				lp = whtemp->w_end;
				goto onward;

			case DELSE:	/* ELSE directive */
				if (execlevel == 1)
					--execlevel;
				else if (execlevel == 0 )
					++execlevel;
				goto onward;

			case DENDIF:	/* ENDIF directive */
				if (execlevel)
					--execlevel;
				goto onward;

			case DGOTO:	/* GOTO directive */
				/* .....only if we are currently executing */
				if (execlevel == 0) {

					/* grab label to jump to */
					eline = token(eline, golabel, NPAT);
					linlen = strlen(golabel);
					glp = lforw(hlp);
					while (glp != hlp) {
						if ((lused(glp) >= linlen) &&
						    (lgetc(glp, 0) == '*') &&
						    (strncmp(((char *)ltext(glp)) + 1,
						            golabel, linlen) == 0)) {
							lp = glp;
							goto onward;
						}
						glp = lforw(glp);
					}
					errormesg(TEXT127, bp, lp);	/* "%%No such label" */
					goto eabort;
				}
				goto onward;
	
			case DRETURN:	/* RETURN directive */
				if (execlevel == 0) {
					free(einit);
					goto eexec;
				}
				goto onward;

			case DENDWHILE:	/* ENDWHILE directive */
				if (execlevel) {
					--execlevel;
					goto onward;
				} else {
					/* find the right while loop */
					whtemp = whlist;
					while (whtemp) {
						if (whtemp->w_type == BTWHILE &&
 						    whtemp->w_end == lp)
							break;
						whtemp = whtemp->w_next;
					}
		
					if (whtemp == NULL) {
						errormesg(TEXT126, bp, lp);	/* "%%Internal While loop error" */
						goto eabort;
					}
		
					/* reset the line pointer back.. */
					lp = lback(whtemp->w_begin);
					goto onward;
				}

			case DFORCE:	/* FORCE directive */
				force = TRUE;

			}
		}

		/* execute the statement */
		status = docmd(eline);
		if (force)		/* force the status */
			status = TRUE;

		/* check for a command error */
		if (status != TRUE) {

			/* look if buffer is showing */
			wp = wheadp;
			while (wp != NULL) {
				if (wp->w_bufp == bp) {
					/* and point it */
					wp->w_dotp = lp;
					wp->w_doto = 0;
					wp->w_flag |= WFHARD;
				}
				wp = wp->w_wndp;
			}

			/* in any case set the buffer . */
			bp->b_dotp = lp;
			bp->b_doto = 0;

			errormesg(TEXT219, bp, lp);			/* "%%Macro Failed" */

			execlevel = 0;
			freewhile(whlist);
			bp->b_exec -= 1;
			free(einit);
			return(status);
		}

onward:		/* on to the next line */
		free(einit);
		lp = lforw(lp);
	}

eexec:	/* exit the current function */
	execlevel = 0;
	freewhile(whlist);
	bp->b_exec -= 1;
        return(TRUE);

eabort:	/* exit the current function with a failure */
	execlevel = 0;
	freewhile(whlist);
	bp->b_exec -= 1;
	free(einit);
        return(FALSE);
}

void errormesg( char *mesg, BUFFER *bp, LINE *lp )	/* errormesg:	display a macro execution error along with the buffer and line currently being executed */
{
	char buf[NSTRING];

	exec_error = TRUE;

	/* build error message line */
	strcpy(buf, "\n");
	strcat(buf, mesg);
	strcat(buf, TEXT229);		/* " in < " */
	strcat(buf, bp->b_bname);
	strcat(buf, TEXT230);		/* "> at line " */
	strcat(buf, long_asc(getlinenum(bp, lp)));
	mlforce(buf);
}

void makelit( char *s )		/* expand all "%" to "%%" */
{
	register char *sp;	/* temp for expanding string */
	register char *ep;	/* ptr to end of string to expand */

	sp = s;
	while (*sp)
	if (*sp++ == '%') {
		/* advance to the end */
		ep = --sp;
		while (*ep++)
			;
		/* null terminate the string one out */
		*(ep + 1) = 0;
		/* copy backwards */
		while(ep-- > sp)
			*(ep + 1) = *ep;

		/* and advance sp past the new % */
		sp += 2;					
	}
}

void freewhile( WHBLOCK *wp )	/* free a list of while block pointers */
{
	if (wp != NULL)
	{
		freewhile(wp->w_next);
		free((char *) wp);
	}
}

int execfile( int f, int n )	/* execute a series of commands in a file */
{
	register int status;	/* return status of name query */
	char fname[NSTRING];	/* name of file to execute */
	char *fspec;		/* full file spec */

	if ((status = mlreply(TEXT129, fname, NSTRING -1)) != TRUE)
		return(status);

	/* look up the path for the file */
	fspec = flook(fname, TRUE);

	/* if it isn't around */
	if (fspec == NULL) {

		/* try to default the extension */
		if (sindex(fname, ".") == 0) {
			strcat(fname, ".cmd");
			fspec = flook(fname, TRUE);
			if (fspec != NULL)
				goto exec1;
		}

		/* complain if we are interactive */
		if (clexec == FALSE)
			mlwrite(TEXT214, fname);		/* "%%No such file as %s" */
		return(FALSE);
	}

exec1:	/* otherwise, execute it */
	while (n-- > 0)
		if ((status=dofile(fspec)) != TRUE)
			return(status);

	return(TRUE);
}

int dofile( char *fname )	/*	dofile:	yank a file into a buffer and execute it if there are no errors, delete the buffer on exit */
{
	register BUFFER *bp;	/* buffer to place file to exeute */
	register BUFFER *cb;	/* temp to hold current buf while we read */
	register int status;	/* results of various calls */
	char bname[NBUFN];	/* name of buffer */

	makename(bname, fname);		/* derive the name of the buffer */
	unqname(bname);			/* make sure we don't stomp things */
	if ((bp = bfind(bname, TRUE, 0)) == NULL) /* get the needed buffer */
		return(FALSE);

	bp->b_mode = MDVIEW;	/* mark the buffer as read only */
	cb = curbp;		/* save the old buffer */
	curbp = bp;		/* make this one current */
	/* and try to read in the file to execute */
	if ((status = readin(fname, FALSE)) != TRUE) {
		curbp = cb;	/* restore the current buffer */
		return(status);
	}
	/* go execute it! */
	curbp = cb;		/* restore the current buffer */
	if ((status = dobuf(bp)) != TRUE) {
		return(status);
	}
	/* if not displayed, remove the now unneeded buffer and exit */
	if (bp->b_nwnd == 0)
		zotbuf(bp);
	return(TRUE);
}

int cbuf( int f, int n, int bufnum )	/* cbuf:	Execute the contents of a numbered buffer	*/
{
        register BUFFER *bp;		/* ptr to buffer to execute */
        register int status;		/* status return */
	static char bufname[] = "[Macro xx]";

	/* make the buffer name */
	bufname[7] = '0' + (bufnum / 10);
	bufname[8] = '0' + (bufnum % 10);

	/* find the pointer to that buffer */
        if ((bp=bfind(bufname, FALSE, 0)) == NULL) {
        	mlwrite(TEXT130);	/* "Macro not defined" */
                return(FALSE);
        }

	/* and now execute it as asked */
	while (n-- > 0)
		if ((status = dobuf(bp)) != TRUE)
			return(status);
	return(TRUE);
}

int cbuf1( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf2( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf3( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf4( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf5( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf6( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf7( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf8( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf9( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf10( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf11( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf12( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf13( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf14( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf15( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf16( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf17( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf18( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf19( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf20( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf21( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf22( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf23( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf24( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf25( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf26( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf27( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf28( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf29( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf30( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf31( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf32( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf33( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf34( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf35( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf36( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf37( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf38( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf39( int f, int n ) { return(cbuf(f, n, 1)); }
int cbuf40( int f, int n ) { return(cbuf(f, n, 1)); }
