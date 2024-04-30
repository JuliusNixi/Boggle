# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -pedantic -pthread

# Header files
HDR = data_structures.h

# Files names
NS = boggle_server
NC = boggle_client

# Executable files
EXES = ../Bin/$(NS)
EXEC = ../Bin/$(NC)

# Object files in Bin
OBJS = $(EXES).o
OBJC = $(EXEC).o

# Object files in Src
OBJSL = $(NS).o
OBJCL = $(NC).o

# Source files
SRCS = boggle_server.c
SRCC = boggle_client.c

# Targets
all: clear $(EXES) $(EXEC) 

$(EXES): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXES) $(OBJS)
$(EXEC): $(OBJC)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJC)

$(OBJS): $(SRCS) $(HDR)
	$(CC) $(CFLAGS) -c $(SRCS) $(HDR) && mv -f $(OBJSL) $(OBJS)
$(OBJC): $(SRCC) $(HDR)
	$(CC) $(CFLAGS) -c $(SRCC) $(HDR) && mv -f $(OBJCL) $(OBJC)

clear:
	clear

clean:
	rm -f ../Bin/*

# End of Makefile
