CC=gcc
CFLAGS=-c -g -Wall -std=c99
LDFLAGS=-lreadline

SOURCES= aventura2.c
LIBRARIES= #.o
INCLUDES= aventura2.h
PROGRAMS=aventura2
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

#$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
#   $(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

aventura2: aventura2.o
	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ *.tmp $(PROGRAMS)
