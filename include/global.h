#ifndef GLOBAL_H_

#define GLOBAL_H_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>


#define MAX 10000
#define BACKLOG 10
#define MAXCLIENTS 4
#define MAXCONNS 3
#define MAXTOTALLENGTH 500
#define MAXARGS 3
#define MAXARGLENGTH 100
#define STDINFD 0

struct clientinfo
{
	unsigned id: 1;
	struct sockaddr_in clientaddrlist;
};

struct unabletoconnect
{
	unsigned id: 1;
	char message[60];
};


int server(char *port);
int client(char *port);



#endif
