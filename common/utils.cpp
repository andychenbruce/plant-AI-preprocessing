#include "utils.hpp"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

__inline__ uint32_t min(uint32_t a, uint32_t b) { return (a < b) ? a : b; }

void
fatal(const char * const fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(1);
}


void
epf(const char * const fmt, ...)
  __attribute__ ((format(printf, 1, 2)))
  __attribute__ ((unused));

void
epf(const char * const fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  return;
}



const char *
syserr(void)
{
  return strerror(errno);
}

void
andyAssert(int x, const char *s, const char *file, int line)
{
  if (x == 0) {
    fatal("Assertion failed, %s line %d: %s\n", file, line, s);
  }
  return;
}

void*
andyMalloc(size_t bytes)
{
  void *p = malloc(bytes);
  if (p == NULL) {
    fatal("andyMalloc(%d): %s\n", (int)bytes, syserr());
  }
  return p;
}

uint8_t *
Readfile(const char * const fn, uint32_t *size)
{
  struct stat statbuf;
  uint32_t filesize;
  uint8_t *data;
  int32_t fd;

  if ((fd = open(fn, O_RDONLY)) < 0) {
    fatal("Readfile: Cannot open %s: %s\n", fn, syserr());
  }
  if (fstat(fd, &statbuf) != 0) {
    fatal("Cannot stat %s: %s\n", fn, syserr());
  }
  filesize = (size_t) statbuf.st_size;
  data = (uint8_t *) andyMalloc(filesize);
  if (read(fd, data, (size_t) filesize) != (ssize_t) filesize) {
    fatal("read error, %s: %s\n", fn, syserr());
  }
  close(fd);
  if (size != NULL) {
    *size = filesize;
  }
  return data;
}


std::vector<std::string>
getListOfFiles(const char *s)
{
  std::vector<std::string> names;
  struct dirent *dp;
  DIR *dirp = opendir(s);
  if (dirp == NULL) {
    fatal("Can't open %s: %s", s, syserr());
  }
  chdir(s);
  while ((dp = readdir(dirp)) != NULL) {
    std::string nm = std::string(dp->d_name);
    if (nm[0] == '.') {
      continue;
    }
    Assert(dp->d_type == DT_REG);
    names.push_back(nm);
    
  }
  std::sort(names.begin(), names.end());
  chdir("..");
  closedir(dirp);
  return names;
}


template <class T> T** make2DPointerArray(int width, int height){
  T** row_pointers = (T**)andyMalloc(height*sizeof(T*));
  for(int y=0; y<height; y++){
    row_pointers[y] = (T*)andyMalloc(width*sizeof(T));
  }
  return row_pointers;
}

template <class T> void free2DPointerArray(T** row_pointers, int height){
  for(int y=0; y<height; y++){
    free(row_pointers[y]);
  }
  free(row_pointers);
}

template unsigned char** make2DPointerArray<unsigned char>(int, int);
template unsigned char*** make2DPointerArray<unsigned char*>(int, int);
template bool** make2DPointerArray<bool>(int, int);
template double** make2DPointerArray<double>(int, int);
template int** make2DPointerArray<int>(int, int);

template void free2DPointerArray<bool>(bool**, int);
template void free2DPointerArray<double>(double**, int);
template void free2DPointerArray<unsigned char>(unsigned char**, int);
template void free2DPointerArray<unsigned char*>(unsigned char***, int);
template void free2DPointerArray<int>(int **, int);


