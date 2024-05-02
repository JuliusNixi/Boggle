# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -pedantic -pthread

# Files names
NS = boggle_server
NC = boggle_client
NSF = support_functions

# Header files
HDRS = data_structures.h
HDRSF = $(NSF).h

# Executable files
EXES = ../Bin/$(NS)
EXEC = ../Bin/$(NC)

# Object files in Bin
OBJS = $(EXES).o
OBJC = $(EXEC).o
OBJSF = ../Bin/$(NSF).o

# Object files in Src
OBJSL = $(NS).o
OBJCL = $(NC).o
OBJSFL = $(NSF).o

# Source files
SRCS = $(NS).c
SRCC = $(NC).c
SRCSF = $(NSF).c

# Targets
all: clear $(EXES) $(EXEC) 

$(EXES): $(OBJS) $(OBJSF)
	$(CC) $(CFLAGS) -o $(EXES) $(OBJS) $(OBJSF)
$(EXEC): $(OBJC) $(OBJSF)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJC) $(OBJSF)

$(OBJS): $(SRCS) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCS) $(HDRSF) && mv -f $(OBJSL) $(OBJS)
$(OBJC): $(SRCC) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCC) $(HDRSF) && mv -f $(OBJCL) $(OBJC)
$(OBJSF): $(SRCSF) $(HDRS) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCSF) $(HDRS) $(HDRSF) && mv -f $(OBJSFL) $(OBJSF)

clear:
	clear

clean: clear
	rm -f ../Bin/*

execs: all
	./$(EXES)

# End of Makefile
