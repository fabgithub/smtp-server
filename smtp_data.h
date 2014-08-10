
#define SMTP_MAIL_FROM_SIZE 256
#define SMTP_RCPT_TO_SIZE 256
#define SMTP_SUBJECT_SIZE 256
#define SMTP_BODY_SIZE 1024*4
#define SMTP_MAIL_SEND_DATETIME_SIZE 256 

#define SMTP_SESSION_TYPE_CONNECT 0
#define SMTP_SESSION_TYPE_ACCEPT 1

#define SMTP_CMD_HELO 0
#define SMTP_CMD_AUTH_LOGIN  1
#define SMTP_CMD_MAIL_FROM 2
#define SMTP_CMD_RCPT_TO 3
#define SMTP_CMD_DATA 4
#define SMTP_CMD_QUIT 5
#define SMTP_CMD_RSET 6
#define SMTP_CMD_NOOP 7 

typedef struct smtp_session{
	int sock_fd;
	int session_type;/* SMTP_SESSION_TYPE_CONNECT ...*/	
		
	int is_auth_user;
	char auth_user[256];
	char auth_passwd[256];

	int next_cmd;/* SMTP_CMD_...  */
	int error_cmd_num;/* error cmd count */
	struct timeval start_time;/* start time */
	
	/* mail content */
	char mail_from[SMTP_MAIL_FROM_SIZE];
	char rcpt_to[SMTP_RCPT_TO_SIZE];
	char subject[SMTP_SUBJECT_SIZE];
	char body[SMTP_BODY_SIZE];
	char send_datetime[SMTP_MAIL_SEND_DATETIME_SIZE];

}smtp_session_t;

