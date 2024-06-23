
import subprocess
import os
import signal
import random
import time

random.seed(42)

nclients = random.randint(0, 10) # Bot included.
nactions = 1
clients = []
actions = ["help\n", "matrix\n", "end\n", "register_user", "p", "invalidcommand\n"]
ALPHABET = "abdcdefghijklmnopqrstuvxyz" # Alphabet used to generate a random matrix and allowed chars for a client name (regitration).
junkchar = "_"
usernamelength = 4
VALID_WORDS_TESTS_FILE_PATH = "../Tests/fileCurrentValidsWords.txt"

os.chdir("../Src/")
subprocess.run("make")

for i in range(nclients):
    filelog = open(f"../Tests/logs/stdout-log-{i}.txt", "w")
    p = subprocess.Popen(["../Bin/boggle_client", "localhost", "8080"], stdin=subprocess.PIPE, stdout=filelog, stderr=filelog)
    clients.append(p)
    time.sleep(0.1)
    print(f"Opened {i + 1} client.")

for a in range(nactions):
    for i in range(nclients):
        p = clients[i]   
        poll = p.poll()
        if poll is None:
            pass # process alive
        else:
            continue # process terminated
        r = random.randint(1, 10)
        # submitting a void action (do nothing) if r == 10
        if r == 10:
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
            word = ""
            # submitting an invalid word if r == 10
            r = random.randint(1, 10)
            if r == 10:
                word = junkchar
            r = random.randint(0, len(words))
            word += words[r]
            action += " " + word + "\n"
        # submitting action
        p.stdin.write(action.encode())
        p.stdin.flush()
        r = random.randint(1, 10)
        # killing the client if r == 10
        if r == 10:
            os.kill(clients[i].pid, signal.SIGQUIT)


input("Actions completed. Press enter to kill all the clients and exit...")

for i in range(nclients):
    os.kill(clients[i].pid, signal.SIGQUIT)

print("Killed all clients, exiting...")
