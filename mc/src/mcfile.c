/* $Id: mcfile.c,v 1.11 2004/05/16 09:15:54 axel Exp $
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arx_def.h>
#include <cur_def.h>
#include "mc.h"
#include "mcutil.h"
#include "mcwindow.h"
#include "mcmessag.h"
#include "mcinput.h"
#include "mcrecalc.h"
#include "mccalc.h"
#include "mcpipe.h"
#include "mcfileio.h"
#include "mcput.h"
#include "mcget.h"
#include "mcdisply.h"
#include "mcprint.h"
#include "mcfile.h"

void clearsheet (void)
/* Clears the current spreadsheet */
{
initvars();
setrightcol();
setbottomrow();
changed = FALSE;
} /* clearsheet */

void loadsheet (int prompt)
/* Loads a new spreadsheet */
{
FILE	*file;
int	loaded;

if (prompt)
	{
	writeprompt(MSGFILENAME);
	if (!editstring (filename, "", MAXINPUT) || filename[0]=='\0') return;
	}
if (access(filename, R_OK))
	{
	errormsg(MSGNOEXIST);
	return;
	}
if ((file = fopen(filename, "r")) == NULL)
	{
	errormsg(MSGNOOPEN);
	return;
	}
message(MSGLOADING);
leftcol=rightcol=toprow=topbotrow=windowrow=bottomrow=curcol=currow=0;
loaded = loadfile (file);
switch (loaded)
 {
 case RET_FATAL:
	errormsg(MSGNOMICROCALC);
	break;
 case RET_ERROR:
	errormsg(MSGFILELOMEM);
	break;
 case RET_SUCCESS:
 	if (autoexec) inpipeall();
	break;
 }
fclose (file);
leftcol = curcol;
setrightcol();
setbottomrow();
recalcworksheet();
displayscreen(NOUPDATE);
message(MSGNULL);
} /* loadsheet */

void savesheet (void)
/* Saves the current spreadsheet */
{
FILE	*file;
int	overwrite;

recalc();
if (autoexec) outpipeall();
if (rdonly) {changed = FALSE; return;}
writeprompt(MSGFILENAME);
if (!editstring(filename, "", MAXFILE) || *filename=='\0') return;
if (!access(filename, W_OK))
	{
	if (!getyesno(&overwrite, MSGOVERWRITE) || (overwrite == 'N'))
		return;
	}
if ((file = fopen(filename, "w")) == NULL)
	{
	errormsg(MSGNOOPEN);
	return;
	}
message(MSGSAVING);
savefile (file, FULL);
fclose (file);
message(MSGNULL);
changed = FALSE;
} /* savesheet */

void saverange (void)
/* Saves the current range */
{
FILE	*file;
int	overwrite;

recalc();
if (rdonly) {changed = FALSE; return;}
writeprompt(MSGFILENAME);
if (!editstring(filename, "", MAXFILE) || *filename=='\0') return;
if (!access(filename, W_OK))
	{
	if (!getyesno(&overwrite, MSGOVERWRITE) || (overwrite == 'N'))
		return;
	}
if ((file = fopen(filename, "a+")) == NULL)
	{
	errormsg(MSGNOOPEN);
	return;
	}
message(MSGSAVING);
savefile (file, RANGES);
fclose (file);
message(MSGNULL);
} /* saverange */

void printsheet (int prompt)
/* Prints a copy of the spreadsheet to a file or to the printer */
{
int	border			= 'N';
char	filenpr[MAXFILE+1]	= "";
FILE	*file;
char	pcmd[MAXFILE+10];

if (prompt)
	{
	writeprompt(MSGPRINT);
	if (!editstring (filenpr, "", MAXFILE)) return;
	if (!getyesno (&border, MSGBORDER)) return;
	}
else	strcpy (filenpr, "-");
border = (border == 'Y');
sprintf (pcmd, "%s/bin/mcprint %s", libpath, filenpr);
if ((file = popen(pcmd, "w")) == NULL)
	{
	errormsg(MSGNOOPEN);
	return;
	}
message(MSGPRINTING);
if (!strcmp (filenpr, "-"))
	{
	cur_rest();
	}
printfile (file, border);
pclose (file);
if (!strcmp (filenpr, "-"))
	{
	cur_fixt();
	mcrefresh();
	}
message(MSGNULL);
} /* printsheet */
