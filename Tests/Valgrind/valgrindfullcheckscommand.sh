# To use Valgrind with all checks.
# https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt "EXEC" "PARAMS"
