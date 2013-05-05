#SSSocket
========

Simple Stupid Socket Interface

This is yet another socket interface, which may be useful for client network programme.

========

##pro :

1.easily use both in single or multithreading by non blocking io.

2.provide read/write buffer.

3.interface suit for integrate message io.

##con :

1.buffer mechanism is just an array, read and discard message from buffer require memory copy.

========

##interface :

```C
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

```