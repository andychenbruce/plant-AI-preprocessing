
#include "utils.hpp"
#include "pnm.hpp"
#include <unistd.h>
#include <fcntl.h>

void
writePnmHeader(int fd, uint32_t width, uint32_t height)
{
  char hdr[64];
  sprintf(hdr, "P6\n%d %d\n255\n", width, height);
  write(fd, hdr, strlen(hdr));
}

void
writePnmToStdout(uint8_t* rgbPixels, uint32_t width, uint32_t height)
{
  writePnmHeader(1, width, height);
  write(1, (char*)rgbPixels, width * height * 3);
}

void
writePpmToFile(uint8_t *rgbPixels, uint32_t width, uint32_t height, const char *filename)
{
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (fd < 0) {
    fatal("Cannot open %s: %s", filename, syserr());
  }
  writePnmHeader(fd, width, height);
  write(fd, rgbPixels, width * height * 3);
  close(fd);
}
