#define _DARWIN_C_SOURCE 1
#include <CoreFoundation/CoreFoundation.h>
#include <DiskArbitration/DiskArbitration.h>
#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
static volatile sig_atomic_t stop_signal;

static void stop(int signal_number) {
    stop_signal = signal_number;
}

static DADissenterRef approve_mount(DADiskRef disk, void *context) {
    DADiskRef whole = DADiskCopyWholeDisk(disk);
    if (!whole) return NULL;
    const char *name = DADiskGetBSDName(whole);
    int matches = name && strcmp(name, context) == 0;
    CFRelease(whole);
    return matches ? DADissenterCreate(NULL, kDAReturnNotPermitted,
                                      CFSTR("Image write and verification in progress")) : NULL;
}

static int spawn_command(pid_t *child, char **arguments) {
    posix_spawnattr_t attributes;
    int error = posix_spawnattr_init(&attributes);
    if (error) return error;
    error = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
    if (!error) error = posix_spawnattr_setpgroup(&attributes, 0);
    if (!error) error = posix_spawnp(child, arguments[0], NULL, &attributes, arguments, environ);
    posix_spawnattr_destroy(&attributes);
    return error;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s /dev/diskN command [args...]\n", argv[0]);
        return 1;
    }
    DASessionRef session = DASessionCreate(NULL);
    if (!session) {
        fprintf(stderr, "Cannot connect to Disk Arbitration\n");
        return 1;
    }
    DADiskRef disk = DADiskCreateFromBSDName(NULL, session, argv[1]);
    DADiskRef whole = disk ? DADiskCopyWholeDisk(disk) : NULL;
    if (!whole || strcmp(DADiskGetBSDName(disk), DADiskGetBSDName(whole)) != 0) {
        fprintf(stderr, "Mount guard requires an existing whole disk\n");
        if (whole) CFRelease(whole);
        if (disk) CFRelease(disk);
        CFRelease(session);
        return 1;
    }
    const char *target = DADiskGetBSDName(whole);
    DARegisterDiskMountApprovalCallback(session, NULL, approve_mount, (void *)target);
    DASessionScheduleWithRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    struct sigaction action = {0};
    action.sa_handler = stop;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);

    pid_t child;
    int error = spawn_command(&child, argv + 2);
    int status = 0;
    int result = 1;
    if (error) {
        fprintf(stderr, "Cannot start flash command: %s\n", strerror(error));
    } else {
        for (;;) {
            if (stop_signal) {
                kill(-child, stop_signal);
                stop_signal = 0;
            }
            siginfo_t info = {0};
            if (waitid(P_PID, (id_t)child, &info, WEXITED | WSTOPPED | WNOHANG | WNOWAIT) < 0) {
                if (errno == EINTR) continue;
                perror("waitid");
                kill(-child, SIGKILL);
                waitpid(child, &status, 0);
                break;
            }
            if (info.si_pid == child) {
                if (info.si_code == CLD_STOPPED) fprintf(stderr, "Flash command stopped; aborting\n");
                kill(-child, SIGKILL);
                pid_t reaped;
                do { reaped = waitpid(child, &status, 0); } while (reaped < 0 && errno == EINTR);
                if (reaped < 0) { perror("waitpid"); break; }
                result = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
                break;
            }
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
        }
    }
    DAUnregisterApprovalCallback(session, approve_mount, (void *)target);
    DASessionUnscheduleFromRunLoop(session, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    CFRelease(whole);
    CFRelease(disk);
    CFRelease(session);
    return result;
}
