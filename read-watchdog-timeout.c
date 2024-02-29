#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>

int fd;

/*
   If a driver supports "Magic Close", the driver will not disable
   the watchdog unless a specific magic character 'V' has been sent to
   /dev/watchdog just before closing the file.
*/

const char  v = 'V';

static void term(int sig)
{
    int ret = write(fd, &v, 1);

    close(fd);
    if (ret < 0)
        printf("\nStopping watchdog ticks failed (%d)...\n", errno);
    else
        printf("\nStopping watchdog ticks...\n");
    exit(0);
}

static void read_timeout_left(void)
{
    int timeleft;
    int ret;

    ret = ioctl(fd, WDIOC_GETTIMELEFT, &timeleft);
    printf("Time timeout was %d seconds\n", timeleft);

}

int main(int argc, char *argv[])
{
    int flags;
    unsigned int ping_rate = 1;
    int ret;
    char *file = "/dev/watchdog";
    struct watchdog_info info;

    fd = open(file, O_WRONLY);

    if (fd == -1) {
         if (errno == ENOENT)
             printf("Watchdog device (%s) not found.\n", file);
         else if (errno == EACCES)
             printf("Run watchdog as root.\n");
         else
             printf("Watchdog device open failed %s\n",
                 strerror(errno));
         exit(-1);
    }

    ret = ioctl(fd, WDIOC_GETSUPPORT, &info);
    printf("WATCHDOG identity: %s, firmware version: %d, options: %d\n",
            info.identity, info.firmware_version, info.options);

    if (ret) {
        printf("WDIOC_GETSUPPORT error '%s'\n", strerror(errno));
        close(fd);
        exit(ret);
    }

    signal(SIGINT, term);

    flags = WDIOS_ENABLECARD;
    ret = ioctl(fd, WDIOC_SETOPTIONS, &flags);
    if (!ret)
        printf("Watchdog card enabled.\n");
    else {
        printf("WDIOS_ENABLECARD error '%s'\n", strerror(errno));
        exit(-1);
    }

    while(1) {
        read_timeout_left();
        sleep(ping_rate);
    }

    return 0;
}
