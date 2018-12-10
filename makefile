CC=gcc
CFLAGS=-c -g -Wall -std=c99
LDFLAGS=-lreadline

<<<<<<< HEAD
SOURCES= my_shell.c
LIBRARIES= #.o
INCLUDES= my_shell.h
PROGRAMS=my_shell
=======
SOURCES= nivel7.c
LIBRARIES= #.o
INCLUDES= nivel7.h
PROGRAMS=nivel7
>>>>>>> nivel-7
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

#$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
#   $(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

<<<<<<< HEAD
my_shell: my_shell.o
=======
nivel7: nivel7.o
>>>>>>> nivel-7
	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ *.tmp $(PROGRAMS)

