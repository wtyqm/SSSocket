#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "fcntl.h"
#include "errno.h"
#include "assert.h"

#include "simple_socket.h"

//simple stupid buffer
struct ssbf{
	char* buf;
	size_t buf_size;
	size_t dat_size;
};

struct ssbf* ssbf_new(size_t size);
void  ssbf_free(struct ssbf* bf);
void  ssbf_put(struct ssbf* bf, const void* data, size_t size);
void* ssbf_data(struct ssbf* bf);
size_t   ssbf_datasize(struct ssbf* bf);
void  ssbf_discard(struct ssbf* bf, size_t size);

struct ssbf* ssbf_new(size_t size){
    struct ssbf* bf = malloc(sizeof(struct ssbf));
    bf->buf = malloc(size);
    bf->buf_size = size;
    bf->dat_size = 0;
    
    return bf;
}

void  ssbf_free(struct ssbf* bf){
    assert(bf != NULL);
    
    free(bf->buf);
    free(bf);
}

void  ssbf_put(struct ssbf* bf, const void* data, size_t put_size){
    assert(bf != NULL);
    assert(bf->dat_size <= bf->buf_size);
    
    if(bf->buf_size - bf->dat_size < put_size){
        size_t new_size = (bf->dat_size + put_size)*2;
        bf->buf = (char*)realloc(bf->buf, new_size);
        assert(bf->buf != NULL);
        bf->buf_size = new_size;
    }
    
    memcpy(bf->buf + bf->dat_size, data, put_size);
    bf->dat_size += put_size;
}

void* ssbf_data(struct ssbf* bf) { return bf->buf; }
size_t   ssbf_datasize(struct ssbf* bf) {return bf->dat_size; }

void  ssbf_discard(struct ssbf* bf, size_t discard_size){
    assert(bf != NULL);
    
    if(discard_size == -1) discard_size = bf->dat_size;
    assert(bf->dat_size >= discard_size);
    
    memcpy(bf->buf, bf->buf + discard_size, bf->dat_size - discard_size);
    bf->dat_size -= discard_size;
}

#define MAX_RECV_BUF 1024*10

//simple stupid socket
struct ssso{
	int fd;
	struct ssbf* wbf;
    struct ssbf* rbf;
    char recv_buf[MAX_RECV_BUF];
    enum SSSO_STATUS status;
    int error_code;
};

void set_error_code(struct ssso* so, int error_code){
    so->error_code = error_code;
    so->status = SSSO_STATUS_ERROR;
}

struct ssso* ssso_new(const char* host_name, int port){
	struct sockaddr_in pin;
    struct hostent *nlp_host;
    
    //解析域名，如果是IP则不用解析，如果出错，显示错误信息
    while ((nlp_host=gethostbyname(host_name))==0){
        printf("Resolve Error!\n");
        return NULL;
    }
    
    bzero(&pin,sizeof(pin));
    pin.sin_family=AF_INET;                 //AF_INET表示使用IPv4
    pin.sin_addr.s_addr=htonl(INADDR_ANY);
    pin.sin_addr.s_addr=((struct in_addr *)(nlp_host->h_addr))->s_addr;
    pin.sin_port=htons(port);
    
    //建立socket
    int sd=socket(AF_INET,SOCK_STREAM,0);
    
    int flags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    int connected = 1;
    if(connect(sd,(struct sockaddr*)&pin,sizeof(pin)) == -1){
        if(errno != EINPROGRESS){
            assert(0);
            return NULL;
        }
        else{
            connected = 0;
        }
    }
    
    struct ssso* so = malloc(sizeof(struct ssso));
    so->fd = sd;
    so->wbf = ssbf_new(MAX_RECV_BUF);
    so->rbf = ssbf_new(MAX_RECV_BUF);
    so->status = (connected == 1) ? SSSO_STATUS_CONNECTED : SSSO_STATUS_CONNECTING;
    return so;
}

void  ssso_free(struct ssso* so){
    assert(so != NULL);
    close(so->fd);
    free(so->wbf);
    free(so->rbf);
    free(so);
}

int check_sock_error(int fd){
    int conn_error;
    socklen_t len = sizeof(conn_error);
    int ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &conn_error, &len);
    
    if(ret == -1){
        return ret;
    }
    return conn_error;
}

int  ssso_check_connect(struct ssso* so){
    assert(so->status == SSSO_STATUS_CONNECTING);
    
    fd_set wfds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    FD_ZERO(&wfds);
    FD_SET(so->fd, &wfds);
    
    int maxfd = so->fd + 1;
    int rc = select(maxfd, NULL, &wfds, NULL, &timeout);
    
    if(rc == -1){
        //error
        set_error_code(so, errno);
        return -1;
    }
    else if(rc == 0){
        //time out, no event
    }
    else{
        if(FD_ISSET(so->fd, &wfds)){
            //writeable
            if((so->error_code = check_sock_error(so->fd)) != 0) {
                printf("sock error %d\n",so->error_code);
                set_error_code(so, so->error_code);
                return -1;
            }
            else{
                so->status = SSSO_STATUS_CONNECTED;
                printf("sock connected \n");
                return 0;
            }
        }
    }
    
    return 0;
}

int  ssso_check(struct ssso* so){
    assert(so != NULL);
    
    if(so->status == SSSO_STATUS_CONNECTING){
        if(ssso_check_connect(so) != 0)
            return -1;
    }
    
    if(so->status != SSSO_STATUS_CONNECTED)
        return -1;
    
    fd_set rfds;
    fd_set wfds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    FD_ZERO(&rfds);
    FD_SET(so->fd, &rfds);
    
    FD_ZERO(&wfds);
    FD_SET(so->fd, &wfds);
    
    int maxfd = so->fd + 1;
    int rc = select(maxfd, &rfds, &wfds, NULL, &timeout);
    
    int readable = (ssbf_datasize(so->rbf) > 0) ? 1 : 0;
    if(rc == -1){
        //error
        set_error_code(so, errno);
        return -1;
    }
    else if(rc == 0){
        //time out, no event
    }
    else{
        if(FD_ISSET(so->fd, &rfds)){
            //readable
            size_t readed = recv(so->fd, so->recv_buf, MAX_RECV_BUF, 0);
            if(readed == 0){
                printf("sock recv return 0, may be closed by server\n");
                so->status = SSSO_STATUS_CLOSED;
                
            }
            else if(readed == -1){
                printf("sock read error %d\n",errno);
                set_error_code(so, errno);
                return -1;
            }
            else{
                ssbf_put(so->rbf, so->recv_buf, readed);
                readable = 1;
            }
        }
        if(FD_ISSET(so->fd, &wfds)){
            //writeable
            if(ssbf_datasize(so->wbf) > 0){
                size_t wrote = write(so->fd, ssbf_data(so->wbf), ssbf_datasize(so->wbf));
                if(wrote == -1){
                    printf("sock write error %d\n",errno);
                    set_error_code(so, errno);
                    return -1;
                }
                else
                    ssbf_discard(so->wbf, wrote);
            }
        }
    }
    
    return readable;
}

void* ssso_peek(struct ssso* so, size_t size){
    assert(so != NULL);
    
    if(ssbf_datasize(so->rbf) < size)
        return NULL;
    
    return ssbf_data(so->rbf);
}

void  ssso_discard(struct ssso* so, size_t size){
    assert(so != NULL);
    
    ssbf_discard(so->rbf, size);
}

void  ssso_write(struct ssso* so, const void* data, size_t size){
    assert(so != NULL);
    
    ssbf_put(so->wbf, data, size);
}

enum SSSO_STATUS ssso_status(struct ssso* so){
    assert(so != NULL);
    
    return so->status;
}