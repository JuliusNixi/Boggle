# Short description of the structure and dependencies of the project's sources files
The "../Bin/boggle_server" ("../paroliere_srv") executable depends from "./Server/boggle_server.c", "./Server/server.c", "./Common/common.c".
The "../Bin/boggle_client" ("../paroliere_cl") executable depends from "./Client/boggle_client.c", "./Client/client.c", "./Common/common.c".
The "../Bin/boggle_tests" ("../paroliere_tests") executable depends from "./Tests/boggle_tests.c" and "./Common/common.c".

The "./Server/boggle_server.c", "./Server/server.c" depend from "./Server/server.h".
The "./Client/boggle_client.c", "./Client/client.c" depend from "./Client/client.h".

The "./Server/server.h", "./Client/client.h" depend from "./Common/common.h".

The "./Common/common.c" depends from "./Common/common.h".

The "./Common/" files include declarations and implementations used in server, client and C tests. The idea behind the project's structure it's that, the "./Server/boggle_server.c" file and the "./Server/server.c", use shared data structure contained by the "./Server/server.h". Similary for the client files. All the ".c" files (client, server and C tests) depend on "./Common/common.c". All ".h" files depend on "./Common/common.h".

## Compiling
See "../README.md", the file in the project's root folder.


