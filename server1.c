
/* server1.c

   (c) Thomas Azelis 2012

*/

#include "utils.h"

#define MY_PORT                 6767
#define BUF_SIZE                1024
#define MAX_PENDING_CONNECTIONS   10
char **filenames;       // files to search in
int NUM_FILES = 10;         
int THREADS = 10;
int STACK_SIZE=10;
struct timeval total_waiting_time,total_service_time;
int completed_requests=0;

pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mymutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t non_empty_stack = PTHREAD_COND_INITIALIZER;
pthread_cond_t non_full_stack = PTHREAD_COND_INITIALIZER;

struct con{
	int fd;
	struct timeval tv;
};
struct con t[10];
int size=0;

int isFull(){
	return(size==STACK_SIZE?1:0);
}

int isEmpty(){
	return(size==0?1:0);
}

void push(int fd){
	if(!isFull()){
		gettimeofday(&t[size].tv,NULL);
		t[size].fd=fd;
		size++;
	}
}

struct con pop(){
	struct con s;
	if(!isEmpty()){
		size--;
		s.fd=t[size].fd;
		s.tv.tv_sec=t[size].tv.tv_sec;
		s.tv.tv_usec=t[size].tv.tv_usec;
		
		return(s);
	}
	return(s);
}
/*=========================================================*
 *                     process_request                     *
 *=========================================================*/
void process_request(int socket_fd)
{
    char buffer[BUF_SIZE] = "\n", filebuf[BUF_SIZE], word_to_search[BUF_SIZE],word[BUF_SIZE];
    int numbytes, i, occurrences;
	FILE *infile[NUM_FILES];
	for (i = 0; i < NUM_FILES; i++) {
		if((infile[i]=fopen(filenames[i],"r"))==NULL){
			printf("Error opening %s\n",filenames[i]);
			exit(0);
		}
	}

    // receive message (word to search)
    numbytes = read_str_from_socket(socket_fd, word_to_search, BUF_SIZE);

    // search in files and send reply
    for (i = 0; i < NUM_FILES; i++) {

        occurrences = 0;

	while(!feof(infile[i])){
		fscanf(infile[i],"%s",word);
		if(!strcmp(word,word_to_search)){
			occurrences++;
		}		
	}
    sprintf(filebuf, "%s:%d\n", filenames[i], occurrences);
    strcat(buffer, filebuf);

    }

    write_str_to_socket(socket_fd, buffer, strlen(buffer));
}

void *work(void *args){
	
	struct con s;
	struct timeval tv,tv1;
	while(1){
		pthread_mutex_lock(&mymutex);
		if(isEmpty()){
			pthread_cond_wait(&non_empty_stack,&mymutex);
		}
		
		s=pop();
		gettimeofday(&tv,NULL);
		pthread_cond_signal(&non_full_stack);
		pthread_mutex_unlock(&mymutex);
		
		process_request(s.fd);
		gettimeofday(&tv1,NULL);

		pthread_mutex_lock(&mymutex2);
		
		completed_requests++;
		
		total_waiting_time.tv_usec+=tv.tv_usec-s.tv.tv_usec;
		total_waiting_time.tv_sec+=tv.tv_sec-s.tv.tv_sec;
		
		if(total_waiting_time.tv_usec<0){
			total_waiting_time.tv_usec*=-1;
			total_waiting_time.tv_sec-=1;
		}
		if(total_service_time.tv_usec>=1000000){
			total_waiting_time.tv_usec-=1000000;
			total_waiting_time.tv_sec+=1;
		}
		
		total_service_time.tv_usec+=tv1.tv_usec-s.tv.tv_usec;
		total_service_time.tv_sec+=tv1.tv_sec-s.tv.tv_sec;
		
				
		if(total_service_time.tv_usec<0){
			total_service_time.tv_usec*=-1;
			total_service_time.tv_sec-=1;
		}
		
		if(total_service_time.tv_usec>=1000000){
			total_service_time.tv_usec-=1000000;
			total_service_time.tv_sec+=1;
		}
		pthread_mutex_unlock(&mymutex2);
	}	
}
void signal_handler(int signo){
	if(signo==SIGINT){
		pthread_mutex_lock(&mymutex2);
		printf("Synolikes aithseis : %d\n",completed_requests);
		if(completed_requests==0)
			completed_requests++;
					printf("Mesos xronos paramonhs : %ld sec %ld usec\n",total_waiting_time.tv_sec,total_waiting_time.tv_usec);

		printf("Mesos xronos paramonhs : %f sec %f usec\n",total_waiting_time.tv_sec/(float)completed_requests,total_waiting_time.tv_usec/(float)completed_requests);
		printf("Mesos xronos e3yphrethshs : %f sec %f usec\n",total_service_time.tv_sec/(float)completed_requests,total_service_time.tv_usec/(float)completed_requests);
		exit(0);
	}
}
/*=========================================================*
 *                          main                           *
 *=========================================================*/
int main(int argc, char *argv[])
{
    int socket_fd,              // listen on this socket for new connections
    new_fd;                     // use this socket to service a new connection
    socklen_t clen;
    struct sockaddr_in server_addr, // my address information
                       client_addr; // connector's address information
    int i;

	pthread_t threads[THREADS];
	for(i=0;i<THREADS;i++){
		pthread_create(&threads[i],NULL,work,NULL);
	}
    // create array of filenames
    filenames = malloc(NUM_FILES * sizeof(char *));
    for (i = 0; i < NUM_FILES; i++) {
        filenames[i] = malloc(BUF_SIZE);
        sprintf(filenames[i], "samples/f%d.txt", i);
    }

    // create socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR("socket()");
    
    // create socket adress of server (type, IP-adress and port number)
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // any local interface
    server_addr.sin_port = htons(MY_PORT);

    // bind socket to address
    if (bind(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
        ERROR("bind()");

    // start listening to socket for incomming connections
    listen(socket_fd, MAX_PENDING_CONNECTIONS);
    printf("Listening for new connections on port %d ...\n", MY_PORT);
    clen = sizeof(client_addr);

	signal(SIGINT,signal_handler);
    // main loop: wait for new connection/requests
    while (1) {

        // wait for incomming connection
        if ((new_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clen)) == -1) {
            ERROR("accept()");
        }

        // got connection, serve request
        printf("Got connection from '%s'\n", inet_ntoa(client_addr.sin_addr));
		
		pthread_mutex_lock(&mymutex);
		if(isFull()){
			pthread_cond_wait(&non_full_stack,&mymutex);
		}
		
		push(new_fd);
		pthread_cond_signal(&non_empty_stack);
		pthread_mutex_unlock(&mymutex);        
    }

    return 0;
}

