%{
/* $Id: mcparl.l,v 1.27 2007/06/15 09:43:37 axel Exp $
 */

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <arx_def.h>
#include <str_def.h>
#include "mc.h"
#include "mcinput.h"
#include "mcell.h"
#include "mcellact.h"
#include "mcelladr.h"
#include "mcparse.h"
#include "mcftab.h"
#include "mcfunc.h"

#include "l.tab.h"

extern int  yytype;
extern int  yyerrorflg;

#ifndef FLEX_SCANNER
#undef input
#define input() mcinput()
#undef unput
#define unput(c) mcunput(c)
#endif

#ifdef LEXDEBUG
extern double parse();
struct Range rg1 = {NULL, {{0,0},{0,0}}, "hello"};
struct Range *rnames	= &rg1;
char libpath[80] = ARXPATH;
char *progname = "mcparse";

int main()
{
extern int 	yydebug;
extern char 	*yyunit;
int		at;
double		val;
char		s[80];
char		*arxpath;

yydebug = 1;
if ((arxpath = getenv("ARX")) != NULL)
	strcpy(libpath, arxpath); /* set my path */
while (gets(s))
	{
	val = parse (s, &at, NULL, NULL);
	printf ("s=%s val=%f unit=%s at=%d\n", s, val, yyunit, at);
	}
return at;
}

CELLPTR	cell (col, row)
	int col, row;
{
#ifdef LINKS
static struct CPREC	cellrec	=
	{1, NULL, {0, 0}, NULL, {2.71828, "e"}};
#else
static struct CELLREC	cellrec	/*=
	{NULL, {0, 0}, {2.71828, "e"}, 0, 0, "2.71828"}*/;
#endif
return &cellrec;
}

double cellvalue (col, row)
	int col; int row;
{return (double)(col+row);}

char *celltext (col, row)
	int col; int row;
{return NULL;}

void	rangevwalk (fcn, cell1, cell2)
	int (*fcn)(); struct CELLADR cell1, cell2;
{
(*fcn) ((double)(cell1.col+cell1.row+cell2.col+cell2.row));
return;
}
#endif
%}

%a 10000
%o 10000

%S UNITCTX
%S COMMAND
%S STRING

u	[A-Z]
l	[a-z]
a	[A-Za-z]
n	[0-9]
aln	[0-9A-Za-z_\$\\]
p	"."
s	[+-]
e	([Ee]{s}?{n}+)
w	[ \t]+
f	"@"?
j	['"\^]
col	({a}{1,2})
row	({n}{1,5})
cell	({cs}{col}{cp}({s})?{row}{ce}|{cs}{s}{row}{cp}({s})?{row}{ce}|{col}{row})
range	({a}|\\){aln}+
unit	("/"{w}*)?({a})+({s}{n})?
sd	\"
cs	"["
cp	","
ce	"]"
lt	"<"
day	[0-3][0-9]
mon	[0-1][0-9]
year	[0-9]{2}
lyear	[0-9]{4}
udate	{mon}\/{day}\/{year}
uldate	{mon}\/{day}\/{lyear}
edate	{day}\.{mon}\.{year}
eldate	{day}\.{mon}\.{lyear}
incmd	{cell}{lt}({lt}|!)?
outcmd	{cell}>
str	.+

%%
^{incmd}	{ BEGIN COMMAND; return INCMD; }
^{outcmd}	{ BEGIN COMMAND; return OUTCMD; }
<COMMAND>{str}	{ return CMD; }

^{j}		{ BEGIN STRING; return (JUST); }
<STRING>{str}	{ return CMD; }

{udate}	{
	yylval.value.value	= date((double)atoi((char*)yytext+6), (double)atoi((char*)yytext), (double)atoi((char*)yytext+3));
	yylval.value.unit	= NULL;
	if (!yyerrorflg) yytype	= DATETYPE;
	return CON;
	}

{uldate}	{
	yylval.value.value	= date((double)atoi((char*)yytext+6), (double)atoi((char*)yytext), (double)atoi((char*)yytext+3));
	yylval.value.unit	= NULL;
	if (!yyerrorflg) yytype	= DATETYPE;
	return CON;
	}

{edate}	{
	yylval.value.value	= date((double)atoi((char*)yytext+6), (double)atoi((char*)yytext+3), (double)atoi((char*)yytext));
	yylval.value.unit	= NULL;
	if (!yyerrorflg) yytype	= DATETYPE;
	return CON;
	}

{eldate}	{
	yylval.value.value	= date((double)atoi((char*)yytext+6), (double)atoi((char*)yytext+3), (double)atoi((char*)yytext));
	yylval.value.unit	= NULL;
	if (!yyerrorflg) yytype	= DATETYPE;
	return CON;
	}

({n})+({p}{n}*)?({e})?	|
{p}({n})+({e})?	{
	yylval.value.value	= atof ((char*)yytext);
	yylval.value.unit	= NULL;
	return CON;
	}

"PI"	|
"pi"	{
	yylval.value.value	= M_PI;
	yylval.value.unit	= NULL;
	return CON;
	}

"COL"	|
"col"	{
	yylval.value.value	= (double)(origcol+1);
	yylval.value.unit	= NULL;
	return CON;
	}

"ROW"	|
"row"	{
	yylval.value.value	= (double)(origrow+1);
	yylval.value.unit	= NULL;
	return CON;
	}

"EOF"		|
"eof"		return (EOFCELL);

{sd}.*{sd}	{
	yylval.string = (char*)yytext+1;
	yylval.string[strlen(yylval.string)-1] = '\0';
	return STR;
	}

{cell}	{
	return cellaadr((char*)yytext, &yylval.cell.col, &yylval.cell.row) ? CELL : BAD;
	}

{f}({a})+"("	{
	struct Functab	*f;
	char		*n;

	n	= *yytext == '@' ? (char*)yytext+1 : (char*)yytext;
	n[strlen(n)-1]	= '\0';
	str_upr (n);
	for (f = functab; f->name && strcmp(f->name, n); f++);
	yylval.func.oc	= f->opcode;
	yylval.func.f	= f->func;
#ifdef DEBUG
fprintf (stderr, "parl: func name=%s opcode=%d\n", f->name, f->opcode);
#endif
	return (f->token);
	}

<UNITCTX>{unit}	{
	yylval.string = (char*)yytext;
	return UNIT;
	}

{range}	{
	extern struct Range	*rnames;
	struct Range		*r;
	char			n[MAXFILE+1];

	for (r = rnames; r != NULL; r = r->next)
		{
#ifdef LEXDEBUG
fprintf (stderr, "range test \"%s\" <-> \"%s\"\n", r->name, yytext);
#endif
		strcpy (n, r->name);
		if (!strcmp (n, (char*)yytext))
			{
			memcpy (yylval.range, r->adr, sizeof(struct CELLADR)*2);
			return (RANGE);
			}
		}
	return RANGEB;
	}

"&"{1,2}	|
#?"AND"#?	|
#?"and"#?	return (AND);
#?"XOR"#?	|
#?"xor"#?	return (XOR);
"|"{1,2}	|
#?"OR"#?	|
#?"or"#?	return (OR);
"="{1,2}	return (EQUAL);
"!="		return (NEQUAL);
">"		return (GREATER);
"<"		return (LETHER);
"!"		return (EMARK);
">="		return (GREATERE);
"<="		return (LETHERE);
"+"		return (PLUS);
"-"		return (MINUS);
"*"		return (TIMES);
"/"		return (DIVIDE);
"^"		return (EXP);
"("		return (OPAREN);
")"		return (CPAREN);
"{"		{BEGIN UNITCTX; return (UNITS);}
"}"		{BEGIN 0; return (UNITE);}
"."{1,2}	return (PERIOD);
","		return (KOMMA);
";"		return (SEMI);
":"		return (COLON);
"#"		return (STRCAT);
{w}		;
\n		return (EOF);
^.		{ BEGIN STRING; return BAD; }
.		return (BAD);

%%
void yybegin ()
{
#ifdef FLEX_SCANNER
yy_init = 1;
#endif

BEGIN 0;
}

#ifdef FLEX_SCANNER
YY_BUFFER_STATE mybuf;
#endif

void yysetbuf(char *buffer)
{
#ifdef FLEX_SCANNER
mybuf = yy_scan_string(buffer);
#endif
}

void yyclrbuf()
{
#ifdef FLEX_SCANNER
yy_delete_buffer(mybuf);
#endif
}
