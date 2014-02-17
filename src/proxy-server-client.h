#ifndef __PROXY_SERVER_CLIENT_H__
#define __PROXY_SERVER_CLIENT_H__
#include <string>
#include "common.h"

/**
 * @brief Class for the proxy client that connects with the remote server
 */
 
class ProxyServerClient
{
public:
   ProxyServerClient(const std::string host, const short port);
   void sendHttpRequest(HttpRequest req);
   FullHttpResponse getHttpResponse();
   ~ProxyServerClient();
private:

   void connectServer();
   short m_port;
   std::string m_host;
   int m_remote_fd;
   

};

#endif 