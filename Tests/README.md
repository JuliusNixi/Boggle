# Tests folder
This folder contains all the project's debugging and testing tools.
Since the difficulty of testing in C a project divided into client and server, most of the tests were done by simulating clients processes. 

## C subfolder
In the "./C/" subfolder there is the "./C/tests.c" file, it contains some project's tests written in C. These are manual tests of some server functions developed at the beginning of the project and then ABANDONED to follow the above methodology. There is also the "./C/clienttester.c" file, it's a basic server that accepts only 1 client and sends to it messages to test its server's responses printing. The really important and complete tests have their source code in "../Src/Tests/boggle_tests.c" and executable in "../Bin/boggle_tests" and "../paroliere_tests".

## Logs subfolder
The logs produced by the execution of the previously described tests binaries will be in the "./Logs/" subfolder, which will contain the input (commands sent to the server) and output (responses obtained from the server) of the client processes.

## Valgrind subfolder
In the "./Valgrind/" subfolder you'll find the "./Valgrind/valgrindfullcheckscommand.sh" file that contains a command to run all checks with Valgrind, and the result of Valgrind execution on server, client and C tests.

## Notes
In the "./debugandtestsnotes.txt" file you'll find some notes (especially commands) wich have been useful for debugging and testing.

## The ./fileCurrentValidsWords.txt file
This file is created by the server. It's updated in the validateDictionary() function, each time that the current game matrix changes. It contais all the current (during the current game) founded words BOTH in the dictionary file and in the current game matrix. It's used by the C tests to know some valid words to submit, to test the correctness of the server's behaviour.

