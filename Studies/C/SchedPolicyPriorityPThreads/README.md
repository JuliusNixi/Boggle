# Data SchedPolicyPriorityPThreads
All these tests files have been made for serious project's reason/problem.

In testing the project, it became apparent that, as the number of players/clients connected increased, the server became slower and slower in handling play/pause phases, in accepting new connections and in responding to players' requests, until the game became unplayable and the server would freeze for ages.

I realized that as the number of threads increased, it was always more likely that a clientHandler() (player management) thread would be scheduled than the main (acceptClient() which accepts new connections) and signalsThread() (game phase handler) (and also its others support threads scorer() e "gamePauseAndNewGame()") threads which are the ones critical to the proper functioning of the entire project. 

This for the way the project was structured was critical.

In fact, the clientHandler() threads, in order to ensure optimal responsiveness to clients never suspend, they simply block on mutexes.

The signalsThread() thread itself attempts to acquire mutexes used by the clientHandler() threads.

The result, exemplifying with an example, is that, if we have a mutex M, 100 clientHandler() threads C, and 1 signalsThread() thread S (the same reasoning applies to the single main thread (acceptClient())) all trying to acquire M, it is much more likely that a C will be scheduled, this one acquires M, does something, releases it, and loop, now comes an endgame signal, the S thread kicks in to handle it, but get stuck on M, since there are 100 C threads, it is very likely that they are scheduled one, more times or potentially infinite, and the S thread waits on the M mutex indefinitely, there is NO DEADLOCK, but the server gets stuck for a long time before accepting new connections or ending the game, precisely because it has to "hope" that "right" thread it's the one that acquires M.

An entire restructuring of the project would perhaps have solved the problem, but not wanting to lose the time invested, I tried not to demoralize myself and look for a solution.

The first idea I came up with was to try to increase the scheduling priority of the acceptClient() and signalsThread() threads. In the files in this folder you will see this attempt, which although successful, did not solve the problem, because even by setting the priority of the clientHander() threads to the lowest possible, they continued to acquire mutexes at the expense of signalsThread() and acceptClient() threads. 

In the end I thought that the solution could be much simpler and painless, just add an additional priority mutex and that solution can be found in the "./prioritypthreadssolution.c" file. This SOLVED the problem!



