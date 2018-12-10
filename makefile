CC=gcc
CFLAGS=-c -g -Wall -std=c99
LDFLAGS=-lreadline

SOURCES= nivel7.c
LIBRARIES= #.o
INCLUDES= nivel7.h
PROGRAMS=nivel7
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

#$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
#   $(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

nivel7: nivel7.o
	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ *.tmp $(PROGRAMS)

