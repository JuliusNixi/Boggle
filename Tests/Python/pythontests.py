import subprocess
import os
import signal
import random
import time
import sys

# Clearing the screen.
os.system("clear")

# Received args checks.
if len(sys.argv) != 3:
    print("Invalid args. Pass the IP and the port to be used for the clients connection (to...).")
    exit()

# Setting random seed.
random.seed(42)

nclients = 32 # Number of clients that will be spawned.
nactions = 10 # Number of actions for each client. Each action is the submission of a command.
ntests = 10 # Number of tests. It's multiplied by the nactions, be moderate so.

clients = []
actions = ["help\n", "matrix\n", "enddisabled\n", "register_user", "p", "invalidcommand\n"] # Actions that clients will try to send to the server.

ALPHABET = "abcdefghijklmnopqrstuvwxyz" # ALPHABET used to generate a random matrix and allowed chars for a client name (regitration).
junkchar = "_" # A char not in the ALPHABET.

usernamelength = 4 # Name length of all names that the clients will try to register.

VALID_WORDS_TESTS_FILE_PATH = "./Tests/fileCurrentValidsWords.txt" # This is the path to a special file that will be used from the server to write ALL the words present in the current game matrix and in the dictionary. It's used by this script to know some valid words to submit some of them and testing the result.

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
            # The process is alive, killing it.
            os.kill(p.pid, signal.SIGKILL) 

    print("Killed all clients, exiting...")
    exit()

print("Remember to start this script with current folder as the project's root '/'.")
print(f"You should open the server on IP: {sys.argv[1]} and port: {sys.argv[2]}.")
print("So the command used should be something like: 'python3 ./Tests/Python/pythontests.py localhost 8080'.")
print(f"Clients: {nclients}. Actions: {nactions}. Tests {ntests}.")
input("When you're ready, press enter to start the tests...")

print("Starting clients...")
for i in range(nclients):
    filestdoutlogs = open(f"./Tests/Python/Logs/stdout-log-{i}.txt", "w") # Creating the stdout logs file.
    p = subprocess.Popen(["./Bin/boggle_client", sys.argv[1], sys.argv[2]], stdin = subprocess.PIPE, stdout = filestdoutlogs, stderr = filestdoutlogs) # Creating the client process.
    clients.append(p)
    filestdinlogs = open(f"./Tests/Python/Logs/stdin-log-{i}.txt", "w") # Creating the stdin logs file.
    filestdinlogs.close()
print("All clients opened.")

time.sleep(1)

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

            filestdinlogs = open(f"./Tests/Python/Logs/stdin-log-{i}.txt", "a") # Appending the stdin logs file.

            # Submitting a void action (do nothing) if r >= 95.
            r = random.randint(0, 100)
            if r >= 95:
                # Logging it.
                filestdinlogs.write("nothing\n")
                filestdinlogs.close()
                continue    
            
            # Chosing a random action from the above list.
            r = random.randint(0, len(actions) - 1)
            action = actions[r]

            if action == actions[3]: # register_user
                user = ""
                # Submitting an invalid username if r >= 80.
                r = random.randint(0, 100)
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
                # Removing last empty word.
                words = words[0:len(words)-1] 
                # No valid words.
                if (len(words) == 0):
                    # Logging it.
                    filestdinlogs.write("no valid words\n")
                    filestdinlogs.close()
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
            # Submitting.
            p.stdin.write(action.encode())
            p.stdin.flush()

            # Logging action.
            filestdinlogs.write(action) 

            # Killing the client if r >= 99.
            r = random.randint(0, 100)
            if r >= 99:
                filestdinlogs.write("kill")
                os.kill(clients[i].pid, signal.SIGINT)

            filestdinlogs.close()

        if someonealiveflag == 0:
            end(0)

end(1)

