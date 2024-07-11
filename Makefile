# Compiler to use.
GC = gcc

# Compiler flags: all warnings, g and ggdb3 for debugging, pthread library used
# and disable a specific warn.
CFLAGS = -Wall -g -ggdb3 -pedantic -pthread -Wno-unused-command-line-argument
# This below post flag (-lm) is needed because the code use the <math.h> lib.
# On macOS this flag can be omitted or used in the above CFLAGS with the others.
# But there is, however, a strange linking problem on Linux (tested on LABII machine)
# with the math.h lib by not using the -lm flag, or also using it in the CFLAG, not works,
# it is mandatory to insert the -lm flag at the end of the whole command (after the targets files)
# (and not at the beginning like the others). But on macOS where i code and mainly compile
# using in this way the flag (in the Linux necessary way)
# generate an annoying warning, that is the suppressed one above.
POSTMATHFLAG = -lm

# General files NAMES.
NS = boggle_server
NC = boggle_client
NT = boggle_tests
# Bin files root NAMES.
RNS = paroliere_srv
RNC = paroliere_cl
RNT = paroliere_tests
# Others support files NAMES.
SS = server
SC = client
SU = common

# Object files bins NAMES.
OBJS = $(NS).o
OBJC = $(NC).o
OBJT = $(NT).o
# Object files support NAMES.
OBJSS = $(SS).o
OBJSC = $(SC).o
OBJSU = $(SU).o

# Source server files NAMES.
SRCS = $(NS).c
SRCS2 = $(SS).c
# Source client files NAMES.
SRCC = $(NC).c
SRCC2 = $(SC).c
# Source common file NAME.
SRCU = $(SU).c
# Source C tests file NAME.
SRCT = $(NT).c

# Header server file NAME.
HS = $(SS).h
# Header client file NAME.
HC = $(SC).h
# Header common file NAME.
HU = $(SU).h

# Special current game matrix valid words file.
SP = fileCurrentValidsWords.txt

# Bin dir.
BINDIR = ./Bin/
# Root dir.
ROOTDIR = ./
# Src dir.
SRCDIR = ./Src/
# Tests dir.
TESTSDIR = ./Tests/
# For the LABII machine.
LABIIDIR = /home/local/ADUNIPI/g.nisi/Boggle/

# Bin files.
EXES = $(BINDIR)$(NS)
EXEC = $(BINDIR)$(NC)
EXET = $(BINDIR)$(NT)

.PHONY: all clear clean execs execc tests

# The first target is the default one, called with "make".
# Targets bins of client, server and C tests.
all: clear $(EXES) $(EXEC) $(EXET)

# Clear screen.
clear:
	clear

# Binaries, linking and copying the binaries in the project's root folder.
$(EXES): $(BINDIR)$(OBJS) $(BINDIR)$(OBJSS) $(BINDIR)$(OBJSU)
	$(GC) $(CFLAGS) $(BINDIR)$(OBJS) $(BINDIR)$(OBJSS) $(BINDIR)$(OBJSU) $(POSTMATHFLAG) -o $(EXES)
	cp $(EXES) $(ROOTDIR)$(RNS)
$(EXEC): $(BINDIR)$(OBJC) $(BINDIR)$(OBJSC) $(BINDIR)$(OBJSU)
	$(GC) $(CFLAGS) $(BINDIR)$(OBJC) $(BINDIR)$(OBJSC) $(BINDIR)$(OBJSU) $(POSTMATHFLAG) -o $(EXEC)
	cp $(EXEC) $(ROOTDIR)$(RNC)
$(EXET): $(BINDIR)$(OBJT) $(BINDIR)$(OBJSU)
	$(GC) $(CFLAGS) $(BINDIR)$(OBJT) $(BINDIR)$(OBJSU) $(POSTMATHFLAG) -o $(EXET)
	cp $(EXET) $(ROOTDIR)$(RNT)

# Compiling objects.
# Server.
$(BINDIR)$(OBJS): $(SRCDIR)Server/$(SRCS) $(SRCDIR)Server/$(HS)
	$(GC) $(CFLAGS) -c $(SRCDIR)Server/$(SRCS) $(POSTMATHFLAG) -o $(BINDIR)$(OBJS)
$(BINDIR)$(OBJSS): $(SRCDIR)Server/$(SRCS2) $(SRCDIR)Server/$(HS)
	$(GC) $(CFLAGS) -c $(SRCDIR)Server/$(SRCS2) $(POSTMATHFLAG) -o $(BINDIR)$(OBJSS)
# Client.
$(BINDIR)$(OBJC): $(SRCDIR)Client/$(SRCC) $(SRCDIR)Client/$(HC)
	$(GC) $(CFLAGS) -c $(SRCDIR)Client/$(SRCC) $(POSTMATHFLAG) -o $(BINDIR)$(OBJC)
$(BINDIR)$(OBJSC): $(SRCDIR)Client/$(SRCC2) $(SRCDIR)Client/$(HC)
	$(GC) $(CFLAGS) -c $(SRCDIR)Client/$(SRCC2) $(POSTMATHFLAG) -o $(BINDIR)$(OBJSC)
# Common.
$(BINDIR)$(OBJSU): $(SRCDIR)Common/$(SRCU) $(SRCDIR)Common/$(HU)
	$(GC) $(CFLAGS) -c $(SRCDIR)Common/$(SRCU) $(POSTMATHFLAG) -o $(BINDIR)$(OBJSU)
# Tests.
$(BINDIR)$(OBJT): $(SRCDIR)Tests/$(SRCT) $(SRCDIR)Common/$(HU)
	$(GC) $(CFLAGS) -c $(SRCDIR)Tests/$(SRCT) $(POSTMATHFLAG) -o $(BINDIR)$(OBJT)

# '-' means continue executing commands even in case of failure.
clean: clear
# Delete all .o and exe files.
	-rm -f $(EXES) $(EXEC) $(EXET) $(BINDIR)$(OBJS) $(BINDIR)$(OBJC) $(BINDIR)$(OBJSS) $(BINDIR)$(OBJSC) $(BINDIR)$(OBJSU) $(BINDIR)$(OBJT) $(ROOTDIR)$(RNS) $(ROOTDIR)$(RNC) $(ROOTDIR)$(RNT)
# Delete all testing logs file.	
	-rm -f $(TESTSDIR)/C/Logs/*.txt $(TESTSDIR)$(SP)
# Killing all processes.
	-pkill -SIGKILL -f $(NS)
	-pkill -SIGKILL -f $(NC)
	-pkill -SIGKILL -f $(NT)

	-pkill -SIGKILL -f $(RNS)
	-pkill -SIGKILL -f $(RNC)
	-pkill -SIGKILL -f $(RNT)
# For the LABII machine.
	-pkill -SIGKILL -f $(LABIIDIR)$(RNS)
	-pkill -SIGKILL -f $(LABIIDIR)$(RNC)
	-pkill -SIGKILL -f $(LABIIDIR)$(RNT)

	find . -name .DS_Store -not -path "./Tests/C/Logs/*" -delete


# Some useful default args for testing.
export IP=localhost
export PORT=8080

# Compile and execute the server with some default/testing args.
execs: clear $(EXES)
	$(ROOTDIR)$(RNS) $(IP) $(PORT)

# Compile and execute the client with some default/testing args.
execc: clear $(EXEC)
	$(ROOTDIR)$(RNC) $(IP) $(PORT)

# Compile and execute the C tests with some default/testing args.
tests: clear $(EXET)
	$(ROOTDIR)$(RNT) $(IP) $(PORT)

