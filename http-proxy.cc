/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include <stdio.h>
#include <cstring>
#include <pthread.h>
#include <string>

//C socket stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "http-request.h"



#define PROXY_SERVER_PORT "14886"
#define MAX_THREAD_NUM    20
#define BUFFER_SIZE 1024



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
   
   
   
   /*
      Read the HTTP request from the client
   */
   string req_data;
   
   // Loop until we get "\r\n\r\n"
   while (memmem(req_data.c_str(), req_data.length(), "\r\n\r\n", 4) == NULL)
   {
      char buf[BUFFER_SIZE];
      if (read(tp->client_id, buf, BUFFER_SIZE) == 0)
      {
         perror("[SERVER]: Can't read incoming request data");
         return NULL;
      }
      req_data.append(buf);
   }
   
   /*
      parse the HTTP request
   */
   
   
   HttpRequest client_req;
   try {
      client_req.ParseRequest(req_data.c_str(), req_data.length());
   
   }catch (ParseException ex) {
      fprintf(stderr, "Exception raised: %s\n", ex.what());
      
      string client_res = "HTTP/1.0 ";
      
      // only 2 bad request response
      string cmp = "Request is not GET";
      if (strcmp(ex.what(), cmp.c_str()) != 0){
         client_res += "400 Bad Request\r\n\r\n";
      }else{
         client_res += "501 Not Implemented\r\n\r\n";
      }
      
      // Send the bad stuff!
      if (write(tp->client_id, client_res.c_str(), client_res.length()) == -1){
         perror("[SERVER]: Can't write response");
      }
      
   }
   
   
   /*
      write the response to the client
   */
   
   cout << "REQUEST" << endl << req_data<<endl;
   
   
   cout <<"[THREAD DEBUG] Thread exit"<<endl;
   return NULL;
}


int main (int argc, char *argv[])
{
   int socket_fd, client_id;
   struct sockaddr_storage client_addr;
   socklen_t client_len;
   
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
      client_len= sizeof client_addr;;
      cout <<"[DEBUG]: accepting connections"<<endl;
      // accept awaiting request
      client_id = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_id == -1){
         perror("[SERVER]: Failed to accept connection");
         continue;
      }
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
  
  
  return 0;
}
