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
#include <vector>

#include "../http-request.h"
#include "../http-response.h"
#include "proxy-server.h"
#include "proxy-server-client.h"

using namespace std;

ProxyServer::ProxyServer(const int port, const int timeout, const int max_connections)
{
   m_port      = port;
   m_timeout   = timeout;
   m_max_connections = max_connections;
   m_connections = 0;
   m_db_name = "cache_proxy.db";
   m_db = NULL;
}

ProxyServer::~ProxyServer()
{
   close(m_listen_fd);
   disconnectSqlite();
   
}

void ProxyServer::start()
{  
   try{
      connectSqlite();    
      setup();
      loop();
      /*
         db->insertCache("TEST","TEST","TEST");
         if(db->containsHostPathCache("TEST")){
         cout <<"CORRECT 1 "<<endl;
         }
         
         vector<string> t = db->getCache("TEST");
         db->updateCache("TEST", "TEST2", "TEST3");
         vector<string> t2 = db->getCache("TEST");
         
         cout << t.at(0) << " " <<t.at(1)<<endl;
         cout << t2.at(0) << " " <<t2.at(1)<<endl;
         if(!db->containsHostPathCache("TEST2")){
         cout <<"CORRECT 2 "<<endl;
         
         }
      */
      
   }catch(ProxyServerException& e){
      cout << "[ProxyServer ERROR]: "<< e.what() << endl;
   }catch(SqliteException& e){
      cout << "[Cache SQlite ERROR]: "<< e.what() << endl;
   }
   
}

/*
   Private Functions
*/
int ProxyServer::createSocket(const int port)
{
   //convert int port to string
   string port_string;
   ostringstream convert;  
   convert << port;
   port_string = convert.str();
   
   // declare address structs
   struct addrinfo hints;
   struct addrinfo *result, *rp;
   int socket_fd, s;
   
   // Set up an addrinfo that specfies the criteria for selecting socket address structures
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;      //allow IPv4 or IPv6
   hints.ai_socktype = SOCK_STREAM;  //socket type: full-duplex..
   hints.ai_flags = AI_PASSIVE;      //wildcard IP addresses
   
   
   s = getaddrinfo(NULL, port_string.c_str(), &hints, &result);
   if (s != 0){
      //fprintf(stderr, "[ProxyServer]: Cannot get address info: %s\n", gai_strerror(s));
      return -1;
   }
   
   // Loop through the list of address structure, and try connect until we bind successfully
   for (rp = result; rp != NULL; rp = rp->ai_next){
      // Create the socket
      socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (socket_fd == -1){
         continue;
      }
   
      // Bind the socket to the port (just the first one we can)
      if( bind(socket_fd, rp->ai_addr, rp->ai_addrlen) ==0)
         break;
         
      close(socket_fd);
   }
   
   // No binds happened
   if (rp == NULL){
      fprintf(stderr, "[ProxyServer]: could not bind\n");
      return -2;
   }
   
   // no need anymore
   freeaddrinfo(result);
   
   return socket_fd;
}



void ProxyServer::setup() 
{
   m_listen_fd = createSocket(m_port);
   
   if(m_listen_fd< 0){
      throw ProxyServerException("Create socket", strerror(errno));
   }
   
   if (listen(m_listen_fd, 100) == -1) {
      throw ProxyServerException("Socket listen", strerror(errno));
   }
}

/*
Cache part using sqlite server
*/
void ProxyServer::connectSqlite()
{
   string create_sql = "CREATE TABLE IF NOT EXISTS CACHE("\
         		"HOST_PATH VARCHAR(1024),"\
         		"EXPIRE VARCHAR(255),"\
         		"FORMATED_RESP VARCHAR(1024)"\
         		");";
   m_db = new Database(m_db_name.c_str());
   //m_db->query("INSERT INTO CACHE VALUES('TEST', 'TEST', 'TEST');");

   m_db->query(create_sql.c_str());
}

void ProxyServer::disconnectSqlite()
{
   cout <<"[DEBUG]: disconnecting to sqlite db" <<endl;
   m_db->close();
}


HttpRequest ProxyServer::getHttpRequest(int client_id)
{
   //read the data
   string req_data;
   if(recv_until(client_id, "\r\n\r\n", 4, req_data) <0){
      throw ProxyServerException("Reading request data", strerror(errno));
   }
   //cout << endl<<req_data << endl;
   //parse the Http request
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
      throw ProxyServerErrorResponseException(client_res);
   }
   return client_req;
}

void ProxyServer::sendHttpResponse(int client_id, string response){
   if (send(client_id, response.c_str(), response.length(),0)<0){
      throw ProxyServerException("Send HTTP response", strerror(errno));
   }
}



void* ProxyServer::onConnect(void *params)
{
   bool persistent = true;
   thread_params *tp = (thread_params *)params;
   //int remote_fd;
   cout << "[THREAD DEBUG] client id: " <<tp->client_id<<endl;   
   
   
   /*
      DB FUNCTIONS HERE
      Database *db = tp->db;
      db->insertCache("TEST","TEST","TEST");
      db->containsHostPathCache("TEST")
      db->updateCache("TEST", "TEST2", "TEST3")
      db->vector<string> t2 = db->getCache("TEST")
   
   */
   
      Database* db = tp->db;



   ProxyServerClient *client;
   do{
      cout <<"READING REQUEST"<<endl;
   //   bool conditional_GET = false;
      try{
         //Read the HTTP request from the client
         HttpRequest client_req = ProxyServer::getHttpRequest(tp->client_id);
          
         //Look for persistent connection header
         string connection = client_req.FindHeader("Connection");
         if(connection == "close"){
            persistent = false;
         }else{
            persistent = true;
         }
         
         //TODO: check for cache here
          //format the request into character array
        size_t req_len = client_req.GetTotalLength()+1;
        char* formated_req = new char[req_len];
        client_req.FormatRequest(formated_req);
   
        //obtain server_name and path name
        string server_name = client_req.GetHost();
        string path_name = client_req.GetPath();

        //obtain server port
        stringstream ss;
        ss << client_req.GetPort();
        string port_num = ss.str();



         //get the host + path as the checking standard in database for the cache file
         //size_t server_len = strlen(server_name);
         //size_t path_len = strlen(path_name);
         //char* host_path = (char*)malloc(server_len + path_len);
         //memcpy(host_path, server_name, server_len);
         //memcpy(host_path + server_len, path_name, path_len);
        string host_path = server_name + path_name;
         
         if(db->containsHostPathCache(host_path))//means the cache exists
         {
            vector<string> entry =  db->getCache(host_path);
            string expir = entry.at(0);
            string formated_resp = entry.at(1);

            char* cache_gmt = (char*)malloc(expir.length() * sizeof(char));
            strcpy(cache_gmt, expir.c_str());

           // check for last modified
     //       string lastmodstr = .FindHeader("Last-Modified");
       //     char* lastmod = (char*)malloc(lastmodstr.length() * sizeof(char));
         //   strcpy(lastmod, lastmodstr.c_str());

            //get current time and standarize both expiration time and current time
            struct tm cache_time;
            strptime(cache_gmt, "%a, %d %b %Y %H:%M:%S GMT", &cache_time);
            time_t currenttm = time(0);
            struct tm* currtm = gmtime(&currenttm);
            time_t currentgmttm = timegm(currtm);
            time_t cache_gmttm = timegm(&cache_time);

            //if expir time < current time, send a req to the remote server
            if(cache_gmttm < currentgmttm)   //means the request is expired
            {
               //Format a conditional GET request to the remote server
//               client_req.AddHeader("If-Modified-Since", lastmod);
 //              req_len = client_req.GetTotalLength();
  //             formatedreq = (char*)malloc(formated_size);
   //            client_req.FormatRequest(formated_req);
    //           formated_req = (string)formatedreq;   //convert to string
     //          conditional_GET = true;
            }
            else
            {  
               //send the cached result back to the client and close the socket
               size_t bytes_sent = send(tp->client_id, formated_resp.c_str(), formated_resp.length(), 0);
               if(bytes_sent < 0)
                  perror("Cannot send back cache result");
               return NULL;
            }


         }


         //create connection with remote server
         cout <<"Create proxy client"<<endl;
         client = new ProxyServerClient(client_req.GetHost(), client_req.GetPort());
         
         //get the response from remote server
         cout <<"send proxy request"<<endl;
         client->sendHttpRequest(client_req);
         
         cout <<"GET proxy response"<<endl;
         FullHttpResponse resp = client->getHttpResponse();
         
         //cout <<"[SIZE]: " <<resp.entire.length()<<endl;
         //cout << resp.entire <<endl;
    /*     if(conditional_GET)
         {
            string resp_status = resp.header.GetStatusCode();
            if(resp_status.compare("304") == 0)
            {
               size_t bytes_sent = send()
            }
         }
     */

         //cache the response
         string resp_content = resp.entire;
         string resp_respiration = resp.header.FindHeader("Expires");

         db -> updateCache(host_path, resp_respiration, resp_content);



         //send response back to client
         cout <<"Send response back to client"<<endl;
         ProxyServer::sendHttpResponse(tp->client_id, resp.entire);
         
         delete client;
         
         cout <<client_req.GetHost() <<endl;
         
      }catch(ProxyServerException& e){
         delete client;
         cout << "[ProxyServer Thread ERROR]: "<< e.what() << endl;
         break;
      }catch(ProxyServerErrorResponseException& e){
         ProxyServer::sendHttpResponse(tp->client_id, e.what());
      }
   
   }while(persistent);
   
   
   close(tp->client_id);
   cout <<"[THREAD DEBUG] Thread exit for "<<tp->client_id<<endl;
   return NULL;
}

void ProxyServer::loop()
{
   int client_id;
   struct sockaddr_storage client_addr;
   socklen_t client_len;
   
   
   
   while(1){
      client_len= sizeof(client_addr);
      cout <<"[ProxyServer DEBUG]: accepting connections"<<endl;
      // accept awaiting request
      client_id = accept(m_listen_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_id == -1){
         continue;
      }
      
      //Set client timeout
      struct timeval tv;
      tv.tv_sec  = m_timeout;  
      tv.tv_usec = 0;
      if(setsockopt( client_id, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
         throw ProxyServerException("Socket listen", strerror(errno));
      }
      
      //spawn a new thread for each new connection
      thread_params *tp = (thread_params *) malloc(sizeof(thread_params));;
      tp->client_id = client_id;
      tp->db        = m_db;
      pthread_t thread;

      if(pthread_create(&thread, NULL, &ProxyServer::onConnect, (void *) tp)){
         close(client_id);
         throw ProxyServerException("Thread creation", strerror(errno));
      }
   }
}
