# This Makefile simply forward the commands to the Makefile present in the Src/ directory.

.PHONY: all clear tests clean execs execc

all: 
	cd Src/ && $(MAKE) -C . $(MAKECMDGOALS)

clear: all

tests: all 

clean: all

execs: all

execc: all

