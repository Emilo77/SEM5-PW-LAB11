#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "err.h"

void handler(int signal_number)
{
    const char s[] = "Child: I received a signal.\n";
    write(STDOUT_FILENO, s, strlen(s));
}

int child()
{
    printf("Child starting, pid %d\n", getpid());

    // Be annoying - replace SIGINT handler to just print a message and do nothing.
    struct sigaction action;
    ASSERT_SYS_OK(sigaction(SIGINT, NULL, &action));
    action.sa_handler = handler;
    ASSERT_SYS_OK(sigaction(SIGINT, &action, NULL));

    for (int i = 0; i < 5; ++i) {
        printf("%d\n", i);
        sleep(1);
    }
    return 0;
}

// Same as sigtimedwait, but takes number of seconds instead of timespec.
int sigtimedwait_seconds(const sigset_t* set, siginfo_t* info, double seconds)
{
    struct timespec timeout = {
        .tv_sec = (int)seconds,
        .tv_nsec = (int)(1e9 * (seconds - (int)seconds))
    };
    return sigtimedwait(set, info, &timeout);
}

int parent(pid_t child_pid)
{
    printf("Signal me with Ctrl+C or:\nkill -SIGINT %d\n\n", getpid());

    sigset_t signals_to_wait_for;
    sigemptyset(&signals_to_wait_for);
    sigaddset(&signals_to_wait_for, SIGINT);

    // Block signals to ensure we don't handle them with default handlers between waits.
    ASSERT_SYS_OK(sigprocmask(SIG_BLOCK, &signals_to_wait_for, NULL));

    siginfo_t info;
    while (true) {
        sigwaitinfo(&signals_to_wait_for, &info);

        printf("Parent: got signal >>%s<< from %d\n", strsignal(info.si_signo), info.si_pid);

        printf("Sending SIGINT to child. Signal again within 1 second to terminate.\n");

        int ret = sigtimedwait_seconds(&signals_to_wait_for, &info, 1.0);
        if (ret == -1) // Timeout.
            continue;
        else {
            printf("Parent: got second signal >>%s<< from %d\n", strsignal(info.si_signo), info.si_pid);
            break;
        }
    }
    // We won't wait for signals anymore, so we unblock default handlers, just in case.
    ASSERT_SYS_OK(sigprocmask(SIG_UNBLOCK, &signals_to_wait_for, NULL));

    printf("Parent: ending.\n");
    int status;
    wait(&status);
    if (WIFEXITED(status))
        printf("Child exited normally with return exit status %d.\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Child terminated by signal %d (%s).\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
    else
        printf("Unexpected wait() result.");
    return 0;
}

int main()
{
    // Fork.
    pid_t child_pid = fork();
    ASSERT_SYS_OK(child_pid);
    if (!child_pid) {
        // Ctrl+C actually sends SIGINT to the whole process group.
        // The child is in the same process group as the parent, after fork.
        // We want the parent to control what to do after Ctrl+C,
        // so the child sets the process group id to its own, new group.
        setpgid(0, 0);

        return child();
    } else {
        return parent(child_pid);
    }
}
