# Copyright (c) 2014 Cesanta Software
# All rights reserved

PROG = daemon
LIBS=-lm
CFLAGS = -W -Wall -I../.. -pthread -g -O0 $(CFLAGS_EXTRA) $(LIBS)
SOURCES = $(PROG).c ../../mongoose.c pidfile.c

all: $(PROG)

run: $(PROG)
	./$(PROG)

$(PROG): $(SOURCES) Makefile
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean:
	rm -rf $(PROG) *.exe *.dSYM *.obj *.exp .*o *.lib *.gc*
