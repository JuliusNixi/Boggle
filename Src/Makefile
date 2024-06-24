# Compiler to use.
GC = gcc

# Compiler flags: all warnings, g and ggdb3 for debugging, pthread library used
# and disable a specific warn.
CFLAGS = -Wall -g -ggdb3 -pedantic -pthread -Wno-unused-command-line-argument
# This below post flag (-lm) is needed because the code use the <math.h> lib.
# On MacOS this flag can be omitted or used in the above CFLAGS with the others.
# But there is, however, a strange linking problem on Linux (tested on LABII machine)
# with the math.h lib by not using the -lm flag, or also using it in the CFLAG, not works,
# it is mandatory to insert the -lm flag at the end of the command
# (and not at the beginning like the others).
POSTMATHFLAG = -lm

# Bin files general NAMES.
NS = boggle_server
NC = boggle_client
NT = tests
# Bin files root NAMES.
RNS = paroliere_srv
RNC = paroliere_cl
# Other support files NAMES.
SS = server
SC = client
SU = common

# Bin dir.
BINDIR = ../Bin/
# Tests dir.
TESTDIR = ../Tests/
# Root dir.
ROOTDIR = ../

# Bin files.
EXES = $(BINDIR)$(NS)
EXEC = $(BINDIR)$(NC)
EXET = $(TESTDIR)$(NT)

# Object files bins.
OBJS = $(EXES).o
OBJC = $(EXEC).o
OBJT = $(EXET).o
# Object files support.
OBJSS = $(SS).o
OBJSC = $(SC).o
OBJSU = $(SU).o

# Source server files.
SRCS = $(NS).c
SRCS2 = $(SS).c
# Source client files.
SRCC = $(NC).c
SRCC2 = $(SC).c
# Source common file.
SRCU = $(SU).c
# Source tests file.
SRCT = $(TESTDIR)$(NT).c

# Header server file.
HS = $(SS).h
# Header client file.
HC = $(SC).h
# Header common file.
HU = $(SU).h

.PHONY: all clear tests clean execs execc

# Targets bins of client, server and tests.
all: clear $(EXES) $(EXEC) $(EXET)
	cp $(EXES) $(ROOTDIR)$(RNS)
	cp $(EXEC) $(ROOTDIR)$(RNC)

# Clear screen.
clear:
	clear

# Binaries, linking.
$(EXES): $(OBJS) $(OBJSS) $(OBJSU)
	$(GC) $(CFLAGS) $(OBJS) $(OBJSS) $(OBJSU) $(POSTMATHFLAG) -o $(EXES)
	cp $(EXES) $(ROOTDIR)$(RNS)
$(EXEC): $(OBJC) $(OBJSC) $(OBJSU)
	$(GC) $(CFLAGS) $(OBJC) $(OBJSC) $(OBJSU) $(POSTMATHFLAG) -o $(EXEC)
	cp $(EXEC) $(ROOTDIR)$(RNC)
$(EXET): $(OBJT) $(OBJSS) $(OBJSU)
	$(GC) $(CFLAGS) $(OBJT) $(OBJSS) $(OBJSU) $(POSTMATHFLAG) -o $(EXET)
	
# Binaries, compiling objects.
$(OBJS): $(SRCS) $(HS)
	$(GC) $(CFLAGS) -c $(SRCS) $(POSTMATHFLAG) -o $(OBJS)
$(OBJSS): $(SRCS2) $(HS)
	$(GC) $(CFLAGS) -c $(SRCS2) $(POSTMATHFLAG) -o $(OBJSS)
$(OBJC): $(SRCC) $(HC)
	$(GC) $(CFLAGS) -c $(SRCC) $(POSTMATHFLAG) -o $(OBJC)
$(OBJSC): $(SRCC2) $(HC)
	$(GC) $(CFLAGS) -c $(SRCC2) $(POSTMATHFLAG) -o $(OBJSC)
$(OBJT): $(SRCT) $(HS)
	$(GC) $(CFLAGS) -c $(SRCT) $(POSTMATHFLAG) -o $(OBJT)
$(OBJSU): $(SRCU) $(HU)
	$(GC) $(CFLAGS) -c $(SRCU) $(POSTMATHFLAG) -o $(OBJSU)

# Some useful default args for testing.
export IP=localhost
export PORT=8080

# Execute tests.
tests: all 
	./$(TESTDIR)tests

# Delete all .o and exe files.
clean: clear
	-rm -f $(EXES) $(EXEC) $(EXET) $(OBJS) $(OBJC) $(OBJT) $(OBJSS) $(OBJSC) $(OBJSU) ../$(RNS) ../$(RNC)
	-rm -f ../Tests/logs/*.txt ../Tests/fileCurrentValidsWords.txt
	-pkill -f boggle_client || true
	-pkill -f boggle_server || true

# Compile and execute the server with some default/testing args.
execs: clear $(EXES)
	./$(ROOTDIR)$(RNS) $(IP) $(PORT)

# Compile and execute the client with some default/testing args.
execc: clear $(EXEC)
	./$(ROOTDIR)$(RNC) $(IP) $(PORT)



