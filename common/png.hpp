#include "utils.hpp"

void
writePng(const char* filename, unsigned char** row_pointers, int width, int height, unsigned char bit_depth, unsigned char color_type);

unsigned char**
readPng(const char *filename, int* width, int* height, unsigned char* bit_depth, unsigned char* color_type);
  
