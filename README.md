# Boggle
Boggle video game written in C. Multi-threaded, POSIX, command line, client/server application. Project exam of programming laboratory course 2, University of Pisa, Computer Science BSc, summer session, academic year 2023-2024.

# Pre-compiled binaries
The binaries in Bin/ as been compiled on:

Apple clang version 15.0.0 (clang-1500.3.9.4)
Target: arm64-apple-darwin23.5.0
Thread model: posix

# Compiling and useful shortcut
- make -> To compile all: client, server and tests.
- make execs -> To compile and run the server, with default args (see in Makefile in Src/).
- make execc -> To compile and run the client, with default args (see in Makefile in Src/).
- make clean -> To remove all objects and executables files.
- make tests -> To compile the server and to compile and run the tests C file.

# Usage
Server Usage: ./Bin/boggle_server server_ip server_port [--matrices matrices_filepath] [--duration game_duration_in_minutes] [--seed rnd_seed] [--dic dictionary_filepath].
Client Usage: ./Bin/boggle_client server_ip server_port.

