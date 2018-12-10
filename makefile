#/******************************************************************************
# Nombre del proyecto: my_shell
# Grupo: Los tres mosqueteros
# Participantes/desarrolladores: 
#                                + Eugenio Doñaque
#                                + Nadal Llabrés
#                                + Álvaro Pons 
# Asignatura: 21708-Sistemas operativos (prácticas)
# Professor/a: Adelaida Delgado
#******************************************************************************/

CC=gcc
CFLAGS=-c -g -Wall -std=c99
LDFLAGS=-lreadline

SOURCES= my_shell.c nivel7.c
LIBRARIES= #.o
INCLUDES= #.h
PROGRAMS=my_shell nivel7
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

#$(PROGRAMS): $(LIBRARIES) $(INCLUDES)
#   $(CC) $(LDFLAGS) $(LIBRARIES) $@.o -o $@

my_shell: my_shell.o
	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)

nivel7: nivel7.o
	$(CC) $@.o -o $@ $(LDFLAGS) $(LIBRARIES)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -rf *.o *~ *.tmp $(PROGRAMS)
