#ifndef _CURS_H_
#define _CURS_H_

#include <stdio.h>

void InitText(void);
void EndText(void);
void MoveCurs(int x, int y);
void GetCurs(int *x, int *y);
void SetForeColor(int color);
void SetBackColor(int color);
void CPrintf(char *format,...);
void RingBell(void);
void FillBox(int x, int y, int width, int height, char c);
void DrawBox(int x, int y, int width, int height);
void DrawFilledBox(int x, int y, int width, int height);
void DrawHLine(int x, int y, int length);
void DrawVLine(int x, int y, int length);
void ScrollText(void);
int IsKey(void);
int WaitKey(void);
int GetKey(void);
void HideCurs(void);
void ShowCurs(void);
void ScrnShutDown(void);
void ScrnInit(void);
void ShowStatusEntry(char *format,...);
void ShowTempEntry(char *format,...);
void ShowWarningEntry(char *format,...);
void TimeUpdate(void);
void CfPrintf(FILE *file, char *format,...);
void *qmalloc(int size);
void qfree(void *mem);

extern int ScrnWidth, ScrnHeight;

enum {
    ANSI_BLACK,
    ANSI_RED,
    ANSI_GREEN,
    ANSI_YELLOW,
    ANSI_BLUE,
    ANSI_MAGENTA,
    ANSI_CYAN,
    ANSI_WHITE,
};

#define ANSI_VIOLET ANSI_MAGENTA
#define ANSI_PURPLE ANSI_MAGENTA
#define ANSI_AQUA ANSI_CYAN

#endif
