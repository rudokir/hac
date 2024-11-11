#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#define PORT "9034"   // port we're listening on
#define DB_FILE "database.txt" // database file (=logger)
#define SW_LIGHT "sw_light.txt"
#define LIGHT "light.txt" // current state
#define SW_BOILER "sw_boiler.txt"
#define BOILER "boiler.txt" // current state
#define MAXDATASIZE 100 // max number of bytes we can get at once

void client_req(); //manage sockets and write to logger and files
void file_hand();  //read from files and calculate time
void bckp();       //scheduled backup files using mmap
void boilers(int state);
void lights(int id, int state);
void switches(int sw_name);
void *get_in_addr(struct sockaddr *sa);
void init_system();
void KILLhandle(int sig);
void getDateTime(int* day, int* month, int* year, int* hours, int* mins);

pthread_mutex_t lock;

FILE *db_file, *sw_light, *light, *sw_boiler, *boiler;
int iret1, iret2, iret3, fd_db, fd_swl, fd_swb, fd_l, fd_b;
char buf[MAXDATASIZE];

void main()
{
	pthread_t client_request, file_handaling, backup;
	char *msg1 = "Client Request...";
	char *msg2 = "Handaling Files...";
	char *msg3 = "Backup Message...";

    signal(SIGINT, KILLhandle);
    init_system();

    pthread_mutex_init(&lock, 0);

	iret1 = pthread_create(&client_request, NULL, client_req, (void*) msg1);
	iret2 = pthread_create(&file_handaling, NULL, file_hand, (void*) msg2);
	iret3 = pthread_create(&backup, NULL, bckp, (void*) msg3);
	
	while(1);
}

void KILLhandle(int sig)
{
    char c;
    signal(sig,SIG_IGN);
    printf("\n[Server] Do you really want to exit? [y/n] ");
    c = getchar();
    if (c == 'y' || c == 'Y')
    {
        /*fclose(db_file);
        fclose(sw_boiler);
        fclose(sw_light);
        fclose(light);
        fclose(boiler);*/

        close(fd_db);
        close(fd_swl);
        close(fd_swb);
        close(fd_l);
        close(fd_b);
        
        pthread_mutex_destroy(&lock);
        pthread_join(iret1,NULL);
        pthread_join(iret2,NULL);
        pthread_join(iret3,NULL);

        printf("Goodbye!\n");
        exit(0);
    }
    else
    {
        signal(SIGINT, KILLhandle);
    }
    getchar();
}

void init_system()
{
	if((db_file = fopen(DB_FILE, "r")) == NULL)
    {
        printf("[Server] DB file doesn't exist. creating.\n");
        db_file = fopen(DB_FILE, "w");
        fclose(db_file);
    }

    if((sw_light = fopen(SW_LIGHT, "r")) == NULL)
    {
        printf("[Server] Light switch file doesn't exist. creating.\n");
        sw_light = fopen(SW_LIGHT, "w");
        fclose(sw_light);
    }

    if((light = fopen(LIGHT, "r")) == NULL)
    {
        printf("[Server] Lights file doesn't exist. creating.\n");
        light = fopen(LIGHT, "w");
        fclose(light);
    }

    if((sw_boiler = fopen(SW_BOILER, "r")) == NULL)
    {
        printf("[Server] Boiler switch file doesn't exist. creating.\n");
        sw_boiler = fopen(SW_BOILER, "w");
        fclose(sw_boiler);
    }

    if((boiler = fopen(BOILER, "r")) == NULL)
    {
        printf("[Server] Switch file doesn't exist. creating.\n");
        boiler = fopen(BOILER, "w");
        fclose(boiler);
    }

    /*db_file = fopen(DB_FILE,"w+");
    sw_light = fopen(SW_LIGHT,"w+");
    light = fopen(LIGHT,"w+");
    sw_boiler = fopen(SW_BOILER,"w+");
    boiler = fopen(BOILER,"w+");*/

    fd_db = open(DB_FILE, O_WRONLY | O_APPEND, 0);
    fd_swl = open(SW_LIGHT, O_RDWR | O_APPEND, 0);
    fd_swb = open(SW_BOILER, O_RDWR | O_APPEND, 0);
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
    {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void getDateTime(int* day, int* month, int* year, int* hours, int* mins)
{
	time_t rawtime = time(NULL);
	struct tm tm = *localtime(&rawtime);

	*day = tm.tm_mday;
	*month = tm.tm_mon + 1;
	*year = tm.tm_year + 1900;
	*hours = tm.tm_hour;
	*mins = tm.tm_min;
}

void client_req() //Thread #1
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    printf("[Thread 1] Initiated | Client Requests | ID = %ld\n", pthread_self());

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) 
    {
		fprintf(stderr, "[Server] %s\n", gai_strerror(rv));
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next) 
    {
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) 
        {
			continue;
		}

		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) 
        {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) 
    {
		fprintf(stderr, "[Server] Failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) 
    {
        perror("[Server] listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    for(;;)
    {
    	read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("[Server] select");
            exit(4);
        }
        
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) 
        {
            if (FD_ISSET(i, &read_fds)) //we have a connection
            { 
                if (i == listener) // handle new connections
                {
                    addrlen = sizeof remoteaddr;
					newfd = accept(listener,
						(struct sockaddr *)&remoteaddr,
						&addrlen);
					if (newfd == -1) 
                    {
                        perror("accept");
                    }
                    else 
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) 
                        {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("[Server] New connection from %s on socket %d\n",
							    inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN),
							    newfd);
                    }
                } 
                else //existing connection.
                {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
                    {
                        // got error or connection closed by client
                        if (nbytes == 0) 
                        {
                            // connection closed
                            printf("[Server] Socket %d hung up\n", i);
                        } else 
                        {
                            perror("[Server] recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } 
                    else 
                    {
                        printf("[Server] Data from client (Socket %d): %s\n", i, buf);
                    }
                }
            }
        }
    } 
}

void file_hand() //Thread #2
{
    int day, month, year, hours, mins;
    char nbuf[MAXDATASIZE], dbuf[MAXDATASIZE];
    time_t start_bt, end_bt, start_swt, end_swt;
    double diff_bt, diff_swt;

    printf("[Thread 2] Initiated | File Handler    | ID = %ld\n", pthread_self());
    for(;;)
    {
        if (!(strcmp(buf,"b_on")))
        {
            getDateTime(&day, &month, &year, &hours, &mins);
            sprintf(nbuf, "%2d-%2d-%4d %2d:%2d  Boiler is on\n", day, month, year, hours, mins);
            write(fd_db, nbuf, strlen(nbuf));
            write(fd_swb, nbuf, strlen(nbuf));
            fd_b = open(BOILER, O_WRONLY, 0);
            write(fd_b, "Boiler is on\n", strlen("Boiler is on\n"));
            close(fd_b);
            time(&start_bt);
        }

        if (!(strcmp(buf,"b_off")))
        {
            getDateTime(&day, &month, &year, &hours, &mins);
            sprintf(nbuf, "%2d-%2d-%4d %2d:%2d  Boiler is off\n", day, month, year, hours, mins);
            write(fd_db, nbuf, strlen(nbuf));
            write(fd_swb, nbuf, strlen(nbuf));
            time(&end_bt);
            diff_bt = difftime(end_bt, start_bt);
            fd_b = open(BOILER, O_WRONLY, 0);
            sprintf(dbuf, "Boiler is off it was on for %.0f:%.0f:%.0f\n", diff_bt/3600, diff_bt/60, diff_bt);
            write(fd_b, dbuf, strlen(dbuf));
            close(fd_b);
        }

        if (!(strcmp(buf,"sw_on")))
        {
            getDateTime(&day, &month, &year, &hours, &mins);
            sprintf(nbuf, "%2d-%2d-%4d %2d:%2d  Light is on\n", day, month, year, hours, mins);
            write(fd_db, nbuf, strlen(nbuf));
            write(fd_swl, nbuf, strlen(nbuf));
            fd_l = open(LIGHT, O_RDWR, 0);
            write(fd_l, "Light is on\n", strlen("Light is on\n"));
            close(fd_l);
            time(&start_swt);
        }

        if (!(strcmp(buf,"sw_off")))
        {
            getDateTime(&day, &month, &year, &hours, &mins);
            sprintf(nbuf, "%2d-%2d-%4d %2d:%2d  Light is off\n", day, month, year, hours, mins);
            write(fd_db, nbuf, strlen(nbuf));
            write(fd_swl, nbuf, strlen(nbuf));
            time(&end_swt);
            diff_swt = difftime(end_swt, start_swt);
            fd_l = open(LIGHT, O_RDWR, 0);
            sprintf(dbuf, "Light is off it was on for %.0f:%.0f:%.0f\n", diff_swt/3600, diff_swt/60, diff_swt);
            write(fd_l, dbuf, strlen(dbuf));
            close(fd_l);
        }
    }
}

void bckp() //Thread #3
{
    printf("[Thread 3] Initiated | Backup          | ID = %ld\n", pthread_self());

    int day, month, year, hours, mins;/*
    while(1)
    {
        if(hours == 3 && mins < 5)
        {
            sleep(10);
            getDateTime(&day, &month, &year, &hours, &mins);
            pthread_mutex_lock(&lock;);
                FILE COPY FROM TIRGUL USING MMAP
            pthread_mutex_unlock(&lock;);
        }
    }*/
}
