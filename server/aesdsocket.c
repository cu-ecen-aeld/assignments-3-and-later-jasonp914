#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/queue.h>
#include <pthread.h>
#include <time.h>
#include "aesdsocket.h"

#define PORT 9000
#define MAXREAD 21000
#define MAXPENDINGQUEUE 5
#define SOCKETDATA_FN "/var/tmp/aesdsocketdata"
#define TSINT 10

/* Signal handler */
bool caught_signal = false;
static void signal_handler(int signal_number){
    if (signal_number == SIGINT){
	caught_signal = true;
    }else{
	caught_signal = true;
    }
}

void* write_timestamps(void* thread_param){ 
    struct thread_data* td = (struct thread_data *) thread_param;
    time_t now; 
    struct tm nowtm;
    char timestr[30];
    FILE *fp;
    bool ts_writing = true;
    unsigned int sleep_left;
    while(ts_writing){
        sleep_left = sleep(TSINT);

	if(sleep_left > 0){
	    ts_writing = false;
	    pthread_exit(NULL);
	}
	time(&now);

        int rc = pthread_mutex_lock(td->mutex);
        if(rc != 0){
    	    printf("pthread_mutex_lock failed with %d\n", rc);
    	    td->thread_complete = true;
    	    pthread_exit(NULL);
        }
	
        fp = fopen(SOCKETDATA_FN, "a");

        nowtm = *localtime(&now);
	strftime(timestr, sizeof(timestr), "%a, %d %b %Y %T %z", &nowtm);

      	//printf("now: %d-%02d-%02d %02d:%02d:%02d\n", nowtm.tm_year + 1900, nowtm.tm_mon + 1, nowtm.tm_mday, nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec);

	fprintf(fp, "timestamp:%s\n", timestr);
	printf("timestamp:%s\n", timestr);
        fclose(fp);
	
	rc = pthread_mutex_unlock(td->mutex);
        if(rc != 0){
            printf("pthread_mutex_unlock failed with %d\n",rc);
            td->thread_complete = true;
            pthread_exit(NULL);
	}
    }


    return td;
}
void* process_connection(void* thread_param){ 
    struct thread_data* td = (struct thread_data *) thread_param;
    struct sockaddr_in cli = td->cli;
    int connfd = td->connfd;

    FILE *fp;
    // Log message to the syslog "Accepted Connection from xxx" IP address
    struct sockaddr_in * addr_in = (struct sockaddr_in *)&cli;
    char * ip = inet_ntoa(addr_in->sin_addr);
    openlog(NULL, 0, LOG_USER);
    syslog(LOG_DEBUG, "Accepted Connection from %s", ip);
    
    int rc = pthread_mutex_lock(td->mutex);
    if(rc != 0){
	printf("pthread_mutex_lock failed with %d\n", rc);
	td->thread_complete = true;
	pthread_exit(NULL);
    }
    
    // Receive data over the connection and append to file SOCKETDATA_FN=/var/tmp/aesdsocketdata
    fp = fopen(SOCKETDATA_FN, "a");
    //    use newline to separate data packets recieved each new line should result in an append
    
    // Read a byte until a packet is received designated by \n
    bool pkt_reading = true;
    char buff_char;
    while (pkt_reading){
        read(connfd, &buff_char, sizeof(buff_char));
        fprintf(fp, "%c", buff_char);
    
         if(buff_char == '\n'){
             //printf(" ... Packet Received ...\n");
             pkt_reading = false;
         }    	    
    }
    
    fclose(fp);
    
    // Return the full content of SOCKETDATA_FN=/var/tmp/aesdsocketdata to the client after a line is received.    
    char rbuff[MAXREAD];
    fp = fopen(SOCKETDATA_FN, "r");
    int line_length, woutlen;
    while(fgets(rbuff, MAXREAD, fp) != 0){
        //printf("Read message : %s\n", rbuff);
        line_length = -1;
        for(int i = 0; i < MAXREAD; i++){
            if (rbuff[i] == '\n'){
                line_length = i+1;
        	printf("\n\n-- Found Line length to be %d  --\n", line_length);
        	break;
            }
        }
        if(line_length == -1){
            line_length = MAXREAD;
        }
        woutlen = write(connfd, rbuff, line_length);
        printf("-- Write out lengh %d  --\n", woutlen);
    }
    
    rc = pthread_mutex_unlock(td->mutex);
    if(rc != 0){
        printf("pthread_mutex_unlock failed with %d\n",rc);
	td->thread_complete = true;
	pthread_exit(NULL);
	// TODO Thread messed up
    }
    // Log message to syslog "Closed connection from XXX"
    syslog(LOG_DEBUG, "Closed Connection from %s", ip);
    //close(sockfd);
    close(connfd);

    td->thread_complete = true;
    pthread_exit(NULL);

}


int main(int argc, char *argv[]){
    printf("Welcome to the server thing!\n");

    int sockfd, connfd;
    unsigned int len;
    struct sockaddr_in servaddr, cli;

    /* Multithreading data */
    pthread_mutex_t mutex;

    pthread_mutex_init(&mutex, NULL);

    //struct thread_data * td = malloc(sizeof *td);
    struct thread_data_list tdl;
    SLIST_INIT(&tdl.headp);
    
    /* Signal handler variables */
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler = signal_handler;
    if(sigaction(SIGTERM, &new_action, NULL) != 0){
	printf("sig action failed... \n");
	return -1;
    }
    if(sigaction(SIGINT, &new_action, NULL) != 0){
	printf("sig action failed... \n");
	return -1;
    }

    // Open a stream socket bound to port 9000, failing and return -1
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        printf("Socket creation failed\n");
	return -1;
    }

    // Reuse port
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0 )
	printf("SetSockOPT failed");

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
	printf("Socket bind failed\n");
	return -1;
    }
    
    if(argc == 2){
	printf("%s\n", argv[1]);
	pid_t pid = fork();

	if(pid == 0){
	    // Child process
	}else{
  	    exit(0);
            setsid();
            chdir("/");
	}


    }
    // Listen for and accept a connection
    if ((listen(sockfd, MAXPENDINGQUEUE)) != 0) {
        printf("socket listen failed\n");
    	return -1;
    }
    
    len = sizeof(cli);
    
    
    struct thread_data * td = malloc(sizeof *td);
    pthread_t thread;
    td->mutex = &mutex; 
    td->thread = &thread; 

    int rc = pthread_create(td->thread, NULL, write_timestamps, td);
    if(rc != 0){
	printf("pthread_create failed\n");
	// TODO some failure here...
    }else{
        // Thread create successful 
    }

    while (!caught_signal){
        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (!caught_signal){
            if (connfd < 0){
    	        printf("server accept failed\n");
    	        return -1;
            }
	    
            struct thread_data * td = malloc(sizeof *td);

            pthread_t thread;
            td->mutex = &mutex; 
            td->thread = &thread; 
            td->cli = cli; 
            td->connfd = connfd; 

	    SLIST_INSERT_HEAD(&tdl.headp, td, nextp);
            
	    rc = pthread_create(td->thread, NULL, process_connection, td);
	    if(rc != 0){
		printf("pthread_create failed\n");
		// TODO some failure here...
            }else{
                // Thread create successful 
	    }
	    
	    //process_connection(td);

	    struct thread_data * tde;
            SLIST_FOREACH(tde,&tdl.headp, nextp){
		if(tde->thread_complete){
                    printf("Read : %d\n", 1);
		}else{
                    printf("Read : %d\n", 0);
		}
	    }

	    
	    // Restart accepting connection in a loop until SIGINT or SIGTERM is recieved
	}else{
	    // Gracefully shutdown when SIGINT or SIGTERM is received:
            // complete any open connection operations
            // closing any open sockets
            close(sockfd);
            // delete the file /var/tmp/aesdsocketdata
	    remove(SOCKETDATA_FN); 
            // Log to syslog "Caught siganl, exiting"
            syslog(LOG_DEBUG, "Caught Signal, Exiting");
            printf("Signal Caught\n");
	}
    }
    
    closelog();
    return 0;
}
