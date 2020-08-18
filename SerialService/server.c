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
#include    <sys/stat.h>
#include    <fcntl.h>
#include    "SerialManager.h"

#define     PORT          "10000"
#define     BACKLOG       10
#define     BAUD_RATE     115200
#define     MAX_MSG_LEN   50
#define     HEADER_INPUT  ">TOGGLE STATE:"
#define     HEADER_STATES ":STATES"
#define     FIFO_1     "q1"
#define     FIFO_2     "q2"

void sigint_handler(int sig);
void sigkill_handler(int sig);
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);
void sigall_block(void);
void sigall_unblock(void);
void* start_thread (void* message);


static	int new_fd;
static	socklen_t sin_size;
static	struct sigaction sa;

pthread_mutex_t mutexData = PTHREAD_MUTEX_INITIALIZER;
static int new_fd_flag=0;

int main(void)
{
	int    sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	char   buf[MAX_MSG_LEN];
	char   s[INET6_ADDRSTRLEN];
	int    byte_count;
	int    rv;
	int    yes=1;

	/* socket data in, serial data out*/
	char sock_data_in[MAX_MSG_LEN];
	char serial_msg_out[MAX_MSG_LEN];
	int out0, out1, out2, out3;

	int ret, rets;


	/* Threading */
	pthread_t th2; // el "file desc." del thread 2
	const char * msg = "Thread th2";

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
    /* Signal SIGTSTP handling */
    void sigint_handler(int sig);
    struct sigaction sa2;
    sa2.sa_handler = sigkill_handler;
    sa2.sa_flags =  SA_RESTART;// or 0
    sigemptyset(&sa2.sa_mask);
    if(sigaction(SIGTSTP, &sa2, NULL) == -1 ){
        perror("sigaction");
        exit(1);
    }
    /* Signal SIGCHLD handling */
   	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL)==-1){
		perror("sigaction");
		exit(1);
	}


    /* Serial interface */
   	printf("Inicio Serial Service\r\n");
	rets = serial_open(1,BAUD_RATE);
	if(rets!=0){
		perror("serial port");
		exit(1);
	}
	printf("serial_open(): %d\n", rets);



    /* TCP Server,  set socket */
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

	printf("server: waiting for connections...\n");


	sigall_block();
	ret = pthread_create(&th2, NULL, start_thread, (void*) msg);
	if(ret==-1){
		errno = ret;
		perror("pthread_create");
	}
	sigall_unblock();

	new_fd = 0;
	while(1){	

		// accept connection 
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1){
		    perror("accept");
		    continue;
		}
		if (new_fd > 0){ //HARDCODED 5:
			pthread_mutex_lock(&mutexData);
			new_fd_flag = 1;
			pthread_mutex_unlock(&mutexData);
		}
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		//printf("server accept(): got connection from %s\n", s);

		
		// socket receive, serial send
		byte_count = recv(new_fd, buf, sizeof buf,0);
		if (byte_count==-1){
			perror("recv()");
		}
		else
		{
			sprintf(sock_data_in, "%s", buf);
			printf("socket data received (%d bytes):%s\n",byte_count, sock_data_in);
			close(new_fd);

			if( strncmp(sock_data_in, HEADER_STATES, 7 ) == 0){
				out0=sock_data_in[7]-'0';
				out1=sock_data_in[8]-'0';
				out2=sock_data_in[9]-'0';
				out3=sock_data_in[10]-'0';
				sprintf(serial_msg_out,">OUTS:%d,%d,%d,%d\r\n",out0,out1,out2,out3);
				printf("%s\n", serial_msg_out);
				serial_send(serial_msg_out,17); //send to serial interface
			}
		}

		sleep(1);
	}

	pthread_join(th2, NULL);


	return 0;

}



void sigint_handler(int sig)
{
    write(0,"myMsg:SIGINT Ctrl+C user entry\n",31);
    kill(getpid(), SIGINT);
    exit(EXIT_SUCCESS);
}

void sigkill_handler(int sig)
{
    write(0,"myMsg:SIGTSTP Ctrl+Z user entry\n",31);
    kill(getpid(), SIGTSTP);
    exit(EXIT_SUCCESS);
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
	char str[MAX_MSG_LEN];
	char buffer[MAX_MSG_LEN];
	char sock_data_out[MAX_MSG_LEN];
	int  retrcv, byte_count;
	int line_num;

	while(1){

		retrcv = serial_receive(buffer, MAX_MSG_LEN);
		if(retrcv){
			sprintf(str,"%s", buffer);
			//printf("Serial received %s", str);

			if( strncmp(str, HEADER_INPUT, 14 ) == 0){
				line_num = str[14] - '0';
				printf("%d\n", line_num);
				sprintf(sock_data_out,":LINE%dTG\n", line_num);
			
				
				if(new_fd_flag==1){

				    // socket send data 
				    byte_count = sizeof(sock_data_out);
					if(send(new_fd, sock_data_out, byte_count, 0) == -1){
						perror("send");
					}

					printf("socket data sent (%d bytes):%s\n",byte_count, sock_data_out);
				}

				pthread_mutex_lock(&mutexData);
				new_fd_flag=0;
				pthread_mutex_unlock(&mutexData);
			}
		}

		usleep(50);
	}
}

