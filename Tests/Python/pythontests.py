import subprocess
import os
import signal
import random
import time
import sys

if len(sys.argv) != 3:
    print("Invalid args. Pass the IP and the port to be used for the clients connection (to...).")
    exit()

random.seed(42)

nclients = 64 # Number of clients that will be spawned.
nactions = 10 # Number of actions for each client. Each action is the submission of a command.
ntests = 15 # Number of tests. It's multiplied by the nactions, be moderate so.

clients = []
actions = ["help\n", "matrix\n", "end\n", "register_user", "p", "invalidcommand\n"] # Actions that clients will try to send to the server.

ALPHABET = "abdcdefghijklmnopqrstuvxyz" # Alphabet used to generate a random matrix and allowed chars for a client name (regitration).
junkchar = "_" # A char not in the ALPHABET.

usernamelength = 4 # Name length of all names that the clients will try to register.

VALID_WORDS_TESTS_FILE_PATH = "./Tests/fileCurrentValidsWords.txt" # This is the path to a special file that will be used from the server to write ALL the words present in the current game matrix and in the dictionary.

stdinclients = []

def end(processalive):

    if processalive:
        print("All actions (and all tests) completed.")
    else:
        print("There's no longer any living client process.")
        exit()

    input("Press enter to kill all the clients processes.")

    for i in range(nclients):
        p = clients[i]
        poll = p.poll()
        if poll is None:
            stdinclients[i].close()
            os.kill(p.pid, signal.SIGKILL) # The process is alive, killing it.

    print("Killed all clients, exiting...")

    exit()

print("Remember to start this script with current folder as the project's root '/'.")
print(f"You should open the server on IP: {sys.argv[1]} and port: {sys.argv[2]}.")
print("So the command used should be something like: 'python3 ./Tests/Python/pythontests.py localhost 8080'.")
input("When you're ready, press enter to start the tests...")

print("Starting clients...")
for i in range(nclients):
    filelog = open(f"./Tests/Python/Logs/stdout-log-{i}.txt", "w") # Creating the stdout logs file.
    p = subprocess.Popen(["./Bin/boggle_client", sys.argv[1], sys.argv[2]], stdin = subprocess.PIPE, stdout = filelog, stderr = filelog) # Creating the client process.
    clients.append(p)
    f = open(f"./Tests/Python/Logs/stdin-log-{i}.txt", "w") # Creating the stdin logs file.
    stdinclients.append(f)

print("All clients opened.")
time.sleep(2)
print("Starting actions tests...")

for t in range(ntests):
    print(f"Test {t + 1}.")
    for a in range(nactions):
        # Sleeping between every action some random time.
        randomactionsleep = random.uniform(0.01, 0.1)
        time.sleep(randomactionsleep)
        
        # Use this flag to see if all process are terminated or there is at least one alive.
        someonealiveflag = 0

        for i in range(nclients):
            p = clients[i]   

            poll = p.poll()
            if poll is None:
                someonealiveflag = 1 # Process alive.
            else:
                continue # Process terminated, skipping.

            r = random.randint(0, 100)
            # Submitting a void action (do nothing) if r >= 95.
            if r >= 95:
                stdinclients[i].write("nothing\n") # Logging it.
                continue    

            # Chosing a random action from the above list.
            r = random.randint(0, len(actions) - 1)
            action = actions[r]

            if action == actions[3]: # register_user
                r = random.randint(0, 100)
                user = ""
                # Submitting an invalid username if r >= 80.
                if r >= 80:
                    user = junkchar
                # Creating a random username.
                for j in range(usernamelength):
                    r = random.randint(0, len(ALPHABET) - 1)
                    user += ALPHABET[r]
                action += " " + user + "\n"

            if action == actions[4]: # p
                content = ""
                # Reading current valid words file.
                with open(VALID_WORDS_TESTS_FILE_PATH, "r") as f:
                    content = f.read()
                    f.close()

                words = content.split("\n")
                words = words[0:len(words)-1] # Removing last empty word.
                # No valid words.
                if (len(words) == 0):
                    continue

                word = ""
                # Submitting an invalid word if r >= 50.
                r = random.randint(0, 100)
                if r >= 50:
                    word = junkchar
                r = random.randint(0, len(words) - 1)

                word += words[r]
                action += " " + word + "\n"

            # Submitting action.
            stdinclients[i].write(action) # Logging action.
            # Submitting.
            p.stdin.write(action.encode())
            p.stdin.flush()

            r = random.randint(0, 100)
            # Killing the client if r >= 98.
            if r >= 98:
                stdinclients[i].close()
                os.kill(clients[i].pid, signal.SIGINT)
                stdinclients[i].write("kill")

        if someonealiveflag == 0:
            end(0)


end(1)
