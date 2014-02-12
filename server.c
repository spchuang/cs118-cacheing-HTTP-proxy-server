#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>


/*
 *  compile both files
 * 	first run ./server
 * 	then run ./client localhost
 */
 
int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in server;
	int mysock;
	char buff[1024];
	int rval;
	char* Data = "Request Confirmed";

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock<0){
		perror("Failed to create socket.");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(5050);

//bind the sock
	if(bind(sock,(struct sockaddr *)&server, sizeof(server))){
		perror("bind failed.");
		exit(1);
	}

//listen
	listen(sock, 20);

//accept and recv
	do {
		mysock = accept(sock, (struct sockaddr *) 0, 0);
		if(mysock == -1){
			perror("accept failed.");
		}
		else{
			memset(buff, 0, sizeof(buff));
			if((rval = recv(mysock, buff, sizeof(buff), 0)) < 0)
				perror("reading stream message error.");
			else if(rval == 0)
				printf("Ending connection\n");
			else
			{
				printf("MSG: %s\n", buff);
				
				//want to send back a comfirmation through the same socket
				if(send(sock, Data, sizeof(Data), 0) < 0)
				{
					printf("Error");
					perror("response failed");
					close(sock);
				}
				else
				{
					printf("send the response %s\n", Data);
				}
			}	
			printf("Got the message (rval = %d)\n", rval);
			
		}
	} while(1);
	

	return 0;


}
