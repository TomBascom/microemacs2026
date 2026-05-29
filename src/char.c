/* char.c
 *
 * functions to manipulate single characters
 *
 * from: MicroEMACS 5.03 (C)Copyright 1993 by Daniel Lawrence
 *
 */

#include	<stdio.h>
#include	"estruct.h"
#include	"eproto.h"
#include	"edef.h"
#include	"english.h"

#define DIFCASE 	0x20		/* DIFCASE represents the integer difference between upper and lower case letters.	*/

int isletter( unsigned int ch )
{
	return(is_upper(ch) || is_lower(ch));
}

int is_lower( unsigned int ch )
{
	return(lowcase[ch] != 0);
}

int is_upper( unsigned int ch )
{
	return(upcase[ch] != 0);
}

unsigned int chcase( unsigned int ch )
{
	if (is_lower(ch)) return(lowcase[ch]);
	if (is_upper(ch)) return(upcase[ch]);
	return(ch);		/* let the rest pass */
}

void  uppercase( unsigned char *cp )
{
	if (is_lower(*cp)) *cp = lowcase[*cp];
}

void  lowercase( unsigned char *cp )
{
	if (is_upper(*cp)) *cp = upcase[*cp];
}

int upperc( char ch ) /* return the upper case equivalant of a character */
{
	if (is_lower(ch))
		return(lowcase[(unsigned char)ch]);
	else
		return(ch);
}

int  lowerc( char ch ) /* return the lower case equivalant of a character */
{
	if (is_upper(ch))
		return(upcase[(unsigned char)ch]);
	else
		return(ch);
}

void  initchars()	/* initialize the character upper/lower case tables */
{
	register int index;	/* index into tables */

	for (index = 0; index < HICHAR; index++) {		/* all of both tables to zero */
		lowcase[index] = 0;
		upcase[index] = 0;
	}

	for (index = 'a'; index <= 'z'; index++) {		/* lower to upper, upper to lower */
		lowcase[index] = index ^ DIFCASE;
		upcase[index ^ DIFCASE] = index;
	}

	for (index = (unsigned char)'\340';			/* and for those international characters! */
	     index <= (unsigned char)'\375'; index++) {
		lowcase[index] = index ^ DIFCASE;
		upcase[index ^ DIFCASE] = index;
	}
}

int  setlower( char *ch, char *val )				/* set a character in the lowercase map */
{
	return(lowcase[*ch & 255] = *val & 255);
}

int  setupper( char *ch, char *val )				/* set a character in the lowercase map */
{
	return(upcase[*ch & 255] = *val & 255);
}

char *strrev( char *our_str )					/*  * strrev -- Reverse string in place	*/
{
	register char	*beg_str, *end_str;
	register char	the_char;

	end_str = beg_str = our_str;
	end_str += strlen(beg_str);

	do {
		the_char = *--end_str;
		*end_str = *beg_str;
		*beg_str++ = the_char;
	} while (end_str > beg_str);

	return(our_str);
}
