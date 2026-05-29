/* file.c
 *
 * The routines in this file handle the reading, writing and lookup of disk
 * files. All of details about the reading and writing of the disk are in
 * "fileio.c".
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int fileread( int f, int n )
{
	char *fname;	/* file name to read */

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());
	if ((fname = gtfilename(TEXT131)) == NULL)		/* "Read file" */
		return(FALSE);
	return(readin(fname, TRUE));
}

int insfile( int f, int n )
{
	register int	s;
	char *fname;	/* file name */
	LINE *curline;

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());
	if (curbp->b_mode&MDVIEW)      /* don't allow this command if  */
		return(rdonly());	/* we are in read only mode	*/

	if ((fname = gtfilename(TEXT132)) == NULL) 		/* "Insert file" */
		return(FALSE);
	/*
	 * Save the local pointers to hold global ".", in case
	 * $yankflag is set to 1.  Insert-file always places the
	 * starting offset point at 0.  Hold *previous* line
	 * position, since the current line may be re-allocated.
	 */
	if (yankflag)
		curline = lback(curwp->w_dotp);

	s = ifile(fname);

	if (yankflag)
		curwp->w_dotp = lforw(curline);

	return (s);
}

int filefind( int f, int n )
{
	char *fname;	/* file user wishes to find */	/* file name */

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());
	if ((fname = gtfilename(TEXT133)) == NULL) 	/* "Find file" */
		return(FALSE);
	return(getfile(fname, TRUE));
}

int viewfile( int f, int n )	/* visit a file in VIEW mode */
{
	char *fname;	/* file user wishes to find */	/* file name */
	register int s;	/* status return */

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());
	if ((fname = gtfilename(TEXT134)) == NULL) 	/* "View file" */
		return(FALSE);
	s = getfile(fname, FALSE);
	if (s) {	/* if we succeed, put it in view mode */
		curwp->w_bufp->b_mode |= MDVIEW;
		upmode();
	}
	return(s);
}

int getfile( char fname[], int lockfl )
{
	register BUFFER *bp;
	register LINE	*lp;
	register int	i;
	register int	s;
	char bname[NBUFN];	/* buffer name to put file */
	char prompt[NSTRING];	/* string for collisions prompt */

	for (bp=bheadp; bp!=NULL; bp=bp->b_bufp) {
		if ((bp->b_flag&BFINVS)==0 && strcmp(bp->b_fname, fname)==0) {
			swbuffer(bp);
			lp = curwp->w_dotp;
			i = curwp->w_ntrows/2;
			while (i-- && lback(lp)!=curbp->b_linep)
				lp = lback(lp);
			curwp->w_linep = lp;
			curwp->w_flag |= WFMODE|WFHARD;
			mlwrite(TEXT135);		/* "[Old buffer]" */
			return(TRUE);
		}
	}
	makename(bname, fname); 			/* New buffer name.	*/

	/* prevent buffer name conflicts */
	while ((bp=bfind(bname, FALSE, 0)) != NULL) {
		unqname(bname);				/* first, come up with our own name */
		if (clexec == FALSE) {			/* if interactive, let em change it if they dislike our names */
			strcpy(prompt, TEXT136);	/* "Buffer name: " */
			strcpy(&prompt[strlen(prompt) - 2], "[");
			strcat(prompt, bname);
			strcat(prompt, "]: ");
			s = mlreply(prompt, bname, NBUFN);

			if (s == ABORT) 	/* ^G to just quit	*/
				return(s);
			if (s == FALSE) {	/* CR to let the computer pick */
				makename(bname, fname);	/* New buffer name. */
				unqname(bname);		/* which is unique */
			}
		}
	}

	/* create the new buffer */
	if (bp==NULL && (bp=bfind(bname, TRUE, 0))==NULL) {
		mlwrite(TEXT137);		/* "Cannot create buffer" */
		return(FALSE);
	}

	swbuffer(bp);			/* switch to the new buffer */
	return(readin(fname, lockfl));	/* Read it in.		*/
}

int  expandlink( char *fname, char *linkedfile, size_t linksize )
{
    char tmplink[1024];
    struct stat st;		/* we need info about the file permissions */
    int index,iter;
    ssize_t rl;

    iter = 0;
    strlcpy(tmplink, fname, sizeof(tmplink));

    while (TRUE) {

        /* avoid infinite loops on cyclic symbolic links */
	if (iter>=32) {
           return(-1);
	}

        /* get the permissions on the file */
	/* return if file does not exist (creating new file?) */
        if (lstat(tmplink, &st) != 0) {
           strlcpy(linkedfile, tmplink, linksize);
           return(0);
	}
        /* also return if file is not a symbolic link */
        if(!S_ISLNK(st.st_mode)) {
           strlcpy(linkedfile, tmplink, linksize);
           return((int)st.st_mode & (int)S_IFMT);
	}
        /* otherwise iterate */
        ++iter ;

        /* get name of file pointed to */
        rl = readlink(tmplink, linkedfile, linksize - 1);
        if (rl < 0) return(-1);
        linkedfile[rl] = '\0' ;

        if (*linkedfile != '/' ) {
           index = strlen(tmplink);
           while (index > 0) {
             --index;
	     if (tmplink[index] == '/') index = 0; else tmplink[index] = '\0';
           }
        strlcat(tmplink, linkedfile, sizeof(tmplink));
	}
        else strlcpy(tmplink, linkedfile, sizeof(tmplink));
    }
}

int readin( char fname[], int lockfl )
{
	register LINE *lp1;
	register LINE *lp2;
	register int i;
	register WINDOW *wp;
	register BUFFER *bp;
	register int s;
	register int nline;
	register int cmark;	/* current mark */
	int nbytes;
	char mesg[NSTRING];

        char fnlink[1024];

        s = expandlink(fname, fnlink, sizeof(fnlink));

        if ((s == 0) || (s == S_IFREG)) {
           strlcpy(fname, fnlink, NFILEN);
           makename(curbp->b_bname, fnlink);
	} else {
        if (s == S_IFDIR ) {
            /* Path may be much longer than NBUFN (32); cap the %s precision
             * so snprintf is provably bounded and the compiler does not warn
             * about possible truncation. */
            snprintf(curbp->b_bname, NBUFN, "? %.*s/", NBUFN - 4, fnlink);
 	  } else {
	    /* Escape the leading "??" to avoid forming the ??! trigraph. */
	    strlcpy(curbp->b_bname, "?\?!!??", NBUFN);
	  }
        fname[0] = '\0';
        return(ABORT);
	}

	bp = curbp;				/* Cheap.		*/
	if ((s=bclear(bp)) != TRUE)		/* Might be old.	*/
		return(s);
	bp->b_flag &= ~(BFINVS|BFCHG);
	if (bp->b_fname != fname)
		strcpy(bp->b_fname, fname);

	/* let a user macro get hold of things...if he wants */
	execkey(&readhook, FALSE, 1);

	/* turn off ALL keyboard translation in case we get a dos error */
	TTkclose();

	if ((s=ffropen(fname)) == FIOERR)	/* Hard file open.	*/
		goto out;

	if (s == FIOFNF) {			/* File not found.	*/
		mlwrite(TEXT138);		/* "[New file]"		*/
		goto out;
	}

	/* read the file in */
	mlwrite(TEXT139);			/* "[Reading file]"	*/
	nline = 0;
	while ((s=ffgetline(&nbytes)) == FIOSUC) {
		if ((lp1=lalloc(nbytes)) == NULL) {
			s = FIOMEM;		/* Keep message on the	*/
			break;			/* display.		*/
		}
		lp2 = lback(curbp->b_linep);
		lp2->l_fp = lp1;
		lp1->l_fp = curbp->b_linep;
		lp1->l_bp = lp2;
		curbp->b_linep->l_bp = lp1;
		for (i=0; i<nbytes; ++i)
			lputc(lp1, i, fline[i]);
		++nline;
	}
	ffclose();				/* Ignore errors.	*/
	strcpy(mesg, "[");
	if (s==FIOERR) {
		strcat(mesg, TEXT141);		/* "I/O ERROR, "	*/
		curbp->b_flag |= BFTRUNC;
	}
	if (s == FIOMEM) {
		strcat(mesg, TEXT142);		/* "OUT OF MEMORY, "	*/
		curbp->b_flag |= BFTRUNC;
	}
	strcat(mesg, TEXT140);			/* "Read "		*/
	strcat(mesg, int_asc(nline));
	strcat(mesg, TEXT143);			/* " line"		*/
	if (nline > 1)
		strcat(mesg, "s");
	strcat(mesg, "]");
	mlwrite(mesg);

out:
	TTkopen();	/* open the keyboard again */
	for (wp=wheadp; wp!=NULL; wp=wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			wp->w_linep = lforw(curbp->b_linep);
			wp->w_dotp  = lforw(curbp->b_linep);
			wp->w_doto  = 0;
			for (cmark = 0; cmark < NMARKS; cmark++) {
				wp->w_markp[cmark] = NULL;
				wp->w_marko[cmark] = 0;
			}
			wp->w_flag |= WFMODE|WFHARD;
		}
	}
	if (s == FIOERR || s == FIOFNF) 	/* False if error.	*/
		return(FALSE);
	return(TRUE);
}

char * makename( char *bname, char *fname )
{
	register char *cp1;
	register char *cp2;
	register char *pathp;

	cp1 = &fname[0];
	while (*cp1 != 0)
		++cp1;

	while (cp1>&fname[1] && cp1[-1]=='/')
		{
                --cp1;
                *cp1 = '\0';
		}
	while (cp1!=&fname[0] && cp1[-1]!='/')
		--cp1;
	/* cp1 is pointing to the first real filename char */
	pathp = cp1;

	cp2 = &bname[0];
	while (cp2!=&bname[NBUFN-1] && *cp1!=0 && *cp1!=';')
		*cp2++ = *cp1++;
	*cp2 = 0;

	return(pathp);
}

void unqname( char *name )	/* make sure a buffer name is unique */
{
	register char *sp;

	/* check to see if it is in the buffer list */
	while (bfind(name, 0, FALSE) != NULL) {

		/* go to the end of the name */
		sp = name;
		while (*sp)
			++sp;
		if (sp == name || (*(sp-1) <'0' || *(sp-1) > '8')) {
			*sp++ = '0';
			*sp = 0;
		} else
		      *(--sp) += 1;
	}
}

int filewrite( int f, int n )
{
	register int s;
	char *fname;

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());

	if ((fname = gtfilename(TEXT144)) == NULL)		/* "Write file: " */
		return(FALSE);
	if ((s=writeout(fname, "w")) == TRUE) {
		strcpy(curbp->b_fname, fname);
		curbp->b_flag &= ~BFCHG;
		/* Update mode lines.	*/
		upmode();
	}
	return(s);
}

int fileapp( int f, int n )	/* append file */
{
	register int s;
	char *fname;

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());
	if ((fname = gtfilename(TEXT218)) == NULL)		/* "Append file: " */
		return(FALSE);
	if ((s=writeout(fname, "a")) == TRUE) {
		curbp->b_flag &= ~BFCHG;
		/* Update mode lines.	*/
		upmode();
	}
	return(s);
}

int filesave( int f, int n )
{
	register int s;

	if (curbp->b_mode&MDVIEW)	/* don't allow this command if	*/
		return(rdonly());	/* we are in read only mode	*/
	if ((curbp->b_flag&BFCHG) == 0) 	/* Return, no changes.	*/
		return(TRUE);
	if (curbp->b_fname[0] == 0) {		/* Must have a name.	*/
		mlwrite(TEXT145);	/* "No file name" */
		return(FALSE);
	}

	/* complain about truncated files */
	if ((curbp->b_flag&BFTRUNC) != 0) {
		if (mlyesno(TEXT146) == FALSE) {	/* "Truncated file..write it out" */
			mlwrite(TEXT8);			/* "[Aborted]" */
			return(FALSE);
		}
	}

	/* complain about narrowed buffers */
	if ((curbp->b_flag&BFNAROW) != 0) {
		if (mlyesno(TEXT147) == FALSE) {	/* "Narrowed Buffer..write it out" */
			mlwrite(TEXT8);			/* "[Aborted]" */
			return(FALSE);
		}
	}

	if ((s=writeout(curbp->b_fname, "w")) == TRUE) {
		curbp->b_flag &= ~BFCHG;
		/* Update mode lines.	*/
		upmode();
	}
	return(s);
}

/*
 * This function performs the details of file writing. It uses
 * the file management routines in the "fileio.c" package. The
 * number of lines written is displayed. Several errors are
 * posible, and cause writeout to return a FALSE result. When
 * $ssave is TRUE,  the buffer is written out to a temporary
 * file, and then the old file is unlinked and the temporary
 * renamed to the original name.  Before the file is written,
 * a user specifyable routine (in $writehook) can be run.
 */

int writeout( char *fn, char *mode)
{
	register LINE *lp;	/* line to scan while writing */
	register char *sp;	/* temporary string pointer */
	register int nline;	/* number of lines written */
	int status;		/* return status */
	int sflag;		/* are we safe saving? */
	char tname[NSTRING];	/* temporary file name */
	char buf[NSTRING];	/* message buffer */
	struct stat st;		/* we need info about the file permissions */
        int fflag;              /* file flag (regular file?) */
        char fnbak[NFILEN];       /* name of bak file */

	/* let a user macro get hold of things...if he wants */
	execkey(&writehook, FALSE, 1);

	/* determine if we will use the save method */
	sflag = FALSE;
	if (ssave && fexist(fn) && *mode == 'w')
		sflag = TRUE;

	/* turn off ALL keyboard translation in case we get a dos error */
	TTkclose();

	/* Perform Safe Save..... */
	if (sflag) {
		/* duplicate original file name, and find where to trunc it */
		sp = tname + (makename(tname, fn) - fn) + 1;
		strcpy(tname, fn);

		/* create a unique name, using random numbers */
		do {
			*sp = 0;
			strcat(tname, int_asc(ernd() & 0xffff));
		} while(fexist(tname));

		/* open the temporary file */
		status = ffwopen(tname, "w");
	} else
		status = ffwopen(fn, mode);

	/* if the open failed.. clean up and abort */
	if (status != FIOSUC) {
		TTkopen();
		return(FALSE);
	}

	/* write the current buffer's lines to the open disk file */
	mlwrite(TEXT148);	/* tell us that we're writing */	/* "[Writing...]" */
	lp = lforw(curbp->b_linep);	/* start at the first line.	*/
	nline = 0;			/* track the Number of lines	*/
	while (lp != curbp->b_linep) {
		if ((status = ffputline(&lp->l_text[0], lused(lp))) != FIOSUC)
			break;
		++nline;
		lp = lforw(lp);
	}

	/* report on status of file write */
	*buf = 0;
	status |= ffclose();
	if (status == FIOSUC) {
		/* report on success (or lack therof) */
		strcpy(buf, TEXT149);		/* "[Wrote " */
		strcat(buf, int_asc(nline));
		strcat(buf, TEXT143);		/* " line" */
		if (nline > 1)
			strcat(buf, "s");

		if (sflag) {
                        fflag = expandlink(fn, fnbak, sizeof(fnbak));
                        if (fflag == 0) {
                           strcpy(fn, fnbak);
                           fflag = rename(tname, fn);
			} else if (fflag == S_IFREG) {
                           strcpy(fn, fnbak);
                           strcat(fnbak, "~");
                           unlink(fnbak);
                           fflag = (rename(fn, fnbak) == 0);
                           if (fflag) {
                               fflag = (rename(tname, fn) == 0);
			   } else {
                               fflag = (unlink(fn) == 0);
                               if (fflag) fflag = (rename(tname, fn) == 0);
			   }				   
			   if (fflag) {
                                stat(fnbak, &st);
				chown(fn, (int)st.st_uid, (int)st.st_gid);
				chmod(fn, (int)st.st_mode);
                                makename(curbp->b_bname, fn);
			   } else fflag = 0;
			}
                        if (fflag) { 
                           makename(curbp->b_bname, fn);
			} else {
				strcat(buf, TEXT150);	/* ", saved as " */
				strcat(buf, tname);
                                strcpy(fn, tname);
                                strcpy(curbp->b_bname, tname);
				status = FIOSUC;    
                                   /* but failed under original name! */
			}
		}
		strcat(buf, "]");
		mlwrite(buf);
	}

	/* reopen the keyboard, and return our status */
	TTkopen();
	return(status == FIOSUC);
}

int filename( int f, int n )
{
	register int	s;
	char		fname[NFILEN];

	if (restflag)		/* don't allow this command if restricted */
		return(resterr());
	if ((s=FILENAMEREPLY(TEXT151, fname, NFILEN)) == ABORT)		/* "Name: " */
		return(s);
	if (s == FALSE)
		strcpy(curbp->b_fname, "");
	else
		strcpy(curbp->b_fname, fname);
	/* Update mode lines.	*/
	upmode();
	curbp->b_mode &= ~MDVIEW;      /* no longer read only mode */
	return(TRUE);
}

int ifile( char fname[] )
{
	register LINE *lp0;
	register LINE *lp1;
	register LINE *lp2;
	register int i;
	register BUFFER *bp;
	register int s;
	register int nline;
	int nbytes;
	int cmark;	/* current mark */
	char mesg[NSTRING];

	bp = curbp;				/* Cheap.		*/
	bp->b_flag |= BFCHG;			/* we have changed	*/
	bp->b_flag &= ~BFINVS;			/* and are not temporary*/
	if ((s=ffropen(fname)) == FIOERR)	/* Hard file open.	*/
		goto out;
	if (s == FIOFNF) {			/* File not found.	*/
		mlwrite(TEXT152);		/* "[No such file]" */
		return(FALSE);
	}
	mlwrite(TEXT153);			/* "[Inserting file]" */

	/* back up a line and save the mark here */
	curwp->w_dotp = lback(curwp->w_dotp);
	curwp->w_doto = 0;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		curwp->w_markp[cmark] = curwp->w_dotp;
		curwp->w_marko[cmark] = 0;
	}

	nline = 0;
	while ((s=ffgetline(&nbytes)) == FIOSUC) {
		if ((lp1=lalloc(nbytes)) == NULL) {
			s = FIOMEM;		/* Keep message on the	*/
			break;			/* display.		*/
		}
		lp0 = curwp->w_dotp;  /* line previous to insert */
		lp2 = lp0->l_fp;	/* line after insert */

		/* re-link new line between lp0 and lp2 */
		lp2->l_bp = lp1;
		lp0->l_fp = lp1;
		lp1->l_bp = lp0;
		lp1->l_fp = lp2;

		/* and advance and write out the current line */
		curwp->w_dotp = lp1;
		for (i=0; i<nbytes; ++i)
			lputc(lp1, i, fline[i]);
		++nline;
	}
	ffclose();				/* Ignore errors.	*/
	curwp->w_markp[0] = lforw(curwp->w_markp[0]);
	strcpy(mesg, "[");
	if (s==FIOERR) {
		strcat(mesg, TEXT141);		/* "I/O ERROR, " */
		curbp->b_flag |= BFTRUNC;
	}
	if (s == FIOMEM) {
		strcat(mesg, TEXT142);		/* "OUT OF MEMORY, " */
		curbp->b_flag |= BFTRUNC;
	}
	strcat(mesg, TEXT154);			/* "Inserted " */
	strcat(mesg, int_asc(nline));
	strcat(mesg, TEXT143);			/* " line" */
	if (nline > 1)
		strcat(mesg, "s");
	strcat(mesg, "]");
	mlwrite(mesg);

out:
	/* advance to the next line and mark the window for changes */
	curwp->w_dotp = lforw(curwp->w_dotp);
	curwp->w_flag |= WFHARD | WFMODE;

	/* copy window parameters back to the buffer structure */
	curbp->b_dotp = curwp->w_dotp;
	curbp->b_doto = curwp->w_doto;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		curbp->b_markp[cmark] = curwp->w_markp[cmark];
		curbp->b_marko[cmark] = curwp->w_marko[cmark];
	}
	curbp->b_fcol = curwp->w_fcol;

	if (s == FIOERR)			/* False if error.	*/
		return(FALSE);
	return(TRUE);
}

int showfiles( int f, int n )
{
	register BUFFER *dirbuf;/* buffer to put file list into */
	char outseq[NSTRING];	/* output buffer for file names */
	char *sp;		/* output ptr for file names */
	char mstring[NSTRING];	/* string to match cmd names to */
	int status;		/* status return */

	/* ask what directory mask to search */
	status = mlreply("Directory to show: ", mstring, NSTRING - 1);
	if (status == ABORT)
		return(status);

	/* get a buffer for the file list */
	dirbuf = bfind("File List", TRUE, BFINVS);
	if (dirbuf == NULL || bclear(dirbuf) == FALSE) {
		mlwrite("Can not display file list");		/* "Can not display function list" */
		return(FALSE);
	}

	/* let us know this is in progress */
	mlwrite("[Building File List]");

	/* get the first file name */
	sp = getffile(mstring);

	while (sp) {

		/* add a name to the buffer */
		strcpy(outseq, sp);
		if (addline(dirbuf, outseq) != TRUE)
			return(FALSE);

		/* and get the next name */
		sp = getnfile();
	}

	/* display the list */
	wpopup(dirbuf);
	mlerase();	/* clear the mode line */
	return(TRUE);
}
