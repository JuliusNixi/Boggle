# Boggle
Boggle video game written in C. Multi-threaded, POSIX, command line, client/server application. Project exam of programming laboratory course 2, University of Pisa, Computer Science BSc, summer session, academic year 2023-2024.

# Pre-compiled binaries
The binaries in Bin/ as been compiled on:

Apple clang version 15.0.0 (clang-1500.1.0.2.5)\
Target: arm64-apple-darwin23.2.0

# Compiling
Enter in the Src\ directory and use:
- make -> To compile all.
- IP=localhost PORT=8080 make execs -> To compile all and then run the server, with localhost and 8080 as args.
- IP=localhost PORT=8080 make execc -> To compile all and then run the client, with localhost and 8080 as args.
- make tests -> To compile all and then run the tests.