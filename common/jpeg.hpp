unsigned char**
readJpeg(const char* filename, uint32_t* width, uint32_t* height);

void
writeJpeg(const char* filename, unsigned char** row_pointer, uint32_t width, uint32_t height);
