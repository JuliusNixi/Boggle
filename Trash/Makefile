# Compiler
CC = gcc

# Compiler flags, all warnings and g for debugging
CFLAGS = -Wall -g -pedantic -pthread

# Files names
NS = boggle_server
NC = boggle_client
NHDRS = data_structures
NSF = support_functions
NT = tests
# Root files names
RNS = paroliere_srv
RNC = paroliere_cl

# Header files
HDRS = $(NHDRS).h # data_strctures
HDRSF = $(NSF).h # support_functions

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
	cp $(EXES) ../$(RNS)
	cp $(EXEC) ../$(RNC)

$(EXES): $(OBJS) $(OBJSF)
	$(CC) $(CFLAGS) -o $(EXES) $(OBJS) $(OBJSF)
$(EXEC): $(OBJC) $(OBJSF) # client support_functions
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJC) $(OBJSF)
$(EXET): $(OBJT) $(OBJSF) # client support_functions
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
	rm -f $(EXES) $(EXEC) $(EXET) $(OBJS) $(OBJC) $(OBJSF) $(OBJT) 
	rm -f ../$(RNS) ../$(RNC)

# Some useful default args for testing
export IP=localhost
export PORT=8080

# Compile and execute the server
execs: all
	cd .. && ./$(RNS) $(IP) $(PORT)

# Compile and execute the client
execc: all
	cd .. && ./$(RNC) $(IP) $(PORT)

# End of Makefile
