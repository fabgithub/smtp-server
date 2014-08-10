#include "util_socket.h"
#include "_head.h"
/**
* first is sockaddr_init(struct sockaddr_in sin 
*  -> sockaddr_init(struct sockaddr_in *sin
*  !!! causion 
* */
static int sockaddr_init(struct sockaddr_in * sin, int port, const char *ip)
{
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);
    long s_addr_long;
    if (NULL == ip)
    {
        s_addr_long = htonl(0);
    }
    else
    {
        s_addr_long = inet_addr(ip);
    }

    if (s_addr_long == -1)
    {
        _ERROR("fail to create s_addr_long");
        return -1;
    }

    sin->sin_addr.s_addr = s_addr_long;
    bzero(&sin->sin_zero, 8);
    return 0;
}

int tcp_connect(char *ip,int port,int timeout_sec)
{
    assert(ip!=NULL);
    assert(0>port && port < 65536);
    assert(timeout_sec>=0);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd < 0)
    {
        _ERROR("socket() fail\n");
        return -1;
    }

    char (opts[])[3] =
    {
        {
            SO_REUSEADDR, 1, 1},
        {
            SO_DONTROUTE, 1, 2},
        {
            SO_KEEPALIVE, 1, 5}
    };

    int opt_name;
    int opt_value;
    int ret;
    int opt_len = sizeof (opts) / sizeof (void *);
    for (int i = 0; i < opt_len; i++)
    {
        opt_name = opts[i][0];
        opt_value = opts[i][1];
        ret =
        setsockopt(clientfd, SOL_SOCKET, opt_name, &(opt_value),
                   sizeof (opt_value));
        if(0!=ret)
        {
            _ERROR("setsockopt fail\n");
            return -1; 
        }
    }


    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    ret =
    setsockopt(clientfd, SOL_SOCKET, SO_SNDTIMEO, &tv,
               sizeof (struct timeval));
    if (ret)
    {
        _ERROR("setsockopt SO_SNDTIMEOUT FAIL\n");
        return -1;
    }
ret =
setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
           sizeof (struct timeval));
if (ret)
{
    _ERROR("setsockopt SO_RCVTIMEO FAIL\n");
    return -1;
}


    struct sockaddr_in sin;
    if (sockaddr_init(&sin, port, ip))
    {
        _ERROR("sockaddr init fail \n");
        return -1;
    }

    if (connect
        (clientfd, (struct sockaddr *) &sin, sizeof (struct sockaddr_in)))
    {
        _ERROR("connect fail \n");
        return -1;
    }

    return clientfd;
}

int tcp_server_init(const char *ip,int port,int timeout_sec)
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        _ERROR("listenfd create fail \n");
        return -1;
    }

    char (opts[])[3] =
    {
        {
            SO_REUSEADDR, 1, 1},
        {
            SO_DONTROUTE, 1, 2},
        {
            SO_KEEPALIVE, 1, 5}
    };

    int opt_name;
    int opt_value;
    int ret;
    int opt_len = sizeof (opts) / sizeof (void *);
    for (int i = 0; i < opt_len; i++)
    {
        opt_name = opts[i][0];
        opt_value = opts[i][1];

        ret = setsockopt(listenfd, SOL_SOCKET, opt_name, &(opt_value),
                         sizeof (int));
        if (ret != 0)
        {
            _ERROR("setsockopt fail info \n");
            return -1;
        }
    }
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    ret =
    setsockopt(listenfd, SOL_SOCKET, SO_SNDTIMEO, &tv,
               sizeof (struct timeval));
    if (ret)
    {
        _ERROR("setsockopt SO_SNDTIMEOUT FAIL\n");
        return -1;
    }
    ret = setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, &tv,
                     sizeof (struct timeval));
    if (ret)
{
    _ERROR("setsockopt SO_RCVTIMEO FAIL\n");
    return -1;
}

    struct sockaddr_in sin;
    if (0 != sockaddr_init(&sin,port,ip))
    {
        _ERROR("sockaddr init fail \n");
        return -1;
    }
    if (bind(listenfd, (struct sockaddr *) &sin, sizeof (sockaddr_in)))
    {
        _ERROR("bind fail error \n");
        return -1;
    }
    return listenfd;
    }

int tcp_listen(int server_socket_fd,int wait_queue_size)
{
    return listen(server_socket_fd, wait_queue_size);
}

int tcp_accept(int server_socket_fd)
{
    {
        struct sockaddr_in recvaddr;
        socklen_t sin_size = sizeof (sockaddr_in);
        int recvfd = accept(server_socket_fd, 
                            (struct sockaddr *) &recvaddr, &sin_size);
        if (recvfd < 0)
        {
            _ERROR("fail to recvfd \n");
            return -1;
        }
        else
        {
            return recvfd; 
        }
    }
    _ERROR("accept return fail\n");
    return -1;
}

int tcp_recv(int fd,void *addr,int len,int flag)
{
    memset(addr,0,len);
    return recv(fd,addr,len,flag);    
}

int tcp_write(int fd, void *addr, int len,int flag)
{
    char *p = (char *) addr;
    int left = len;
    int wl;
    while (left)
    {
        wl = send(fd, p + (len - left), left, flag);
        if (wl < 0)
        {
            return -1;
        }
        left -= wl;
    }
    return (len - left);
}


