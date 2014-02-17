#ifndef __COMMON_H__
#define __COMMON_H__

#define BUFFER_SIZE 1024

#include <string>
#include <exception>

#include "../http-request.h"
#include "../http-response.h"

/*
   Class for proxy server exceptions
*/
class ProxyServerException: public std::exception
{
public:
  ProxyServerException (const std::string &desc, const std::string &reason) 
  { m_reason = desc + ": " + reason;  }
  virtual ~ProxyServerException () throw () { }
  virtual const char* what() const throw ()
  { return m_reason.c_str (); }
private:
  std::string m_reason;

} ;

class ProxyServerErrorResponseException: public std::exception
{
public:
  ProxyServerErrorResponseException (const std::string &response):m_response(response) {}
  virtual ~ProxyServerErrorResponseException () throw () { }
  virtual const char* what() const throw ()
  { return m_response.c_str (); }
private:
  std::string m_response;

} ;

/*
   Contains the entire response string and parsed response header
*/
class FullHttpResponse{
   public:
      HttpResponse header;
      std::string body;
      std::string entire;
};


int recv_until(int socket_fd, const char* end_string, const int length, std::string& data);
int recv_nth(int socket_fd, int length, std::string& data);

#endif