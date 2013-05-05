//
//  simple_socket.h
//  SSSocket
//
//  Created by 张 鹏 on 13-4-1.
//  Copyright (c) 2013年 张 鹏. All rights reserved.
//

#ifndef SSSocket_simple_socket_h
#define SSSocket_simple_socket_h

struct ssso;

enum SSSO_STATUS{
    SSSO_STATUS_CONNECTING,
    SSSO_STATUS_CONNECTED,
    SSSO_STATUS_CLOSED,
    SSSO_STATUS_ERROR,
};

//create a socket , ready to connect to host define by host_name and port
struct ssso* ssso_new(const char* host_name, int port);

//close socket
void  ssso_free(struct ssso* so);

//update socket, write data from buffer to socket, and if there is data readable, it return 1
int   ssso_check(struct ssso* so);

//peek whether has data of size in read buffer, return data or NULL
void* ssso_peek(struct ssso* so, size_t size);

//discard data of size in read buffer, mainly use after geting integrate message data using ssso_peek
void  ssso_discard(struct ssso* so, size_t size);

//write data to buffer, after do ssso_check, the data may be send through socket
void  ssso_write(struct ssso* so, const void* data, size_t size);

enum SSSO_STATUS ssso_status(struct ssso* so);
#endif
