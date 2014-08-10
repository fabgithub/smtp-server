#include"_head.h"

/**
*  soket 
*  bind
*  listen
*		accept 
*		read/write
*		close
*	socket
*		connect 
*		send/sendTo
*		recv/recfFrom
*		close
* */

/* connect to one tcp server */
int tcp_connect(char *ip,int port,int timeout_sec=3);

/* init one server socket */
int tcp_server_init(const char *ip,int port,int timeout_sec);

/* listen() and accept() one */
int tcp_listen(int server_socket_fd,int wait_queue_size=10);

int tcp_accept(int server_socket_fd);

int tcp_recv(int fd,void *addr,int len,int flag=0);

int tcp_write(int fd, void *addr, int len,int flag=0);



