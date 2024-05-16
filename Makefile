# Why 2 makes, one here in the root, and another in Src? Are you crazy?
#
# Because I started the project with the introductory slides before the specification document was published.
# So, I structured the project with all files sources with Makefile in the Src.
# After the document came out, I saw that the project had to be compilable
# from the root with the make command. 
# Not wanting to rewrite the entire Makefile (and all paths) that was already made,
# I created this one that does simply forward all the targets to the one in the Src folder.
#

.PHONY: all tests clean execs execc

all: 
	$(MAKE) -C ./Src/ $(MAKECMDGOALS)

tests: all 

clean: all

execs: all

execc: all

