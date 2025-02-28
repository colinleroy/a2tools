#define CYCLES_PER_SEC 1023000
/* Determine the number of available cycles according to the desired carrier */
#define AVAIL_CYCLES   (CYCLES_PER_SEC/CARRIER_HZ)
/* The number of levels is determined by the number of wasted cycles.
 * We have to take into account the necessary cycles and figure out
 * what's left:
 * jsr into the function:               6 cycles
 * jsr into the sublevel:               6 cycles
 * ldx at start of the function:        2 cycles
 * sta / sta, the two speaker triggers: 8 cycles
 * rts:                                 6 cycles
 */
#define NUM_LEVELS     (AVAIL_CYCLES - 6 - 6 - 2 - 8 - 6)
#define STEP           1
