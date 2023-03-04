#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "err.h"

void handler(int signal_number)
{
    const char s[] = "I received a signal.\n";
    ASSERT_SYS_OK(write(STDOUT_FILENO, s, strlen(s)));
}

int main(void)
{
    printf("My pid is %d\n", getpid());
    printf("For 5 seconds I will handle SIGINT by myself.");
    printf("Try signalling me with Ctrl + C or by executing:\n");
    printf("kill -SIGINT %d\n", getpid());

    struct sigaction new_action;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    new_action.sa_handler = handler;
    ASSERT_SYS_OK(sigaction(SIGINT, &new_action, NULL));

    // Note that `sleep` immediately ends after handling a signal.
    for (int i = 0; i < 5; ++i) {
        printf("%d\n", i);
        sleep(1);
    }

    new_action.sa_handler = SIG_DFL;
    ASSERT_SYS_OK(sigaction(SIGINT, &new_action, NULL));

    printf("I restored the default signal handling action (termination).\n");
    printf("I'm waiting 5 more seconds for signals.\n");

    sleep(5);

    printf("No more signals received.\n");

    return 0;
}
