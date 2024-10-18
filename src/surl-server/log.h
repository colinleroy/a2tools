#ifndef __log_h
#define __log_h

#define LOG(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)

#endif
