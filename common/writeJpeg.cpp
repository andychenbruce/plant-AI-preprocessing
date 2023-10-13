//
//  writeJpeg.cpp
//

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <jpeglib.h>

#define IMG_WIDTH   90
#define IMG_HEIGHT  90

static const char fn[] = "rgb.jpg";

int
main(void)
{
  unsigned char buffer[90][90][3];
  unsigned char *p = &buffer[0][0][0];
  for (int i = 0; i < 30; ++i) {
    for (int j = 0; j < 90; ++j) {
      buffer[i][j][0] = 0xff;
      buffer[i][j][1] = 0;
      buffer[i][j][2] = 0;
    }
  }
  for (int i = 30; i < 60; ++i) {
    for (int j = 0; j < 90; ++j) {
      buffer[i][j][0] = 0;
      buffer[i][j][1] = 0xff;
      buffer[i][j][2] = 0;
    }
  }
  for (int i = 60; i < 90; ++i) {
    for (int j = 0; j < 90; ++j) {
      buffer[i][j][0] = 0;
      buffer[i][j][1] = 0;
      buffer[i][j][2] = 0xff;
    }
  }
  
  FILE *outfile = fopen(fn, "wb");
  if (outfile == NULL) {
    fprintf(stderr, "Can't open %s: %s\n", fn, strerror(errno));
  }
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, outfile);
  cinfo.image_width      = IMG_WIDTH;
  cinfo.image_height     = IMG_HEIGHT;
  cinfo.input_components = 3;
  cinfo.in_color_space   = JCS_RGB;
  
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality (&cinfo, 75, 1); // set the quality [0..100]
  jpeg_start_compress(&cinfo, 1);
  JSAMPROW row_pointer;  // pointer to a single row
  printf("foobar 1\n");
  while (cinfo.next_scanline < IMG_HEIGHT) {
    int n = 3 * IMG_WIDTH * cinfo.next_scanline;
    printf("n=%d\n", n);
    row_pointer = (JSAMPROW) &p[n];
    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }
  printf("foobar 2\n");
  jpeg_finish_compress(&cinfo);
  fclose(outfile);
  return 0;
}
