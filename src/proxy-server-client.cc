#include "proxy-server-client.h" 
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
#include <errno.h>

using namespace std;

ProxyServerClient::ProxyServerClient(const std::string host, const short port)
{
   m_port = port;
   m_host = host;
   
   try{
      connectServer();
   }catch(ProxyServerException& e){
      cout << "[ProxyServerClient ERROR]: "<< e.what() << endl;
      //throw ProxyServerException("ProxyServerClient", "failed to establish connection with the remote server");
   }
   

}


ProxyServerClient::~ProxyServerClient()
{
   close(m_remote_fd);
}

void ProxyServerClient::connectServer()
{
   //converr short to string
   stringstream ss;
   ss << m_port;

   //socket stuff
   struct addrinfo hints, *res;
   
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;
   int t;
   if( ( t =getaddrinfo(m_host.c_str(), ss.str().c_str(), &hints, &res)) <0){
      throw ProxyServerException("Get addr info", strerror(errno));
   }

   //create the socket
   m_remote_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
   if(m_remote_fd <0){
      //perror("[SERVER CLIENT]: cannot open socket");
      throw ProxyServerException("Open socket", strerror(errno));

   }

   //connect through the socket
   if(connect(m_remote_fd, res->ai_addr, res->ai_addrlen) < 0){
      close(m_remote_fd);
      throw ProxyServerException("Connect", strerror(errno));
   }
}

void ProxyServerClient::sendHttpRequest(HttpRequest req)
{
   size_t req_len = req.GetTotalLength()+1;
   char* parsed_req = new char[req_len];
   req.FormatRequest(parsed_req);
   
   //send the msg
   if(send(m_remote_fd, parsed_req, req_len, 0) <0){
      close(m_remote_fd);
      delete parsed_req;
      throw ProxyServerException("Send request to remote server", strerror(errno));
   }
   delete parsed_req;

}

FullHttpResponse ProxyServerClient::getHttpResponse()
{
   FullHttpResponse resp;
   string response;
   
   //READ the http header
   int r = recv_until(m_remote_fd, "\r\n\r\n", 4, response);
   if(r==EAGAIN){
      throw ProxyServerErrorResponseException("HTTP/1.1 408 Request Timeout");
   }else if(r<0){
      throw ProxyServerException("Reading request data", strerror(errno));
   }
   
   //cout <<"READ SIZE: " <<response.length()<<endl;

   try {
      resp.header.ParseResponse(response.c_str(), response.length());
   }catch (ParseException ex) {
      throw ProxyServerException("Failed to parse http response", "");
   }
   
   //read the content length
   string cl = resp.header.FindHeader("Content-Length");
   if(cl == ""){
      throw ProxyServerException("Missing Content-Length header", "");
   }
   int body_size;
   istringstream ( cl ) >> body_size;
   //cout <<"GET BODY"<<endl;
   //read the response body
   
   r = recv_nth(m_remote_fd, body_size, resp.body);
   if(r==EAGAIN){
      throw ProxyServerErrorResponseException("HTTP/1.1 408 Request Timeout");
   }else if(r<0){
      throw ProxyServerException("Reading request data", strerror(errno));
   }
      
   //cout <<"[DEBUG]READ BODY RESPONSE: " <<endl<<"'"<<resp.body<<"'"<<endl;
   resp.entire = response+resp.body;
   
   return resp;
}

