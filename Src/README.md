## Short description of the structure of the project source files.
The boggle_server (paroliere_srv) executable depends from boggle_server.c, server.c, common.c..
The boggle_client (paroliere_cl) executable depends from boggle_client.c, client.c, common.c..

The boggle_server.c, server.c depend from server.h..
The boggle_client.c, client.c depend from client.h..

server.h, client.h depend from common.h..

common.c depends from common.h..

Note that common.h and common.c include declarations and implementations used in both server and client.