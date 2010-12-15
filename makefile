# Makefile for bitflurry
# gcc -o bitflurry bitflurry.c database.c -lm -lsqlite3

CC      = gcc
CFLAGS  = -lm -lsqlite3
OBJECTS = bitflurry.c database.c filesystem.c
EFILE   = bitflurry

$(EFILE): $(OBJECTS)
		$(CC) -o $(EFILE) $(OBJECTS) $(CFLAGS)
		
clean: 
		rm $(EFILE)
