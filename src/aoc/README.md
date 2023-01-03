== Day 1 ==
Nothing particular

== Day 2 ==
```
gcc day-2-2.c && cat inputs/i2.txt | ./a.out
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
ascii-xfr -nsv -l 7000 -c 30 inputs/i3.txt > /dev/ttyUSB0
```

Result: 
![Result of the algorithm on the Apple //c screen](images/0a31609a44893454.jpeg)

== Day 4 ==

Same procedure

== Day 5 ==

Spaces at start of lines are ignored if entered raw, we have to prefix a double-quote
to have full lines before feeding them to the Apple //c.

Hence, the input file is preprocessed with sed:

```
sed -i 's/^/"/' inputs/i5.txt
```

== Day 6 ==

Input consists of a very long line, so we will adapt the inter-character delay 
of ascii-xfr:

```
ascii-xfr -nsv -l 100 -c 100 inputs/i6.txt > /dev/ttyUSB0
```

Also, I used ingest.basic to save the data to a floppy, and read it from the
floppy. It allowed the main program to run faster.

== Day 7 ==

Input starting with $ is interpreted as hexadecimal number, so we preprocess 
input with sed to prefix each line with a quote:

```
sed -i 's/^/"/' inputs/i7.txt
```
I tried to save it locally using my ingest.basic file, but the Apple //c hangs
at around 6.2kB and I can't understand why, so back to ascii-xfr.

Once input stops, the user has to hit ENTER until we're back at the / level of
the "filesystem" before getting the result.

Luckily my tracing PRINT showed the total when exiting a directory, saving me
an extra run before doing part 2!

== Day 8 ==

Now that's complicated. Memory's limited and I can't use as many arrays as I
want to. DIM A(100,100) directly gives an OUT OF MEMORY ERROR.

DIM A$(100,100) is OK so we're going to use ASC() and CHR$ a lot.

First try failed because accessing memory is slower the further you are in the
array, and I missed starts of lines sent via serial. Didn't want to do multiple
runs to find out minimal safe delay.

Second try failed : I dumped the contents to floppy using lingest.basic and
ascii-xfr with a 5s inter-line delay. Then tried to OPEN and READ the file from
BASIC, but things got REAL slow (1 char every 10s), probably due to memory usage.

Third try, reading line by line and splitting into two-dimensional TREE$ array
during reading, got too slow after a few lines too.

Fourth try is one more loop, first reading all data, not building a bidimensional
array but instead making use of MID$, then iterating twice on that (from top-left,
then from bottom-right), then one last time to count visible trees. It failed with
an OUT OF MEMORY ERROR.

Figured out about INTEGER variables, DIM TREE%(100,100) for example. Back to reading
char by char, but this time reading everything first THEN doing the job.
Less elegant, but this one worked in a reasonable timeframe (2 hours for the 
beautiful O(4n²) algorithm of part 1).

I made myself a checklines.sh helper to verify I don't overwrite a line by 
having two of them numbered identically.

Sent data with `-l 1000 -c 350`.

== Day 9 ==

Day 9 was worse. 

Simple way to count "squares I've been in" without counting some squares multiple
times is to push ones in a bidimensional array, then walk the array. BUT. The
rope moves around a 267x220 rectangle (for my input), which means 57kB of data,
which does NOT fit in the Apple //c.

The worst way is to push each square in an array *if* it's not there already. 
Fine, but that makes the algorithm O(n^3), which is a big turnoff for me even on
a 4 GHz CPU, let alone a 1MHz CPU running interpreted code. I've still tried it,
it was agonizing and I aborted the run.

In the end, I ditched BASIC for C, using cc65. This allowed me to make my own
makeshift bidimensional array of... booleans. This of course required quite a bit
of pointer arithmetic, then quite a bit of debugging as variable overflows occur
really easily when sizeof(int) == 2...

But in the end it works, and best of all, it works for both parts!

Debugging via printf: 
![Debugging via printf](images/day-9-1.png)

== Day 10 ==

Day 10 was nice and easy.

== Day 11 ==

Implementing the parser was a good occasion to add strsplit() and trim() to my
own little library for the Apple //c.

Part 2 had me hit a wall really hard, implementing an extremely slow bigint
library working on... strings and doing grade schooler additions and multiplications
in it, before I FINALLY thought of the correct solution to have this finish 
before 2026 (even on the laptop it wouldn't go past round 7/10000 in less than
a minute).

== Day 12 ==

For day 12 I implemented a simply-linked list in order to code cleanly and be
able to retrace the shortest path. But as much as it worked on the sample input,
it was much too much large in RAM to run on the Apple. So I made another dirty
version using simples tables, counting steps and not being able to retrace the
path. (19kB in RAM instead of 9.6MB...).

== Day 13 ==

I didn't figure out the smart way to get the answer to part 2 without sorting
the whole dataset (you can. I figured it out later).

So instead I did an external sort, where the dataset is pre-sorted by manageable
chunks of 10 packets into files saved to ... the floppy.
Then files are read in pair and merge sorted into a new set of twice-bigger files.
Then again, and again until there's one file left with the whole dataset. It's
really easy on the RAM but of course, given the latency and bandwidth of an 1980
5.25" floppy disk drive, it's not real fast (25 minutes).

== Day 18 ==

My process for "successfully run my code on the Apple //c" has now quite a few steps:

- Figure out the algorithm (often the hardest part)
- Figure out how to do it in a reasonable number of cycles and bytes of RAM
- Program, do not go any further until the compiler gives 0 warning
- Program, do every local test run using `valgrind --leak-check=full --show-leak-kinds=all`, do not go any further as long as it says anything else than "ERROR SUMMARY: 0 errors from 0 contexts".
- Revert to gdb for debugging purposes
- Run `rm callgrind.out.* ; valgrind --tool=callgrind ./a.out && kcachegrind callg*`, do not go any further if the number of CPU cycles seems really crap (100M is crap but tolerable, it means approx. 2 hours runtime. 10M is OK.)
- Run `rm mass*; valgrind --tool=massif ./a.out && massif-visualizer mass*`, do not go any further if the RAM use is way off. You get about 22k useable for malloc()ing with the normal linking. This steps may require a bit of math because 80k of `char` on the PC means 80k of `char` on the Apple //c, but 80k of `void *` on the PC means 20k of `void *` on the Apple //c, as pointers are 16bit there. (reminder: sizeof(char) = 1, sizeof(short) = 2, sizeof(int) = 2, sizeof(long) = 4, sizeof(void *) = 2).

But it was not enough for day18, where the recursive DFS reached really deep levels, which happily grew the stack in the rest of the RAM and crashed. So I'll be adding `--check-stack` to cc65's CFLAGS to this process, and get a much quicker idea of the problem when I'm suddenly dropped to the asm debug prompt of the Apple //c (had to sleep on it for day18's problem).
