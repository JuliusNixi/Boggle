# To use valgrind.
# https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./../Bin/boggle_server localhost 8080