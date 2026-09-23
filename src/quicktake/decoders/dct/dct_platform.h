#include "platform.h"

#define DESCALE_FACTOR 1

void get_coeffs(void);
void get_bitval(void);
void advance_block(void);
void idct_common(void);
void idct_1d_rows(void);
void idct_1d_cols(void);
void init_idx(void);
void update_idx(void);
