#define CYCLES_PER_SEC 1023000

/* Determine the number of available cycles according to the desired carrier */

#ifdef CPU_65c02
/* 30 is the overhead of the byte reading loop */
#define READ_OVERHEAD 30
#else
/* 43 is the overhead of the byte reading loop,
 * due to the expensive lda/sta replacing the jmp ($nnnn,x) */
#define READ_OVERHEAD 43
#endif

#define DUTY_CYCLE_LENGTH (CYCLES_PER_SEC/CARRIER_HZ)
#define AVAIL_CYCLES   (DUTY_CYCLE_LENGTH-READ_OVERHEAD)
#define NUM_LEVELS     (AVAIL_CYCLES-8) /* Overhead of the double STA SPKR */

#define STEP           1

#define PAGE_CROSSER   0x32
