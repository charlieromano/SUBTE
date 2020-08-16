/*
 * server.c -- a stram socket server demo
 * run in one terminal "./server"
 * open other terminal and run "telnet localhost 3490"
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <errno.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <netdb.h>
#include    <arpa/inet.h>
#include    <sys/wait.h>
#include    <signal.h>
#include    <pthread.h>

#define    PORT    "10000"
#define    BACKLOG    10

void sigint_handler(int sig);
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);
void sigall_block(void);
void sigall_unblock(void);
void* start_thread (void* message);

int main(void)
{

	/* Threading */
	pthread_t th2; // el "file desc." del thread 2
	const char * msg = "Thread secundario";
	int ret;

	/* Signal SIGINT handling */
    void sigint_handler(int sig);
    struct sigaction sa1;

    sa1.sa_handler = sigint_handler;
    sa1.sa_flags =  SA_RESTART;// or 0
    sigemptyset(&sa1.sa_mask);

    if(sigaction(SIGINT, &sa1, NULL) == -1 ){
            perror("sigaction");
            exit(1);
    }

	sigall_block();
	/* Thread secundario */
	ret = pthread_create(&th2, NULL, start_thread, (void*)msg);
	/*if(!ret){
		errno = ret;
		perror("pthread_create");
		return -1;
	}*/

	//pthread_join(th2, NULL);
	sigall_unblock();


    /* TCP Server */
	int sockfd, new_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	/* Set socket */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/*
	 * 1: getaddrinfo()
	 * 2: socket()
	 * 3: bind()
	 * 4: listen()
	 * 
	 */

	if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo))!=0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p=servinfo; p != NULL; p = p->ai_next){
	    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
	        perror("server: socket");
	        continue;
	    }
	    /* Reuse the port if bind() fails */
	    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
	        perror("setsockopt");
	        exit(1);
	    }

	    if (bind(sockfd, p->ai_addr, p->ai_addrlen)==-1){
	        close(sockfd);
	        perror("server: bind");
	        continue;
	    }

	    break;
	}


	freeaddrinfo(servinfo);

	if(p==NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1){
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL)==-1){
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	/* accept() connections create a new socket (their_addr) each time, then
	 * fork() creating a child process to manage the incoming connection while
	 * continue listen()ing.
	 */
	while(1){
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1){
		    perror("accept");
		    continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		printf("server: got connection from %s\n", s);

		if(!fork()){
			close(sockfd);
			if(send(new_fd, "Hello, world!", 13, 0) == -1)
			    perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}


	
	return 0;

}



void sigint_handler(int sig)
{
        write(0,"mymsg:SIGINT Ctrl+C user entry\n",53);
}

void sigchld_handler(int s)
{
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG)>0);

	errno = saved_errno;
}



void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET){
	    return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigall_block(void)
{
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void sigall_unblock(void)
{
	sigset_t set;
	int s;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void* start_thread (void* message)
{
	while(1){
	printf("%s\n",(const char *) message);
	sleep(5);

	}//return NULL;
}

