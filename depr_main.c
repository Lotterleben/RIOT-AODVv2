#include <string.h>
#include <stdio.h>
#include <time.h>
#include <thread.h>

#include <sys/types.h>
#include <unistd.h>

#include "vtimer.h"
#include "board_uart0.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "posix_io.h"
#include "nativenet.h"
#include "msg.h"
#include "destiny.h"
#include "destiny/socket.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused)))
{
    shell_t shell;
    struct tm localt;

    printf ("HALLOHALLO");

    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    /* fancy greeting */
    printf("Hold on half a second...\n");
    LED_RED_TOGGLE;
    vtimer_usleep(500000);
    LED_RED_TOGGLE;
    LED_GREEN_ON;
    LED_GREEN_OFF;

    printf("\n\t\t\tWelcome to RIOT\n\n");

    return 0;
}
