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

#include "SerialManager.h"

#define     BAUD_RATE     115200
#define     MAX_MSG_LEN   50
#define     HEADER_INPUT  ">TOGGLE STATE:"

int main(void)
{
	FILE *fdout;
	int rets, retrcv, num;
	char buffer[MAX_MSG_LEN];
	char str[MAX_MSG_LEN];

	printf("Inicio Serial Service\r\n");
	
	rets = serial_open(1,BAUD_RATE);
	printf("serial_open(): %d\n", rets);
	

	while(1)
	{
		retrcv = serial_receive(buffer, MAX_MSG_LEN);
		if(retrcv){

			printf("%s\n", buffer);
			sprintf(str,"%s", buffer);
			//printf("string %s\n", str);
			if( strncmp(str, HEADER_INPUT, 14 ) == 0){
				printf("El archivo es: %c\r\n",str[14]);
				fdout = fopen("/tmp/out4.txt","w");
				if ((num = fwrite("1",1, 1, fdout)) == -1)
				    perror("write");
				else{
					printf("wrote %s, %d bytes in /tmp/out4.txt\n", "1",num );
					fclose(fdout);
				}				

			}

		}
		//sleep(5);
	}
	exit(EXIT_SUCCESS);
	return 0;
}
