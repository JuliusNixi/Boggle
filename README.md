# Boggle
"The Boggle" online video game written in C. Multi-threaded, POSIX, command line, client/server application. Project exam of programming laboratory course 2, University of Pisa, Computer Science BSc, summer session, academic year 2023-2024.

https://github.com/JuliusNixi/Boggle

## Compiling
- make -> To compile all: client and server.
- make execs -> To compile and run the server, with default args (see in Makefile).
- make execc -> To compile and run the client, with default args (see in Makefile).
- make clean -> To remove all objects, executables files, tests logs files and kills all the server and client processes.

## Usage and execution
WARNING: The italian args (as described in the project's text) are ALSO accepted!
Server Usage: ./paroliere_srv server_ip server_port [--matrices matrices_filepath] [--duration game_duration_in_minutes] [--seed rnd_seed] [--dic dictionary_filepath].
Client Usage: ./paroliere_cl server_ip server_port.

## Working directory
The project uses some absolute hard-coded files paths. It assumes that the working directory is the project's root (the folder containing this file). So NEVER EVER do "cd Bin/" and "./something".
