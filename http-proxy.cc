/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <string>
#include <sstream>

//C socket stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "http-request.h"
#include "http-response.h"


#define PROXY_SERVER_PORT "14885" //"14886"
#define MAX_THREAD_NUM    20
#define BUFFER_SIZE 1024


using namespace std;

typedef struct str_thdata{
  int client_id;
}thread_params;

class FullHttpResponse{
   public:
      HttpResponse header;
      string body;
      string entire;
};

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

int create_proxy_remote_connection(const char* server_name, const char* port_num){
   //socket stuff
   struct addrinfo hints, *res;
   int client_sock_fd, t;
   
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   
   t = getaddrinfo(server_name, port_num, &hints, &res);
   if(t != 0){
      //fprintf(stderr, "[CLIENT]: Cannot get addr info: %s\n", gai_strerror(t));
      return -1;
   }
   
   //create the socket
   client_sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
   if(client_sock_fd <0){
      //perror("[SERVER CLIENT]: cannot open socket");
      return -1;
   }
   
   //connect through the socket
   if(connect(client_sock_fd, res->ai_addr, res->ai_addrlen) < 0){
      //perror("[SERVER CLIENT]: Connection failed");
      close(client_sock_fd);
      return -1;
   }
   return client_sock_fd;
   
}

int read_until(int client_id, const char* end_string, const int length, string& data)
{
   data = "";
   size_t bytes_rcv;
   char buf[BUFFER_SIZE];
   while (memmem(data.c_str(), data.length(), end_string, length) == NULL)
   {
      
      memset(buf, 0, BUFFER_SIZE);
      bytes_rcv = recv(client_id, buf, sizeof(buf)-1 , 0);
      if (bytes_rcv < 0)
      {
         perror("[SERVER]: Can't read incoming request data");
         return -1;
      }
      
      data.append(buf, bytes_rcv);
      //cout <<"[TEMP BUFFER]'"<<buf<<"'"<<endl;
      
   }
   return 1;
}
int get_remote_response(int client_sock_fd, FullHttpResponse& resp)
{
   string response;
   int bytes_rcv;
   
   //receive the response
   //READ the http header
   if(read_until(client_sock_fd, "\r\n\r\n", 4, response) <0){
      return -1;
   }
   //cout <<"READ SIZE: " <<response.length()<<endl;
   //cout <<"[DEBUG]READ RESPONSE: " <<endl<<"'"<<response<<"'"<<endl;
   
   try {
      resp.header.ParseResponse(response.c_str(), response.length());
   }catch (ParseException ex) {
      return -1;
   }
   //read the content length
   string cl = resp.header.FindHeader("Content-Length");
   if(cl == ""){
      cout << "[CLIENT]: Missing Content-Length header";
      return -1;
   }
   int body_size;
   istringstream ( cl ) >> body_size;
      
   //read the response body
   resp.body="";
   char recv_buff[BUFFER_SIZE];
   while(body_size>0)
   {
      memset(recv_buff, 0, BUFFER_SIZE);
      bytes_rcv = recv(client_sock_fd, recv_buff, sizeof(recv_buff)-1, 0);
      //cout <<"BYTES RECV IS " <<bytes_rcv<<endl;
      if( bytes_rcv < 0){
         cout << "[CLIENT]: Failed to get the body response";
         return -1;
      }
      body_size -= bytes_rcv;
      resp.body.append(recv_buff);
   }
   //cout <<"[DEBUG]READ BODY RESPONSE: " <<endl<<"'"<<resp.body<<"'"<<endl;
   resp.entire = response+resp.body;
   
   //cout << body_size << " : " <<resp.body<<endl;
   //cout << "[SERVER CLIENT] Received: " << endl<< response << endl;

   return 1;
}



int send_response(int client_id, const FullHttpResponse response)
{
   size_t resp_len = response.header.GetTotalLength();
   char* format_resp = new char[resp_len];
   response.header.FormatResponse(format_resp);
   resp_len = strlen(format_resp);
   
   //send the header
   /*if(send(client_id, format_resp, resp_len, 0) < 0){
     perror("[SERVER]: Failed to send the response");
     delete format_resp;
     return -1;
   }
   delete format_resp;
   //send the body
   if(send(client_id, response.body.c_str(), response.body.length(), 0) < 0){
     perror("[SERVER]: Failed to send the response");
     
     return -1;
   }*/
   cout <<"[DEBUG]SIZE: " <<response.entire.length()<<endl;
   cout<<"[DEBUG]WRITE: '"<<response.entire.c_str()<<"'"<<endl;
   
   if(send(client_id, response.entire.c_str(), response.entire.length(), 0) < 0){
     perror("[SERVER]: Failed to send the response");
     return -1;
   }
   
   return 1;
}


void* ptread_connection(void *params){
   bool persistent = true;
   thread_params *tp;
   tp = (thread_params *)params;
   int remote_fd;
   cout << "[THREAD DEBUG] client id: " <<tp->client_id<<endl;
   
   do{
      
      /* 
         Read the HTTP request from the client
      */
      string req_data;
      // Loop until we get "\r\n\r\n"
      if(read_until(tp->client_id, "\r\n\r\n", 4, req_data) <0){
         return NULL;
      }
      cout << "REQUEST IS " << endl << req_data<<endl;
      
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
         if (send(tp->client_id, client_res.c_str(), client_res.length(),0)<0){
            perror("[SERVER]: Can't write error response");
         }
      }
      /*
         Find persistent connection header
      */ 
      string connection = client_req.FindHeader("Connection");
      if(connection == "close"){
         persistent = false;
      }else{
         persistent = true;
      }
      /*
         get the remote server name and the port
      */
      
      //obtain server_name
      const char* server_name = client_req.GetHost().c_str();
      
      //obtain server port
      stringstream ss;
      ss << client_req.GetPort();
      const char* port_num = ss.str().c_str();
      
      /*
         Create connection with the remote server
      */
      cout << "[SERVER CLIENT]: making connection with the remote server." << endl;
      remote_fd= create_proxy_remote_connection(server_name, port_num);
      if(remote_fd<0){
         perror("[SERVER CLIENT]: Can't create remote connection");
         return NULL;
      }
      
      /*
         Send request to remote server
      */
   
      //parse the request into character array
      size_t req_len = client_req.GetTotalLength()+1;
      char* parsed_req = new char[req_len];
      client_req.FormatRequest(parsed_req);
      
      //send the msg
      cout << "[SERVER CLIENT]: Sending request..." << endl;
      if(send(remote_fd, parsed_req, req_len, 0) <0){
        perror("[SERVER CLIENT]: Sending request to remote server failed");
        close(remote_fd);
        return NULL;
      }
      delete parsed_req;
      
      /*
         Get the reponse from remote server or from local cache
         TODO: cache
      */
      FullHttpResponse response;
      cout << "[SERVER CLIENT]: get remote response..." << endl;
      if(get_remote_response(remote_fd, response) <0){
         perror("[SERVER CLIENT]: can't get response from remote server");
         delete parsed_req;
         close(remote_fd);
         return NULL;
      }
      close(remote_fd);
      //cout <<"[DEBUG]SIZE: " <<response.entire.length()<<endl;
      //cout <<"[DEBUG]READ BODY RESPONSE: " <<endl<<"'"<<response.entire<<"'"<<endl;
      
      /*
         write the response to the client
      */
      cout << "[SERVER CLIENT]: send remote response..." << endl;
      if(send_response(tp->client_id, response) <0){
         perror("[SERVER CLIENT]: can't get send response to client");
         delete parsed_req;
         return NULL;
      }
        
      
   
      
   /*   if (write(tp->client_id, response.c_str(), response.length()) == -1)
      {
         perror("[SERVER]: Can't write response");
         free(remote_req);
         return NULL;
      }
   */   
   
   }while(persistent);
   
   
   close(tp->client_id);
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
