== Day 1 ==
Nothing particular

== Day 2 ==
```
gcc day-2-2.c && cat inputs/2.txt | ./a.out
```

== Day 3 ==

For part 1, install the serial monster between laptop and Apple //c

```
sudo apt install minicom
sudo stty -F /dev/ttyUSB0 9600 -parity cs8 -parenb -parodd -crtscts
```

On the Apple //c, boot prodos, go to Basic then configure serial input:

```
] IN #2
] <Ctrl-A>14B
```

On the laptop, send the source code:

```
ascii-xfr -nsv -l 500 -c 5 day-3-1.applesoft.basic > /dev/ttyUSB0
```

On the Apple //c, type

```
] RUN
```
On the laptop, send the data file, one line every 7 seconds, 30ms between each character:

```
ascii-xfr -nsv -l 7000 -c 30 inputs/3.txt > /dev/ttyUSB0
```
