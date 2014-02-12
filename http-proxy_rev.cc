/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>


#include <stdio.h>
#include <cstring>
#include <pthread.h>

//C socket stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#include "http-request.h"


#define PROXY_SERVER_PORT "14886"
#define MAX_THREAD_NUM    20
#define DEFAULT_HTTP_PORT "80"


using namespace std;

typedef struct str_thdata{
  int client_id;
}thread_params;


/*
   Create a socket that could be binded to the port successfully
   returns the socket fd, or -1 if error
*/
int create_socket (const char *port)
{
   // declare address structs
   struct addrinfo hints;
   struct addrinfo *result, *rp;
   int socket_fd, s;
   
   // Set up an addrinfo that specfies the criteria for selecting socket address structures
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;      //allow IPv4 or IPv6
   hints.ai_socktype = SOCK_STREAM;  //socket type: full-duplex..
   hints.ai_flags = AI_PASSIVE;      //wildcard IP addresses
   
   
   s = getaddrinfo(NULL, port, &hints, &result);
   if (s != 0){
      fprintf(stderr, "[SERVER]: Cannot get address info: %s\n", gai_strerror(s));
      return -1;
   }
   
   // Loop through the list of address structure, and try connect until we bind successfully

   for (rp = result; rp != NULL; rp = rp->ai_next){		
      // Create the socket
      socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (socket_fd == -1){
         perror("[SERVER]:  cannot open socket");
         continue;
      }
   
      // Bind the socket to the port (just the first one we can)
      if( bind(socket_fd, rp->ai_addr, rp->ai_addrlen) ==0)
         break;
         
      close(socket_fd);
   }
   
   // No binds happened
   if (rp == NULL){
      fprintf(stderr, "[SERVER]: could not bind\n");
      return -2;
   }
   
   // no need anymore
   freeaddrinfo(result);
   
   return socket_fd;
}

void* ptread_connection(void *params){
   thread_params *tp;
   tp = (thread_params *)params;
   
   cout << "[THREAD DEBUG] client id: " <<tp->client_id<<endl;
   cout <<"Thread exit"<<endl;
   return NULL;
}


int main (int argc, char *argv[])
{
   int socket_fd, client_id;
   struct sockaddr_storage client_addr;
   socklen_t client_len = sizeof client_addr;;
 
//create a rval for storing the numbers of byte proxy receives from the client
//create a buffer to hold the client request
	int rval;
	char buff[1024];
   
   cout <<"[DEBUG]: create socket at port "<<PROXY_SERVER_PORT<<endl;
   // command line parsing
   socket_fd = create_socket(PROXY_SERVER_PORT);
   if(socket_fd< 0){
      fprintf(stderr, "Can't create socket\n");
      return -1;
   }
   
   cout <<"[DEBUG]: listen to client"<<endl;
   // now start listening for the incoming client connections
   if (listen(socket_fd, 10) == -1) {
      perror("[SERVER]: Failed to listen");
      return -1;
   }
   
   
   
   while(1){
      cout <<"[DEBUG]: accepting connections"<<endl;
      // accept awaiting request
      client_id = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_id == -1){
         perror("[SERVER]: Failed to accept connection");
         continue;
      }
      
//need to receive the header information from the client
//and store it somewhere in a buffer
	  else{
		  memset(buff, 0, sizeof(buff));
		  if((rval = recv(socket_fd, buff, sizeof(buff), 0)) < 0)
				perror("[SERVER]: Failed to read the imcoming message");
		  else if(rval == 0)
				cout << "[SERVER]: Ending connections" << endl;
		  else
				cout << "[SERVER]: Message received." << endl;
	  }
//The request message is stored in the buff variable

      cout <<"New connection!"<<endl;
      //spawn a new thread for each new connection
      thread_params tp;
      tp.client_id = client_id;
      pthread_t thread;
      if(pthread_create(&thread, NULL, ptread_connection, (void *) &tp)){
         perror("[SERVER]: thread creation failed \n");
         return -1;
      }
      pthread_detach(thread);
   }
  
  
//Then check the rval whether be properly formatted, (in abosulte URI)
//invalid request should be answered with correspondent error codes.
//if valid, Parse the request
//make a connection to the remote server
//send the parsed request to the server (also need to support non-persis connections)
//receive the response and send it back to the client via appropriate socket
//The proxy as the client part

/*
 * Parsing the request, save it in a buffer for future caching
*/

//after the parsing, suppose the parsed request is stored in variables
//server name = server_name, port = server_port, 
//whole parsed request = parsed_request	
	char* parsed_request;
	char* server_name;
	char* server_port;	//default port should be 80 if not specified
	
	int len_sent = strlen(parsed_request);
	int bytes_sent, bytes_received;  //for send and recv
	char recvbuff[1024];		//recv buffer for the request
	
	struct addrinfo hints, *res;
	int client_sock_fd, t;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

//load up the remote server address result
	t = getaddrinfo(server_name, server_port, &hint, &res);
	if (t != 0)
	{
      fprintf(stderr, "[CLIENT]: Cannot get address info: %s\n", gai_strerror(t));
      return -1;
    }
    
//make a client socket on the proxy
//didn't use for loop here
	client_sock_fd = socket(res -> ai_family, res->ai_socktype, res->ai_protocol);
	
//connect to the remote server
	if(connect(client_sock_fd, res->ai_addr, res->ai_addrlen) < 0)
	{
		perror("[CLIENT]connection failed");
		close(client_sock_fd);
	}
	else
		cout << "[CLIENT]Connection established." << endl;
	
//send the parsed request to the remote server
	bytes_sent = send(client_sock_fd, parsed_request, len_sent, 0);
	
	if(bytes_sent < 0)
	{
		perror("[CLIENT]sending request failed");
		close(client_sock_fd);
	}
	else
		cout << "[CLIENT]sending request successful" << endl;

//close the sending part of the socket, this tells the server it's time to send the feedback
	shutdown(client_sock_fd, SHUT_WR);
	
//waiting for the response from the remote server
//not sure if it's working
	bytes_received = recv(client_sock_fd, recvbuff, sizeof(recvbuff), 0);
	recvbuff[bytes_received] = 0;
	
	cout << "Received data = " << recvbuff << endl;
	
//close the socket
	close(client_sock_fd);
		


/*	Once the proxy gets the response, it caches the response and corresponds it to the request stored in the proxy
 *  Send back the response to the client
 */











  return 0;
}
