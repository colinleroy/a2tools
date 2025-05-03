#define CYCLES_PER_SEC 1023000
#define CARRIER_HZ 16000
#define DEFAULT_SAMPLING_HZ 4000

/* Determine the number of available cycles according to the desired carrier */
#define JUMP_OVERHEAD 6

#define DUTY_CYCLE_LENGTH (CYCLES_PER_SEC/CARRIER_HZ)
#define AVAIL_CYCLES   (DUTY_CYCLE_LENGTH)

#ifdef ENABLE_SLOWER
#define SLOWER_OVERHEAD 25
#define NUM_LEVELS     ((AVAIL_CYCLES-JUMP_OVERHEAD-SLOWER_OVERHEAD)-1)
#else
#define SLOWER_OVERHEAD 0
#define NUM_LEVELS     ((AVAIL_CYCLES-JUMP_OVERHEAD-8)-1) /* 8 is the overhead of the double STA SPKR */
#endif

#define STEP           1

#define PAGE_CROSSER   0x32
#define PAGE_CROSS_STR "$32"
