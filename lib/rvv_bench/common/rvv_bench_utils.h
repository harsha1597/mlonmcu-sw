#ifndef RVV_BENCH_UTILS_H
#define RVV_BENCH_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "target.h"


#if __riscv_xlen == 32
typedef uint32_t ux;
typedef float fx;
#else
typedef uint64_t ux;
typedef double fx;
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define IFHOSTED(...) __VA_ARGS__

static void
memwrite(void const *ptr, size_t len)
{
	fwrite(ptr, 1, len, stdout);
}

static size_t
memread(void *ptr, size_t len)
{
	return fread(ptr, 1, len, stdin);
}
static void
memswap(void *a, void *b, size_t size)
{
	unsigned char *A = (unsigned char*)a, *B = (unsigned char*)b;
	unsigned char *aEnd = A + size;
	while (A < aEnd) {
		unsigned char temp = *A;
		*A++ = *B;
		*B++ = temp;
	}
}
void
qsort(void *base, size_t len, size_t size, int (*cmp)(const void *, const void *))
{
	if (len < 2) return;
	unsigned char *b = base;
	size_t pivot = len >> 1, l = 0, h = len - 1;
	for (; 1; ++l, --h) {
		while (cmp(b + l*size, b + pivot*size) < 0) ++l;
		while (cmp(b + h*size, b + pivot*size) > 0) --h;
		if (l >= h) break;
		memswap(b + l*size, b + h*size, size);
		pivot = pivot == l ? h : pivot == h ? l : pivot;
	}
	qsort(b, l, size, cmp);
	qsort(b + h * size + size, len - h - 1, size, cmp);
}

static void
rv_fencei() { __asm volatile("fence.i"); }

static ux
usqrt(ux y)
{
	ux L = 0, R = y + 1;
	while (L != R - 1) {
		ux M = (L + R) / 2;
		if (M * M <= y) L = M;
		else            R = M;
	}
	return L;
}

static ux
uhash(ux x)
{
#if __riscv_xlen == 32
	/* MurmurHash3 32-bit finalizer */
	x ^= x >> 16;
	x *= 0x85ebca6b;
	x ^= x >> 13;
	x *= 0xc2b2ae35;
	x ^= x >> 16;
#else
	/* splitmix64 finalizer */
	x ^= x >> 30;
	x *= 0xbf58476d1ce4e5b9U;
	x ^= x >> 27;
	x *= 0x94d049bb133111ebU;
	x ^= x >> 31;
#endif
	return x;
}

typedef struct { ux x, y, z; } URand;

#define ROTL(x,n) (((x) << (n)) | ((x) >> (8*sizeof(x) - (n))))

/* RomuDuoJr, see https://romu-random.org/ */
static inline ux
urand(URand *r)
{
	ux xp = r->x, yp = r->y, zp = r->z;
#if __riscv_xlen == 32
	r->x = 3323815723u * zp;
	r->y = ROTL(yp - xp, 6);
	r->z = ROTL(zp - yp, 22);
#else
	r->x = 15241094284759029579u * zp;
	r->y = ROTL(yp - xp, 12);
	r->z = ROTL(zp - yp, 44);
#endif
	return xp;
}

static inline float
urandf(URand *r) {
	uint32_t x = urand(r);
	return (x >> (32-24)) * (1.0f / (((uint32_t)1) << 24));
}

static void
memrand(URand *r, void *ptr, size_t n)
{
	unsigned char *p = (unsigned char*)ptr;
#ifdef __GNUC__
	typedef ux __attribute__((__may_alias__)) uxa;
	for (; n && (uintptr_t)p % sizeof(uxa); --n) *p++ = urand(r);
	uxa *px = (uxa*)p;
	for (; n > sizeof(ux); n -= sizeof(ux)) *px++ = urand(r);
	p = (unsigned char*)px;
#endif
	while (n--) *p++ = urand(r);
}


/* string conversions */

#if __riscv_xlen == 32
#define UXTOA_MAX 10
#else
#define UXTOA_MAX 20
#endif

static size_t
uxtoa(char *str, ux val)
{
	char buf[UXTOA_MAX], *end = buf + sizeof buf, *it = end;
	do *--it = (val % 10) + '0'; while ((val /= 10));
	val = end - it;
	memcpy(str, it, val);
	return val;
}


#define FTOA_MAX (20+9+2)

static inline size_t
ftoa(char *str, fx val, size_t prec)
{
	static const fx pow10[] = {
		1, 10, 100, 1000, 10000, 100000, 1000000,
		10000000, 100000000, 1000000000
	};
	char buf[FTOA_MAX], *end = buf + sizeof buf, *it = end;
	prec = prec > 9 ? 9 : prec;

	if (val != val) return memcpy(buf, "NaN", 3), 3;
	if (val < 0) val = -val, *--it = '-';

	ux ival = val, frac = (val-ival) * pow10[prec];
	do *--it = frac % 10 + '0'; while ((frac /= 10, --prec));
	*--it = '.';
	do *--it = ival % 10 + '0'; while ((ival /= 10));

	prec = end - it;
	memcpy(str, it, prec);
	return prec;
}
/* print API */

static char printBuffer[1024];
static char *printIt = printBuffer;
static char *const printEnd = printBuffer + sizeof printBuffer;

#define print_lit(s) print_raw(s, (sizeof s) - 1)

static void
print_flush(void)
{
	memwrite(printBuffer, printIt - printBuffer);
	printIt = printBuffer;
}

static void
print_raw(const char *s, size_t len)
{
	if ((uintptr_t)(printEnd - printIt) > len) {
		memcpy(printIt, s, len);
		printIt += len;
	} else {
		print_flush();
		memwrite(s, len);
	}
}

static void print_s(const char *s) { print_raw(s, strlen(s)); }
static void print_c(const char s) { print_raw(&s, 1); }

static void
print_u(ux val)
{
	if (printEnd - printIt < UXTOA_MAX)
		print_flush();
	printIt += uxtoa(printIt, val);
}

static void
print_h(ux val, size_t n)
{
	if ((n << 2) >= sizeof printBuffer)
		return;
	if ((size_t)(printEnd - printIt) < n)
		print_flush();
	while (n--) {
		*printIt++ = "0123456789abcdef"[(val >> (n << 2)) & 0xf];
	}
}

static void
print_b(ux val, size_t n)
{
	if (n >= sizeof printBuffer)
		return;
	if ((size_t)(printEnd - printIt) < n)
		print_flush();
	while (n--) {
		*printIt++ = ((val >> n) & 1) + '0';
	}
}

static void
print_fn(size_t prec, fx val)
{
	if (printEnd - printIt < FTOA_MAX)
		print_flush();
	printIt += ftoa(printIt, val, prec);
}

static void print_f(fx val) { print_fn(7, val); }


#define ARR_LEN(x) (sizeof x / sizeof *(x))



#define PRINT_AT_1(a,b,...) b
#define PRINT_SELECT(a,b,...) PRINT_AT_1(b,PRINT_FUNC,)
#define PRINT_LIT ,print_lit
#define PRINT_FUNC(f,...) print_##f(__VA_ARGS__)
#define PRINT_A(...) PRINT_SELECT(__VA_ARGS__,PRINT_LIT,)(__VA_ARGS__), (void)PRINT_B
#define PRINT_B(...) PRINT_SELECT(__VA_ARGS__,PRINT_LIT,)(__VA_ARGS__), (void)PRINT_A
#define print PRINT_A
static const char PRINT_A = 0, PRINT_B = 0;


#define PRINT_TEST_A(_) 1+PRINT_TEST_B
#define PRINT_TEST_B(_) 1+PRINT_TEST_A
#if PRINT_TEST_A(0)(0)(0) != 3 // if you get an error here, see below
// Your preprocessor implementation doesn't support sequence iteration,
// you can make the code compile by continuing the following pattern
# undef PRINT_B
# define PRINT_B(...)  PRINT_NEXT(__VA_ARGS__) PRINT_B1
# define PRINT_B1(...) PRINT_NEXT(__VA_ARGS__) PRINT_B2
#endif


#if __riscv_xlen == 32
#define IF64(...)
#else
#define IF64(...) __VA_ARGS__
#endif

#if __riscv_v_elen >= 64
#define IF_VE64(...) __VA_ARGS__
#else
#define IF_VE64(...)
#endif

#if __riscv_zfh >= 1000000
#define IF_F16(...) __VA_ARGS__
#else
#define IF_F16(...)
#endif

#if __riscv_zvfh >= 1000000 && IF_F16(1)+0
#define IF_VF16(...) __VA_ARGS__
#else
#define IF_VF16(...)
#endif

#if __riscv_flen == 64
#define IF_F64(...) __VA_ARGS__
#else
#define IF_F64(...)
#endif

#if __riscv_v_elen_fp == 64
#define IF_VF64(...) __VA_ARGS__
#else
#define IF_VF64(...)
#endif


#endif  // RVV_BENCH_UTILS_H
