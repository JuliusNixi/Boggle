# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -pedantic -pthread

# Files names
NS = boggle_server
NC = boggle_client
NHDRS = data_structures
NSF = support_functions
NT = tests

# Header files
HDRS = $(NHDRS).h
HDRSF = $(NSF).h

# Executable files
EXES = ../Bin/$(NS)
EXEC = ../Bin/$(NC)
EXET = ../Tests/$(NT)

# Object files in Bin and Tests
OBJS = $(EXES).o
OBJC = $(EXEC).o
OBJSF = ../Bin/$(NSF).o
OBJT = $(EXET).o

# Object files in Src
OBJSL = $(NS).o
OBJCL = $(NC).o
OBJSFL = $(NSF).o
OBJTL = $(NT).o

# Source files
SRCS = $(NS).c
SRCC = $(NC).c
SRCSF = $(NSF).c
SRCT = ../Tests/$(NT).c

.PHONY: all tests clear clean execs execc

# Targets exe of client, server and tests
all: clear $(EXES) $(EXEC) $(EXET)

$(EXES): $(OBJS) $(OBJSF)
	$(CC) $(CFLAGS) -o $(EXES) $(OBJS) $(OBJSF)
$(EXEC): $(OBJC) $(OBJSF)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJC) $(OBJSF)
$(EXET): $(OBJT) $(OBJSF)
	$(CC) $(CFLAGS) -o $(EXET) $(OBJT) $(OBJSF)

$(OBJS): $(SRCS) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCS) $(HDRSF) && mv -f $(OBJSL) $(OBJS)
$(OBJC): $(SRCC) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCC) $(HDRSF) && mv -f $(OBJCL) $(OBJC)
$(OBJSF): $(SRCSF) $(HDRS) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCSF) $(HDRS) $(HDRSF) && mv -f $(OBJSFL) $(OBJSF)
$(OBJT): $(SRCT) $(HDRSF)
	$(CC) $(CFLAGS) -c $(SRCT) $(HDRSF) && mv -f $(OBJTL) $(OBJT)

# Execute tests
tests: all 
	./../Tests/tests

# Clear screen
clear:
	clear

# Delete all .o and exe file to recompile all
clean: clear
	rm -f ../Bin/* ../Tests/tests.o ../Tests/tests

# Compile and execute the server
execs: all
	./$(EXES) $(IP) $(PORT)

# Compile and execute the client
execc: all
	./$(EXEC) $(IP) $(PORT)

# End of Makefile
