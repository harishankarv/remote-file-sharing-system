
#include "../include/global.h"


int client(char *port)
{

	int listenfd /*to listen*/, 
		newfd /*accepting new connection*/,
	 	regfd /*for registering with server*/, 
	 	connfd /*for connecting with client*/; 
		
	int getaddrval =0 ,  len1, len2, len3; 
	int maxfdnumber, clientlistindex, i, j, k, l, argsc, ready, copyfd, numrecv, numsent ;
	int clientfdlist[MAXCONNS];
	
	struct addrinfo hints, hints2, hints3, *result, *result2, *result3, *p, *p2, *p3;
	struct sockaddr_in *server, client;
	
	fd_set masterfdset /*all sockets or fds to monitor*/, 
			readfdset /*this will be modified after select()*/;
	
	void *buffer;
	struct clientinfo info[MAXCLIENTS]; 
	struct unabletoconnect unablemessage_s;
	struct unabletoconnect *unablemessage_r;

	u_short clientport, myport; 
	u_short serverport = 0;
	char clientip[INET_ADDRSTRLEN], myip[INET_ADDRSTRLEN], serverip[INET_ADDRSTRLEN];
	char myhostname[40], serverhostname[40], clienthostname[40];
	
	//for tokenizing input commands
	char command[MAXTOTALLENGTH];
	char *token;
	char args[MAXARGS][MAXARGLENGTH];

	len1 = sizeof(struct sockaddr_in);
	len2 = MAXCLIENTS*(sizeof(struct clientinfo));
	len3 = sizeof(struct unabletoconnect);
	
	struct sockaddr_in clientconns[MAXCONNS];
	struct sockaddr_in *temp, *temp2;

	int found1;
	int found2;
	void* addr;

	buffer = malloc(MAX);
	temp = malloc(len1);
	temp2 = malloc(len1);
	

	bzero(&command, MAXTOTALLENGTH);


	// load up hints for getaddrinfo
	// getaddrinfo code concept taken from http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#getaddrinfo

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_INET; //ipv4
	hints.ai_socktype = SOCK_STREAM; //tcp
	hints.ai_flags = AI_PASSIVE; //use my ip

	for (i=0; i<MAXCONNS; i++)
	{
		bzero(&clientconns[i], sizeof(struct sockaddr_in));	
	}

	for (i=0; i<MAXCLIENTS; i++)
	{
		bzero(&info[i], sizeof(struct clientinfo));	
	}
 //--------------------------------------------------------------------------------------------------------

	// call getaddrinfo()
	
	if ((getaddrval = getaddrinfo(NULL, port, &hints, &result)) < 0)
	{
		printf ("%d\n",getaddrval);
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
			printf ("%d\n",listenfd);
			perror ("bind() error");
			exit(-1);
		} 	

		break; //if successful, break

		p=p->ai_next;
	
	} while(p->ai_next !=NULL);


	if (p==NULL) 
	{
        printf("socket() and/or bind() not successful\n");
		exit(-1);
	}

	temp= (struct sockaddr_in*)p->ai_addr;
	


 //----------------------------------------------------------------------------------------------------
	

	//for MYIP and MYPORT	
	// code concept taken from http://jhshi.me/2013/11/02/how-to-get-hosts-ip-address/

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
    local_addr.sin_port = temp->sin_port;	
	myport = ntohs(local_addr.sin_port);

	struct hostent *myhe;
	struct in_addr myipv4addr;								
	inet_pton(AF_INET, myip, &myipv4addr);
	myhe = gethostbyaddr(&myipv4addr, sizeof myipv4addr, AF_INET);
	
	strcpy(myhostname, myhe->h_name);


	//--------------------------------------------------------------------------------------------------------

	//listen() for new connections

	if ((listen (listenfd, BACKLOG)) < 0)
	{
		printf ("%d\n",listenfd);
		perror ("listen() error");
		exit(-1);
	}

	maxfdnumber = listenfd; // to put into select. initially, maxfdnumber is the socket we just got after socket()
	clientlistindex = -1;  

	for (i=0; i<MAXCONNS; i++)
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
		   argsc=0;
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

		   
		   else if (!strcmp(args[0], "REGISTER"))
		   {
		   		if (argsc != 3)
				{
	    			printf("Not enough arguments\n");
	    			continue;
				}

				fflush(stdout);
			   	
			   	bzero(&hints2, sizeof(struct addrinfo));
				hints2.ai_family = AF_INET;
				hints2.ai_socktype = SOCK_STREAM;
				
				if ((getaddrval = getaddrinfo(args[1], args[2], &hints2, &result2)) < 0)
				{
					printf ("%d\n",getaddrval);
					printf("Please verify IP address and Port number\n");
					perror ("getaddrinfo() error");
					continue;
				}

				p2 = result2;

				do
				{
					
					if ((regfd = socket(p2->ai_family, p2->ai_socktype, p2->ai_protocol)) < 0)
					{
						printf ("%d\n",regfd);
						perror ("socket error");
						continue;
					}

					
					if ((connect(regfd, p2->ai_addr, p2->ai_addrlen)) < 0)
					{	
						printf ("%d\n",regfd);
						perror ("connect error");
						close (regfd);
						continue;
					} 

					break; //if successful, break

					p2=p2->ai_next;
				
				} while(p2->ai_next !=NULL);

				if (p2==NULL) 
				{
			        printf("Client: socket() and/or connect() not successful\n");
					exit(-1);
				} 	


				//add new descriptor to the masterfdset
				FD_SET(regfd, &masterfdset);
				
				if (regfd > maxfdnumber)
				{	
				maxfdnumber = regfd;
				}

				
				//get serverip
				server = (struct sockaddr_in*)p2->ai_addr;
				addr = &(server->sin_addr);
				inet_ntop(p2->ai_family, addr, serverip, sizeof(serverip));
				
				//get serverport
				serverport = ntohs(server->sin_port);

				//get serverhostname
				struct hostent *serverhe;
				struct in_addr serveripv4addr;								
				inet_pton(AF_INET, serverip, &serveripv4addr);
				serverhe = gethostbyaddr(&serveripv4addr, sizeof serveripv4addr, AF_INET);				
				
				strcpy(serverhostname, serverhe->h_name);
				
				//send own listening port info
				numsent = send(regfd, &local_addr, len1, 0);
				if (numsent <0)
				{
					printf ("%d\n", numsent);
					perror ("send error");
				}

		   }

		   
		   else if (!strcmp(args[0], "CONNECT"))
		   {
		   	 	if ( (!strcmp(args[1], serverhostname)) || (!strcmp(args[1], serverip)) )
		   	 	{
		   	 		printf("You can only CONNECT to clients. CONNECT is not allowed for the server.\n");
					continue;   	 			
		   	 	}

		   	 	for (l =0; l<MAXCONNS; l++)
		   	 	{
		   	 		if (clientfdlist[l]>0)
		   	 		{	
		   	 				continue;
		   	 		}
		   	 		
		   	 		if (clientfdlist[l]<0)
		   	 		{
		   	 			break;
		   	 		}	
		   	 	}

		   	 	if (l==MAXCONNS)
		   	 	{
		   	 		printf("More than 3 connections not allowed\n");
		   	 		continue;
		   	 	}

		   		if (argsc != 3)
				{
	    			printf("Not enough arguments\n");
	    			continue;
				}

		   		found1 = 0;

		   		//self connection check
		   		
		   	 	//ip check
		   	 	if (!strcmp(args[1], myip))
		   		{	
		   			printf("Self connection not allowed.\n");
		   			continue;
		   		}

		   		//hostname check
		   		if(!strcmp(args[1], myhostname))
		   		{
		   			printf("Self connection not allowed.\n");
		   			continue;
		   		}
				
				//duplicate connection check
		   		
		   		//IP check 

	   			for (k =0; k<MAXCONNS; k++)
				{
					if (clientfdlist[k]< 0)
					{	
						continue;
					}							

					
					char tempip[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(clientconns[k].sin_addr), tempip, sizeof(tempip));					
					
					if (!strcmp(tempip, args[1]))
					{
							printf("Connection already exits. Duplicate connection not allowed.\n");
							found1 =1;
							continue;
					}
				}	
				
				//hostname check

				for (k =0; k<MAXCONNS; k++)
				{
					if (clientfdlist[k]< 0)
					{	
						continue;
					}		

					//check if hostname is same
					
					struct hostent *he;
					struct in_addr ipv4addr;								
					inet_ntop(AF_INET, &(clientconns[k].sin_addr), clientip, sizeof(clientip));
					inet_pton(AF_INET, clientip, &ipv4addr);
					he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);

					if (!strcmp (he->h_name, args[1]))
					{
						found1 =1;
						printf("Connection already exits. Duplicate connection not allowed.\n");
						continue;
					}

					
				}

				//check if there in Server-IP List

				found2 = 0;
				
				//IP check
				for (k=0; k<MAXCLIENTS; k++)
							
				{	
					if ((info[k].clientaddrlist.sin_port) == 0)
						{	
							continue;
						}
					
					char tempip[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &(info[k].clientaddrlist.sin_addr), tempip, sizeof(tempip));					
					
					
					if (!strcmp(tempip, args[1]))
					{
							found2 = 1;
							break;
					}


				}


				//hostnme check
				for (k =0; k<MAXCLIENTS; k++)
				{
					
					if ((info[k].clientaddrlist.sin_port) == 0)
						{	
							continue;
						}

					struct hostent *he;
					struct in_addr ipv4addr;								
					inet_ntop(AF_INET, &(info[k].clientaddrlist.sin_addr), clientip, sizeof(clientip));
					inet_pton(AF_INET, clientip, &ipv4addr);
					he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);

					
					if (!strcmp (he->h_name, args[1]))
					{
						found2 =1;
						break;
					}

					
				}



				if (found1 == 1)
				{	
					continue;	
				}

				else if (found2 == 0)
				{
					if(serverport == 0)
					{
						printf("Please register with Server first\n");
						continue;
					}
					
					else 
					{
						printf("%s is not registered with the Server. Unable to connect.\n", args[1]);
						continue;
					}
				}


		   		//connect with client
		   		else 
			   	{
				   	int conn;
				   	bzero(&hints3, sizeof(struct addrinfo));
					hints3.ai_family = AF_INET;
					hints3.ai_socktype = SOCK_STREAM;
						
					if ((getaddrval = getaddrinfo(args[1], args[2], &hints3, &result3)) < 0)
					{
						printf ("%d\n",getaddrval);
						perror ("getaddrinfo() error");
						exit(-1);
					}

					p3 = result3;
					do
					{
						if ((connfd = socket(p3->ai_family, p3->ai_socktype, p3->ai_protocol)) < 0)
						{
							printf ("%d\n", connfd);
							perror ("socket() error");
							continue;
						
						}

						conn = connect(connfd, p3->ai_addr, p3->ai_addrlen);
						if (conn < 0)
						{	
							printf ("%d\n",connfd);
							perror ("connect() error");
							close(connfd);
							continue;
						} 

						break; //if successful, break
						p3=p3->ai_next;
					
					} while(p3->ai_next !=NULL);

					if (p3==NULL) 
					{
				        printf("socket() and/or connect() not successful\n");
						continue;
					}

					if (conn == 0)
					{
						printf("Success. Connected to %s\n", args[1]);
					}
					//save connected descriptor to clientfdlist[] array
					//save client info to clientconns[] array
					
					for (i=0; i<MAXCONNS; i++)
					{
						if (clientfdlist[i]< 0)
						{
							clientfdlist[i]= connfd; 
							temp2 = (struct sockaddr_in*)p3->ai_addr;
							void *ptr2 = &clientconns[i];
							memcpy(ptr2, p3->ai_addr, len2);
							break;					
						}		
								
					}


					if (i >clientlistindex)
					{
						clientlistindex = i;
					}

					//add new descriptor to the masterfdset
					FD_SET(connfd, &masterfdset);

					if (connfd> maxfdnumber)
					{	
						maxfdnumber = connfd;
					}
				}
			}

		   else if (!strcmp(args[0], "LIST")) 
		   {
		   		if(serverport == 0)
				{
					printf("Please register with Server first\n");
					continue;
				}

				int k = 1;

				printf("%-5d%-35s%-20s%-8d\n", k, serverhostname, serverip, serverport);

		   		for (k =0; k<MAXCONNS; k++)
				{
					if (clientfdlist[k]< 0)
					{	
						continue;
					}	
					
					else
					{	
						char tempip1[INET_ADDRSTRLEN], tempip2[INET_ADDRSTRLEN]; 
						inet_ntop(AF_INET, &(clientconns[k].sin_addr), tempip1, sizeof(tempip1));

						for (l=0; l<MAXCLIENTS; l++)
							
							{
								inet_ntop(AF_INET, &(info[l].clientaddrlist.sin_addr), tempip2, sizeof(tempip2));
								if (!strcmp(tempip1, tempip2))
								{
									clientport = ntohs(info[l].clientaddrlist.sin_port);
									break;
								}
							}
						
						struct hostent *he;
						struct in_addr ipv4addr;
						inet_ntop(AF_INET, &(clientconns[k].sin_addr), clientip, sizeof(clientip));
						inet_pton(AF_INET, clientip, &ipv4addr);
						he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);

						printf("%-5d%-35s%-20s%-8d\n", clientfdlist[k], he->h_name, clientip, clientport);
					}
				}
			}


		   
		   else if (!strcmp(args[0], "TERMINATE"))
		   {
		   		if (argsc != 2)
				{
	    			printf("Not enough arguments\n");
	    			continue;
				}

		   		int connid;
		   		int found3 = 0;
		   		connid = atoi(args[1]);

		   		if (connid == 1)
		   		{
		   			printf("De-registering from server and closing existing connections...\n");
		   			close(regfd);
		   			
		   			for (j=0; j<MAXCONNS; j++) //clear clientconns array
					{
						bzero(&clientconns[j], sizeof(struct sockaddr_in));	
					}

					for (j=0; j<MAXCLIENTS; j++)  //clear Server-IP List
					{
						bzero(&info[j], sizeof(struct clientinfo));	
					}

					bzero(&serverip, sizeof(serverip));
					serverport = 0;


		   			for (j =0; j<MAXCONNS; j++)
		   			{
		   				if (clientfdlist[j]< 0)
						{
							continue;
						}

						copyfd = clientfdlist[j];
						close(copyfd);
			   			FD_CLR(copyfd, &masterfdset);  //clear the entry from masterfdset
						clientfdlist[j] = -1; // remove from client list
						break;
		   			
		   			}
		   			
		   			continue;
		   		}

		   		for (j =0; j<MAXCONNS; j++)
		   		{
		   			if (connid == clientfdlist[j])
		   			{
		   				found3 = 1;
		   				close(connid);
		   				FD_CLR(connid, &masterfdset);  //clear the entry from masterfdset
						clientfdlist[j] = -1; // remove from client list
						bzero(&clientconns[j], len1);
		   				break;
		   			}
		   		}


		   		if(found3 == 0)
		   		{
		   			printf("Please enter a valid connection id\n");
		   			continue;
		   		} 	

		   	}

		  
		   else if (!strcmp(args[0], "EXIT"))
		   {
		   		for (j=0; j<MAXCONNS; j++)
				{

					if (clientfdlist[j]< 0)
					{
						continue;
					}

					copyfd = clientfdlist[j];
							
					close(copyfd);
					FD_CLR(copyfd, &masterfdset);  //clear the entry from masterfdset
					clientfdlist[j] = -1; // remove from client list
					bzero(&clientconns[j], len1);
					
				}
				
				close(regfd);
				close(listenfd);
				
				exit(0);
		   }

		   else if (!strcmp(args[0], "UPLOAD"))
		   {
		   		printf("UPLOAD not implemented yet\n");
		   }


		   else if (!strcmp(args[0], "DOWNLOAD"))
		   {
		   		printf("DOWNLOAD not implemented yet\n");
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

		
		//checking for new connections by checking listenfd

		if (FD_ISSET (listenfd, &readfdset)) 
		{
						
			if ( (newfd = accept(listenfd, (struct sockaddr*)&client, &len1)) < 0) //accept connection from client
			{	
				printf ("%d\n",newfd);
				perror ("accept error");
				exit(-1);
			}

			inet_ntop(AF_INET, &client.sin_addr, clientip, sizeof(clientip));
			printf("Accepted connection from %s\n", clientip);
						
			for (i=0; i<MAXCONNS; i++)
			{
				if (clientfdlist[i]< 0)
				{
					clientfdlist[i]= newfd;  //add descriptor to clientfdlist[] array
					clientconns[i] = client;
					break;
				}		
		
			}

			//clientfdlist[] is already full. do not accept more connections.
			
			if (i == MAXCONNS)
			{
				printf("Error: Too many clients. Unable to accept connection\n"); 				
				strcpy(unablemessage_s.message,"Error Too many clients. Unable to connect");
				
				if (numsent = send(newfd, &unablemessage_s, len3, 0) <0)
				{
					printf ("%d\n", numsent);
					perror ("send error");
				}

				continue;
			}


			if (i >clientlistindex)
			{
				clientlistindex = i;
			}

			//add new descriptor to the masterfdset
			FD_SET(newfd, &masterfdset);

			if (newfd> maxfdnumber)
			{	
				maxfdnumber = newfd;
			}

			
			ready--;
			if (ready <= 0) 
			{
				continue;
			}


		}

		
		//for receiving Server-IP list from server or unable to connect message

		if (FD_ISSET (regfd, &readfdset)) 
		{
			numrecv = recv(regfd, buffer, len2, 0) ;

			if (numrecv < 0)
			{
			    printf ("%d\n",numrecv);
				perror ("recv() error");
				exit(-1);
	
			}

			else if (numrecv == 0)
			{
					close(regfd);   // SERVER TERMINATED
			}

			else 
			{

				void *ptr = info;
				memcpy(ptr, buffer, len2);


				if (info[0].id == 1)
				{
					printf("New Server-IP List obtained:\n");

					struct hostent *he;
					struct in_addr ipv4addr;
					inet_pton(AF_INET, serverip, &ipv4addr);  //TODO remove?
					he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
					int host_id = 1;

					
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

				else if (info[0].id == 0)
				{
					unablemessage_r= (struct unabletoconnect*)buffer;
					fflush(stdout);
					printf("%s", unablemessage_r->message);
					fflush(stdout);
				}
			}


			ready--;
			if (ready <= 0) 
			{
				continue;
			}
		}

		//check if connected clients have terminated or exited
		//check if clients have data
		for (i=0; i <= clientlistindex; i++)   
											
		{
			if (clientfdlist[i]<0)
			{
				continue;
			}

			copyfd = clientfdlist[i];			
			
			if (FD_ISSET(copyfd, &readfdset))
			{
				
				numrecv = recv(copyfd, buffer, MAX, 0);
				if ( numrecv == 0) //connection closed by client, client TCP sends a FIN 
				{
					printf("Connection closed by a client\n");
					close(copyfd);  
					FD_CLR(copyfd, &masterfdset);  //clear the entry from masterfdset
					clientfdlist[i] = -1; // remove from client list
					bzero(&clientconns[i], len1);

				}
				
				
				else if (numrecv < 0)
				{
				    printf ("%d\n",numrecv);
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

