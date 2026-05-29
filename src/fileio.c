/* fileio.c
 *
 * low level file i/o routines
 *
 * The routines in this file read and write ASCII files from the disk. All of
 * the knowledge about file handling resides here.
 */

#include <stdio.h>
#include "estruct.h"
#include "eproto.h"
#include "edef.h"
#include "english.h"

FILE *ffp;			/* File pointer, all functions. */
static int eofflag;		/* end-of-file flag */

int ffropen( char *fn )		/* Open a file for reading. */
{
	if ((ffp=fopen(fn, "r")) == NULL)
		return(FIOFNF);
	eofflag = FALSE;
	return(FIOSUC);
}

int ffwopen( char *fn, char *mode)		/* Open a file for writing. Return TRUE if all is well, and FALSE on error (cannot create). */
{
	char xmode[6];		/* extended file open mode */

	/* nonstandard line terminators? */
	if (*lterm) {				/* open in binary mode */
		strcpy(xmode, mode);
		strcat(xmode, "b");
		ffp = fopen(fn, xmode);
	} else {				/* open in ascii(text) mode */
		ffp = fopen(fn, mode);
	}

	if (ffp == NULL) {
		mlwrite(TEXT155);		/* "Cannot open file for writing" */
		return(FIOERR);
	}

	return(FIOSUC);
}

int ffclose()		/* Close a file. Should look at the status in all systems. */
{
	if (fline) {		/* free this since we do not need it anymore */
		free(fline);
		fline = NULL;
	}
        if (fclose(ffp) != FALSE) {
                mlwrite(TEXT156);		/* "Error closing file" */
                return(FIOERR);
        }
        return(FIOSUC);
}

int ffputline( char buf[], int nbuf )
{
        register int i;		/* index into line to write */
	register char *lptr;	/* ptr into the line terminator */
        for (i = 0; i < nbuf; ++i)
                putc(buf[i], ffp);

	/* write out the appropriate line terminator(s) */
	if (*lterm) {
		lptr = &lterm[0];
		while (*lptr)
			putc(*lptr++, ffp);
	} else {
	        putc('\n', ffp);
	}

        if (ferror(ffp)) {			/* check for write errors */
                mlwrite(TEXT157);		/* "Write I/O error" */
                return(FIOERR);
        }

        return(FIOSUC);
}

int ffgetline( int *nbytes )
{
        register int c;		/* current character read */
        register int i;		/* current index into fline */

	/* if we are at the end...return it */
	if (eofflag)
		return(FIOEOF);

	/* dump fline if it ended up too big */
	if (flen > NSTRING && fline != NULL) {
		free(fline);
		fline = NULL;
	}

	if (fline == NULL)							/* if we don't have an fline, allocate one */
		if ((fline = malloc(flen = NSTRING)) == NULL)
			return(FIOMEM);

        i = 0;
        while ((c = getc(ffp)) != EOF && c != '\n') {				/* read the line in */
                fline[i++] = c;
		/* if it's longer, get more room */
                if (i >= flen) {
			flen *= 2;
			if ((fline = realloc(fline, flen)) == NULL) {
			    return(FIOMEM);
			}
		}
	}

	while (i > 0 && (fline[i-1] == 10 || fline[i-1] == 13))			/* dump any extra line terminators at the end */
		i--;

	*nbytes = i;								/* and save the length for our caller... */

        if (c == EOF) {								/* test for any errors that may have occured */
                if (ferror(ffp)) {
                        mlwrite(TEXT158);					/* "File read error" */
                        return(FIOERR);
                }
                if (i != 0)
			eofflag = TRUE;
		else
			return(FIOEOF);
        }

        fline[i] = 0;								/* terminate the string */
        return(FIOSUC);
}

int fexist( char *fname )	/* does <fname> exist on disk? */
{
	FILE *fp;

	fp = fopen(fname, "r");							/* try to open the file for reading */
	if (fp == NULL)								/* if it fails, just return false! */
		return(FALSE);

	fclose(fp);								/* otherwise, close it and report true */
	return(TRUE);
}
