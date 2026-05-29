/* bind.c
 *
 * This file is for functions having to do with key bindings, descriptions,
 * help commands and startup file.
 * 
 * written 11-feb-86 by Daniel Lawrence
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"

const char *pathname[] =
{
	".emacsrc",
	"emacs.hlp",
	"/usr/local/",
	"/usr/local/etc/",
	"/usr/local/lib/",
	""
};

#define	NPNAMES	(sizeof(pathname)/sizeof(char *))

int help( int f, int n )	/* give me some help!!!! bring up a fake buffer and read the help file into it with view mode */
{
	register BUFFER *bp;	/* buffer pointer to help */
	char *fname;		/* file name of help file */

	bp = bfind("emacs.hlp", FALSE, BFINVS);			/* first check if we are already here */

	if (bp == NULL) {
		fname = flook((char *)pathname[1], FALSE);
		if (fname == NULL) {
			mlwrite(TEXT12);			/* "[Help file is not online]" */
			return(FALSE);
		}
	}

	if (splitwind(FALSE, 1) == FALSE)			/* split the current window to make room for the help stuff */
			return(FALSE);

	if (bp == NULL) {					/* and read the stuff in */
		if (getfile(fname, FALSE) == FALSE)
			return(FALSE);
	} else
		swbuffer(bp);

	curwp->w_bufp->b_mode |= MDVIEW;			/* make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_flag |= BFINVS;
	upmode();
	return(TRUE);
}

int deskey( int f, int n )	/* describe the command for a certain key */
{
	register int c; 	/* key to describe */
	register char *ptr;	/* string pointer to scan output strings */
	char outseq[NSTRING];	/* output buffer for command sequence */

	/* prompt the user to type us a key to describe */
	mlwrite(TEXT13);	/* ": describe-key " */

	ostring(cmdstr(c = getckey(FALSE), &outseq[0]));	/* get the command sequence to describe, change it to something we can print as well */
	ostring(" ");

	if ((ptr = getfname(getbind(c))) == NULL)		/* find the right ->function */
		ptr = "Not Bound";

	ostring(ptr);						/* output the command sequence */
	return(TRUE);
}

int bindtokey( int f, int n )					/* bindtokey:	add a new key to the key binding table		*/
{
	register unsigned int c;/* command key to bind */
	register int ( *kfunc)();/* ptr to the requested function to bind to */
	register KEYTAB *ktp;	/* pointer into the command table */
	register int found;	/* matched command flag */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* prompt the user to type in a key to bind & get the function name to bind it to */
	kfunc = getname(TEXT15);	/* ": bind-to-key " */
	if (kfunc == NULL) {
		mlwrite(TEXT16);	/* "[No such function]" */
		return(FALSE);
	}
	if (clexec == FALSE) {
		ostring(" ");
		TTflush();
	}

	c = getckey((kfunc == meta) || (kfunc == cex) ||		/* get the command sequence to bind */
		    (kfunc == unarg) || (kfunc == ctrlg));

	if (clexec == FALSE) {
		ostring(cmdstr(c, &outseq[0]));				/* change it to something we can print as and dump it out */
	}

	if (kfunc == unarg || kfunc == ctrlg || kfunc == quote) {	/* if the function is a unique prefix key */
		ktp = &keytab[0];					/* search for an existing binding for the prefix key */
		while (ktp->k_type != BINDNUL) {
			if (ktp->k_type == BINDFNC && ktp->k_ptr.fp == kfunc)
				unbindchar(ktp->k_code);
			++ktp;
		}
		if (kfunc == unarg)					/* reset the appropriate global prefix variable */
			reptc = c;
		if (kfunc == ctrlg)
			abortc = c;
		if (kfunc == quote)
			quotec = c;
	}

	ktp = &keytab[0];						/* search the table to see if it exists */
	found = FALSE;
	while (ktp->k_type != BINDNUL) {
		if ((unsigned int)ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (found) {							/* it exists, just change it then */
		ktp->k_ptr.fp = kfunc;
		ktp->k_type = BINDFNC;
	} else {							/* otherwise we need to add it to the end */
		if (ktp >= &keytab[NBINDS]) {				/* if we run out of binding room, bitch */
			mlwrite(TEXT17);				/* "Binding table FULL!" */
			return(FALSE);
		}
		ktp->k_code = c;					/* add keycode */
		ktp->k_ptr.fp = kfunc;					/* and the function pointer */
		ktp->k_type = BINDFNC;					/* and the binding type */
		++ktp;							/* and make sure the next is null */
		ktp->k_code = 0;
		ktp->k_type = BINDNUL;
		ktp->k_ptr.fp = NULL;
	}

	if (kfunc == meta) {						/* if we have rebound the meta key, make the search terminators follow it */
		sterm = c;
		isterm = c;
	}

	return(TRUE);
}

int macrotokey( int f, int n )						/* macrotokey:	Bind a key to a macro in the key binding table */
{
	register unsigned int c;/* command key to bind */
	register BUFFER *kmacro;/* ptr to buffer of macro to bind to key */
	register KEYTAB *ktp;	/* pointer into the command table */
	register int found;	/* matched command flag */
	register int status;	/* error return */
	char outseq[80];	/* output buffer for keystroke sequence */
	char bufn[NBUFN];	/* buffer to hold macro name */

	/* get the buffer name to use */
	if ((status=mlreply(TEXT215, &bufn[1], NBUFN-2)) != TRUE)	/* ": macro-to-key " */
		return(status);

	/* build the responce string for later */
	strcpy(outseq, TEXT215);					/* ": macro-to-key " */
	strcat(outseq, &bufn[1]);

	bufn[0] = '[';							/* translate it to a buffer pointer */
	strcat(bufn, "]");
	if ((kmacro=bfind(bufn, FALSE, 0)) == NULL) {
		mlwrite(TEXT130);					/* "Macro not defined"*/
		return(FALSE);
	}

	strcat(outseq, " ");
	mlwrite(outseq);

	c = getckey(FALSE);						/* get the command sequence to bind */
	ostring(cmdstr(c, &outseq[0]));					/* change it to something we can print and dump it out */

	ktp = &keytab[0];						/* search the table to see if it exists */
	found = FALSE;
	while (ktp->k_type != BINDNUL) {
		if ((unsigned int)ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (found) {	/* it exists, just change it then */
		ktp->k_ptr.buf = kmacro;
		ktp->k_type = BINDBUF;
	} else {	/* otherwise we need to add it to the end */
		if (ktp >= &keytab[NBINDS]) {				/* if we run out of binding room, bitch */
			mlwrite(TEXT17);				/* "Binding table FULL!" */
			return(FALSE);
		}
		ktp->k_code = c;					/* add keycode */
		ktp->k_ptr.buf = kmacro;				/* and the function pointer */
		ktp->k_type = BINDBUF;					/* and the binding type */
		++ktp;							/* and make sure the next is null */
		ktp->k_code = 0;
		ktp->k_type = BINDNUL;
		ktp->k_ptr.fp = NULL;
	}

	return(TRUE);
}

int unbindkey( int f, int n )						/* unbindkey:	delete a key from the key binding table */
{
	register int c; 	/* command key to unbind */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* prompt the user to type in a key to unbind */
	mlwrite(TEXT18);						/* ": unbind-key " */

	/* get the command sequence to unbind */
	c = getckey(FALSE);		/* get a command sequence */

	ostring(cmdstr(c, &outseq[0]));					/* change it to something we can print and dump it out */
	if (unbindchar(c) == FALSE) {					/* if it isn't bound, bitch */
		mlwrite(TEXT19);					/* "[Key not bound]" */
		return(FALSE);
	}
	return(TRUE);
}

int unbindchar( int c )
{
	register KEYTAB *ktp;	/* pointer into the command table */
	register KEYTAB *sktp;	/* saved pointer into the command table */
	register int found;	/* matched command flag */

	ktp = &keytab[0];						/* search the table to see if the key exists */
	found = FALSE;
	while (ktp->k_type != BINDNUL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (!found)							/* if it isn't bound, bitch */
		return(FALSE);

	sktp = ktp;							/* save the pointer and scan to the end of the table */
	while (ktp->k_type != BINDNUL)
		++ktp;
	--ktp;								/* backup to the last legit entry */

	sktp->k_code = ktp->k_code;					/* copy the last entry to the current one */
	sktp->k_type = ktp->k_type;
	if (sktp->k_type == BINDFNC)
		sktp->k_ptr.fp	 = ktp->k_ptr.fp;
	else if (sktp->k_type == BINDBUF)
		sktp->k_ptr.buf   = ktp->k_ptr.buf;

	ktp->k_code = 0;						/* null out the last one */
	ktp->k_type = BINDNUL;
	ktp->k_ptr.fp = NULL;
	return(TRUE);
}

int desbind( int f, int n )						/* describe bindings */
{
	return(buildlist(TRUE, ""));
}

int apro( int f, int n )	/* Apropos (List functions that match a substring) */
{
	char mstring[NSTRING];	/* string to match cmd names to */
	int status;		/* status return */

	status = mlreply(TEXT20, mstring, NSTRING - 1);			/* "Apropos string: " */
	if (status != TRUE)
		return(status);

	return(buildlist(FALSE, mstring));
}

int buildlist( int type, char *mstring )  /* build a binding list (limited or full) */
{
	register KEYTAB *ktp;	/* pointer into the command table */
	register NBIND *nptr;	/* pointer into the name binding table */
	register BUFFER *listbuf;/* buffer to put binding list into */
	register BUFFER *bp;	/* buffer ptr for function scan */
	int cpos;		/* current position to use in outseq */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* get a buffer for the binding list */
	listbuf = bfind(TEXT21, TRUE, BFINVS);				/* "Binding list" */
	if (listbuf == NULL || bclear(listbuf) == FALSE) {
		mlwrite(TEXT22);					/* "Can not display binding list" */
		return(FALSE);
	}

	mlwrite(TEXT23);						/* "[Building binding list]" */

	nptr = &names[0];						/* build the contents of this window, inserting it line by line */
	while (nptr->n_func != NULL) {
		strcpy(outseq, nptr->n_name);				/* add in the command name */
		cpos = strlen(outseq);

		if (type == FALSE &&					/* if we are executing an apropos command and current string doesn't include the search string */
		    strinc(outseq, mstring) == FALSE)
			goto fail;

		ktp = &keytab[0];					/* search down any keys bound to this */
		while (ktp->k_type != BINDNUL) {
			if (ktp->k_type == BINDFNC &&
			    ktp->k_ptr.fp == nptr->n_func) {		/* pad out some spaces */
				while (cpos < 25)
					outseq[cpos++] = ' ';

				cmdstr(ktp->k_code, &outseq[cpos]);	/* add in the command sequence */
				if (addline(listbuf, outseq) != TRUE)	/* and add it as a line into the buffer */
					return(FALSE);

				cpos = 0;				/* and clear the line */
			}
			++ktp;
		}

		if (cpos > 0) {						/* if no key was bound, we need to dump it anyway */
			outseq[cpos] = 0;
			if (addline(listbuf, outseq) != TRUE)
				return(FALSE);
		}

fail:		/* and on to the next name */
		++nptr;
	}

	if (addline(listbuf, "") != TRUE)				/* add a blank line between the key and macro lists */
		return(FALSE);

	bp = bheadp;							/* scan all buffers looking for macroes and their bindings */
	while (bp) {

		if (bp->b_bname[0] != '[')				/* is this buffer a macro? */
			goto bfail;

		strcpy(outseq, bp->b_bname);				/* add in the command name */
		cpos = strlen(outseq);

		if (type == FALSE &&					/* if we are executing an apropos command..... */
		    strinc(outseq, mstring) == FALSE)			/* and current string doesn't include the search string */
			goto bfail;

		ktp = &keytab[0];					/* search down any keys bound to this macro */
		while (ktp->k_type != BINDNUL) {
			if (ktp->k_type == BINDBUF &&
			    ktp->k_ptr.buf == bp) {
				/* padd out some spaces */
				while (cpos < 25)
					outseq[cpos++] = ' ';

				cmdstr(ktp->k_code, &outseq[cpos]);	/* add in the command sequence */
				if (addline(listbuf, outseq) != TRUE)	/* and add it as a line into the buffer */
					return(FALSE);

				cpos = 0;	/* and clear the line */
			}
			++ktp;
		}

		if (cpos > 0) {						/* if no key was bound, we need to dump it anyway */
			outseq[cpos] = 0;
			if (addline(listbuf, outseq) != TRUE)
				return(FALSE);
		}

bfail:		/* and on to the next buffer */
		bp = bp->b_bufp;
	}

	wpopup(listbuf);
	mlerase();	/* clear the mode line */
	return(TRUE);
}

int strinc( char *source, char *sub ) /* does source include sub? */
{
	char *sp;	/* ptr into source */
	char *nxtsp;	/* next ptr into source */
	char *tp;	/* ptr into substring */

	sp = source;							/* for each character in the source string */
	while (*sp) {
		tp = sub;
		nxtsp = sp;
		while (*tp) {						/* is the substring here? */
			if (*nxtsp++ != *tp)
				break;
			else
				tp++;
		}
		if (*tp == 0)						/* yes, return a success */
			return(TRUE);
		sp++;							/* no, onward */
	}
	return(FALSE);
}

unsigned int  getckey( int mflag )					/* get a command key sequence from the keyboard */
{
	register unsigned int c;	/* character fetched */
	char tok[NSTRING];		/* command incoming */

	if (clexec) {							/* check to see if we are executing a command line */
		macarg(tok);	/* get the next token */
		return(stock(tok));
	}

	if (mflag)							/* or the normal way */
		c = getkey();
	else
		c = getcmd();
	return(c);
}

int startup( char *sfname )						/* execute the startup file */
{
	char *fname;	/* resulting file name to execute */
	char name[NSTRING];	/* name with extention */

	if (*sfname != 0) {						/* look up the startup file */
		strcpy(name, sfname);	 				/* default the extention */
		if (sindex(name, ".") == 0)
			strcat(name, ".cmd");
		fname = flook(name, TRUE);
	} else
		fname = flook((char *)pathname[0], TRUE);

	if (fname == NULL)						/* if it isn't around, don't sweat it */
		return(TRUE);

	printf( "Using startup file: %s\n", fname );
	return(dofile(fname));						/* otherwise, execute the sucker */
}

/*	Look up the existance of a file along the normal or PATH
	environment variable.

	LOOKUP ORDER:

		if contains path:

			absolute

		else

			HOME environment directory
			all directories along PATH environment
			directories in table from EPATH.H
*/

char * flook( char *fname, int hflag )
{
	register char *home;	/* path to home directory */
	register char *path;	/* environmental PATH variable */
	register char *sp;	/* pointer into path spec */
	register int i; 	/* index */
	static char fspec[NFILEN];	/* full path spec to search */

	sp = fname;			/* if we have an absolute path.. check only there! */
	while (*sp) {
		if (*sp == ':' || *sp == '\\' || *sp == '/') {
			if (ffropen(fname) == FIOSUC) {
				ffclose();
				return(fname);
			} else
				return(NULL);
		}
		++sp;
	}

	if (hflag) {
		home = getenv("HOME");
		if (home != NULL) {					/* build home dir file spec */
			strcpy(fspec, home);
			strcat(fspec, DIRSEPSTR);
			strcat(fspec, fname);
			if (ffropen(fspec) == FIOSUC) {			/* and try it out */
				ffclose();
				return(fspec);
			}
		}
	}

	if (ffropen(fname) == FIOSUC) {					/* current directory now overides everything except HOME var */
		ffclose();
		return(fname);
	}

	path = getenv("PATH");						/* get the PATH variable */
	if (path != NULL)
		while (*path) {
			sp = fspec;					/* build next possible file spec */
			while (*path && (*path != PATHCHR))
				*sp++ = *path++;
			if ((sp != fspec) && (*(sp-1) != DIRSEPCHAR))		/* add a terminating dir separator if we need it */
				*sp++ = DIRSEPCHAR;
			*sp = 0;
			strcat(fspec, fname);
			if (ffropen(fspec) == FIOSUC) {			/* and try it out */
				ffclose();
				return(fspec);
			}
			if (*path == PATHCHR)
				++path;
		}

	for (i=2; i < (int)NPNAMES; i++) {				/* look it up via the old table method */
		strcpy(fspec, pathname[i]);
		strcat(fspec, fname);
		if (ffropen(fspec) == FIOSUC) {				/* and try it out */
			ffclose();
			return(fspec);
		}
	}

	return(NULL);	/* no such luck */
}

char * cmdstr( int c, char *seq )					/* Change a key command to a string we can print out.  Return the string passed in. */
{
	char *ptr;	/* pointer into current position in sequence */

	ptr = seq;

	if (c & CTLX) {							/* apply ^X sequence if needed */
		*ptr++ = '^';
		*ptr++ = 'X';
	}

	if (c & ALTD) {							/* apply ALT key sequence if needed */
		*ptr++ = 'A';
		*ptr++ = '-';
	}


	if (c & SHFT) {							/* apply Shifted sequence if needed */
		*ptr++ = 'S';
		*ptr++ = '-';
	}

	if (c & MOUS) {							/* apply MOUS sequence if needed */
		*ptr++ = 'M';
		*ptr++ = 'S';
	}

	if (c & META) {							/* apply meta sequence if needed */
		*ptr++ = 'M';
		*ptr++ = '-';
	}

	if (c & SPEC) {							/* apply SPEC sequence if needed */
		*ptr++ = 'F';
		*ptr++ = 'N';
	}

	if (c & CTRL) {							/* apply control sequence if needed */
		if (ptr == seq && ((c & 255) == ' '))			/* non normal spaces look like @ */
			c = '@';
		*ptr++ = '^';
	}

	c = c & 255;	/* strip the prefixes */
	*ptr++ = c;	/* and output the final sequence */
	*ptr = 0;	/* terminate the string */
	return (seq);
}

KEYTAB *getbind( int c )			/*	This function looks a key binding up in the binding table	*/
{
	register KEYTAB *ktp;

	ktp = &keytab[0];			/* scan through the binding table, looking for the key's entry */
	while (ktp->k_type != BINDNUL) {
		if (ktp->k_code == c)
			return(ktp);
		++ktp;
	}

	return((KEYTAB *)NULL);			/* no such binding */
}

char * getfname( KEYTAB *key )			/* getfname:	This function takes a ptr to KEYTAB entry and gets the name associated with it */
{
	int ( *func)(); /* ptr to the requested function */
	register NBIND *nptr;	/* pointer into the name binding table */
	register BUFFER *bp;	/* ptr to buffer to test */
	register BUFFER *kbuf;	/* ptr to requested buffer */

	if (key == NULL)			/* if this isn't a valid key, it has no name */
		return(NULL);

	if (key->k_type == BINDFNC) {		/* skim through the binding table, looking for a match */
		func = key->k_ptr.fp;
		nptr = &names[0];
		while (nptr->n_func != NULL) {
			if (nptr->n_func == func)
				return(nptr->n_name);
			++nptr;
		}
		return(NULL);
	}

	if (key->k_type == BINDBUF) {		/* skim through the buffer list looking for a match */
		kbuf = key->k_ptr.buf;
		bp = bheadp;
		while (bp) {
			if (bp == kbuf)
				return(bp->b_bname);
			bp = bp->b_bufp;
		}
		return(NULL);
	}
	return(NULL);
}

/* fncmatch: match fname to a function in the names table and return any match or NULL if none */
int ( * fncmatch(char *fname))(int, int)
{
	int nval;

	if ((nval = binary(fname, namval, numfunc)) == -1)
		return(NULL);
	else
		return(names[nval].n_func);
}

char * namval( int index )
{
	return(names[index].n_name);
}

/*	stock() 	String key name TO Command Key

	A key binding consists of one or more prefix functions followed by
	a keystroke.  Allowable prefixes must be in the following order:

	^X	preceeding control-X
	A-	similtaneous ALT key (on PCs mainly)
	S-	shifted function key
	MS	mouse generated keystroke
	M-	Preceding META key
	FN	function key
	^	control key

	Meta and ^X prefix of lower case letters are converted to upper
	case.  Real control characters are automatically converted to
	the ^A form.
*/

unsigned int stock( char *keyname )
{
	register unsigned int c;	/* key sequence to return */

	c = 0;				/* parse it up */

	if(*keyname == '^' && *(keyname+1) == 'X') {				/* Do ^X prefix */
		if(*(keyname+2) != 0) { /* Key is not bare ^X */
		    c |= CTLX;
		    keyname += 2;
		}
	}

	if (*keyname == 'A' && *(keyname+1) == '-') {			/* and the ALT key prefix */
		c |= ALTD;
		keyname += 2;
	}

	if (*keyname == 'S' && *(keyname+1) == '-') {			/* and the SHIFTED prefix */
		c |= SHFT;
		keyname += 2;
	}

	if (*keyname == 'M' && *(keyname+1) == 'S') {			/* and the mouse (MOUS) prefix */
		c |= MOUS;
		keyname += 2;
	}

	if (*keyname == 'M' && *(keyname+1) == '-') {			/* then the META prefix */
		c |= META;
		keyname += 2;
	}

	if (*keyname == 'F' && *(keyname+1) == 'N') {			/* next the function prefix */
		c |= SPEC;
		keyname += 2;
	}

	if (*keyname == '^' && *(keyname+1) != 0) {			/* a control char?  (NOT Always upper case anymore) */
		c |= CTRL;
		++keyname;
		if (*keyname == '@')
			*keyname = ' ';
	}

	if (*keyname < 32) {						/* A literal control character? (Boo, hiss) */
		c |= CTRL;
		*keyname += '@';
	}

	/* make sure we are not lower case if used with ^X or M- */
	if(!(c & (MOUS|SPEC|ALTD|SHFT)))	/* If not a special key */
	    if( c & (CTLX|META))		/* If is a prefix */
		uppercase((unsigned char *)keyname);		/* Then make sure it's upper case */

	c |= *keyname;						/* the final sequence... */
	return(c);
}

char * transbind( char *skey )	/* string key name to binding name.... */
{
	char *bindname;

	bindname = getfname(getbind(stock(skey)));
	if (bindname == NULL)
		bindname = (char *)errorm;

	return(bindname);
}

int  execkey( KEYTAB *key, int f, int n )	/* execute a function bound to a key */
{
	register int status;	/* error return */

	if (key->k_type == BINDFNC)
		return((*(key->k_ptr.fp))( f, n ));
	if (key->k_type == BINDBUF) {
		while (n--) {
			status = dobuf(key->k_ptr.buf);
			if (status != TRUE)
				return(status);
		}
	}
	return(TRUE);
}

int xsetkey( KEYTAB *key, char *name )		/* set a KEYTAB to the given name of the given type */
{
	int ( *ktemp)(int, int);	/* temp function pointer to assign */
	register BUFFER *kmacro;	/* ptr to buffer of macro to bind to key */
	char bufn[NBUFN];		/* buffer to hold macro name */

	if (*name == 0) {				/* are we unbinding it? */
		key->k_type = BINDNUL;
		return(TRUE);
	}

	if ((ktemp = fncmatch(name)) != NULL) {		/* bind to a built in function? */
		key->k_ptr.fp = ktemp;
		key->k_type = BINDFNC;
		return(TRUE);
	}

	strcpy(bufn, "[");				/* is it a procedure/macro? */
	strcat(bufn, name);
	strcat(bufn, "]");
	if ((kmacro=bfind(bufn, FALSE, 0)) != NULL) {
		key->k_ptr.buf = kmacro;
		key->k_type = BINDBUF;
		return(TRUE);
	}

	mlwrite(TEXT16);				/* "[No such function]" */		/* not anything we can bind to */
	return(FALSE);
}
