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

struct ssso* ssso_new(const char* host_name, int port);
void  ssso_free(struct ssso* so);
int   ssso_check(struct ssso* so);
void* ssso_peek(struct ssso* so, size_t size);
void  ssso_discard(struct ssso* so, size_t size);
void  ssso_write(struct ssso* so, const void* data, size_t size);
enum SSSO_STATUS ssso_status(struct ssso* so);
int   ssso_error(struct ssso* so);

#endif
