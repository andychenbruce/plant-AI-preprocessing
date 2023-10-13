void writePnmHeader(int fd, uint32_t width, uint32_t height);

void writePnmToStdout(uint8_t* rgbPixels, uint32_t width, uint32_t height);

void writePpmToFile(uint8_t *rgbPixels, uint32_t width, uint32_t height, const char *filename);
