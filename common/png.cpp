#include <assert.h>
#include <png.h>
#include "png.hpp"
#include "utils.hpp"

void
createPngStructuresREAD(png_structp* png_ptr, png_infop* info_ptr){
  *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(*png_ptr == NULL){
    fatal("png_create_read_struct failed\n");
  }
  *info_ptr = png_create_info_struct(*png_ptr);
  if(*info_ptr == NULL){
    fatal("png_create_info_struct failed\n");
  }
}

void
createPngStructuresWRITE(png_structp* png_ptr, png_infop* info_ptr){
  *png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(*png_ptr == NULL){
    fatal("png_create_write_struct failed\n");
  }
  *info_ptr = png_create_info_struct(*png_ptr);
  if(*info_ptr == NULL){
    fatal("png_create_info_struct failed\n");
  }
}


/*
void
writePng(const char *filename, UInt8 *buf, UInt imgWidth, UInt imgHeight)
{
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    fatal("Could not open file %s for writing: %s", filename, syserr());
  }
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fatal("Could not allocate write struct: %s", syserr());
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fatal("Could not allocate info struct: %s", syserr());
  }
  // Setup Exception handling
  // if (setjmp(png_jmpbuf(png_ptr))) {
  // fatal("Error during png creation\n");
  // }
  png_init_io(png_ptr, fp);
  // Write header (8 bit colour depth)
  png_set_IHDR(png_ptr, info_ptr, imgWidth, imgHeight,
	       8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  for (UInt y = 0 ; y < imgHeight ; ++y) {
    png_write_row(png_ptr, (png_bytep) &buf[y*3*imgWidth]);
  }
  png_write_end(png_ptr, NULL);
  fclose(fp);
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  return;
}
*/
/*
void
writePng(const char* filename, png_bytep* row_pointers, int width, int height, png_byte bit_depth, png_byte color_type)
{
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    fatal("Could not open file %s for writing: %s", filename, syserr());
  }
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fatal("Could not allocate write struct: %s", syserr());
  }
  printf("oooooooo\n");
  png_infop info_ptr = png_create_info_struct(png_ptr);
  printf("eeeeeeeeeee\n");
  if (info_ptr == NULL) {
    fatal("Could not allocate info struct: %s", syserr());
  }
  // Setup Exception handling
  // if (setjmp(png_jmpbuf(png_ptr))) {
  // fatal("Error during png creation\n");
  // }
  png_init_io(png_ptr, fp);
  // Write header (8 bit colour depth)
  png_set_IHDR(png_ptr, info_ptr, width, height,
	       8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  for (UInt y = 0 ; y < height ; ++y) {
    png_write_row(png_ptr, row_pointers[y]);
  }
  png_write_end(png_ptr, NULL);
  fclose(fp);
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  return;
}
*/

void
writePng(const char* filename, png_bytep* row_pointers, int width, int height, png_byte bit_depth, png_byte color_type)
{
  assert(color_type == 2);
  assert(bit_depth == 8);
  FILE *fp = fopen(filename, "wb");
  if (fp==NULL){
    fatal("Cannot open %s: %s\n", filename, syserr());
  }
  
  png_structp png_ptr;
  png_infop info_ptr;

  createPngStructuresWRITE(&png_ptr, &info_ptr);

  if(setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during init_io");
  }

  
  png_init_io(png_ptr, fp);
  

  if(setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during writing header");
  }
  
  png_set_IHDR(png_ptr, info_ptr, width, height,
	       bit_depth, color_type, PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  
  png_write_info(png_ptr, info_ptr);

  if (setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during writing bytes");
  }

  
  png_write_image(png_ptr, row_pointers);
  
  if(setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during end of write");
  }
  png_write_end(png_ptr, NULL);
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
  if(fclose(fp)){
    fatal("Cannot close %s: %s\n", filename, syserr());
  }
  printf("png saving done\n");
}

const int headerCheckBytes = 8;

bool
fileIsPng(FILE* fp){
  char header[8];
  fread(header, 1, headerCheckBytes, fp);
  int bytesDifferent = png_sig_cmp((png_const_bytep)header, 0, headerCheckBytes);
  return (bytesDifferent == 0);
}




unsigned char **
readPng(const char *filename, int* width, int* height, unsigned char* bit_depth, unsigned char* color_type)
{
  FILE *fp = fopen(filename, "rb");
  if(fp == NULL){
    fatal("Cannot open %s: %s\n", filename, syserr());
  }
  
  if(!fileIsPng(fp)){
    fatal("File %s is not recognized as a PNG file\n", filename);
  }
  
  png_structp png_ptr;
  png_infop info_ptr;
  createPngStructuresREAD(&png_ptr, &info_ptr);
  
  if(setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during init_io\n");
  }
  
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, headerCheckBytes);

  if(setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during reading info\n");
  }
  png_read_info(png_ptr, info_ptr);
  
  *width = png_get_image_width(png_ptr, info_ptr);
  *height = png_get_image_height(png_ptr, info_ptr);
  *color_type = png_get_color_type(png_ptr, info_ptr);
  *bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    
  //int number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
  
  if(setjmp(png_jmpbuf(png_ptr))){
    fatal("Error during read_image");
  }

  
  int bytesPerRow = png_get_rowbytes(png_ptr,info_ptr);

  assert(*color_type == 2);//RGB
  assert(*bit_depth == 8);
  assert((3*((*bit_depth)/8)*(*width)) == bytesPerRow);

  png_bytep* row_pointers = make2DPointerArray<unsigned char>((*width)*3, *height);

  
  png_read_image(png_ptr, row_pointers);
  if(fclose(fp)){
    fatal("Cannot close %s: %s\n", filename, syserr());
  }
  
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  
  return row_pointers;
}
