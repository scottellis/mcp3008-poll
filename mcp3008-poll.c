/*
   Copyright (C) 2017, Jumpnow Technologies, LLC

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>

#define ADC_READ_ERROR -100000

#define MAX_ADC 8

char iiosyspath[] = "/sys/bus/iio/devices/iio:device0/";

void register_sig_handler();
void sigint_handler(int sig);
void show_elapsed(struct timeval *start, struct timeval *end, int count);
int loop(int delay_us, int *list);
int open_adc(int adc);
int read_adc(int fd);

int abort_read;

void usage(char *argv_0)
{
    printf("\nUsage: %s <options> [adc-list]\n", argv_0);
    printf("  -d<delay-us>       Microsecond delay between reads, default 10000, min 0\n");
    printf("  adc-list           Space separated list of ADCs to monitor, 0-7\n");
    printf("\nExample:\n\t%s -d100 0 1\n", argv_0);

    exit(0);
}

int main(int argc, char **argv)
{
    int opt, delay_us, adc, i;
    struct timeval start, end;
    int adc_list[MAX_ADC];

    register_sig_handler();

    delay_us = 10000;

    while ((opt = getopt(argc, argv, "d:h")) != -1) {
        switch (opt) {
        case 'd':
            delay_us = atoi(optarg);

            if (delay_us < 0)
                delay_us = 0;

            break;

        case 'h':
        default:
            usage(argv[0]);
            break;
        }
    }

    // now get the adc list
    if (optind == argc) {
        printf("List of ADCs is required\n");
        usage(argv[0]);
    }

    memset(adc_list, 0, sizeof(adc_list));

    for (i = optind; i < argc; i++) {
        adc = atoi(argv[i]);

        if (adc < 0 || adc > 7) {
            printf("adc %d is out of range\n", adc);
            usage(argv[0]);
        }

        if (adc_list[adc]) {
            printf("adc %d listed more then once\n", adc + 2);
            usage(argv[0]);
        }

        adc_list[adc] = 1;
    }

    if (gettimeofday(&start, NULL) < 0) {
        perror("gettimeofday: start");
        return 1;
    }

    int count = loop(delay_us, adc_list);

    if (gettimeofday(&end, NULL) < 0)
        perror("gettimeofday: end");
    else
        show_elapsed(&start, &end, count);

    return 0;
}

// We know the diff is never going to be that big
void show_elapsed(struct timeval *start, struct timeval *end, int count)
{
    double diff;
    double rate;

    if (end->tv_usec > start->tv_usec) {
        diff = (double) (end->tv_usec - start->tv_usec);
    }
    else {
        diff = (double) ((1000000 + end->tv_usec) - start->tv_usec);
        end->tv_sec--;
    }

    diff /= 1000000.0;

    diff += (double)(end->tv_sec - start->tv_sec);

    if (diff > 0.0)
        rate = count / diff;
    else
        rate = 0.0;

    printf("Summary\n  Elapsed: %0.2lf seconds\n    Reads: %d\n     Rate: %0.2lf Hz\n\n",
        diff, count, rate);
}

int loop(int delay_us, int *list)
{
    int count, i, update, update_reset;
    int val[MAX_ADC];
    int fd[MAX_ADC];

    count = 0;
    memset(fd, 0, sizeof(fd));
    memset(val, 0, sizeof(val));

    // update_reset is just a quick hack to throttle screen updates
    if (delay_us < 1) {
        // with no usleep events, max sampling rate is around 20k Hz
        update_reset = 2000;
    }
    else {
        // with usleep, we top out around 8k Hz
        update_reset = 800000 / delay_us;

        if (update_reset > 800)
            update_reset = 800;
        else if (update_reset < 1)
            update_reset = 1;
    }

    fprintf(stdout, "\n(use ctrl-c to stop)\n\n");

    fprintf(stdout, "ADC          ");

    for (i = 0; i < MAX_ADC; i++) {
        if (list[i])
            fprintf(stdout, "      %d", i);
    }

    fprintf(stdout, "\n");

    for (i = 0; i < MAX_ADC; i++) {
        if (list[i]) {
            fd[i] = open_adc(i);

            if (fd[i] < 0)
                goto loop_done;
        }
    }

    // update first read
    update = 1;

    while (!abort_read) {
        for (i = 0; i < MAX_ADC; i++) {
            if (!list[i])
                continue;

            val[i] = read_adc(fd[i]);

            // reset for next read
            lseek(fd[i], 0, SEEK_SET);

            if (val[i] == ADC_READ_ERROR)
                break;
        }

        update--;

        if (update == 0) {
            update = update_reset;

            fprintf(stdout, "\rRead %8d: ", count + 1);

            for (i = 0; i < MAX_ADC; i++) {
                if (list[i])
                    fprintf(stdout, " %4d  ", val[i]);
            }

            fflush(stdout);
        }

        count++;

        if (delay_us)
            usleep(delay_us);
    }

    fprintf(stdout, "\n\n");

loop_done:

    for (i = 0; i < MAX_ADC; i++) {
        if (fd[i] > 0)
            close(fd[i]);
    }

    return count;
}

int read_adc(int fd)
{
    char buff[8];

    int val = ADC_READ_ERROR;

    memset(buff, 0, sizeof(buff));

    if (read(fd, buff, 8) < 0)
        perror("read()");
    else
        val = atoi(buff);

    return val;
}

int open_adc(int adc)
{
    char path[128];
    sprintf(path, "%sin_voltage%d_raw", iiosyspath, adc);

    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        perror("open()");
        printf("%s\n", path);
    }

    return fd;
}

void register_sig_handler()
{
    struct sigaction sia;

    bzero(&sia, sizeof sia);
    sia.sa_handler = sigint_handler;

    if (sigaction(SIGINT, &sia, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(1);
    }
}

void sigint_handler(int sig)
{
    abort_read = 1;
}
