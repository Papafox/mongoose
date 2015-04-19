PROG = auto_scale
CFLAGS = -W -Wall -I../.. -pthread -DMONGOOSE_ENABLE_THREADS -g -O0 $(CFLAGS_EXTRA)
SOURCES = $(PROG).c ../../mongoose.c pidfile.c

all: $(PROG)

run: $(PROG)
	./$(PROG)

$(PROG): $(SOURCES) Makefile
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean:
	rm -rf $(PROG) *.exe *.dSYM *.obj *.exp .*o *.lib *.gc*
