.PHONY: all clear tests clean execs execc

all: 
	$(MAKE) -C ./Src/ $(MAKECMDGOALS)

tests: all 

clean: all

execs: all

execc: all

