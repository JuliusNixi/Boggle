
import subprocess
import os
import signal
import random
import time
import sys

if len(sys.argv) != 3:
    print("Invalid args. Pass the IP and the port to be used for the clients connection.")
    exit()

random.seed(42)

nclients = 50
nactions = 30
ntests = 10
clients = []
actions = ["help\n", "matrix\n", "end\n", "register_user", "p", "invalidcommand\n"]

ALPHABET = "abdcdefghijklmnopqrstuvxyz" # Alphabet used to generate a random matrix and allowed chars for a client name (regitration).
junkchar = "_"

usernamelength = 4

VALID_WORDS_TESTS_FILE_PATH = "../Tests/fileCurrentValidsWords.txt"

stdinclients = []

def end():
    input("Actions completed. Press enter to kill all the clients and exit...")

    for i in range(nclients):
        p = clients[i]
        poll = p.poll()
        if poll is None:
            os.kill(p.pid, signal.SIGQUIT) # process alive
        stdinclients[i].close()

    print("Killed all clients, exiting...")

    exit()

os.chdir("../Src/")

print("Starting clients...")
for i in range(nclients):
    filelog = open(f"../Tests/logs/stdout-log-{i}.txt", "w")
    p = subprocess.Popen(["../Bin/boggle_client", sys.argv[1], sys.argv[2]], stdin=subprocess.PIPE, stdout=filelog, stderr=filelog)
    clients.append(p)
    f = open(f"../Tests/logs/stdin-log-{i}.txt", "w")
    stdinclients.append(f)

print("All clients opened.")
time.sleep(1)

for t in range(ntests):
    for a in range(nactions):
        # sleeping between every action
        randomactionsleep = random.uniform(0.01, 0.1)
        time.sleep(randomactionsleep)
        r = random.randint(1, 10)
        someonealiveflag = 0
        for i in range(nclients):
            p = clients[i]   
            poll = p.poll()
            if poll is None:
                someonealiveflag = 1 # process alive
            else:
                continue # process terminated
            # submitting a void action (do nothing) if r == 10
            if r == 10:
                stdinclients[i].write("nothing\n")
                continue    
            r = random.randint(0, len(actions) - 1)
            action = actions[r]
            if action == actions[3]: # register_user
                r = random.randint(1, 10)
                user = ""
                # submitting an invalid username if r == 10
                if r == 10:
                    user = junkchar
                for j in range(usernamelength):
                    r = random.randint(0, len(ALPHABET) - 1)
                    user += ALPHABET[r]
                action += " " + user + "\n"
            if action == actions[4]: # p
                content = ""
                with open(VALID_WORDS_TESTS_FILE_PATH, "r") as f:
                    content = f.read()
                    f.close()
                words = content.split("\n")
                words = words[0:len(words)-1] # removing last empty word
                if (len(words) == 0):
                    continue
                word = ""
                # submitting an invalid word if r == 10
                r = random.randint(1, 10)
                if r == 10:
                    word = junkchar
                r = random.randint(0, len(words) - 1)
                word += words[r]
                action += " " + word + "\n"
            # submitting action
            r = 0
            try:
                p.stdin.write(action.encode())
                p.stdin.flush()
                stdinclients[i].write(action)
                r = random.randint(1, 100)
            except BrokenPipeError:
                print(f"Broken pipe client {i}.")
                r = 100
            # killing the client if r == 10
            if r >= 95:
                os.kill(clients[i].pid, signal.SIGQUIT)
                stdinclients[i].write("kill")

        if someonealiveflag == 0:
            end()


end()
