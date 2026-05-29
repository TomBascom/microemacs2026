#	LINUX makefile for
#		MicroEMACS 5.03
#		(C)Copyright 1993 by Daniel Lawrence
#		all rights reserved

# modify the following lines with the proper relative directories

SDIR = .
ODIR = .
HDIR = .
BDIR = .
LDIR = lib

INSD = /usr/lib/microemacs
BIND = /usr/local/bin
MV   = /bin/mv

# options and arguments to the C compiler
CC ?= cc

CFLAGS =	-I$(SDIR) -I$(HDIR) -g

# list of header files

HFILES = $(HDIR)/estruct.h $(HDIR)/edef.h $(HDIR)/efunc.h $(HDIR)/ebind.h $(HDIR)/eproto.h $(HDIR)/termcap.h

#list of object files

F1 = $(ODIR)/basic.o $(ODIR)/bind.o $(ODIR)/buffer.o
F2 = $(ODIR)/char.o $(ODIR)/display.o
F3 = $(ODIR)/eval.o $(ODIR)/exec.o $(ODIR)/file.o
F4 = $(ODIR)/fileio.o $(ODIR)/unix.o $(ODIR)/input.o
F5 = $(ODIR)/isearch.o $(ODIR)/line.o
F6 = $(ODIR)/main.o $(ODIR)/screen.o
F7 = $(ODIR)/random.o $(ODIR)/region.o $(ODIR)/search.o $(ODIR)/replace.o
F8 = $(ODIR)/window.o $(ODIR)/word.o
F9 = $(ODIR)/termcap.o $(ODIR)/tparam.o

# product list

exe:	buildemacs

all:	buildemacs install

OFILES =	$(F1) $(F2) $(F3) $(F4) $(F5) $(F6) $(F7) $(F8) $(F9)

buildemacs:	$(OFILES)
		$(CC) $(CFLAGS) $(OFILES) -o $(BDIR)/microemacs
		strip $(BDIR)/microemacs

$(ODIR)/basic.o: $(SDIR)/basic.c $(HFILES)
	$(CC) $(CFLAGS) -c -o basic.o $(SDIR)/basic.c

$(ODIR)/bind.o: $(SDIR)/bind.c $(HFILES)
	$(CC) $(CFLAGS) -c -o bind.o $(SDIR)/bind.c

$(ODIR)/buffer.o: $(SDIR)/buffer.c $(HFILES)
	$(CC) $(CFLAGS) -c -o buffer.o $(SDIR)/buffer.c

$(ODIR)/char.o: $(SDIR)/char.c $(HFILES)
	$(CC) $(CFLAGS) -c -o char.o $(SDIR)/char.c

$(ODIR)/cmdfix.o: $(SDIR)/cmdfix.c $(HFILES)
	$(CC) $(CFLAGS) -c -o cmdfix.o $(SDIR)/cmdfix.c

$(ODIR)/eval.o: $(SDIR)/eval.c $(HFILES) $(HDIR)/evar.h
	$(CC) $(CFLAGS) -c -o eval.o $(SDIR)/eval.c

$(ODIR)/exec.o: $(SDIR)/exec.c $(HFILES)
	$(CC) $(CFLAGS) -c -o exec.o $(SDIR)/exec.c

$(ODIR)/file.o: $(SDIR)/file.c $(HFILES)
	$(CC) $(CFLAGS) -c -o file.o $(SDIR)/file.c

$(ODIR)/fileio.o: $(SDIR)/fileio.c $(HFILES)
	$(CC) $(CFLAGS) -c -o fileio.o $(SDIR)/fileio.c

$(ODIR)/input.o: $(SDIR)/input.c $(HFILES)
	$(CC) $(CFLAGS) -c -o input.o $(SDIR)/input.c

$(ODIR)/isearch.o: $(SDIR)/isearch.c $(HFILES)
	$(CC) $(CFLAGS) -c -o isearch.o $(SDIR)/isearch.c

$(ODIR)/line.o: $(SDIR)/line.c $(HFILES)
	$(CC) $(CFLAGS) -c -o line.o $(SDIR)/line.c

$(ODIR)/main.o: $(SDIR)/main.c $(HFILES)
	$(CC) $(CFLAGS) -c -o main.o $(SDIR)/main.c

$(ODIR)/unix.o: $(SDIR)/unix.c $(HFILES)
	$(CC) $(CFLAGS) -c -o unix.o $(SDIR)/unix.c

$(ODIR)/random.o: $(SDIR)/random.c $(HFILES)
	$(CC) $(CFLAGS) -c -o random.o $(SDIR)/random.c

$(ODIR)/region.o: $(SDIR)/region.c $(HFILES)
	$(CC) $(CFLAGS) -c -o region.o $(SDIR)/region.c

$(ODIR)/screen.o: $(SDIR)/screen.c $(HFILES)
	$(CC) $(CFLAGS) -c -o screen.o $(SDIR)/screen.c

$(ODIR)/search.o: $(SDIR)/search.c $(HFILES)
	$(CC) $(CFLAGS) -c -o search.o $(SDIR)/search.c

$(ODIR)/replace.o: $(SDIR)/replace.c $(HFILES)
	$(CC) $(CFLAGS) -c -o replace.o $(SDIR)/replace.c

$(ODIR)/window.o: $(SDIR)/window.c $(HFILES)
	$(CC) $(CFLAGS) -c -o window.o $(SDIR)/window.c

$(ODIR)/word.o: $(SDIR)/word.c $(HFILES)
	$(CC) $(CFLAGS) -c -o word.o $(SDIR)/word.c

$(ODIR)/display.o: $(SDIR)/display.c $(HFILES)
	$(CC) $(CFLAGS) -c -o display.o $(SDIR)/display.c

$(ODIR)/tparam.o: $(SDIR)/tparam.c
	$(CC) $(CFLAGS) -c -o tparam.o $(SDIR)/tparam.c

$(ODIR)/termcap.o: $(SDIR)/termcap.c
	$(CC) $(CFLAGS) -c -o termcap.o $(SDIR)/termcap.c

install:
	cp -f microemacs $(BIND)
	mkdir -p $(INSD)
	cp -f $(LDIR)/emacs.rc $(INSD)/emacs.rc
	cp -f $(LDIR)/emacs.hlp $(INSD)/emacs.hlp

clean:	
	rm -f *~ */*~ $(ODIR)/*.o ; rm -f $(BDIR)/microemacs
