/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>


#include <stdio.h>
#include <cstring>

//C socket stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#include "http-request.h"


#define PROXY_SERVER_PORT "14886"

using namespace std;
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
      socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
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
      fprintf(stderr, "[SERVER]:  could not bind\n");
      return -2;
   }
   
   // no need anymore
   freeaddrinfo(result);
   
   return socket_fd;
}



int main (int argc, char *argv[])
{
   int socket_fd, client_id;
   struct sockaddr_storage client_addr;
   socklen_t client_len = sizeof client_addr;;
   
   cout <<"[DEBUG]: create socket at port "<<PROXY_SERVER_PORT<<endl;
   // command line parsing
   socket_fd = create_socket(PROXY_SERVER_PORT);
   
   cout <<"[DEBUG]: listen to client"<<endl;
   // now start listening for the incoming client connections
   if (listen(socket_fd, 10) == -1) {
      perror("[SERVER]: Failed to listen");
      return -1;
   }
   
   
  cout <<"[DEBUG]: accepting connections"<<endl;
   while(1){
      // accept awaiting request
      client_id = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_id == -1){
         perror("[SERVER]: Failed to accept connection");
         continue;
      }
      
      //strcpy(sendBuff, "Message from server");
      //write(connfd, sendBuff, strlen(sendBuff));
      
      close(client_id);    
      sleep(1);
   }
  
  
  return 0;
}
