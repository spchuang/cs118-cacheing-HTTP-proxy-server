#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <sys/socket.h>

using namespace std;

int recv_until(int socked_fd, const char* end_string, const int length, string& data)
{
   data = "";
   ssize_t bytes_rcv;
   char buf[BUFFER_SIZE];
   while (memmem(data.c_str(), data.length(), end_string, length) == NULL)
   {
      
      memset(buf, 0, BUFFER_SIZE);
      bytes_rcv = recv(socked_fd, buf, sizeof(buf)-1 , 0);
      if(bytes_rcv == EAGAIN){
         cout <<"BLOCK"<<endl;
         return -2;
      }
      if (bytes_rcv < 0)
      {
         return -1;
      }
      
      data.append(buf, bytes_rcv);
      //cout <<"[TEMP BUFFER]'"<<buf<<"'"<<endl;
   }
   return 1;

}

int recv_nth(int socked_fd, int length, std::string& data)
{
   data = "";
   char recv_buff[BUFFER_SIZE];
   ssize_t bytes_rcv;
   while(length>0)
   {
      memset(recv_buff, 0, BUFFER_SIZE);
      //cout <<"[]"<<recv_buff<<endl;
      bytes_rcv = recv(socked_fd, recv_buff, sizeof(recv_buff)-1, 0);
      //cout <<"BYTES RECV IS " <<bytes_rcv<<endl;
      if( bytes_rcv < 0){
         return -1;
      }
      length -= bytes_rcv;
      data.append(recv_buff, bytes_rcv);
   }
   return 1;
}