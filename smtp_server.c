#include "_head.h"
#include "smtp_data.h"
#include "util_socket.h" 
#include "util_pthread_pool.h"

#define SMTP_PORT 2525
#define SMTP_SERVER_IP "127.0.0.1" 
#define SMTP_CONNECT_TIMEOUT 240


//#include "util_signal.h"
/*
 * do two things:
 *      1.fetch local emails which had not been send before 
 *      and try to send to other server.
 *      2.listen for port 25 waiting for
 *              -- other server(like mail.qq.com)
 *              -- user client agent
 **/


/*
static int smtp_send_mail(struct smtp_session session)
{
    
    return 0;
}
*/
/*
static void *smtp_send_mail_to_server_pthread(void *data)
{

        
    for(;;)
    {
         
        sleep(10);
    }

    pthread_exit(0);
    return (void*)0;
}
*/

#define smtp_session_check(s)\
{\
    if(s.error_cmd_num>5)\
        goto _QUIT;\
    struct timeval ntv;\
    gettimeofday(&ntv,NULL);\
    if((ntv.tv_sec-s.start_time.tv_sec)>240)\
        goto _QUIT;\
}

static int smtp_cmd_cmp(const char *a,char *b)
{
    int n=strlen(a);
    for(int i=0;i<n;i++)
    {
        b[i]=tolower(b[i]);
    }

    return strncmp(a,b,n);
}

static void smtp_cpy_mail_address(char *t,char *s)
{
    /* mail from:<sss@ss.com>*/  
    int i=0;
    while(s[i++]!=':')
        ;    

    /* ! here should check email format */
    int si=0;
    while(s[i]!='\0')
        t[si++]=s[i++];
    t[si]='\0';
}
static void smtp_cpy_rcpt_to(char *t,char *s)
{
    /* mail from:<sss@ss.com>*/  
    int i=0;
    while(s[i++]!=':')
        ;    

    /* ! here should check email format */
    int si=0;
    while(s[i]!='\0')
        t[si++]=s[i++];
    t[si]='\0';
}

static void smtp_store_mail(struct smtp_session session)
{
    printf("mail from<%s>,rcpt to<%s>,body<%s>",
          session.mail_from,
          session.rcpt_to,
          session.body);
}

void *smtp_server_job(void *data)
{
    int fd=*(int*)data;
    free(data);

    struct timeval tv;
    gettimeofday(&tv,NULL);
    struct smtp_session session;
    memset(&session,0,sizeof(struct smtp_session));
    session.sock_fd=fd;
    session.session_type=0;
    session.is_auth_user=0;
    session.next_cmd=SMTP_CMD_HELO;
    session.start_time=tv;
    session.error_cmd_num=0;


    char buf[256];
    int ret;

    int body_size=0;
    char *addr;
    int left;

    /* RSET NOOP */
    /*    _START: */
    snprintf(buf,sizeof(buf),"220 Anti-span mail system.kb\n");
    ret=tcp_write(session.sock_fd,buf,strlen(buf));

    _HELO:
    smtp_session_check(session);

    session.next_cmd=SMTP_CMD_HELO;
    ret=tcp_recv(session.sock_fd,buf,sizeof(buf),0); 
    if(ret==-1)
    {
        goto _QUIT; 
    }

    if(0!=smtp_cmd_cmp("helo ",buf))
    {
        snprintf(buf,sizeof(buf),"500 error..\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        session.error_cmd_num++;
        goto _HELO;
    }else{
        snprintf(buf,sizeof(buf),"225 mail ok\n");
        tcp_write(session.sock_fd,buf,strlen(buf));

    }

    _AUTH_OR_MAIL:
    smtp_session_check(session);

    ret=tcp_recv(session.sock_fd,buf,sizeof(buf),0);
    if(ret==-1)
    goto _QUIT;
    if(0==smtp_cmd_cmp("auth login",buf))
    { 
        snprintf(buf,sizeof(buf),"334 auth.login\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        goto _AUTH_LOGIN;
    }else if(0==smtp_cmd_cmp("mail from:",buf)){
        smtp_cpy_mail_address(session.mail_from,buf);
        /* here we asume it success */
        snprintf(buf,sizeof(buf),"250 Mail from ok\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        goto _MAIL_FROM;
    }else{
        snprintf(buf,sizeof(buf),"500 cmd not found..\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        session.error_cmd_num++;
        goto _AUTH_OR_MAIL; 
    }


    _AUTH_LOGIN:
        smtp_session_check(session);

        ret=tcp_recv(session.sock_fd,buf,sizeof(buf));
        if(ret==-1)
        goto _QUIT;
        strncpy(session.auth_user,buf,sizeof(session.auth_user));
        snprintf(buf,sizeof(buf),"344 auth.user\n");
        tcp_write(session.sock_fd,buf,strlen(buf));


        ret=tcp_recv(session.sock_fd,buf,sizeof(buf));
        if(ret==-1)
        goto _QUIT;
        
        strncpy(session.auth_passwd,buf,sizeof(session.auth_passwd));

        /* verify user*/
        if(strncmp("keenbo",session.auth_user,6))
        {
            snprintf(buf,sizeof(buf),"500 auth fail..\n");
            tcp_write(session.sock_fd,buf,strlen(buf));
            goto _AUTH_OR_MAIL; 
        }else{
            snprintf(buf,sizeof(buf),"250 auth success.\n");
            tcp_write(session.sock_fd,buf,strlen(buf));

            session.is_auth_user=1; 
        }

    _AFTER_AUTH:
        smtp_session_check(session);

    ret=tcp_recv(session.sock_fd,buf,sizeof(buf),0);
    if(ret==-1)
    goto _QUIT;
    if(0==smtp_cmd_cmp("mail from:",buf)){
        smtp_cpy_mail_address(session.mail_from,buf);
        /* here we asume it success */
        snprintf(buf,sizeof(buf),"250 Mail from ok\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        goto _MAIL_FROM;
    }else{
        snprintf(buf,sizeof(buf),"500 _mail_from cmd not found..\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        session.error_cmd_num++;
        goto _AFTER_AUTH;
    }

    _MAIL_FROM:
        smtp_session_check(session);
        ret=tcp_recv(session.sock_fd,buf,sizeof(buf),0);
        if(ret==-1)
        {
            goto _QUIT;
        }
        if(0==smtp_cmd_cmp("rcpt to:",buf)){
            smtp_cpy_rcpt_to(session.rcpt_to,buf);
            /* here we asume it success */
            snprintf(buf,sizeof(buf),"250 rcpt to ok\n");
            tcp_write(session.sock_fd,buf,strlen(buf));
        }else{
            snprintf(buf,sizeof(buf),"500 _rcpt_to cmd not found..\n");
            tcp_write(session.sock_fd,buf,strlen(buf));
            session.error_cmd_num++;
            goto _MAIL_FROM;
        }


    _RCPT_TO: 
        smtp_session_check(session);
        ret=tcp_recv(session.sock_fd,buf,sizeof(buf),0);
        if(ret==-1)
        goto _QUIT;

    if(0==smtp_cmd_cmp("data",buf)){
        snprintf(buf,sizeof(buf),"250 data to ok\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        goto _DATA;
    }else{
        snprintf(buf,sizeof(buf),"500 _data cmd not found..\n");
        tcp_write(session.sock_fd,buf,strlen(buf));
        session.error_cmd_num++;
        goto _RCPT_TO;
    }



    _DATA: 
        
        addr=&(session.body[0]);
        left=sizeof(session.body);
        body_size=0;
        for(;;)
        {
            ret=tcp_recv(session.sock_fd,addr+body_size,left,0);
            if(ret==-1)
            goto _QUIT;

            int is_end=0;
            for(int i=0;i<ret;i++)
            {
                if(addr[body_size+i]=='.')
                {
                    is_end=1; 
                }
            }

            body_size += ret;
            left -= ret;
            if(left<=0||is_end)
            break;
        }

/*    _SUCCESS: */
        smtp_store_mail(session);       

    _QUIT:

        close(session.sock_fd);

    return 0;
}

#define SMTP_PTHREAD_SIZE 10
#define SMTP_MAX_CONNECT 10
static void start_listen_job()
{
    /* start listen */
    /* use pthread pool to process one job*/

    int server_fd=tcp_server_init(SMTP_SERVER_IP,SMTP_PORT,SMTP_CONNECT_TIMEOUT);
    if(server_fd==-1)
    return;

    struct pthread_pool *pool=NULL;
    pool = pthread_pool_init(SMTP_PTHREAD_SIZE,SMTP_MAX_CONNECT);
    if(NULL==pool)
    {
        close(server_fd);
        _ERROR("pthread pool init fail\n");
        return ;
    }
    
    int ret;
    while(!tcp_listen(server_fd))
    {
        int client_fd=tcp_accept(server_fd);
        if(client_fd == -1)
        {
            _ERROR("accept one client_fd fail\n"); 
            continue;
        }

        /* create one job */
        int * a=(int*)malloc(sizeof(int));
        *a=client_fd;
        printf("add client fd:%d\n",client_fd);
        ret=pthread_pool_add_job(pool,smtp_server_job,a);
        if(ret)
        {
            _ERROR("add one smtp server job fail\n"); 
        }
        
    }

    pthread_pool_cancel_pthread(pool);
    pthread_pool_join_pthread(pool);
    pthread_pool_destroy(pool);

}



 int 
 main(int argc,char **argv)
 {

     /* register signals */
 //    register_signal();

     /* start send mail pthread */

     /* start listen job */
     start_listen_job();     

     return 0;
 }
