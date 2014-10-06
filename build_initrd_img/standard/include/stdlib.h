/*stdlib.h the stdlib header*/

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include "common.h"
#include "syscall.h"
#include "task.h"
#include "fs.h"

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

typedef unsigned int size_t;

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

int atoi(const char *nptr);

void initscr();

void clrscr();

void endwin();

void move(int y, int x);

void cputs_line(const char * str);

void cputs(const char * str);

void putch(char c);

int getch(void);

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

char *tmpnam(char *s);

FILE * tmpfile();

void unlink_tmpfile();

int fseek(FILE * file, long offset, int whence);

int fread(void *ptr, size_t size, size_t nmemb, FILE * file);

int fwrite(const void *ptr, size_t size, size_t nmemb, FILE * file);

void exit(int code);

//printf.c
int sprintf(char *buf, const char *fmt, ...);
// printf.c
int printf(const char *fmt, ...);
// printf.c
int cprintf(const char *fmt, ...);

#endif // _STDLIB_H_

