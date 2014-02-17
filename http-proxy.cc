/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include "src/proxy-server.h"


#define PROXY_SERVER_PORT 14886 //"14886"
#define MAX_THREAD_NUM    20

#define PERSISTENT_TIME_OUT 5 //5 seconds

int main (int argc, char *argv[])
{
  ProxyServer server(PROXY_SERVER_PORT, PERSISTENT_TIME_OUT, MAX_THREAD_NUM);
  server.start();
  std::cout<<"DONE"<<std::endl;
  return 0;
}
