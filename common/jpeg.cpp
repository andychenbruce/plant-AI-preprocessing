#include <assert.h>
#include <stdio.h>
#include <jpeglib.h>

#include "utils.hpp"

unsigned char**
readJpeg(const char* filename, uint32_t* width, uint32_t* height)
{
  int channels;
  struct jpeg_decompress_struct info;
  struct jpeg_error_mgr err;

  FILE* fp = fopen(filename, "rb");
  if(fp==NULL) {
    fatal("Cannot open %s: %s\n", filename, syserr());
  }

  info.err = jpeg_std_error(&err);     
  jpeg_create_decompress(&info);
  
  jpeg_stdio_src(&info, fp);    
  jpeg_read_header(&info, TRUE);

  jpeg_start_decompress(&info);

  *width = info.output_width;
  *height = info.output_height;
  channels = info.num_components;
  switch(channels){
  case 3:
    //good
    break;
  case 4:
    fatal("program does not support 4 channel jpegs\n");
    break;
  default:
    fatal("Unknown jpeg channel number: %d\n", channels);
    break;
  }

  assert(channels == 3);

  unsigned char** row_pointers = (unsigned char**) andyMalloc(*height*sizeof(unsigned char*));
  int bytesPerRow = channels*(*width);
  for(uint32_t y=0; y<*height; y++){
    row_pointers[y] = (unsigned char*)andyMalloc(bytesPerRow);
  }

  
  while(info.output_scanline < info.output_height)
  {
    jpeg_read_scanlines(&info, &row_pointers[info.output_scanline], 1);
  }
  jpeg_finish_decompress(&info);
  jpeg_destroy_decompress(&info);

  if(fclose(fp)){
    fatal("Cannot close %s: %s\n", filename, syserr());
  }

  return row_pointers;
}


/*
UInt8 *
readJpeg(const char *fn, UInt8 *rgbData, uint32_t *widthP, uint32_t *heightP)
{
  Assert(widthP != NULL);
  Assert(heightP != NULL);
  
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  uint32_t sz;
  const UInt8 *p = Readfile(fn, &sz);
  cinfo.err = jpeg_std_error(&jerr);	
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, p, sz);
  int rc = jpeg_read_header(&cinfo, TRUE);
  if (rc != 1) {
    fatal("%s is not a normal JPEG", fn);
  }
  jpeg_start_decompress(&cinfo);
  uint32_t width = cinfo.output_width;
  uint32_t height = cinfo.output_height;
  uint32_t pixelSize = cinfo.output_components;
  Assert(pixelSize == 3);
  int rgbSize = width * height * pixelSize;

//    epf("width=%d, height=%d, pixelSize=%d",
//      cinfo.output_width,
//      cinfo.output_height,
//      cinfo.output_components);
//    Assert(cinfo.output_width == ImageWidth);
//    Assert(cinfo.output_height == ImageHeight);
//    Assert(rgbSize == sizeof(pixelData));

  if (rgbData == NULL) {
    rgbData = (UInt8 *) andyMalloc(rgbSize);
    *widthP = width;
    *heightP = height;
  } else {
    Assert(width == *widthP);//if messes up main.cpp has the wrong size
    Assert(height == *heightP);
  }
  int rowStride = width * pixelSize;
  while (cinfo.output_scanline < cinfo.output_height) {
    unsigned char *buffer_array[1];
    buffer_array[0] = rgbData + (cinfo.output_scanline) * rowStride;
    jpeg_read_scanlines(&cinfo, buffer_array, 1);
  }
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free((void *) p);
  return rgbData;
}
*/


void
writeJpeg(const char* filename, unsigned char** row_pointer, uint32_t width, uint32_t height) {
  printf("writing jpeg size %d, %d\n", width, height);
  struct jpeg_compress_struct info;
  struct jpeg_error_mgr err;

  //unsigned char* lpRowBuffer[1];
  FILE *fp = fopen(filename, "wb");

  if(fp == NULL){
    fatal("Cannot open %s: %s\n", filename, syserr());
  }

  
  info.err = jpeg_std_error(&err);
  jpeg_create_compress(&info);

  
  
  jpeg_stdio_dest(&info, fp);

  info.image_width = width;
  info.image_height = height;
  info.input_components = 3;
  info.in_color_space = JCS_RGB;

  
  jpeg_set_defaults(&info);
  
  jpeg_set_quality(&info, 100, TRUE);

  
  jpeg_start_compress(&info, TRUE);

  while(info.next_scanline < info.image_height) {
    jpeg_write_scanlines(&info, row_pointer, 1);
    row_pointer++;
  }

  jpeg_finish_compress(&info);
  if(fclose(fp)){
    fatal("Cannot close %s: %s\n", filename, syserr());
  }

  jpeg_destroy_compress(&info);
}
