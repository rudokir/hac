#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

// Network settings
#define PORT "8080"	// Server listening port

// File paths
#define DB_FILE "database.txt"      // Main database/log file
#define SW_LIGHT "sw_light.txt"	    // Light switch history
#define LIGHT "light.txt"           // Current light state
#define SW_BOILER "sw_boiler.txt"   // Boiler switch history
#define BOILER "boiler.txt"         // Current boiler state

// Buffer sizes and state messages
#define MAXDATASIZE 128             // Maximum data buffer size
#define STATESIZE 30		    // State message buffer size

// Device state messages
#define LIGHT_ON "Light is on"
#define LIGHT_OFF "Light is off"
#define BOILER_ON "Boiler is on"
#define BOILER_OFF "Boiler is off"

// Function declarations
void *client_req();                 // Thread 1: Handle client connections
void *file_hand();                  // Thread 2: Handle file operations
void *bckp();                       // Thread 3: Handle backup operations
void *get_in_addr(struct sockaddr *sa);
void init_system();
void KILLhandle(int sig);
void getDateTime(int* day, int* month, int* year, int* hours, int* mins);
void logPrint(char* str, char* file_path, int isToCmd, int printDateTime);
ssize_t safe_write(int fd, const void *buf, size_t count);

// Global mutex for backup operations
pthread_mutex_t lock_bu;

// Global file pointers and descriptors
FILE *db_file, *sw_light, *light, *sw_boiler, *boiler;
int iret1, iret2, iret3, fd_db, fd_swl, fd_swb, fd_l, fd_b, enable = 0;
char buf[MAXDATASIZE], dbuf[MAXDATASIZE], light_last[STATESIZE], boiler_last[STATESIZE], temp_buf[MAXDATASIZE];

void main()
{
	pthread_t client_request, file_handaling, backup;
	char *msg1 = "Client Request...";
	char *msg2 = "Handaling Files...";
	char *msg3 = "Backup Message...";

   	signal(SIGINT, KILLhandle);
    	init_system();

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
    	logPrint("[Server] Do you really want to exit? [y/n] \n", DB_FILE, 0, 1);
    	c = getchar();
    	if (c == 'y' || c == 'Y')
    	{
        	close(fd_db);
        	close(fd_swl);
        	close(fd_swb);
        	close(fd_l);
        	close(fd_b);
        	logPrint("[Server] All files closed\n", DB_FILE, 1, 1);

        	pthread_mutex_destroy(&lock_bu); //forcibly remove mutex
        	logPrint("[Server] Mutex terminated\n", DB_FILE, 1, 1);

        	pthread_join(iret1,NULL);
        	pthread_join(iret2,NULL);
        	pthread_join(iret3,NULL);
        	logPrint("[Server] All threads joined\n", DB_FILE, 1, 1);

        	logPrint("[Server] Server Closed successfully.\n", DB_FILE, 1, 1);
        	exit(0);
    	} else {
        	signal(SIGINT, KILLhandle);
    	}
    	getchar();
}

void init_system()
{
	if((db_file = fopen(DB_FILE, "r")) != NULL) {
        	fclose(db_file);
    	} else {
        	printf("[Server] DB file doesn't exist. Creating...\n");
        	db_file = fopen(DB_FILE, "w+");
        	logPrint("[Server] DB file has been created\n", DB_FILE, 1, 1);
        	fprintf(db_file, "%s", "*********** SMART HOME SERVER ***********\n");
        	fclose(db_file);
    	}

    	if((sw_light = fopen(SW_LIGHT, "r")) != NULL) {
        	fclose(sw_light);
    	} else {
        	logPrint("[Server] Light switch file doesn't exist. Creating...\n", DB_FILE, 1, 1);
        	sw_light = fopen(SW_LIGHT, "w+");
        	logPrint("[Server] Light switch file has been created\n", DB_FILE, 1, 1);
        	fprintf(sw_light, "%s", "Light switch log file:\n");
        	fclose(sw_light);
    	}

    	if((sw_boiler = fopen(SW_BOILER, "r")) != NULL) {
        	fclose(sw_boiler);
    	} else {
        	logPrint("[Server] Boiler switch file doesn't exist. Creating...\n", DB_FILE, 1, 1);
        	sw_boiler = fopen(SW_BOILER, "w+");
        	logPrint("[Server] Boiler switch file has been created\n", DB_FILE, 1, 1);
        	fprintf(sw_boiler, "%s", "Boiler switch log file:\n");
        	fclose(sw_boiler);
    	}

    	if((light = fopen(LIGHT, "r")) != NULL) {
        	size_t bytes_read = fread(&light_last, sizeof(light_last) - 1, STATESIZE, light);
		if (bytes_read >= 0) {
			light_last[bytes_read] = '\0';
		}
        	fclose(light);
    	} else {
        	logPrint("[Server] Lights file doesn't exist. Creating...\n", DB_FILE, 1, 1);
        	sprintf(light_last, LIGHT_OFF);

        	light = fopen(LIGHT, "w+");
        	logPrint("[Server] Lights file has been created\n", DB_FILE, 1, 1);
        	fprintf(light, "%s", light_last);
        	fclose(light);
    	}

    	if((boiler = fopen(BOILER, "r")) != NULL) {
        	size_t bytes_read = fread(&boiler_last, sizeof(boiler_last) - 1, STATESIZE, boiler);
		if (bytes_read >= 0) {
			boiler_last[bytes_read] = '\0';
		}
        	fclose(boiler);
    	} else {
        	logPrint("[Server] Boiler file doesn't exist. Creating...\n", DB_FILE, 1, 1);
        	sprintf(boiler_last, BOILER_OFF);

        	boiler = fopen(BOILER, "w+");
        	logPrint("[Server] Boiler file has been created\n", DB_FILE, 1, 1);
        	fprintf(boiler, "%s", boiler_last);
        	fclose(boiler);
    	}

    	fd_db = open(DB_FILE, O_RDWR | O_APPEND, 0);
    	fd_swl = open(SW_LIGHT, O_RDWR | O_APPEND, 0);
    	fd_swb = open(SW_BOILER, O_RDWR | O_APPEND, 0);

    	pthread_mutex_init(&lock_bu, 0); //initialize mutex variable

    	enable = 0;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
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

void logPrint(char* str, char* file_path, int isToCmd, int printDateTime)
{
	int day, month, year, hours, mins;
	FILE* fd = fopen(file_path, "a+");
	if (printDateTime) {
		getDateTime(&day, &month, &year, &hours, &mins);
		fprintf(fd, "%2d-%2d-%2d %2d:%2d    ", day, month, year, hours, mins);
	}
	fputs(str, fd);
	fclose(fd);
	if (isToCmd) {
		printf("%s", str);
	}
	fd = NULL;
}

// Error checking function for writes
ssize_t safe_write(int fd, const void *buf, size_t count)
{
	ssize_t ret = write(fd, buf, count);
	if (ret == -1) {
		perror("Write failed");
	}
	return ret;
}

void *client_req() //Thread #1
{
    	fd_set master;    // master file descriptor list
    	fd_set read_fds;  // temp file descriptor list for select()
    	int fdmax;        // maximum file descriptor number

    	int listener;     // listening socket descriptor
    	int newfd;        // newly accept()ed socket descriptor
    	struct sockaddr_storage remoteaddr; // client address
    	socklen_t addrlen;
    	char arrlog[200];

    	int nbytes;

    	char remoteIP[INET6_ADDRSTRLEN];

    	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    	int i, rv;

    	struct addrinfo hints, *ai, *p;

    	sprintf(arrlog,"[Server] Thread 1 Initiated |  User Requests  | ID = %ld\n", pthread_self());
    	logPrint(arrlog, DB_FILE, 1, 1);

    	FD_ZERO(&master);    // clear the master and temp sets
    	FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "[Server] %s\n", gai_strerror(rv));
        	sprintf(arrlog, "[Server] %s", gai_strerror(rv));
        	logPrint(arrlog, DB_FILE, 0, 1);
		exit(1);
	}

	for(p = ai; p != NULL; p = p->ai_next) {
    		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}

		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		//fprintf(stderr, "[Server] Failed to bind\n");
        	//sprintf(arrlog, "[Server] %s", gai_strerror(rv));
        	logPrint("[Server] Error: Failed to bind\n", DB_FILE, 1, 1);
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

    	// listen
    	if (listen(listener, 10) == -1) {
        	//perror("[Server] listen error:");
		logPrint("[Server] Error: Failed to create a listener",DB_FILE,1,1);
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
            		//perror("[Server] select");
            		sprintf(arrlog,"[Server] Error: %s",strerror(errno));
            		logPrint(arrlog,DB_FILE,1,1);
            		exit(4);
        	}

        	// run through the existing connections looking for data to read
        	for(i = 0; i <= fdmax; i++) {
            		if (FD_ISSET(i, &read_fds)) { //we have a connection
                		if (i == listener) { // handle new connections
                    			addrlen = sizeof remoteaddr;
					newfd = accept(listener,
						(struct sockaddr *)&remoteaddr,
						&addrlen);
					if (newfd == -1) {
                        			//perror("[Server] accept");
                        			//logPrint("[Server] Error: Failed to create a new socket descriptor",DB_FILE,1,1);
                        			sprintf(arrlog,"[Server] Error accept: %s",strerror(errno));
                        			logPrint(arrlog,DB_FILE,1,1);
                    			} else {
                        			FD_SET(newfd, &master); // add to master set
                        			if (newfd > fdmax) { // keep track of the max
                            				fdmax = newfd;
                        			}
                        			sprintf(arrlog,"[Server] New connection from %s on socket %d\n",
							inet_ntop(remoteaddr.ss_family,
							get_in_addr((struct sockaddr*)&remoteaddr),
							remoteIP, INET6_ADDRSTRLEN),
							newfd);
                        			logPrint(arrlog,DB_FILE,1,1);
                    			}
                		} else { //existing connection.
                    			// handle data from a client
                    			enable = 0;
                    			if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
                        			// got error or connection closed by client
                        			if (nbytes == 0) {
                            				// connection closed
                            				sprintf(arrlog,"[Server] Socket %d hung up\n", i);
                        			} else {
                            				sprintf(arrlog,"[Server] Error rcv: %s",strerror(errno));
                        			}
                        			logPrint(arrlog,DB_FILE,1,1);

                        			close(i); // bye!
                        			FD_CLR(i, &master); // remove from master set
                    			} else {
                        			strcpy(temp_buf,buf);
                        			sprintf(arrlog,"[Server] Data from client (Socket %d): %s\n", i, temp_buf);
                        			logPrint(arrlog,DB_FILE,1,1);

                        			sleep(0.1);
                        			while(1) {
                            				if(enable == 1) {
                                				break;
                            				}
                        			}
                        			send(i, dbuf, sizeof(dbuf), 0);
                        			memset(&temp_buf[0], 0, sizeof(temp_buf));
                        			enable = 0;
                    			}
                		}
            		}
        	}
    	}
}

void *file_hand() //Thread #2
{
    	int day, month, year, hours, mins;
    	char nbuf[MAXDATASIZE];
    	char arrlog[200];
    	struct timespec start_bt, end_bt, start_swt, end_swt;
    	long diff_bt, diff_swt;

    	sprintf(arrlog,"[Server] Thread 2 Initiated |   File Handler  | ID = %ld\n", pthread_self());
    	logPrint(arrlog,DB_FILE,1,1);

    	for(;;) {
        	if (strcmp(buf,"b_on") == 0) {
            		strcpy(temp_buf,buf);
            		if(strcmp(boiler_last, BOILER_OFF) == 0) { //boiler was off, can turn it on
                		getDateTime(&day, &month, &year, &hours, &mins);

                		sprintf(nbuf, "[Server] %2d-%2d-%4d %2d:%2d  Boiler turned on\n", day, month, year, hours, mins);
                		logPrint(nbuf,DB_FILE,1,1);

                		if (safe_write(fd_swb, nbuf, strlen(nbuf)) == -1) {
					logPrint("[Server] Error writing to boiler switch file\n", DB_FILE, 1, 1);
				}

                		clock_gettime(CLOCK_REALTIME, &start_bt);
                		sprintf(dbuf, BOILER_ON);

                		fd_b = open(BOILER, O_WRONLY, 0);
                		if (safe_write(fd_b, BOILER_ON, strlen(BOILER_ON)) == -1) {
					logPrint("[Server] Error writing boiler state\n", DB_FILE, 1, 1);
				}
                		close(fd_b);

                		sprintf(boiler_last, BOILER_ON);
            		} else { //boiler was on, DONT turn it on
                		sprintf(boiler_last, BOILER_ON);
                		sprintf(dbuf, "Boiler is already on\n");
                		logPrint("[Server] user tried changing boiler state to ON, while already ON.\n",DB_FILE,0,1);
            		}
            		enable = 1;
            		memset(&buf[0], 0, sizeof(buf));
        	} else if (strcmp(buf,"b_off") == 0) {
            		strcpy(temp_buf,buf);
            		if(strcmp(boiler_last, BOILER_ON) == 0) { //boiler was on, can turn it off
                		getDateTime(&day, &month, &year, &hours, &mins);

                		sprintf(nbuf, "[Server] %2d-%2d-%4d %2d:%2d  Boiler turned off\n", day, month, year, hours, mins);
                		logPrint(nbuf,DB_FILE,1,1);

                		if (safe_write(fd_swb, nbuf, strlen(nbuf)) == -1) {
					logPrint("[Server] Error writing to boiler switch file\n", DB_FILE, 1, 1);
				}

                		clock_gettime(CLOCK_REALTIME, &end_bt);
                		diff_bt = end_bt.tv_sec - start_bt.tv_sec;
                		sprintf(dbuf, "Boiler is off, it was on for %ld:%ld:%ld\n", diff_bt/3600, diff_bt/60, diff_bt%60);

                		fd_b = open(BOILER, O_WRONLY, 0);
                		if (safe_write(fd_b, BOILER_OFF, strlen(BOILER_OFF)) == -1) {
					logPrint("[Server] Error writing boiler state\n", DB_FILE, 1, 1);
				}
                		close(fd_b);

                		sprintf(boiler_last, BOILER_OFF);
            		} else {
                		sprintf(boiler_last, BOILER_OFF);
                		sprintf(dbuf, "Boiler is already off\n");
                		logPrint("[Server] user tried changing boiler state to OFF, while already OFF.\n",DB_FILE,0,1);
            		}
        		enable = 1;
        		memset(&buf[0], 0, sizeof(buf));
		} else if (strcmp(buf,"sw_on") == 0) {
            		strcpy(temp_buf,buf);
            		if(strcmp(light_last, LIGHT_OFF) == 0) {
                		getDateTime(&day, &month, &year, &hours, &mins);

                		sprintf(nbuf, "[Server] %2d-%2d-%4d %2d:%2d  Light turned on\n", day, month, year, hours, mins);
                		logPrint(nbuf,DB_FILE,1,1);

                		if (safe_write(fd_swl, nbuf, strlen(nbuf)) == -1) {
					logPrint("[Server] Error writing to light switch file\n", DB_FILE, 1, 1);
				}

                		clock_gettime(CLOCK_REALTIME, &start_swt);
                		sprintf(dbuf, LIGHT_ON);

                		fd_l = open(LIGHT, O_RDWR, 0);
                		if (safe_write(fd_l, LIGHT_ON, strlen(LIGHT_ON)) == -1) {
					logPrint("[Server] Error writing light state\n", DB_FILE, 1, 1);
				}
                		close(fd_l);

                		sprintf(light_last, LIGHT_ON);
            		} else {
                		sprintf(light_last, LIGHT_ON);
                		sprintf(dbuf, "Light is already on\n");
                		logPrint("[Server] user tried changing light state to ON, while already ON.\n",DB_FILE,0,1);
            		}
            		enable = 1;
            		memset(&buf[0], 0, sizeof(buf));
        	} else if (strcmp(buf,"sw_off") == 0) {
            		strcpy(temp_buf,buf);
            		if(strcmp(light_last, LIGHT_ON) == 0) {
                		getDateTime(&day, &month, &year, &hours, &mins);

                		sprintf(nbuf, "[Server] %2d-%2d-%4d %2d:%2d  Light turned off\n", day, month, year, hours, mins);
                		logPrint(nbuf,DB_FILE,1,1);

                		if (safe_write(fd_swl, nbuf, strlen(nbuf)) == -1) {
					logPrint("[Server] Error writing to light switch file\n", DB_FILE, 1, 1);
				}

                		clock_gettime(CLOCK_REALTIME, &end_swt);
                		diff_swt = end_swt.tv_sec - start_swt.tv_sec;
                		sprintf(dbuf, "Light is off, it was on for %ld:%ld:%ld\n", diff_swt/3600, diff_swt/60, diff_swt%60);

                		fd_l = open(LIGHT, O_RDWR, 0);
                		if (safe_write(fd_l, LIGHT_OFF, strlen(LIGHT_OFF)) == -1) {
					logPrint("[Server] Error writing light state\n", DB_FILE, 1, 1);
				}
                		close(fd_l);

                		sprintf(light_last, LIGHT_OFF);
            		} else {
                		sprintf(light_last, LIGHT_OFF);
                		sprintf(dbuf, "Light is already off\n");
                		logPrint("[Server] user tried changing light state to OFF, while already OFF.\n",DB_FILE,0,1);
            		}
            		enable = 1;
            		memset(&buf[0], 0, sizeof(buf));
        	}
    	}
}

void *bckp() //Thread #3
{
    	FILE *bckp;
    	int day, month, year, hours, mins, bu_day, fp_prev, fp_eof, trunc;
    	char *psrc, *pdest;
    	char arrlog[200];

    	sprintf(arrlog,"[Server] Thread 3 Initiated |     Backup      | ID = %ld\n", pthread_self());
    	logPrint(arrlog,DB_FILE,1,1);
    	bu_day = 0;

    	while(1) {
        	getDateTime(&day, &month, &year, &hours, &mins);
        	sleep(10);
        	if((hours == 03 && mins == 00) && (day != bu_day)) {
            		if(pthread_mutex_lock(&lock_bu)) {
                		logPrint("[Server] Backup Failed, Mutex lock Error.\n",DB_FILE,1,1); 
                		continue;
            		} else {
                		sprintf(arrlog,"[Server] Backup started at %2d-%2d-%4d %2d:%2d  \n", day, month, year, hours, mins);
                		logPrint(arrlog,DB_FILE,1,1);
            		}

            		close(fd_db);
            		close(fd_swl);
            		close(fd_swb);

            		//Backup DB file
            		db_file = fopen(DB_FILE,"r");
            		bckp = fopen("backup_db.txt", "w+");
            		fp_prev = ftell(db_file);
            		fseek(db_file,0,SEEK_END);
            		fp_eof = ftell(db_file);
            		fseek(db_file,fp_prev,SEEK_SET);
            		psrc = mmap((caddr_t)0, fp_eof, PROT_READ, MAP_SHARED, fileno(db_file), 0);
            		if (psrc == (caddr_t)(-1)) {
		        	//perror("mmap DB source file");
                		sprintf(arrlog,"[Server] Error mmap DB source file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
				exit(1);
	        	}
            		trunc = ftruncate(fileno(bckp), fp_eof);
	        	if (trunc == -1) {
	        		//perror ("ftruncate - size of dest file as source file");
                		sprintf(arrlog,"[Server] Error ftruncate - size of dest file as source file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
	        	}
            		pdest = mmap((caddr_t)0, fp_eof, PROT_WRITE | PROT_READ, MAP_SHARED, fileno(bckp), 0);
            		if (pdest == (caddr_t)(-1)) {
                		//perror("mmap backup DB destination file");
                		sprintf(arrlog,"[Server] Error mmap backup DB destination file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
                		exit(1);
            		}
            		memcpy(pdest,psrc,fp_eof);
            		munmap(pdest,fp_eof);
            		munmap(psrc,fp_eof);
            		fclose(db_file);
            		fclose(bckp);

            		//Backup Light SW file
            		sw_light = fopen(SW_LIGHT,"r");
            		bckp = fopen("backup_sw_light.txt", "w+");
            		fp_prev = ftell(sw_light);
            		fseek(sw_light,0,SEEK_END);
            		fp_eof = ftell(sw_light);
            		fseek(sw_light,fp_prev,SEEK_SET);
            		psrc = mmap((caddr_t)0, fp_eof, PROT_READ, MAP_SHARED, fileno(sw_light), 0);
            		if (psrc == (caddr_t)(-1)) {
		        	//perror("mmap Light SW source file");
                		sprintf(arrlog,"[Server] Error mmap Light SW source file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
		        	exit(1);
	        	}

            		trunc = ftruncate(fileno(bckp), fp_eof);
	        	if (trunc == -1) {
	        		//perror ("ftruncate - size of dest file as source file");
                		sprintf(arrlog,"[Server] Error ftruncate - size of dest file as source file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
	        	}
            		pdest = mmap((caddr_t)0, fp_eof, PROT_WRITE | PROT_READ, MAP_SHARED, fileno(bckp), 0);
            		if (pdest == (caddr_t)(-1)) {
                		//perror("mmap Light SW backup destination file");
                		sprintf(arrlog,"[Server] Error mmap Light SW backup destination file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
                		exit(1);
            		}
            		memcpy(pdest,psrc,fp_eof);
            		munmap(pdest,fp_eof);
            		munmap(psrc,fp_eof);
            		fclose(sw_light);
            		fclose(bckp);

            		//Backup Boiler SW file
            		sw_boiler = fopen(SW_BOILER,"r");
            		bckp = fopen("backup_sw_boiler.txt", "w+");
            		fp_prev = ftell(sw_boiler);
            		fseek(sw_boiler,0,SEEK_END);
            		fp_eof = ftell(sw_boiler);
            		fseek(sw_boiler,fp_prev,SEEK_SET);

            		psrc = mmap((caddr_t)0, fp_eof, PROT_READ, MAP_SHARED, fileno(sw_boiler), 0);

            		if (psrc == (caddr_t)(-1)) {
		        	//perror("mmap Boiler SW source file");
                		sprintf(arrlog,"[Server] Error mmap Boiler SW source file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
		        	exit(1);
	        	}
            		trunc = ftruncate(fileno(bckp), fp_eof);
	        	if (trunc == -1) {
	        		//perror ("ftruncate - size of dest file as source file");
                		sprintf(arrlog,"[Server] Error ftruncate - size of dest file as source file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
	        	}
            		pdest = mmap((caddr_t)0, fp_eof, PROT_WRITE | PROT_READ, MAP_SHARED, fileno(bckp), 0);
            		if (pdest == (caddr_t)(-1)) {
                		//perror("mmap Boiler SW backup destination file");
                		sprintf(arrlog,"[Server] Error mmap Boiler SW backup destination file: %s",strerror(errno));
                		logPrint(arrlog,DB_FILE,1,1);
                		exit(1);
            		}
            		memcpy(pdest,psrc,fp_eof);
            		munmap(pdest,fp_eof);
            		munmap(psrc,fp_eof);
            		fclose(sw_boiler);
            		fclose(bckp);

            		fd_db = open(DB_FILE, O_RDWR | O_APPEND, 0);
            		fd_swl = open(SW_LIGHT, O_RDWR | O_APPEND, 0);
            		fd_swb = open(SW_BOILER, O_RDWR | O_APPEND, 0);
            		if(!pthread_mutex_unlock(&lock_bu)) {
                		getDateTime(&day, &month, &year, &hours, &mins);
                		sprintf(arrlog,"[Server] Backup Finished at %2d-%2d-%4d %2d:%2d  \n", day, month, year, hours, mins);
                		logPrint(arrlog,DB_FILE,1,1);
            		}
            		bu_day = day;
        	}
    	}
}
