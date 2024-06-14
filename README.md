# Boggle
Boggle video game written in C. Multi-threaded, POSIX, command line, client/server application. Project exam of programming laboratory course 2, University of Pisa, Computer Science BSc, summer session, academic year 2023-2024.

# Pre-compiled binaries
The binaries in Bin/ as been compiled on:

Apple clang version 15.0.0 (clang-1500.3.9.4)
Target: arm64-apple-darwin23.5.0
Thread model: posix

# Compiling and useful shortcut
Review...
- make -> To compile all.
- make execs -> To compile all and then run the server, with default localhost and 8080 as args.
- make execc -> To compile all and then run the client, with default localhost and 8080 as args.
- make clean -> To remove all objects and executables files.
- make tests -> To compile all and then run the tests.
...

# Usage
Server Usage: ./Bin/boggle_server server_ip server_port [--matrices matrices_filepath] [--duration game_duration_in_minutes] [--seed rnd_seed] [--dic dictionary_filepath].
Client Usage: ./Bin/boggle_client server_ip server_port.

