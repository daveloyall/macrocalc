/* $Id: mcellpar.c,v 1.5 2001/11/06 15:58:30 axel Exp $
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arx_def.h>
#include "mc.h"
#include "mcellstr.h"
#include "mcparse.h"
#include "mcell.h"

CELLPTR parsecell (char *strvalue, int col, int row)
/* Parses a cell */
{
double	value;
int	type;
unsigned char format		= defaultformat;
char	vbuf[MAXINPUT+1]	= "";
char	unit[MAXINPUT+1]	= "";
char	parsed[MAXPARSED+1];
char	*s;

CELLPTR	cp, allocated;

origcol	= col;
origrow	= row;
s = vbuf+1;
strcpy(s, strvalue);
value = parse (s, &type, unit, parsed);
#ifdef DEBUG
fprintf (stderr, "par: %s->%e type:%d unit=%s errno:%d\n", s, value, type, unit, errno);
#endif

/* set format of new cell depending of cell above new cell */
if (row>0 && cell(col, row)==NULL && (cp = cell(col, row-1))!=NULL) {
	format = cpform(cp);
	}

switch (type) {
 case DATETYPE:
	type = CONSTANT;
	format = SPECIAL|DATE;
	break;
 case SYNERROR:
	type = TEXT;
	/*FALLTHRU*/
 case TEXT:
	switch (*s) {
	 case STRLEFT:
	 case STRRIGHT:
	 case STRCENTER:
	 case STRREPEAT:
		break;
	 default:
		*--s = STRLEFT;
	 }
	break;
 }
allocated = initcell (col, row, type, format, s, value, unit);
#ifdef DEBUG
fprintf (stderr, "inited: type:%d\n", cptype(allocated));
#endif
return allocated;
} /* parsecell */
