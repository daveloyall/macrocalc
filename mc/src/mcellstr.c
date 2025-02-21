/* $Id: mcellstr.c,v 1.12 2016/03/14 18:53:42 axel Exp $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <arx_def.h>
#include <str_def.h>
#include <cur_def.h>
#include "mc.h"
#include "mctime.h"
#include "mcell.h"
#include "mcellact.h"
#include "mcellstr.h"

static void textstring (char *instring, char *outstring,
			int col, unsigned int format, int formatting)
/* Sets the string representation of text */
{
char		*ljust = "%-*s", *rjust = "%*s ";
char		*just	= ljust;
unsigned char	width	= colwidth[col];

if (formatting!=NOFORMAT && formatting!=FVALUE && format==(SPECIAL|HIDDEN)) {*outstring = '\0'; return;}
switch (*instring)
 {
 case STRRIGHT:
	if (formatting) {just = rjust; width--;}
	/*FALLTHRU*/
 case STRLEFT:
 case STRCENTER:
	instring++;
 default:
	sprintf(outstring, just, width, instring);
	switch (formatting) {
	 case FORMAT:
	 case FPRINT:
		outstring[colwidth[col]] = '\0';
		break;
	 }
	break;
 case STRREPEAT:
 	if (formatting)
		{
		for (width=0; width<colwidth[col]; width++)
			outstring[width] = *(instring+1);
		outstring[width] = '\0';
		}
	else	strcpy(outstring, instring);
	break;
 }
} /* textstring */

static void valuestring (double value, char *vstring, int col,
		unsigned int format, int *color, int formatting)
/* Sets the string representation of a value */
{
char		s[MAXINPUT+1];
char		*tstring;
char		*fstring;
char		*fnormal	= "%*s ";
char		*fpercent	= "%*s%%";
static char	fdollar[]	= "?%*s ";
char		*fleft		= "%-*s ";
int		width, prec, pos;
struct tm	*vtime;

#ifdef DEBUG
fprintf(stderr, "valuestring: %f\n", value);
#endif
fstring	= fnormal;
*fdollar= currency;
width	= colwidth[col] - 1;
if (value == HUGE_VAL)
	{
	strcpy(vstring, TXTERROR);
	*color = ERRORCOLOR;
	return;
	}
switch (formatting) {
 case NOFORMAT:
    sprintf(vstring, "%*.*f", 1, (int)(format & PLACES), value);
    break;
 case FVALUE:
    width = 18;
    format = SPECIAL|DEFAULT;
    /*FALLTHRU*/
 default:
    switch (format & FORMATM)
     {
     case SPECIAL:
	fstring	= fleft;
	mclocal(value, &vtime);
	tstring = asctime(vtime);
	switch (format & PLACES)
	 {
	 case HIDDEN:
		s[0] = '\0';
		break;
	 case DATE:
		strncpy(s, tstring + 4, 7);
		s[7] = '\0';
		strncat(s, tstring + 20, 4);
		s[11] = '\0';
		break;
	 case EDATE:
		sprintf(s, "%02d.%02d.%04d",
			vtime->tm_mday, vtime->tm_mon+1, vtime->tm_year+1900);
		break;
	 case RDBDATE:
		sprintf(s, "%02d%02d%02d",
			vtime->tm_year%100, vtime->tm_mon+1, vtime->tm_mday);
		break;
	 case TIME:
		if (value > EPOCH)
			{
			strncpy(s, tstring + 11, 8);
			s[8] = '\0';
			}
		else
			{
			int	hour, min, sec;
			
			if (value<1000.)
				value	-= floor(value);
			value	*= 24.;
			hour	=  value;
			value	-= hour;
			value	*= 60.;
			min	=  value;
			value	-= min;
			value	*= 60.;
			sec	=  value;
			sprintf(s, "%2d:%02d:%02d", hour, min, sec);
			}
		break;
	 case DEFAULT:
		fstring	= fnormal;
		prec =  width		/* max. precision for column is width */
			-(value<0.?3:2)	/* - 1 column at the right
					 * - 1 column before '.'
					 * - 1 column for '.'
					 * - sign if negative
					 */
			-(int)log10(ceil(fabs(value)+DBL_MIN));
					/* - more columns before '.'	*/
		if (prec<0) prec = 0;
		sprintf(s, "%*.*f", 1, prec, value);
#ifdef DEBUG
fprintf(stderr, "valuestring: %f=\"%s\"\n", value, s);
#endif
		/* erase trailing zeros after '.' */
		while (strspn(s+strlen(s)-1,".0") && strchr(s,'.'))
			s[strlen(s)-1] = '\0';
		if (strlen(s) > (size_t)width)
			sprintf(s, "%*.*e", 1, width>7?width-7:0, value);
#ifdef DEBUG
fprintf(stderr, "valuestring: %f=\"%s\"\n", value, s);
#endif
		break;
	 }
	 break;
     case SCIENTIFIC:
	sprintf(s, "%*.*e", 1, (int)(format & PLACES), value);
	break;
     case CURRENCY:
	fstring = fdollar;
	width--;
	goto formfix;
     case PERCENT:
	fstring = fpercent;
	value	*= 100.;
	goto formfix;
     formfix:
     case FIXED:
	sprintf(s, "%*.*f", 1, (int)(format & PLACES), value);
	break;
     case COMMA:
	sprintf(s, "%*.*f", 1, (int)(format & PLACES), value);
	pos = strcspn(s, ".");
	while (pos > 3)
		{
		pos -= 3;
		memmove(&s[pos+1], &s[pos], strlen(s)-pos+1);
		s[pos] = ',';
		}
     }
    if (formatting==FVALUE) {
	strcpy(vstring, s);
	}
    else {
	sprintf(vstring, fstring, width, s);
	vstring[colwidth[col]] = '\0';
	}
 }
*color = VALUECOLOR;
#ifdef DEBUG
fprintf(stderr, "valuestring: %f=\"%s\" in %8x\n", value, vstring, color);
#endif
} /* valuestring */

char *cellstring (int col, int row, int *color, int formatting)
/* Creates an output string for the data in the cell in (col, row), and
   also returns the color of the cell */
{
int		newcol;
static char	s[MAXINPUT+1];
char		temp[MAXINPUT+1];
char		*p;
int		pos;
CELLPTR		cp;
unsigned char	typ = EMPTY;
int		visible = FALSE;

cp = cell (col, row);
if (cp == NULL)
	{
	for (	newcol = col-1;
		newcol>=0 && (cp = cell(newcol, row))==NULL;
		newcol--	)
		;
	if (formatting==NOCOLUMN)
		{
		s[0] = '\0';
		*color = BLANKCOLOR;
		}
	typ = (cp!=NULL) ? cptype(cp) : EMPTY;
	if (formatting==NOFORMAT || formatting==NOCOLUMN || newcol<0 ||
	    (typ!=TEXT && typ!=RETRIEVED && typ!=STRING) ||
	    (cpform(cp)==(SPECIAL|HIDDEN)))
		{
		sprintf(s, "%*s", colwidth[col], "");
		*color = BLANKCOLOR;
		}
	else
		{
		p = celltext(newcol, row);
		for (pos = 0; newcol < col; pos += colwidth[newcol++]);
		p += min(strlen(p), (size_t)pos);
		visible = (*p != '\0');
		strncpy(temp, p, colwidth[col]);
		temp[colwidth[col]] = '\0';
		sprintf(s, "%s%*s", temp, (int)(colwidth[col] - strlen(temp)), "");
		*color = TEXTCOLOR;
		}
	}
else
	{
	visible = TRUE;
	switch (cptype(cp))
	 {
	 case EOFPIPE:
		textstring(formatting==FPRINT ? "" : "EOF",
				s, col, cpform(cp), formatting);
		*color = EOFCOLOR;
		break;
	 case INCOMMAND:
	 case OUTCOMMAND:
		*color = COMMANDCOLOR;
		goto textform;
	 case TEXT:
	 case UNITT:
	 case RETRIEVED:
		*color = TEXTCOLOR;
		goto textform;
	 case FORMULA:
	 case CONSTANT:
	 case VRETRIEVED:
		if (formdisplay)
			{
			*color = FORMULACOLOR;
			goto textform;
			}
		else
			{
			*color = VALUECOLOR;
			valuestring
			      (cpvalue(cp),s,col,cpform(cp),color,formatting);
			}
		break;
	 case STRING:
		*color = STRINGCOLOR;
		if (formatting)
		     textstring (cpstring(cp), s, col, cpform(cp), formatting);
		else strcpy(s, cptext(cp));
		break;
	 textform:
		textstring(cptext(cp), s, col, cpform(cp), formatting);
	 } /* switch */
	}
if (visible && formatting==FPRINT && cp!=NULL)
	{
	char	*fp;

	switch (cpattrib(cp))
	 {
	 case BOLD:	fp = "\\fB%s\\fP";	break;
	 case ITALIC:	fp = "\\fI%s\\fP";	break;
	 default:	fp = "%s";		break;
	 }
 	sprintf(temp, fp, s);
	strcpy(s, temp);
	}
if (visible && cp!=NULL)
	switch (cpattrib(cp))
	 {
	 case BOLD:	*color |= BOLDCOLOR;	break;
	 case ITALIC:	*color |= ITALICOLOR;	break;
	 }
if (formatting==NOCOLUMN) p = str_trim(s); else p = s;
if (formatting==FVALUE && cp!=NULL) {
	str_trim(p);
	if (cpunit(cp)) { sprintf(temp, "%s{%s}", p, cpunit(cp)+1); strcpy(p, temp); }
	}
#ifdef DEBUG
fprintf(stderr, "string: s:\"%s\" color:%08x=%d\n", p, *color, *color);
#endif
return p;
} /* cellstring */
