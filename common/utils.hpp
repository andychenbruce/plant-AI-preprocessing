#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
#include <algorithm>
#define Assert(x)	andyAssert(x, #x, __FILE__, __LINE__)

/*
typedef unsigned int   UInt32;
typedef unsigned short UInt16;
typedef unsigned char  UInt8;

typedef signed int   SInt32;
typedef signed short SInt16;
typedef signed char  SInt8;
*/

void
fatal(const char * const fmt, ...) __attribute__ ((format(printf, 1, 2)));


void
epf(const char * const fmt, ...)
  __attribute__ ((format(printf, 1, 2)))
  __attribute__ ((unused));



const char * syserr(void);

void
andyAssert(int x, const char *s, const char *file, int line);


void * andyMalloc(size_t n);

uint8_t *
Readfile(const char * const fn, uint32_t *size)
  __attribute__ ((unused));

std::vector<std::string> getListOfFiles(const char *s);



template <class T> T** make2DPointerArray(int height, int width);
template <class T> void free2DPointerArray(T** p, int height);
