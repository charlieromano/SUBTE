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
	FILE *fd_out;
	int rets, retrcv, num;
	char buffer[MAX_MSG_LEN];
	char str[MAX_MSG_LEN];
	char msg_aux[MAX_MSG_LEN];
	char file_name[MAX_MSG_LEN];
	char opt, output[MAX_MSG_LEN];

	int value=1;

	printf("Inicio Serial Service\r\n");
	
	rets = serial_open(1,BAUD_RATE);
	printf("serial_open(): %d\n", rets);
	

	while(1)
	{
		retrcv = serial_receive(buffer, MAX_MSG_LEN);
		if(retrcv){
			sprintf(str,"%s", buffer);
			printf("%s", str);


			if( strncmp(str, HEADER_INPUT, 14 ) == 0){
				
				opt = str[14];
				sprintf(file_name, "/tmp/out%c.txt",opt);
				value = !value;
				sprintf(output, "%d",value);

				//printf("%s", output);
				
				fd_out = fopen(file_name,"w");
				if (fd_out == NULL) {
					perror("fopen");
					return 1;
				}
				if ((num = fwrite(output,1, 1, fd_out)) == -1)
				    perror("write");
				else{
					sprintf(msg_aux,"wrote %s, %d bytes in /tmp/out%c.txt\n", "1",num, opt);
					printf("%s", msg_aux);
					fclose(fd_out);
				}				

			}

		}
		sleep(1);
		serial_send(">OUTS:1,1,1,1\r\n",17);
		sleep(1);
		serial_send(">OUTS:0,0,0,0\r\n",17);
		sleep(1);
		serial_send(">OUTS:1,0,0,0\r\n",17);
		sleep(1);
		serial_send(">OUTS:1,1,0,0\r\n",17);
		sleep(1);
		serial_send(">OUTS:1,1,1,0\r\n",17);

	}
	exit(EXIT_SUCCESS);
	return 0;
}
/*
int serial_open(int pn,int baudrate);
void serial_send(char* pData,int size);
void serial_close(void);
int serial_receive(char* buf,int size);
*/

