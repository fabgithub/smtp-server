#include "util_signal.h"


static void exit_signal_handler(int sig, siginfo_t * info, void *context)
{
	/* do something ,like print_trace() */
	_ERROR("signal no %d\n",signo);

	exit(2);
}

void register_signal(int signal_reg_type=0)
{

	/* ignore part */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	signal(SIGCLD, SIG_IGN);
	signal(SIGURG,SIG_IGN); /* ignore tcp urg pointer */
#ifdef SIGTSTP
	signal(SIGTSTP, SIG_IGN);/* background tty read */
#endif
#ifdef SIGTTIN
	signal(SIGTTIN, SIG_IGN);/* background tty read */
#endif
#ifdef SIGTTOU 
	signal(SIGTTOU, SIG_IGN); /* background tty write */
#endif

	struct sigaction act;
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = NULL;
	act.sa_sigaction = exit_signal_handler;

	sigaction(SIGILL, &act, NULL);
	/* sigaction(SIGKILL, &act, NULL); */
	/* sigaction(SIGINT, &act,NULL); */
	sigaction(SIGQUIT, &act, NULL);
#ifdef SIGEMT
	sigaction(SIGEMT, &act, NULL);
#endif
	sigaction(SIGFPE, &act, NULL);
	sigaction(SIGBUS, &act, NULL);   /*  bus */
	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGSYS, &act, NULL);
	sigaction(SIGPWR, &act, NULL);
	sigaction (SIGTERM, &act,NULL);
#ifdef SIGSTKFLT
	sigaction(SIGSTKFLT, &act, NULL);
#endif

}

