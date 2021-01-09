/* cmdlib.h */

#ifdef WIN32

#include <windows.h>
#include <conio.h>

#define __BYTEBOOL__
#define false FALSE
#define true TRUE


#endif


#ifndef __CMDLIB__
#define __CMDLIB__

#include <fcntl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(UNIX)
#include <unistd.h>
#include <sys/file.h>
#else
#include <io.h>
#endif
#include <stdarg.h>

#ifdef NeXT
#include <libc.h>
#endif

#ifndef __BYTEBOOL__
#define __BYTEBOOL__

typedef enum {
    false, true
} boolean;

typedef unsigned char byte;

#endif

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)

extern FILE *LogFile;

// set these before calling CheckParm
extern int myargc;
extern char **myargv;

char *strupr(char *in);
char *strlower(char *in);

//int filelength (int handle);
//int tell (int handle);

double I_FloatTime(void);

void Error(char *error,...);
int CheckParm(char *check);

int SafeOpenWrite(char *filename);
int SafeOpenRead(char *filename);
void SafeRead(int handle, void *buffer, long count);
void SafeWrite(int handle, void *buffer, long count);
void *SafeMalloc(long size);

long LoadFile(char *filename, void **bufferptr);
void SaveFile(char *filename, void *buffer, long count);

void DefaultExtension(char *path, char *extension);
void DefaultPath(char *path, char *basepath);
void StripFilename(char *path);
void StripExtension(char *path);

void ExtractFilePath(char *path, char *dest);
void ExtractFileBase(char *path, char *dest);
void ExtractFileExtension(char *path, char *dest);

long ParseNum(char *str);

short BigShort(short l);
short LittleShort(short l);
long BigLong(long l);
long LittleLong(long l);
float BigFloat(float l);
float LittleFloat(float l);

char *COM_Parse(char *data);

#ifdef UNIX
int filelength(int in);

#endif

extern char com_token[1024];
extern int com_eof;

#endif
