#define CYCLES_PER_SEC 1023000
/* Determine the number of available cycles according to the desired carrier */

#ifdef CPU_65c02
/* 9 is the overhead of the jmp($nnnn,x)+jmp back */
#define AVAIL_CYCLES   ((CYCLES_PER_SEC/CARRIER_HZ)-9)
#else
/* 9 is the overhead of the jmp($nnnn,x)+jmp back,
 * 13 is the overhead of the lda/sta replacing the jmp ($nnnn,x) */
#define AVAIL_CYCLES   ((CYCLES_PER_SEC/CARRIER_HZ)-9-13)
#endif

#define NUM_LEVELS     (AVAIL_CYCLES - 28) /* Overhead of the reading */

#define STEP           1

#define PAGE_CROSSER   0x32
