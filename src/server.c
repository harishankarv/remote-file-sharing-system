

#include "../include/global.h"


int server(char *port)

{
	int listenfd /*to listen*/, 
		newfd; /*new connection*/
		

	int getaddrval, len1, len2, len3; 
	int maxfdnumber, clientlistindex, i, j, k, ready, copyfd, argsc ;
	
	int clientfdlist[MAXCLIENTS];
	struct addrinfo hints, *result, *p;
	struct sockaddr_in client;
	
	fd_set masterfdset /*all sockets or fds to monitor*/, 
			readfdset /*this will be modified after select()*/;

	int numrecv, numsent;
	void *buffer;
	
	struct clientinfo info[MAXCLIENTS]; 
	struct unabletoconnect unablemessage;

	u_short clientport, myport;
	char clientip[INET_ADDRSTRLEN], myip[INET_ADDRSTRLEN];
	char myhostname[40], clienthostname[40];

	char command[MAXTOTALLENGTH];
	char *token;
	char args[MAXARGS][MAXARGLENGTH];

	struct sockaddr_in *temp, *temp2;

	
	// load up hints
	// getaddrinfo code concept taken from http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#getaddrinfo

	buffer = malloc (len1);

	len1 = sizeof(struct sockaddr_in);
	len2 = MAXCLIENTS*(sizeof(struct clientinfo));
	len3 = sizeof(struct unabletoconnect);

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; //ipv4
	hints.ai_socktype = SOCK_STREAM; //tcp
	hints.ai_flags = AI_PASSIVE; //use my ip

	for (i=0; i<MAXCLIENTS; i++)
	{
		bzero(&info[i], sizeof(struct clientinfo));	
		info[i].id= 1;
	}
	
	unablemessage.id = 0; 		
//--------------------------------------------------------------------------------------------------------

	// call getaddrinfo()
	
	if ((getaddrval = getaddrinfo(NULL, port, &hints, &result)) < 0)
	{
		perror ("getaddrinfo() error");
		exit(-1);
	}

//--------------------------------------------------------------------------------------------------------
	p=result;


	// try to create socket() and bind() for each element in result
	do
	{
		if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
		{
			printf ("%d\n", listenfd);
			perror ("socket() error");
			exit(-1);
		}

		if ((bind (listenfd, p->ai_addr, p->ai_addrlen)) < 0)
		{	
			printf ("%d\n", listenfd);
			perror ("bind() error");
			exit(-1);
		} 	

		break; //if successful, break

		p=p->ai_next;
	
	} while(p->ai_next !=NULL);


	if (p==NULL) 
	{
        printf("Server: socket() and/or bind() not successful\n");
		exit(-1);
	}


//--------------------------------------------------------------------------------------------------------
	
	//for MYIP and MYPORT	
	// code taken from http://jhshi.me/2013/11/02/how-to-get-hosts-ip-address/

	char hostname[256]; 
	int sock; /*for getting myip and myport */ 
	struct addrinfo hints4, *result4;
	char* target_name = "8.8.8.8"; //Google DNS IP and Port
	char* target_port = "53";
	struct sockaddr_in local_addr;
	socklen_t addr_len;

	if (gethostname(hostname, sizeof(hostname)) < 0) 
	{
	    perror("gethostname() error while getting own hostname");
	    return -1;
	}

	/* get peer server */
	memset(&hints4, 0, sizeof(hints4));
	hints4.ai_family = AF_INET;
	hints4.ai_socktype = SOCK_STREAM;

	if ((getaddrval = getaddrinfo(target_name, target_port, &hints4, &result4)) < 0) 
	{
	  	printf ("%d\n",getaddrval);
		perror ("getaddrinfo() error while getting own hostname");
		exit(-1);
	}

	if ((sock = socket(result4->ai_family, result4->ai_socktype, result4->ai_protocol)) < 0)
	{
	    printf ("%d\n", sock);
		perror("socket error while getting own hostname");
	    exit(-1);
	}

	/* connect to server */
	if (connect(sock, result4->ai_addr, result4->ai_addrlen) < 0) 
	{
	    perror("connect error while getting own hostname");
	    close(sock);
	    exit(-1);
	}

	/* get local socket info */
	
	addr_len = sizeof(local_addr);
	if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) < 0) 
	{
	    perror("getsockname while getting own hostname");
	    close(sock);
	    exit(-1);
	 }


	inet_ntop(local_addr.sin_family, &(local_addr.sin_addr), myip, sizeof(myip));
   	myport = atoi(port); 

	struct hostent *myhe;
	struct in_addr myipv4addr;								
	inet_pton(AF_INET, myip, &myipv4addr);
	myhe = gethostbyaddr(&myipv4addr, sizeof myipv4addr, AF_INET);
	
	strcpy(myhostname, myhe->h_name);

	//--------------------------------------------------------------------------------------------------------

	//listen() for new connections

	if ((listen (listenfd, BACKLOG)) < 0)
	{
		printf ("%d\n", listenfd);
		perror ("listen() error");
		exit(-1);
	}


	maxfdnumber = listenfd; // to put into select. initially, maxfdnumber is the socket we just got after socket()
	clientlistindex = -1;  

	for (i=0; i<MAXCLIENTS; i++)
	{
		clientfdlist[i]= -1;
	}

	FD_ZERO(&masterfdset);
	FD_SET(listenfd, &masterfdset); //Add listenfd to the master set.
	FD_SET(STDINFD, &masterfdset); //add STDINFD to master

	
 //--------------------------------------------------------------------------------------------------------

	while(1)  
	{

		fflush(stdout);

		readfdset = masterfdset;

		ready = select (maxfdnumber+1, &readfdset, NULL, NULL, NULL);
		//after select() returns, if an fd in readfds is set, a read on that fd will not block

		//select() function concept taken from 
		//UNIX Network Programming : Networking APIs : Sockets and XTI : Volume 1, Second Edition,
		// W. Richard Stevens, Prentice Hall, Oct 1997, ISBN: 013490012X
		//page 162
		

		if (FD_ISSET (STDINFD, &readfdset)) //check for user input
		{
			//tokenize inputs and store in args[]
		   argsc = 0;
		   bzero(&args, sizeof(MAXARGS*MAXARGLENGTH));
		   
		   fgets(command, MAXTOTALLENGTH, stdin);
		   token = strtok(command, "\n ");
		   
		    while( token != NULL ) 
		   {
		      strcpy(args[argsc] ,token);
		      args[argsc][strlen(token)] = '\0';  
		      token = strtok(NULL, "\n ");
		      argsc++;
		   }

		   if (!strcmp(args[0], "CREATOR"))
		   {
		   		printf(
		   		"Name                           : Harishankar Vishwanathan\n"
		   		"UBIT Name                      : harishan\n"
		   		"UB Email                       : harishan@buffalo.edu\n"
				"I have read and understood the course academic integrity policy located at\n" 
				"http:www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity\n");
				fflush(stdout);			   
		   }
		  
		   else if (!strcmp(args[0], "HELP"))
		   {
		   		printf(
		   		"CREATOR                             : Display name, UBIT name and UB Email\n"
		   		"                                      of the creator of this program.\n"
		   		"MYIP                                : Display IP Address of this process.\n"
		   		"MYPORT                              : Display PORT number on which this process\n"
		   		"                                      is listening for incoming connections.\n"
		   		"REGISTER <server_IP> <port_no>      : Register with the server \n"
		   		"                                      and obtain the Server-IP List.\n"
		   		"CONNECT <destination> <port_no>     : Establish a new TCP connection to the\n"
		   		"                                      specified <destination> at the specified\n" 
		   		"                                      <port_no>.\n"
		   		"LIST                                : Display list of all connections this \n"
		   		"                                      process is a part of.\n"
		   		"UPLOAD <connection_id> <filename>   : Upload <file_name> to host specified on\n"
		   		"                                      on <connection_id>\n"
		   		"DOWNLOAD <connection_id1><file1> ...: Download a file from each specified host\n" 
		   		"                                      in the command\n"
		   		"TERMINATE <connection_id>           : Terminate the connection specified by\n" 
		   		"                                      <connection_id>, obtained from the LIST\n" 
		   		"                                      command.\n"
		   		"EXIT                                : Close all connections and terminate the\n" 
		   		"                                      process.\n");
		   		fflush(stdout);
		   }

		   else if (!strcmp(args[0], "MYIP"))
		   {
		   		printf("IP address:%s", myip);
		   		printf("\n");
		   		fflush(stdout);
		   }

		   else if (!strcmp(args[0], "MYPORT"))
		   {
		   		printf("Port number:%d", myport);
		   		printf("\n");
		   		fflush(stdout);
		   }

		    else if (!strcmp(args[0], "LIST")) 
		   {
		   		struct hostent *he;
				struct in_addr ipv4addr;
				for (k =0; k<MAXCLIENTS; k++)
				{
					if ((info[k].clientaddrlist.sin_port) == 0)
					{	
						continue;
					}	
				
					inet_ntop(AF_INET, &(info[k].clientaddrlist.sin_addr), clientip, sizeof(clientip));
					inet_pton(AF_INET, clientip, &ipv4addr);
					he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
					clientport = ntohs(info[k].clientaddrlist.sin_port);

					printf("%-5d%-35s%-20s%-8d\n", k+1, he->h_name, clientip, clientport);

				}
			}

		   
		   else if (!strcmp(args[0], "EXIT"))
		   {
		   		for (j=0; j<MAXCLIENTS; j++)
				{
					if (clientfdlist[j]< 0)
					{
						continue;
					}

					copyfd = clientfdlist[j];
							
					close(copyfd);
					FD_CLR(copyfd, &masterfdset);  //clear the entry from masterfdset
					clientfdlist[j] = -1; // remove from client list
					
				}
				
				close(listenfd);
				
				exit(0);
		   }


		   else if (!strcmp(args[0], "STATISTICS"))
		   {
		   		printf(" STATISTICS not implemented yet\n");
		   }


		   else
		   {
		   	printf("Please enter a valid command\n");
		   }

		   ready--;
			if (ready <= 0) 
			{
				continue;
			}

		}

		///checking for new clients trying to register connections by checking listenfd
		if (FD_ISSET (listenfd, &readfdset)) 
		{
						
			if ( (newfd = accept(listenfd, (struct sockaddr*)&client, &len1)) < 0)
			{	
				printf ("%d\n", newfd);
				perror ("accept error");
				exit(-1);			
			}

			for (i=0; i<MAXCLIENTS; i++)
			{
				if (clientfdlist[i]< 0)
				{
					clientfdlist[i]= newfd;  //add descriptor to clientfdlist[] array
					
					numrecv = recv(clientfdlist[i], buffer, len1, 0);


					if (numrecv < 0)
					{
				   	 	//printf("Client: recv() error\n");
				   	 	printf ("%d\n", numrecv);
						perror ("recv error");
						exit(-1);
						
					}

					else
					{
						struct sockaddr_in temp;
						void *ptr = &temp;
						memcpy(ptr, buffer, len1);
						info[i].clientaddrlist = temp; 
					}

					break;
				}		

			}


			if (i == MAXCLIENTS)
			{
				printf("Too many clients. Unable to register any more clients\n"); 
				
				strcpy(unablemessage.message,"Server: error: Too many clients. Unable to connect");
				if (numsent = send(newfd, &unablemessage, len3, 0) <0)
				{
					printf ("%d\n",numsent);
					perror ("send error");
					exit(-1);
				}

				continue;
			}

			if (i >clientlistindex)
			{
				clientlistindex = i;
				//clientlistindex++;
			}

			//add new descriptor to the masterfdset
			FD_SET(newfd, &masterfdset);

			if (newfd> maxfdnumber)
			{	
				maxfdnumber = newfd;
			}

			
			//send new info array to connected clients.
			for (j=0; j<=clientlistindex; j++)
			{
				
				copyfd = clientfdlist[j];

				if (clientfdlist[j]< 0)
				{
					continue;
				}				
				
				else if (numsent = send(copyfd, &info, len2, 0) < 0)
				{
					printf ("%d\n", numsent);
					perror ("send error");
					continue;
				}

			}


			ready--;
			if (ready <= 0) 
			{
				continue;
			}


		}

		for (i=0; i <= clientlistindex; i++)  //check all clients for data 
											   //for checking if client has terminated
		{
			
			if (clientfdlist[i]<0)
			{
				continue;
			}

			copyfd = clientfdlist[i];
			
			
			if (FD_ISSET(copyfd, &readfdset))
			{
				
				numrecv = recv(copyfd, buffer, len1, 0);
				if ( numrecv == 0) //connection closed by client, client TCP sends a FIN 
				{
					printf("Connection closed by a client\n");
					close(copyfd);  
					FD_CLR(copyfd, &masterfdset);  //clear the entry from masterfdset
					clientfdlist[i] = -1; // remove from client list
					bzero(&info[i].clientaddrlist, len1);

					for (j=0; j<=clientlistindex; j++)
					{
				
						copyfd = clientfdlist[j];
						if (clientfdlist[j]< 0)
						{
							continue;
						}				
						else if (send(copyfd, &info, len2, 0) == -1)
						{
							printf("send() error\n");  //send updated info to all clients
						}

					}

				}
				
				
				else if (numrecv < 0)
				{
			   	 	perror ("recv error");
					exit(-1);
					
				}

				ready--;
		    	if (ready <= 0) 
				{
					break;
				}

			}	
			
		}

	}

	return 0;
}


