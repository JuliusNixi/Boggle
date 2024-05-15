# Boggle
Boggle video game written in C. Multi-threaded, POSIX, command line, client/server application. Project exam of programming laboratory course 2, University of Pisa, Computer Science BSc, summer session, academic year 2023-2024.

# Pre-compiled binaries
The binaries in Bin/ as been compiled on:

Apple clang version 15.0.0 (clang-1500.1.0.2.5)\
Target: arm64-apple-darwin23.2.0

# Compiling and useful shortcut
- make -> To compile all.
- make execs -> To compile all and then run the server, with default localhost and 8080 as args.
- make execc -> To compile all and then run the client, with default localhost and 8080 as args.
- make clean -> To remove all objects and executables files.
- make tests -> To compile all and then run the tests.

# Usage
Usage: ./Bin/boggle_server nome_server porta_server [--matrici data_filename] [--durata durata_in_minuti] [--seed rnd_seed] [--diz dizionario].


