typedef char bigint;

bigint *bigint_new(const bigint *a);
bigint *trim_leading_zeroes(bigint *a);
bigint *bigint_add(const bigint *a, const bigint *b);
int bigint_bigger(const bigint *a, const bigint *b);
bigint *bigint_sub(const bigint *a, const bigint *b);
bigint *bigint_mul(const bigint *a, const bigint *b);
bigint *bigint_div(bigint *a, bigint *b);
bigint *bigint_mod(bigint *a, bigint *b);
