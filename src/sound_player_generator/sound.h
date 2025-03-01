#define CYCLES_PER_SEC 1023000

/* Determine the number of available cycles according to the desired carrier */

#define DUTY_CYCLE_LENGTH (CYCLES_PER_SEC/CARRIER_HZ)
#define AVAIL_CYCLES   (DUTY_CYCLE_LENGTH-6)
#define NUM_LEVELS     (AVAIL_CYCLES-8)-1 /* Overhead of the double STA SPKR */

#define STEP           1

#define PAGE_CROSSER   0x32
