#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#define DATA "Hello World of socket!"

/*
 *  compile both files
 * 	first run ./server
 * 	then run ./client localhost
 */

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in server;
	struct hostent *hp;
	char buff[1024];

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		perror("socket failed.");
		exit(1);
	}

	server.sin_family = AF_INET;

//get host by the argument passed in
	hp = gethostbyname (argv[1]);
	if(hp == 0)
	{
		perror("gethostbyname failed.");
		close(sock);
		exit(1);
	}

//set the add struct
	memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_port = htons(5050);

//connect
	if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("connect failed");
		close(sock);
		exit(1);
	}

//send DATA
	if(send(sock, DATA, sizeof(DATA), 0) < 0)
	{
		perror("send failed.");
		close(sock);
		exit(1);
	}

//close the sending part of the socket, only need to receive for comfirmation	
	shutdown(sock, SHUT_WR);
	
//ready to receive from the server after sending is done
	char recvBuff[1024];


	int n;
	while ( (n = read(sock, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    } 

//print both sent msg and received comfirmation	
	printf("Sent %s\n", DATA);
	printf("Received %s\n", recvBuff);
	close(sock);

	

	return 0;
}
