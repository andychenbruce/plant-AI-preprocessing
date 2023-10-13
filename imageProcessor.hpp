
class plantImage {
public:
  uint8_t** RGB_row_pointers;
  uint8_t** HSV_row_pointers;
  
  uint8_t ***hsvData;//[][][3]
  uint8_t ***rgbData;//[][][3]
  bool **mask;
  int **floodedMask;
  std::vector<Chunk> chunkInfo;
  std::vector<Chunk> badChunkInfo;

  uint32_t width;
  uint32_t height;
  

  double **depthMapOffset;
  double **diffFromMean;

  uint8_t **blockMatchOutput_row_pointers;
  uint8_t ***blockMatchOutputData;//[][][3]

  double **depthMap;
  uint8_t **matchesForPixel;
  //plantImage(const plantImage&);
  //bool operator=(const plantImage&);
  
  void convertRgbImageToHsv(void);
  void convertHsvImageToRgb(void);


  
  void normalizeImage(bool doSat, bool doVal, bool ignoreMask);
  void normalizeBlock(uint32_t xx, uint32_t yy, uint32_t imgAvgSat, uint32_t imgAvgVal, uint32_t blkAvgSat, uint32_t blkAvgVal, bool doSat, bool doVal, bool ignoreMask);

  void getBlockAverageSatVal(int xx, int yy, uint32_t *satP, uint32_t *valP, uint32_t *pixelCountP, bool ignoreMask);

  void getImageAverageSatVal(uint32_t *satP, uint32_t *valP, bool ignoreMask);



  void drawBox(uint32_t x, uint32_t y, uint32_t bsz, uint32_t c)  __attribute__((unused));
  
  void dLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t c);
  void hLine(uint32_t x, uint32_t y, uint32_t len, uint32_t c);
  void vLine(uint32_t x, uint32_t y, uint32_t len, uint32_t c);

  void hLineHSV(uint32_t x, uint32_t y, uint32_t len, uint32_t hue);
  void vLineHSV(uint32_t x, uint32_t y, uint32_t len, uint32_t hue);

  void drawRectangleHSV(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t hue) __attribute__((unused));

  
  void setHsvPixel(uint32_t x, uint32_t y, uint8_t hue, uint8_t sat, uint8_t val);
  void setRgbPixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue);


  double getMatchScore(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
  void matchBlockLeftToRight(uint32_t x0, uint32_t y0);
  void matchBlockRightToLeft(uint32_t x0, uint32_t y0);
  void saveMapToFile(void);
  void getMapFromFile(void);
  void doBlockMatch(void);

  void thresholdHSV(void);
  void doFloodingUsingMask(void);
  int hLineNumChunkPixles(int x, int y, int len, int chunkNum);
  int vLineNumChunkPixels(int x, int y, int len, int chunkNum);
  void splitChunkInHalf(int chunkIndex, bool horizontalSplit, int newFloodValue);
  void floodFromThisPixel(int x, int y, int chunkNum);
  void refloodFromThisPixelInRange(int x, int y, int x0, int y0, int x1, int y1, int oldFloodValue, int newFloodValue);
  int refloodChunk(int chunkIndex, int startingNewFloodValue);
  void calculateChunks(int numChunks);
  int splitChunk(int chunkIndexToSplit, int startingNewChunkValue, bool horizontalSplit);
  void drawChunkRectangles(void);
  void destroyBadChunks(void);

  
  plantImage(void);
  plantImage(uint8_t** RGB_row_pointers_, int width_, int height_);
};
