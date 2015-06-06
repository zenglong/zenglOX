/*stdlib.h the stdlib header*/

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include "common.h"
#include "syscall.h"
#include "task.h"
#include "fs.h"
#include "kheap.h"

#define FILE_SIGN 0x13141588
#define EOF -1
#define LIBC_CK_SHIFT	1
#define LIBC_CK_ALT		2
#define LIBC_CK_CTRL		4

#ifndef va_arg

#ifndef _VALIST
#define _VALIST
typedef char *va_list;
#endif	

/*
 * Storage alignment properties
 */
#define  _AUPBND                (sizeof (SINT32) - 1)
#define  _ADNBND                (sizeof (SINT32) - 1)

/*
 * Variable argument list macro definitions
 */
#define _bnd(X, bnd)            (((sizeof (X)) + (bnd)) & (~(bnd)))
#define va_arg(ap, T)           (*(T *)(((ap) += (_bnd (T, _AUPBND))) - (_bnd (T,_ADNBND))))
#define va_end(ap)              (void) 0
#define va_start(ap, A)         (void) ((ap) = (((char *) &(A)) + (_bnd (A,_AUPBND))))

#endif				/* va_arg */

#define SEEK_SET 0
#define SEEK_CUR -2
#define SEEK_END -4

#define CLOCKS_PER_SEC 50
//#define CLOCKS_PER_SEC 1000

#define LONG_MIN (-2147483647L - 1)
#define LONG_MAX 2147483647L
#define INT_MAX 2147483647L
#define JMP_BUF_SIZE 6

typedef unsigned int size_t;
typedef unsigned int clock_t;
typedef unsigned short wchar_t;
typedef unsigned int time_t;
typedef int jmp_buf[JMP_BUF_SIZE];

typedef enum _FILE_OP_TYPE
{
	FILE_OP_TP_RD = 1,
	FILE_OP_TP_WR = 2,
	FILE_OP_TP_RW = 3,
} FILE_OP_TYPE;

typedef struct _FILE
{
	UINT32 sign;
	UINT32 cur;
	BOOL isdirty;
	FS_NODE * fsnode;
	CHAR * buf;
	UINT32 buf_size;
	FILE_OP_TYPE op_type;
} FILE;

SINT32 strnlen(const char *s, SINT32 count);

int isdigit(int x);

// checks for an alphabetic character
// see http://code.google.com/p/minilib-c/source/browse/trunk/ctype/isalpha.c
int isalpha(int c);

// checks for an alphanumeric character; it is equivalent to (isalpha(c) || isdigit(c)).
// see http://code.google.com/p/minilib-c/source/browse/trunk/ctype/isalnum.c
int isalnum(int c);

// checks for hexadecimal digits, that is, one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F.
// see http://code.google.com/p/minilib-c/source/browse/trunk/ctype/isxdigit.c
int isxdigit (int c);

// uppercase character predicate
// see http://code.google.com/p/minilib-c/source/browse/trunk/ctype/isupper.c
int isupper(int c);

/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
// see http://code.google.com/p/minilib-c/source/browse/trunk/stdlib/strtol.c
long strtol (const char *nptr, char **ptr, int base);

// it only compares the first (at most) n bytes of s1 and s2
// see http://code.google.com/p/minilib-c/source/browse/trunk/string/strncmp.c
int strncmp(const char *s1, const char *s2, size_t n);

// counted copy string
// see http://code.google.com/p/minilib-c/source/browse/trunk/string/strncpy.c
char *strncpy(char *dst0, const char *src0, size_t count);

int atoi(const char *nptr);

// convert ASCII string to long
// see http://code.google.com/p/minilib-c/source/browse/trunk/stdlib/atol.c
long atol(const char *s);

// convert ASCII string to long long
// also see http://code.google.com/p/minilib-c/source/browse/trunk/stdlib/atol.c
long long atoll(const char *s);

// So given a string like "2.23" your function should return double 2.23. This might seem easy in the first place but this is a highly 
// ambiguous question. Also it has some interesting test cases. So overall a good discussion can revolve around this question. 
// We are not going to support here scientific notation like 1.45e10 etc. 
// We will also not support hex and octal strings just for the sake of simplicity. 
// But these are good assumption to state upfront. Let's write code for this.
// see http://crackprogramming.blogspot.com/2012/10/implement-atof.html
double atof(const char* num);

void initscr();

void clrscr();

void endwin();

void move(int y, int x);

void cputs_line(const char * str);

void cputs(const char * str);

void putch(char c);

int getch(void);

int getchar(void);

FILE * fopen(char * filename, char * op_type_str);

int fclose(FILE * file);

int fgetc(FILE * file);

int fputc(int c, FILE * file);

int fputs(const char * s, FILE * file);

int unlink(const char * pathname);

int rename(const char * oldpath, const char * newpath);

char * strchr( char *str, char ch );

int tolower(int ch);

UINT8 get_control_key();

UINT8 release_control_keys(UINT8 key);

int deleteln();

int insertln();

void srand(unsigned int seed);

/* This algorithm is mentioned in the ISO C standard, here extended
   for 32 bits. you can see 
   http://stackoverflow.com/questions/1026327/what-common-algorithms-are-used-for-cs-rand
 */
int rand_r(unsigned int *seed);

int rand(void);

int timer_get_tick();

clock_t clock(void);

time_t time(time_t *t);

char *tmpnam(char *s);

FILE * tmpfile();

void unlink_tmpfile();

int fseek(FILE * file, long offset, int whence);

long ftell(FILE *stream);

int feof(FILE *stream);

int fread(void *ptr, size_t size, size_t nmemb, FILE * file);

int fwrite(const void *ptr, size_t size, size_t nmemb, FILE * file);

int fprintf(FILE *stream, const char *format, ...);

void * malloc(size_t size);

void free(void * ptr);

void * realloc(void * ptr, size_t size);

void exit(int code);

int abs(int j);

//printf.c
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
// printf.c
int printf(const char *fmt, ...);
int vprintf(const char *format, va_list ap);
// printf.c
int cprintf(const char *fmt, ...);

// vsnprintf.c
/*
** This vsnprintf() emulation does not implement the conversions:
**	%e, %E, %g, %G, %wc, %ws
** The %f implementation is limited.
*/
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int snprintf(char *buf, size_t size, const char *fmt, ...);

// qsort.c
void qsort(void *base, unsigned num, unsigned width, int (*comp)(const void *, const void *));

#endif // _STDLIB_H_

