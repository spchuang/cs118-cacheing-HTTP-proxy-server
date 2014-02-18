#ifndef __PROXY_SERVER_H__
#define __PROXY_SERVER_H__
#include <string>
#include "common.h"
//#include <sqlite3.h>
#include <string>
#include "cache-db.h"

/*
   struct that holds in data passed to each thread
*/
typedef struct str_thdata{
  int client_id;
}thread_params;

/**
 * @brief Class for the proxy server 
 */

class ProxyServer
{
public:
   ProxyServer(const int port, const int timeout, const int max_connections);
   ~ProxyServer();
   void start();
private:
   static void *onConnect(void *params);
   static HttpRequest getHttpRequest(int client_id);
   static void sendHttpResponse(int client_id, std::string response);
   
   void connectSqlite();
   void disconnectSqlite();
   void insertCache(std::string s);
   
   void setup();
   void loop();
   int createSocket(const int port);
   int m_port;
   int m_timeout;
   int m_listen_fd;
   int m_max_connections;
   int m_connections;
   //sqlite3 *db;
   Database *db;
   
};



#endif 