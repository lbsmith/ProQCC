/*
 * screen.c - Various screen display functions
 *
 * Written by: Lee Smith, Modified by David Nugent
 * Modified by Roger Cross
 */

#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "screen.h"
#include "cmdlib.h"

#ifdef UNIX
/*
 * ncurses has color! 
 */
#if defined(HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#define HAVE_NCURSES
#elif defined(HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#define HAVE_NCURSES
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#define HAVE_NCURSES
#else
#include <curses.h>
/*#define HAVE_NCURSES*/
#endif

#include <sys/types.h>
#include <unistd.h>
#if defined(linux) || defined(__linux__)
#include <sys/time.h>
#endif
#else
#include <conio.h>
#endif

#ifdef OS2
#define INCL_VIO
#define INCL_KBD
#define INCL_NOPMAPI
#include <os2.h>
#endif

//////////////////////////////////////////////////////////////
//		Begin of Win32 Screen Functions								//
//																				//
//		Rlc added 6/16/98													//
//////////////////////////////////////////////////////////////
//#ifdef WIN32

//#include <wtypes.h>
//#include <wincon.h>

#define _NOCURSOR       0
#define _NORMALCURSOR   1

static HANDLE hConsoleOutput = 0;
CONSOLE_SCREEN_BUFFER_INFO ScrBufInfo;

//																				//
void _setcursortype(int type){} // not used

//																				//
void textmode(int x){}			// not used

//																				//
void textcolor(int x)			// fixed colors, Bright Yellow on Blue
{
	SetConsoleTextAttribute (hConsoleOutput
		,FOREGROUND_GREEN|FOREGROUND_RED| FOREGROUND_INTENSITY| BACKGROUND_BLUE);
}

//																				//
void textbackground(int x){} //not used

//																				//
static void clrscr(void)
{
	DWORD NumChar;
	COORD BaseCor = {0,0};
	DWORD nLen = ScrBufInfo.dwMaximumWindowSize.X *ScrBufInfo.dwMaximumWindowSize.Y; 

	FillConsoleOutputCharacter(hConsoleOutput,' ', nLen, BaseCor,&NumChar); 
}

//																				//
static void gotoxy(int x, int y)
{
	COORD dwCursorPosition;

	dwCursorPosition.X = x-1;
	dwCursorPosition.Y = y-1;

	SetConsoleCursorPosition(hConsoleOutput,  // handle of console screen buffer		
							dwCursorPosition ); // new cursor position coordinates 

}

//#endif
//////////////////////////////////////////////////////////////
//		End of Win32 Screen Functions									//
//////////////////////////////////////////////////////////////

#ifdef __WATCOMC__
/*
 * Partial conio emulation for Watcom C, DOS & OS2
 */
#define _NOCURSOR       0
#define _NORMALCURSOR   1

#ifdef OS2
static VIOMODEINFO vm;
#else
#include <bios.h>
typedef unsigned short USHORT;
typedef unsigned char BYTE;
void textmode(int);
#pragma aux	textmode = 0x30 0xe4 0xcd 0x10 parm [ah];
#endif

static unsigned char curattr = 7;	/*
					 * white on black 
					 */
static unsigned char curshow = 1;	/*
					 * show cursor 
					 */
static USHORT atCol = 0;
static USHORT atRow = 0;

static void 
_setcursortype(int type)
{
#ifdef OS2
    VIOCURSORINFO ci;

    VioGetCurType(&ci, 0);
    if (type == _NOCURSOR) {
	ci.attr = (USHORT)-1;
	ci.yStart = ci.cEnd = 0;
	curshow = 0;
    } else {
	ci.attr = 1;
	curshow = 1;
	gotoxy(atCol + 1, atRow + 1);
    }
    VioSetCurType(&ci, 0);
#else
    (void)type;
#endif
}

static void 
gotoxy(int x, int y)
{
    atCol = (USHORT)(x - 1);
    atRow = (USHORT)(y - 1);

    if (curshow) {
#ifdef OS2
	VioSetCurPos(atRow, atCol, 0);
#else
#endif
    }
}

static int 
wherex(void)
{
    return atCol + 1;
}

static int 
wherey(void)
{
    return atRow + 1;
}

static void 
clrscr(void)
{
#ifdef OS2
    BYTE b[2];

    b[0] = ' ';
    b[1] = curattr;
    VioScrollUp(0, 0, (USHORT)-1, (USHORT)-1, (USHORT)-1, b, 0);
#else
#endif
}

int 
cputs(const char *buf)
{
    int len = 0;

    if (buf && *buf) {
#ifdef OS2
	len = strlen(buf);
	VioWrtCharStrAtt((PCH)buf, (USHORT)len, atRow, atCol, &curattr, 0);
	atCol += (USHORT)len;
	while (atCol >= vm.col) {
	    atRow += 1;
	    atCol -= vm.col;
	}
	if (curshow)
	    VioSetCurPos(atRow, atCol, 0);
#else
#endif
    }
    return len;
}
#endif

int StatusLine;			/*
				 * Output line for status info 
				 */
long int StartTime;
long int LastCheck;
int TotalStatusEntries;		/*
				 * How many status entries have gone by (out of 32) 
				 */
extern int logging;

long int TotalAlloc = 0L;
long int LastAlloc = 0L;	/*
				 * To keep speed up 
				 */

#ifdef UNIX
/*
 * This is not strictly necessary since ANSI_* == COLOR_*,
 * but we don't really want to rely on that since it is
 * just a happy coincidence.
 */
int colortable[8] =
{
#ifdef HAVE_NCURSES
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
#else
    7, 7, 7, 7, 7, 7, 7, 7
#endif
};

int Foreground = 7, Background = 0;
int HasColors = 0;

#define MAX_PAIRS 16

short MaxPairs[MAX_PAIRS] =
{
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1
};

/*
 * Optimize attribute changes under UNIX
 */
void 
SetColor(void)
{
#ifdef HAVE_NCURSES
    static int OldForeground = -1;
    static int OldBackground = -1;

    if (HasColors &&
	(OldForeground != Foreground || OldBackground != Background)) {
	register int i;
	int cp = (Foreground & 0xf) + ((Background & 0xf) << 4);

	for (i = 0; i < MAX_PAIRS && MaxPairs[i] != cp; i++)
	    if (MaxPairs[i] == -1) {
		init_pair(i + 1, Foreground, Background);
		MaxPairs[i] = cp;
		break;
	    }
	if (i == MAX_PAIRS)
	    i = 0;
	else
	    ++i;
	attron(A_BOLD | COLOR_PAIR(i));
	OldForeground = Foreground;
	OldBackground = Background;
    }
#endif
}

/*
 * We need a cputs() for UNIX
 */
int 
cputs(char *str)
{
    int len = 0;

    if (str && *str) {
	SetColor();
	len = strlen(str);
	addstr(str);
    }
    return len;
}

/*
 * And a gotoxy()
 */
void 
gotoxy(int x, int y)
{
    move(y - 1, x - 1);
}

#else
int backtable[8] =
{0, 4, 2, 6, 1, 5, 3, 7};
int foretable[8] =
{0, 12, 10, 14, 9, 13, 11, 15};
#endif

int ScrnWidth, ScrnHeight;

void 
InitText(void)
{

    /* default to text mode */
    textmode(3);
    ScrnWidth = 80;
    ScrnHeight = 24; // If it were 25, it would scroll, damn it. 

#ifdef WIN32

	if (!hConsoleOutput)
		hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE); 

 	if (GetConsoleScreenBufferInfo( hConsoleOutput  // handle of console screen buffer 
 	 										, &ScrBufInfo))  // address of screen buffer info. 
	{
   	ScrnWidth = ScrBufInfo.dwMaximumWindowSize.X ;
    	ScrnHeight = ScrBufInfo.dwMaximumWindowSize.Y-20; 

		SetConsoleTitle("ProQCC - Advanced Quake-C Compiler/Decompiler");
	}

#endif

#ifdef UNIX
    if (initscr() == NULL) {
	fprintf(stderr, "initscr() failed\n");
	exit(1);
    }
    cbreak();
    noecho();
#ifdef HAVE_NCURSES
    start_color();
    HasColors = has_colors();
#endif
    nonl();
    ScrnWidth = COLS;
    ScrnHeight = LINES - 1;	/*
				 * if it were 25, it would scroll, damn it. 
				 */
    clear();
#else
#ifdef OS2

    vm.cb = sizeof(vm);
    if (VioGetMode(&vm, 0) != 0) {
	fprintf(stderr, "VioGetModeInfo() failed.\n");
	exit(1);
    }
    ScrnWidth = vm.col;
    ScrnHeight = vm.row - 1;	/*
				 * If it were all, it would scroll, damn it. 
				 */
#endif
    _setcursortype(_NOCURSOR);	/*
				 * Hide the cursor 
				 */
    clrscr();
#endif
    gotoxy(1, 1);		/*
				 * Home the cursor 
				 */
}

void 
EndText(void)
{
#ifdef UNIX
    MoveCurs(0, ScrnHeight);
    ShowCurs();
    refresh();
    endwin();
#else

#if defined(OS2) || defined(MSDOS) 
    MoveCurs(0, ScrnHeight - 1);	/* stupid dosish cmd interpreters */
#else
    MoveCurs(0, ScrnHeight);
#endif
    ShowCurs();
#endif
    fflush(stdout);
}

void 
MoveCurs(int x, int y)
{
#ifdef UNIX
    move(y, x);			/*
				 * X and Y are zero-based 
				 */
    refresh();
#else
    gotoxy(x + 1, y + 1);	/*
				 * X and Y are zero-based 
				 */
#endif
}

void 
GetCurs(int *x, int *y)
{
#ifdef UNIX
    getsyx(*y, *x);
#else

#ifdef WIN32
 	GetConsoleScreenBufferInfo( hConsoleOutput, &ScrBufInfo); 
   *x = ScrBufInfo.dwCursorPosition.X;
   *y = ScrBufInfo.dwCursorPosition.Y;
#else
    /* X and Y are zero-based */
    *x = wherex() - 1;
    *y = wherey() - 1;
#endif
#endif
}

void 
SetForeColor(int color)
{
    if (color > 7 || color < 0)
	color = 7;
#ifdef UNIX
    Foreground = colortable[color];
#else
#ifdef __WATCOMC__
    curattr = (BYTE)((curattr & 0xf0) | foretable[color] | 0x8);
#else
    textcolor(foretable[color]);
#endif
#endif
}

void 
SetBackColor(int color)
{
    if (color > 7 || color < 0)
	color = 7;
#ifdef UNIX
    Background = colortable[color];
#else
#ifdef  __WATCOMC__
    curattr = (BYTE)((curattr & 0xf) | (backtable[color] << 4));
#else
    textbackground(backtable[color]);
#endif
#endif
}

void 
CPrintf(char *format,...)
{
    register char c;
    register char *ptr, *src;
    int x, y;
    char text[1024];
    char text2[1024];
    va_list v;

    va_start(v, format);
    vsprintf(text, format, v);

    GetCurs(&x, &y);

    for (ptr = text2, src = text; *src != '\0'; src++) {
	c = *src;

	if (c == '$') {
	    switch (*++src) {
	    case '$':
		*ptr++ = '$';
		break;
	    case 'a':
		*ptr = '\0';
		cputs(ptr = text2);
		RingBell();
		break;
	    case 'n':
		*ptr = '\0';
		cputs(ptr = text2);
		MoveCurs(x, ++y);
		break;
	    case 'f':
		*ptr = '\0';
		cputs(ptr = text2);
		SetForeColor(toupper(*++src) - '0');
		break;
	    case 'b':
		*ptr = '\0';
		cputs(ptr = text2);
		SetBackColor(toupper(*++src) - '0');
		break;
	    case 'c':
		*ptr = '\0';
		cputs(ptr = text2);
		SetForeColor(toupper(*++src) - '0');
		SetBackColor(toupper(*++src) - '0');
		break;
	    }
	} else if (c >= 32 && c <= 126)
	    *ptr++ = c;
    }
    *ptr = '\0';
    cputs(ptr = text2);
#ifdef UNIX
    refresh();
#endif
}

void 
RingBell(void)
{
#ifdef UNIX
    refresh();
    beep();
#else
    putch('\a');
#endif
}

void 
FillBox(int x, int y, int width, int height, char c)
{
    register int i;
    char boxdata[256];

    for (i = 0; i < width; i++)
	boxdata[i] = c;
    boxdata[i] = '\0';

    for (i = 0; i < height; i++) {
	gotoxy(x + 1, i + 1 + y);
	cputs(boxdata);
    }
#ifdef UNIX
    refresh();
#endif
}

void 
DrawBox(int x, int y, int width, int height)
{
#ifdef UNIX
    SetColor();
    move(y, x);
    addch(ACS_ULCORNER);
    move(y, x + 1);
    hline(ACS_HLINE, width - 2);
    move(y, x + width - 1);
    addch(ACS_URCORNER);
    move(y + 1, x);
    vline(ACS_VLINE, height - 2);
    move(y + 1, x + width - 1);
    vline(ACS_VLINE, height - 2);
    move(y + height - 1, x);
    addch(ACS_LLCORNER);
    move(y + height - 1, x + 1);
    hline(ACS_HLINE, width - 2);
    move(y + height - 1, x + width - 1);
    addch(ACS_LRCORNER);
    refresh();
#else
    register int i;

    gotoxy(x + 1, y + 1);
    cputs("+");
    for (i = 0; i < width - 2; i++)
	cputs("-");
    cputs("+");

    for (i = 1; i < height - 1; i++) {
	gotoxy(x + 1, i + 1 + y);
	cputs("|");
	gotoxy(x + width, i + 1 + y);
	cputs("|");
    }

    gotoxy(x + 1, y + height);
    cputs("+");
    for (i = 0; i < width - 2; i++)
	cputs("-");
    cputs("+");
#endif
}

void 
DrawFilledBox(int x, int y, int width, int height)
{
    register int i;
    char boxdata[384];

    for (i = 0; i < width; i++)
		boxdata[i] = ' ';

#ifdef UNIX
    boxdata[i - 2] = '\0';
    DrawBox(x, y, width, height);
    for (i = 0; i++ < height - 2;) 
	 {
	 	move(y + i, x + 1);
	 	cputs(boxdata);
    }
    refresh();
#else
    boxdata[0] = '|';
    boxdata[i - 1] = '|';
    boxdata[i] = '\0';

    gotoxy(x + 1, y + 1);

    cputs("+");

    for (i = 0; i < width - 2; i++)
		cputs("-");

    cputs("+");

    for (i = 1; i < height - 1; i++) 
	 {
		gotoxy(x + 1, i + 1 + y);
		cputs(boxdata);
    }

    gotoxy(x + 1, y + height);

    cputs("+");

    for (i = 0; i < width - 2; i++)
		cputs("-");

    cputs("+");

#endif
}

void 
DrawHLine(int x, int y, int length)
{
#ifdef UNIX
    move(y, x);
    SetColor();
    hline(ACS_HLINE, length);
    refresh();
#else
    register int i;

    gotoxy(x + 1, y + 1);
    for (i = 0; i < length; i++)
	cputs("-");
#endif
}

void 
DrawVLine(int x, int y, int length)
{
#ifdef UNIX
    move(y, x);
    SetColor();
    vline(ACS_VLINE, length);
    refresh();
#else
    register int i;

    for (i = 0; i < length; i++) {
	gotoxy(x + 1, i + 1 + y);
	cputs("|");
    }
#endif
}

#if !defined(UNIX) && !defined(OS2) && !defined(__WATCOMC__) && !defined(WIN32)
static void 
CopyText(int x1, int y1, int width, int height, int x2, int y2)
{
    static void *mem = NULL;

    mem = realloc(mem, width * height * 2);

    gettext(x1 - 1, y1 - 1, x1 + width, y1 + height, mem);
    puttext(x2 - 1, y2 - 1, x2 + width, y2 + height, mem);
}

#endif

void 
ScrollText(void)
{
#ifdef UNIX
    register int i;

    /*
     * Ugh, is this dirty.
     * A better approach would be to use subwin() and
     * scroll the subwindow, but this doesn't work
     * very well in some versions of ncurses.
     */
    for (i = 5; i < ScrnHeight - 2; i++)
    /* This is actually ncurses/curses dependant */

#if defined(linux) || defined(_linux)
	memcpy(stdscr->_line[i].text, stdscr->_line[i+1].text,
#else
	memcpy(stdscr->_line[i], stdscr->_line[i + 1],
#endif
		42 * sizeof(chtype));
    touchline(stdscr, 5, ScrnHeight - 2);
    SetColor();
    FillBox(1, ScrnHeight - 2, 42, 1, ' ');
    refresh();
#else
#ifdef OS2
    BYTE b[2];

    b[0] = ' ';
    b[1] = curattr;
    VioScrollUp(5, 1, (USHORT)(ScrnHeight - 2), (USHORT)43, 1, b, 0);
#else
#ifdef __WATCOMC__
#else

#ifndef WIN32
    CopyText(2, 8, 42, ScrnHeight - 9, 2, 7);
    FillBox(1, ScrnHeight - 2, 42, 1, ' ');
#endif

#endif
#endif
#endif
}

int 
IsKey(void)
{
#ifdef UNIX
    fd_set fds;
    struct timeval tv;

    refresh();
    memset(&tv, 0, sizeof tv);
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    select(1, &fds, NULL, NULL, &tv);
    return FD_ISSET(0, &fds);
#else
    return (kbhit());
#endif
}

int 
WaitKey(void)
{
    return (getch());
}

int 
GetKey(void)
{
    return IsKey()? getch() : 0;
}

void 
HideCurs(void)
{
#ifndef UNIX
    _setcursortype(_NOCURSOR);
#endif
}

void 
ShowCurs(void)
{
#ifndef UNIX
    _setcursortype(_NORMALCURSOR);
#endif
}

void 
ScrnShutDown(void)
{
    long int now = (long)I_FloatTime();
    long int diff = now - StartTime;

    EndText();

    if (LogFile != NULL) {
	fflush(LogFile);
	fclose(LogFile);
    }
    printf("Ctrl+C hit.  Aborting compile after %02ld:%02ld:%02ld.\n",
	diff / 360, (diff / 60) % 59, diff % 59		/*
							 * , TotalAlloc 
							 */ );

    exit(1);
}

void 
SevereErrorSIGINT(int sig)
{
    (void)sig;

    EndText();

    printf("A severe error has occured, ProQCC cannot continue.\n\n");
    printf("Error Signal: SIGINT\n");

    exit(1);
}

#ifdef SIGQUIT
void 
SevereErrorSIGQUIT(int sig)
{
    (void)sig;

    EndText();

    printf("A severe error has occured, ProQCC cannot continue.\n\n");
    printf("Error Signal: SIGQUIT\n");

    exit(1);
}

#endif

void 
SevereErrorSIGSEGV(int sig)
{
    EndText();

    (void)sig;
    printf("A severe error has occured, ProQCC cannot continue.\n\n");
    printf("Error Signal: SIGSEGV\n");

    exit(1);
}

void 
ScrnInit(void)
{
    InitText();

    signal(SIGINT, SevereErrorSIGINT);
    signal(SIGSEGV, SevereErrorSIGSEGV);
#ifdef SIGQUIT
    signal(SIGQUIT, SevereErrorSIGQUIT);
#endif

    LastCheck = StartTime = (long)I_FloatTime();

    SetForeColor(ANSI_WHITE);
    SetBackColor(ANSI_BLUE);
    DrawFilledBox(0, 0, ScrnWidth, ScrnHeight);

    StatusLine = 5;

    MoveCurs(2, 1);
    SetForeColor(ANSI_WHITE);
    SetBackColor(ANSI_BLUE);
    DrawVLine(45, 5, ScrnHeight - 6);
    DrawHLine(1, 4, ScrnWidth - 2);
    DrawHLine(1, 2, ScrnWidth - 2);
    MoveCurs(2, 3);
    CPrintf("$f3Elapsed time: $f200:00:00                                $f7By Lee Smith <$f6lbsmithvt@gmail.com$f7>$f3");
    remove("error.log");
}

void 
ShowStatusEntry(char *format,...)
{
    char text[1024];
    va_list v;

    va_start(v, format);
    vsprintf(text, format, v);

    SetBackColor(ANSI_BLUE);
    SetForeColor(ANSI_WHITE);

    if (StatusLine > ScrnHeight - 2) {
	ScrollText();
	StatusLine--;
    }
    MoveCurs(2, StatusLine);
    CPrintf("%s", text);

    StatusLine++;

    TimeUpdate();
}

void 
ShowTempEntry(char *format,...)
{
    char text[1024];
    va_list v;

    va_start(v, format);
    vsprintf(text, format, v);

    SetBackColor(ANSI_BLUE);

    if (StatusLine > ScrnHeight - 2) {
	SetForeColor(ANSI_WHITE);
	ScrollText();
	StatusLine--;
    }
    SetForeColor(ANSI_YELLOW);

    MoveCurs(4, StatusLine);
    CPrintf("%s", text);

    StatusLine++;

    TimeUpdate();
}

void 
ShowWarningEntry(char *format,...)
{
    char text[1024];
    va_list v;

    va_start(v, format);
    vsprintf(text, format, v);

    SetBackColor(ANSI_BLUE);

    if (StatusLine > ScrnHeight - 2) {
	SetForeColor(ANSI_WHITE);
	ScrollText();
	StatusLine--;
    }
    SetForeColor(ANSI_RED);

    MoveCurs(2, StatusLine);
    CPrintf("$f1*** %s", text);

    if (logging) {
	LogFile = fopen("error.log", "a+");
	if (LogFile != NULL)
	    CfPrintf(LogFile, "*** %s", text);
	fclose(LogFile);
    }
    StatusLine++;

    TimeUpdate();
}

void 
TimeUpdate(void)
{
    long int now = (long)I_FloatTime();

    if (now != LastCheck) {
	long int diff = now - StartTime;

	if (diff > 0) {
	    MoveCurs(16, 3);
	    CPrintf("$f2%02ld:%02ld:%02ld        ", diff / 360, (diff / 60) % 59, diff % 59);
	}
	LastCheck = now;
    }
}

void 
CfPrintf(FILE *file, char *format,...)
{
    register char c;
    register char *ptr, *src;
    char text[1024];
    char text2[1024];
    va_list v;

    va_start(v, format);
    vsprintf(text, format, v);

    for (ptr = text2, src = text; *src != '\0'; src++) {
	c = *src;

	if (c == '$') {
	    switch (*++src) {
	    case '$':
		*ptr++ = '$';
		break;
	    case 'a':
		*ptr = '\0';
		fputs(ptr = text2, file);
		break;
	    case 't':
		*ptr = '\0';
		fputs(ptr = text2, file);
		fputs("\t", file);
		break;
	    case 'n':
		*ptr = '\0';
		fputs(ptr = text2, file);
		fputs("\n", file);
		break;
	    case 'f':
		*ptr = '\0';
		fputs(ptr = text2, file);
		++src;
		break;
	    case 'b':
		*ptr = '\0';
		fputs(ptr = text2, file);
		++src;
		break;
	    case 'c':
		*ptr = '\0';
		fputs(ptr = text2, file);
		++src;
		break;
	    }
	} else if (c >= 32 && c <= 126)
	    *ptr++ = c;
    }
    *ptr = '\0';
    fputs(ptr = text2, file);
    fputs("\n", file);

    fflush(file);
}

void *
qmalloc(int size)
{
#undef malloc
    int total = size + sizeof(int);
    void *mem = malloc(total);
    long int diff;

    if (mem == NULL)
	Error("Insufficient memory.");

    *((int *)mem) = size + sizeof(int);

    TotalAlloc += total;

    diff = TotalAlloc - LastAlloc;
    if (diff < 0L)
	diff = -diff;
    if (diff > 131072L) {
	LastAlloc = TotalAlloc;
	LastCheck = -1L;
	TimeUpdate();
    }
    return ((char *)mem + sizeof(int));
}

void 
qfree(void *mem)
{
#undef free
    long int diff;
    char *tmp = mem;

    mem = tmp - sizeof(int);

    TotalAlloc -= *((int *)mem);

    diff = TotalAlloc - LastAlloc;
    if (diff < 0L)
	diff = -diff;
    if (diff > 131072L) {
	LastAlloc = TotalAlloc;
	LastCheck = -1L;
	TimeUpdate();
    }
    free(mem);
}

/* used for debugging in xterm only */
/*int endwin() { refresh(); return 0; } */

