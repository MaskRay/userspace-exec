#pragma once

#define REP(i, n) FOR(i, 0, n)
#define FOR(i, a, b) for (int i = (a); i < (b); i++)
#define ROF(i, a, b) for (int i = (b); --i >= 0; )
#define PAGESZ 0x1000

typedef unsigned long ul;
#define ALIGN_DOWN(x) ((x) & -PAGESZ)
#define ALIGN_UP(x) ((x)+PAGESZ-1 & -PAGESZ)
//#define INLINE static inline __attribute__((always_inline))
#define INLINE static inline
#define SHELLCODE __attribute__((section(".shellcode")))

#ifdef DEBUG
# define DP(fmt, ...) fprintf(stderr, "%s:%d:%s " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
# define DP(...)
#endif
