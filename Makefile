#
# UNIX Makefile for ProQcc
#

# Select whichever is appropriate
# Define none of these for standard curses (may need minor code adjustments)
#
NCURSES_H=	-DHAVE_NCURSES
#NCURSES_H=	-DHAVE_NCURSES_H
#NCURSES_H=	-DHAVE_CURSES_H
#NCURSES_H=	-DHAVE_NCURSES_NCURSES_H

PROG=		proqcc
CDEFS=		-DINLINE -DUNIX $(NCURSES_H)
CWARN=		-Wall -W
CDEBUG=		#-g
CFLAGS= 	$(CDEBUG) -O3 $(CWARN) $(CDEFS)
OBJFILES=	src/cmdlib.o src/decomp.o src/pr_comp.o src/pr_lex.o src/qcc.o src/screen.o
LIBS=		-lncurses

all:	$(PROG)

$(PROG): $(OBJFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJFILES) $(LIBS)

clean:
	rm -rf $(PROG) $(OBJFILES) *.bak *~ make.log error.log
