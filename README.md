## mcp3008-poll

A command line program for reading mcp3008 ADCs.

Developed for testing MCP3008 ADCs connected to an RPi3, but should
work with other ADC devices showing up under sysfs.

Build with make.

Run

    root@rpi3:~/mcp3008-poll# ./mcp3008-poll -h

    Usage: ./mcp3008-poll <options> [adc-list]
      -d<delay-us>       Microsecond delay between reads, default 10000, min 0
      adc-list           Space separated list of ADCs to monitor, 0-7

    Example:
            ./mcp3008-poll -d100 0 1


Some sample runs with the SPI clock running at 1MHz (ADC powered by 3.3v)

One channel

    root@rpi3:~/mcp3008-poll# ./mcp3008-poll -d0 0

    (use ctrl-c to stop)

    ADC                0
    Read  1036000:   621  ^C

    Summary
      Elapsed: 56.93 seconds
        Reads: 1037028
         Rate: 18215.51 Hz

Two channels

    root@rpi3:~/mcp3008-poll# ./mcp3008-poll -d0 0 1

    (use ctrl-c to stop)

    ADC                0      1
    Read   626000:   620    621  ^C

    Summary
      Elapsed: 69.15 seconds
        Reads: 627694
         Rate: 9077.10 Hz

Three channels

    root@rpi3:~/mcp3008-poll# ./mcp3008-poll -d0 0 1 2

    (use ctrl-c to stop)

     ADC                0      1      2
     Read   540000:   621    621      0  ^C

    Summary
      Elapsed: 89.53 seconds
        Reads: 541640
         Rate: 6049.50 Hz

Four channels

    root@rpi3:~/mcp3008-poll# ./mcp3008-poll -d0 0 1 2 3

    (use ctrl-c to stop)

    ADC                0      1      2      3
    Read   446000:   620    621      0      0  ^C

    Summary
      Elapsed: 97.90 seconds
        Reads: 446401
         Rate: 4559.73 Hz

Eight channels

    root@rpi3:~/mcp3008-poll# ./mcp3008-poll -d0 0 1 2 3 4 5 6 7

    (use ctrl-c to stop)

    ADC                0      1      2      3      4      5      6      7
    Read   202000:   621    621      0      0      0      0      0      0  ^C

    Summary
      Elapsed: 88.20 seconds
        Reads: 202710
         Rate: 2298.37 Hz

