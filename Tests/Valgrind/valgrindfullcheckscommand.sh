# To use Valgrind with all checks.
# https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks
# Why pthread_create leaks? (--gen-suppressions=yes could be used)
# https://stackoverflow.com/questions/75006198/pthread-create-memory-leak-in-valgrind
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt "EXEC" "PARAMS"
