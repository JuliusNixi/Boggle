# Boggle
"The Boggle" online video game written in C. Multi-threaded, POSIX, command line, client/server application. Project exam of programming laboratory course 2, University of Pisa, Computer Science BSc, summer session, academic year 2023-2024.  

<div style="text-align: center;">
  <img src="./Boggle-Demo.gif" alt="Git Logo" width="500"/>
</div>


## Compiling
Some useful phony targets:
- make -> To compile all: client, server and tests.
- make execs -> To compile and run the server, with default args (see in "./Makefile").
- make execc -> To compile and run the client, with default args (see in "./Makefile").
- make tests -> To compile and run the tests, with default args (see in "./Makefile").
- make clean -> To remove all objects files, executables files, tests logs files and kill all the server, client and tests processes.  
  
Of course is also possible to invoke make with directly the desired target.

## Usage and execution
WARNING: The italian args (as described in the project's text) are ALSO accepted!  

WARNING: The italian client commands (as described in the project's text) are ALSO accepted!

Server Usage: ./paroliere_srv server_ip server_port [--matrices matrices_filepath] [--duration game_duration_in_minutes] [--seed rnd_seed] [--dic dictionary_filepath].  

Client Usage: ./paroliere_cl server_ip server_port.  

Tests Usage: ./paroliere_tests server_ip server_port.  


On the client (after opening it) use 'help' to see the avaible in-game commands.

## Working directory
The project uses some absolute hard-coded files paths. It assumes that the working directory is the project's root (the folder containing this file). So NEVER EVER do things like "cd Bin/" and "./something".

## Game rules
- The words could be composed using adjacent letters (up, down, left, right), but NOT the diagonals one. 
- A word, to be valid, must be in the current game matrix AND in the dictionary file used. 
- A matrix letter could be used only one time to form a word, but it can be used multiple times to form different words.
- The game is divided in a game phase and a pause phase. During the pause phase it's only possible to register and view the final scoreboard of the previous ended game, but there's not a matrix and it's not possible to submit words.
- A word could be submitted only one time in a game to receive its points. 
- A word composed by N characters gives, when submitted (the first time in the game) N points, but the 'Qu' character values 1 point. 
- By default there's no constraint on the word's length, but this could be setted modifying "WORD_LEN" in "./Src/Server/server.c".

