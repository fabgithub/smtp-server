#include "_head.h"
#include "util_signal.c"

#define SIGNAL_REG_TYPE_DEF 0
#define SIGNAL_REG_TYPE_SOCKET 1
static void exit_signal_handler(int sig, siginfo_t * info, void *context);
void dregister_signal(int signal_reg_type=0);


