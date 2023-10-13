#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <assert.h>
#include <iostream>

#include "chunk.hpp"

#include "common/utils.hpp"
#include "imageProcessor.hpp"
#include "common/hsv.hpp"

static const uint32_t BlockSize =  20;
static const uint32_t VerticalSearchWindow = 30;
static const uint32_t HorizontalSearchWindow = 30;
static const uint32_t RightShift = 550;
static const uint32_t DownShift =  48;

static const int MinPixelsWorthSaving = 1000;

plantImage::plantImage(void):
  RGB_row_pointers(nullptr),
  HSV_row_pointers(nullptr),
  hsvData(nullptr),
  rgbData(nullptr),
  mask(nullptr),
  floodedMask(nullptr),
  chunkInfo(),
  badChunkInfo(),
  width(0),
  height(0),
  depthMapOffset(nullptr),
  diffFromMean(nullptr),
  blockMatchOutput_row_pointers(nullptr),
  blockMatchOutputData(nullptr),
  depthMap(nullptr),
  matchesForPixel(nullptr){
  
}

const int numChannels = 3;
plantImage::plantImage(uint8_t** _RGB_row_pointers, int _width, int _height):
  RGB_row_pointers(_RGB_row_pointers),
  HSV_row_pointers((uint8_t**)make2DPointerArray<uint8_t>(_width*numChannels, _height)),
  hsvData(nullptr),
  rgbData(nullptr),
  mask(nullptr),
  floodedMask(nullptr),
  chunkInfo(),
  badChunkInfo(),
  width(_width),
  height(_height),
  depthMapOffset(nullptr),
  diffFromMean(nullptr),
  blockMatchOutput_row_pointers(nullptr),
  blockMatchOutputData(nullptr),
  depthMap(nullptr),
  matchesForPixel(nullptr)
{
  
  
  
  HSV_row_pointers = 
  blockMatchOutput_row_pointers = (uint8_t**)make2DPointerArray<uint8_t>(width*numChannels, height);
  
  printf("making new image w h = %d, %d\n", width, height);
  
  

  
  rgbData = (uint8_t***)make2DPointerArray<uint8_t*>(width, height);
  for(uint32_t y = 0; y < height; y++){
    for(uint32_t x = 0; x < width; x++){
      rgbData[y][x] = &(_RGB_row_pointers[y][x*numChannels]);
    }
  }
  
  hsvData = (uint8_t***)make2DPointerArray<uint8_t*>(width, height);
  for(uint32_t y = 0; y < height; y++){
    for(uint32_t x = 0; x < width; x++){
      hsvData[y][x] = &(HSV_row_pointers[y][x*numChannels]);
    }
  }

  blockMatchOutputData = (uint8_t***)make2DPointerArray<uint8_t*>(width, height);
  for(uint32_t y = 0; y < height; y++){
    for(uint32_t x = 0; x < width; x++){
      blockMatchOutputData[y][x] = &(blockMatchOutput_row_pointers[y][x*numChannels]);
    }
  }
  
  mask = (bool**)make2DPointerArray<bool>(width, height);
  floodedMask = (int**)make2DPointerArray<int>(width, height);
  
  depthMapOffset = (double**)make2DPointerArray<double>(width, height);
  diffFromMean = (double**)make2DPointerArray<double>(width, height);

  depthMap = (double**)make2DPointerArray<double>(width, height);

  matchesForPixel = (uint8_t**)make2DPointerArray<uint8_t>(width, height);
}

void
plantImage::convertRgbImageToHsv(void)
{
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      uint8_t red = rgbData[y][x][0];
      uint8_t grn = rgbData[y][x][1];
      uint8_t blu = rgbData[y][x][2];
      uint32_t hsv = rgb2hsv(red, grn, blu);
      uint8_t hue = (hsv >> 16) & 0xff;
      uint8_t sat = (hsv >>  8) & 0xff;
      uint8_t val = (hsv >>  0) & 0xff;
      setHsvPixel(x, y, hue, sat, val);
    }
  }
}

void
plantImage::convertHsvImageToRgb(void)
{
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      uint8_t hue = hsvData[y][x][0];
      uint8_t sat = hsvData[y][x][1];
      uint8_t val = hsvData[y][x][2];
      uint32_t rgb = hsv2rgb(hue, sat, val);
      uint8_t red = (rgb >> 16) & 0xff;
      uint8_t grn = (rgb >>  8) & 0xff;
      uint8_t blu = (rgb >>  0) & 0xff;
      setRgbPixel(x, y, red, grn, blu);
    }
  }
}



void
plantImage::getImageAverageSatVal(uint32_t *satP, uint32_t *valP, bool ignoreMask)
{
  uint32_t valSum = 0;
  uint32_t satSum = 0;
  int pixelCount = 0;
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      if (mask[y][x] || ignoreMask){
	pixelCount++;
	satSum += hsvData[y][x][1];
	valSum += hsvData[y][x][2];
      }
    }
  }
  Assert(pixelCount > 0);
  *satP = satSum/pixelCount;
  *valP = valSum/pixelCount;
}

void
plantImage::getBlockAverageSatVal(int xx, int yy, uint32_t *satP, uint32_t *valP, uint32_t *pixelCountP, bool ignoreMask)
{
  uint32_t NormHorizontalBlocks = 16;
  uint32_t NormVerticalBlocks   = 16;
  uint32_t NormXPixelsPerBlock = width / NormHorizontalBlocks;
  uint32_t NormYPixelsPerBlock = height / NormVerticalBlocks;

  
  uint32_t valSum = 0;
  uint32_t satSum = 0;
  int pixelCount = 0;
  uint32_t xEnd = xx + NormXPixelsPerBlock;
  uint32_t yEnd = yy + NormYPixelsPerBlock;
  for (uint32_t y = yy; y < yEnd; ++y) {
    for (uint32_t x = 0; x < xEnd; ++x) {
      if((x<width)&&(y<height)){
	if (mask[y][x] || ignoreMask){
	  pixelCount++;
	  satSum += hsvData[y][x][1];
	  valSum += hsvData[y][x][2];
	}
      }
    }
  }
  *pixelCountP = pixelCount;
  if (pixelCount == 0) {
    *satP = 0;
    *valP = 0;
  } else {
    *satP = satSum/pixelCount;
    *valP = valSum/pixelCount;
  }
}

void
plantImage::normalizeBlock(uint32_t xx, uint32_t yy, uint32_t imgAvgSat, uint32_t imgAvgVal, uint32_t blkAvgSat, uint32_t blkAvgVal, bool doSat, bool doVal, bool ignoreMask)
{
  uint32_t NormHorizontalBlocks = 16;
  uint32_t NormVerticalBlocks   = 16;
  uint32_t NormXPixelsPerBlock = width / NormHorizontalBlocks;
  uint32_t NormYPixelsPerBlock = height / NormVerticalBlocks;

  
  uint32_t xEnd = xx + NormXPixelsPerBlock;
  uint32_t yEnd = yy + NormYPixelsPerBlock;
  for (uint32_t y = yy; y < yEnd; y++) {
    for (uint32_t x = xx; x < xEnd; x++) {
      if((x<width)&&(y<height)){
	if (mask[y][x] || ignoreMask){
	  if(doSat){
	    uint32_t s = (hsvData[y][x][1] * imgAvgSat) / blkAvgSat;
	    hsvData[y][x][1] = s;
	    s = (s > 255) ? 255 : s;
	  }
	  if(doVal){
	    uint32_t v = (hsvData[y][x][2] * imgAvgVal) / blkAvgVal;
	    v = (v > 255) ? 255 : v;
	    hsvData[y][x][2] = v;
	  }
	}
      }
    }
  }
}

void
plantImage::normalizeImage(bool doSat, bool doVal, bool ignoreMask)
{
  uint32_t NormHorizontalBlocks = 16;
  uint32_t NormVerticalBlocks   = 16;
  //uint32_t NormXPixelsPerBlock = width / NormHorizontalBlocks;
  //uint32_t NormYPixelsPerBlock = height / NormVerticalBlocks;

  
  
  uint32_t imgAvgSat;
  uint32_t imgAvgVal;

  getImageAverageSatVal(&imgAvgSat, &imgAvgVal, ignoreMask);
  Assert(imgAvgSat < 256);
  Assert(imgAvgVal < 256);
  
  std::cerr << "image avg sat:" << imgAvgSat << "val:" << imgAvgVal << std::endl; 
  
  uint32_t dy = height / NormVerticalBlocks;
  uint32_t dx = width  / NormHorizontalBlocks;
  
  for (uint32_t y = 0; y < height; y += dy) {
    for (uint32_t x = 0; x < width; x += dx) {
      uint32_t blkAvgVal;
      uint32_t blkAvgSat;
      uint32_t pixelCount;
      getBlockAverageSatVal(x, y, &blkAvgSat, &blkAvgVal, &pixelCount, ignoreMask);
      if(pixelCount > 0){
	normalizeBlock(x, y, imgAvgSat, imgAvgVal, blkAvgSat, blkAvgVal, doSat, doVal, ignoreMask);
      }
    }
  }
}







void
plantImage::setRgbPixel(uint32_t x, uint32_t y, uint8_t red, uint8_t green, uint8_t blue)
{
  Assert(x < width);
  Assert(y < height);
  //Assert(x >= 0);
  //Assert(y >= 0);
  rgbData[y][x][0] = red;
  rgbData[y][x][1] = green;
  rgbData[y][x][2] = blue;
      
}

void
plantImage::setHsvPixel(uint32_t x, uint32_t y, uint8_t hue, uint8_t sat, uint8_t val)
{
  hsvData[y][x][0] = hue;
  hsvData[y][x][1] = sat;
  hsvData[y][x][2] = val;
}


void
plantImage::dLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t c)
{
  uint8_t red = (c >> 16) & 0xff;
  uint8_t grn = (c >>  8) & 0xff;
  uint8_t blu = (c >>  0) & 0xff;

  if (x0 > x1) {
    int tmp;
    tmp = x1;
    x1 = x0;
    x0 = tmp;
    tmp = y1;
    y1 = y0;
    y0 = tmp;
  }
  double slope;
  slope = ((double) y0 - y1);
  slope /= ((double) x0 - x1);
  for(uint32_t i = 0; i < x1-x0; i++){
    uint32_t xPix = x0 + i;
    uint32_t yPix = (int)(y0+(slope*i));
    setRgbPixel(xPix, yPix, red, grn, blu);
    setRgbPixel(xPix, yPix+1, red, grn, blu);
  }
  
}

void
plantImage::hLine(uint32_t x, uint32_t y, uint32_t len, uint32_t c)
{
  uint8_t red = (c >> 16) & 0xff;
  uint8_t grn = (c >>  8) & 0xff;
  uint8_t blu = (c >>  0) & 0xff;
  for (uint32_t i = 0; i < len; ++i) {
    setRgbPixel(x+i, y+0, red, grn, blu);
    setRgbPixel(x+i, y+1, red, grn, blu);
  }
}

void
plantImage::vLine(uint32_t x, uint32_t y, uint32_t len, uint32_t c)
{
  uint8_t red = (c >> 16) & 0xff;
  uint8_t grn = (c >>  8) & 0xff;
  uint8_t blu = (c >>  0) & 0xff;
  for (uint32_t i = 0; i < len; ++i) {
    setRgbPixel(x,   y+i, red, grn, blu);
    setRgbPixel(x+1, y+i, red, grn, blu);
  }
}

void
plantImage::drawBox(uint32_t x, uint32_t y, uint32_t bsz, uint32_t c)
{
  hLine(x,     y,     bsz, c);
  hLine(x,     y+bsz, bsz, c);
  vLine(x,     y,     bsz, c);
  vLine(x+bsz, y,     bsz, c);
}

void
plantImage::hLineHSV(uint32_t x, uint32_t y, uint32_t len, uint32_t hue)
{
  for (uint32_t i = 0; i < len; ++i) {
    setHsvPixel(x+i, y+0, hue, 0xff, 0xff);
    setHsvPixel(x+i, y+1, hue, 0xff, 0xff);
  }
}

void
plantImage::vLineHSV(uint32_t x, uint32_t y, uint32_t len, uint32_t hue)
{
  for (uint32_t i = 0; i < len; ++i) {
    setHsvPixel(x,   y+i, hue, 0xff, 0xff);
    setHsvPixel(x+1, y+i, hue, 0xff, 0xff);
  }
}


void
plantImage::drawRectangleHSV(uint32_t x, uint32_t y, uint32_t dx, uint32_t dy, uint32_t hue)
{
  hLineHSV(x,     y,     dx, hue);
  hLineHSV(x,     y+dy, dx, hue);
  vLineHSV(x,     y,     dy, hue);
  vLineHSV(x+dx,  y,     dy, hue);
}



class LineThing {
public:
  int x0;
  int y0;
  int x1;
  int y1;
  int c;

  LineThing(int _x0, int _y0, int _x1, int _y1, int _c):
    x0(_x0),
    y0(_y0),
    x1(_x1),
    y1(_y1),
    c(_c)
  {}
};

std::vector<LineThing> listOfLines;


double
plantImage::getMatchScore(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
  
  uint32_t score = 0;
  
  uint32_t testX0;
  uint32_t testY0;
  uint32_t testX1;
  uint32_t testY1;
  
  int magenta = (0xff * 300) / 360;  
  
  int pixelsChecked = 0;
  for (uint32_t dy = 0; dy < BlockSize; ++dy) {
    for (uint32_t dx = 0; dx < BlockSize; ++dx) {
      testX0 = x0 + dx;
      testY0 = y0 + dy;
      testX1 = x1 + dx;
      testY1 = y1 + dy;
      
      if((testX0 < width)&&(testX1 < width)&&(testY0 < height)&&(testY1 < height)){
	
	if((abs(hsvData[testY0][testX0][0]-magenta) > 5)&&(abs(hsvData[testY0][testX0][0]-magenta) > 5)){
	
	  score += abs(hsvData[testY0][testX0][0] - hsvData[testY1][testX1][0]);
	  score += abs(hsvData[testY0][testX0][1] - hsvData[testY1][testX1][1]);
	  score += abs(hsvData[testY0][testX0][2] - hsvData[testY1][testX1][2]);
	  pixelsChecked++;
	}
      }
    }
  }
  
  
  if(pixelsChecked > 30){
    return score/pixelsChecked;
  } else {
    return 10000000;
  }
}

void
plantImage::matchBlockLeftToRight(uint32_t x0, uint32_t y0)
{
  double lowestScore = 10000001.0;
  uint32_t x1 = x0;
  uint32_t y1 = y0;
  int triedBlocks = 0;
  //Assert(getMatchScore(x0, y0, x0, y0) == 0);
  for (uint32_t j = 0; j < VerticalSearchWindow; ++j) {
    uint32_t y = y0 + j + DownShift;
    for (uint32_t i = 0; i < HorizontalSearchWindow; ++i) {
      uint32_t x = x0 + i + RightShift;
      uint32_t score = getMatchScore(x0, y0, x, y);
      triedBlocks++;
      if (score < lowestScore) {
	lowestScore = score;
	x1 = x;
	y1 = y;
      }
    }
  }
  
  if(lowestScore != 10000000){

    int dx = x0-x1;
    int dy = y0-y1;
    for(int y = 0; y < (int)BlockSize; y++){
      for(int x = 0; x < (int)BlockSize; x++){
	if((y0+y < height)&&(x0+x<width)){
	  depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    
    dLine(x0, y0, x1, y1, 0xff0000);
    listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void plantImage::matchBlockRightToLeft(uint32_t x0, uint32_t y0)
{
  double lowestScore = 10000001.0;
  uint32_t x1 = x0;
  uint32_t y1 = y0;
  int triedBlocks = 0;
  //Assert(getMatchScore(x0, y0, x0, y0) == 0);
  for (uint32_t j = 0; j < VerticalSearchWindow; ++j) {
    uint32_t y = y0 - j - DownShift;
    for (uint32_t i = 0; i < HorizontalSearchWindow; ++i) {
      uint32_t x = x0 - i - RightShift;
      uint32_t score = getMatchScore(x0, y0, x, y);
      triedBlocks++;
      if (score < lowestScore) {
	lowestScore = score;
	x1 = x;
	y1 = y;
      }
    }
  }
  
  if(lowestScore != 10000000){

    int dx = x0-x1;
    int dy = y0-y1;
    for(int y = 0; y < (int)BlockSize; y++){
      for(int x = 0; x < (int)BlockSize; x++){
	if((y0+y < height)&&(x0+x<width)){
	  depthMap[y0+y][x0+x] += sqrt(dx*dx + dy*dy);
	  matchesForPixel[y0+y][x0+x] += 1;
	}
      }
    }
    //listOfLines.push_back(LineThing(x0, y0, x1, y1, 0xff0000));
  }
}

void
plantImage::saveMapToFile(void){
  int poop;
  const char *fn = "depthThing.dat";
  poop = open(fn, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (poop < 0) {
    fatal("Open failed, %s: %s\n", fn, syserr());
  }
  uint32_t n = sizeof(diffFromMean);
  Assert(n == sizeof(double) * width * height);
  int w = write(poop, diffFromMean, n);
  if (w != (int) n) {
    fatal("Write failed, %s: %s\n", fn, syserr());
  }
  close(poop);
}

void
plantImage::getMapFromFile(void){
  int poop;
  const char *fn = "depthThindat";
  poop = open(fn, O_RDONLY);
  if (poop < 0) {
    fatal("Open failed, %s: %s", fn, syserr());
  }
  uint32_t n = sizeof(depthMapOffset);
  Assert(n == sizeof(double) * width * height);
  int r = read(poop, depthMapOffset, n);
  if (r != (int) n) {
    fatal("Read failed, %s: %s", fn, syserr());
  }
  close(poop);
}

void
plantImage::doBlockMatch(void)
{
  bool saveFile = false;
  bool readFile = false;
  int matchedBlocks = 0;
  
  for (uint32_t y = 0; y < height; y += 5) {
    for (uint32_t x = 0; x < width/2; x += 5) {
      matchBlockLeftToRight(x, y);
      matchedBlocks++;
    }
    std::cerr << "y = " << y << std::endl;
  }
  std::cerr << "other thing" << std::endl;
  for (uint32_t y = 0; y < height; y += 5) {
    for (uint32_t x = width/2; x < width; x += 5) {
      matchBlockRightToLeft(x, y);
      matchedBlocks++;
    }
    std::cerr << "y = " << y <<std::endl;
  }
  
  std::cerr << "mathed " << matchedBlocks << " blocks" << std::endl;

  
  /*
  for(int i = 0; i < (int)listOfLines.size(); i+=10){
    LineThing *poop = &listOfLines[i];
    //dLine(poop->x0, poop->y0, poop->x1, poop->y1, poop->c);
    //dLine(poop->x0, poop->y0, poop->x1, poop->y1, 0x00f00f*i);
  }
  */
  
  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	depthMap[y][x] /= matchesForPixel[y][x];
      }else{
	depthMap[y][x] = 0;
      }
    }
  }

  double meanDepthLen = 0;
  int totalPixlesAdded = 0;
  
  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	totalPixlesAdded++;
	meanDepthLen += depthMap[y][x];
      }
    }
  }
  meanDepthLen /= totalPixlesAdded;
  if(saveFile){
    for(int y = 0; y < (int)height; y++){
      for(int x = 0; x < (int)width; x++){
	diffFromMean[y][x] = depthMap[y][x]-meanDepthLen;
      }
    }
    saveMapToFile();
  }
  
  if(readFile){
    getMapFromFile();
    for(int y = 0; y < (int)height; y++){
      for(int x = 0; x < (int)width; x++){
	depthMap[y][x] -= depthMapOffset[y][x];
      }
    }
  }
  
  
  double maxValue = 0;
  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	if(depthMap[y][x] > maxValue){
	  maxValue = depthMap[y][x];
	}
      }
    }
  }

  for(int y = 0; y < (int)height; y++){
    for(int x = 0; x < (int)width; x++){
      if(matchesForPixel[y][x] != 0){
	blockMatchOutputData[y][x][0] = 0;
	blockMatchOutputData[y][x][1] = 0;
	blockMatchOutputData[y][x][2] = 0;
	double val = depthMap[y][x];
	
	val -= meanDepthLen;
	val *= -1;
	val *= 40;
	std::cerr << "val = " << val << std::endl;
	if(val > 0){
	  if(val > 255){
	    val = 255;
	  }
	  blockMatchOutputData[y][x][0] = (int)(val);
	}else{
	  val *= -1;
	  if(val > 255){
	    val = 255;
	  }
	  blockMatchOutputData[y][x][2] = (int)(val);
	}
	
      }else{
	blockMatchOutputData[y][x][0] = 0;
	blockMatchOutputData[y][x][1] = 255;
	blockMatchOutputData[y][x][2] = 0;
      }
    }
  }
  std::cerr << "depth map generated" << std::endl;
}







int
plantImage::hLineNumChunkPixles(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(x+len-1 < (int)width);
  for(int dx = 0; dx < len; dx++) {
    if(floodedMask[y][x+dx] == chunkNum) {
      score++;
    }
  }
  return score;
}

int
plantImage::vLineNumChunkPixels(int x, int y, int len, int chunkNum)
{
  int score = 0;
  Assert(y+len-1 < (int)height);
  for(int dy = 0; dy < len; dy++) {
    if(floodedMask[y+dy][x] == chunkNum) {
      score++;
    }
  }
  return score;
}

void
plantImage::splitChunkInHalf(int chunkIndex, bool horizontalSplit, int newFloodValue)
{
  
  int n = chunkInfo.size();
  assert(chunkIndex <= n);
  int floodValue = chunkInfo[chunkIndex].floodValue;
  int chunkX = chunkInfo[chunkIndex].x;
  int chunkY = chunkInfo[chunkIndex].y;
  int chunkW = chunkInfo[chunkIndex].w;
  int chunkH = chunkInfo[chunkIndex].h;
  
  int halfPixels = chunkInfo[chunkIndex].pixelCount/2;
  
  if (horizontalSplit) {
    int bestHLinePosY = 0;
    int totalPixels = 0;
    int topHalfPixels = 0;
    int bottomHalfPixels = 0;
    for(int y = chunkY; y < chunkY + chunkH; y++) {
      int x = chunkX;
      totalPixels += hLineNumChunkPixles(x, y, chunkW, floodValue);
      if (totalPixels >= halfPixels) {//keep moving line down untill more than half of pixels counted
	bestHLinePosY = y;
	break;
      }else{
	topHalfPixels = totalPixels;
      }
    }
    bottomHalfPixels = chunkInfo[chunkIndex].pixelCount - topHalfPixels;
    
    
    chunkInfo[chunkIndex].h = bestHLinePosY-chunkY+1;
    chunkInfo[chunkIndex].pixelCount = topHalfPixels;

    int newX = chunkX;
    int newY = bestHLinePosY+1;
    int newW = chunkW;
    int newH = (chunkY+chunkH)-(newY);
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++){
      for(int x = newX; x < newX+newW; x++){
	if (floodedMask[y][x] == floodValue){
	  floodedMask[y][x] = newFloodValue;
	}
      }
    }
  } else {
    int bestVLinePosX = 0;
    int totalPixels = 0;
    int topHalfPixels = 0;
    int bottomHalfPixels = 0;
    for(int x = chunkX; x < chunkX + chunkW; x++) {
      int y = chunkY;
      totalPixels += hLineNumChunkPixles(x, y, chunkH, floodValue);
      if (totalPixels >= halfPixels) {//keep moving line right untill more than half of pixels counted
	bestVLinePosX = x;
	break;
      }else{
	topHalfPixels = totalPixels;
      }
    }
    
    
    chunkInfo[chunkIndex].w = bestVLinePosX-chunkX;
    chunkInfo[chunkIndex].pixelCount = topHalfPixels;
    
    int newX = bestVLinePosX;
    int newY = chunkY;
    int newW = (chunkX+chunkW)-bestVLinePosX;
    int newH = chunkH;
    Chunk ck(newX, newY, newW, newH, newFloodValue, bottomHalfPixels);
    chunkInfo.push_back(ck);
    for(int y = newY; y < newY+newH; y++) {
      for(int x = newX; x < newX+newW; x++) {
	if (floodedMask[y][x] == floodValue) {
	  floodedMask[y][x] = newFloodValue;
	}
      }
    }
  }
  
}

void
plantImage::floodFromThisPixel(int x, int y, int chunkNum) {
  Assert((x >= 0)&&(x < (int)width)&&(y >= 0)&&(y < (int)height));
  if (mask[y][x]) {
    floodedMask[y][x] = chunkNum;
  }
  
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= 0)&&(testX < (int)width)&&(testY >= 0)&&(testY < (int)height)) {
	  if (floodedMask[testY][testX] == 0) {
	    if (mask[testY][testX]) {
	      floodFromThisPixel(testX, testY, chunkNum);
	    }
	  }
	}
      }
    }
  }
}

void
plantImage::refloodFromThisPixelInRange(int x, int y, int x0, int y0, int x1, int y1, int oldFloodValue, int newFloodValue) {
  Assert((x >= x0)&&(x < x1)&&(y >= y0)&&(y < y1));
  if (floodedMask[y][x] == oldFloodValue) {
    floodedMask[y][x] = newFloodValue;
  }
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (!((dx == 0)&&(dy == 0))) {
	int testX = x + dx;
	int testY = y + dy;
	if ((testX >= x0)&&(testX < x1)&&(testY >= y0)&&(testY < y1)) {
	  if (floodedMask[testY][testX] == oldFloodValue) {
	    assert(mask[testY][testX]);
	    refloodFromThisPixelInRange(testX, testY, x0, y0, x1, y1, oldFloodValue, newFloodValue);
	  }
	}
      }
    }
  }
}

int
plantImage::refloodChunk(int chunkIndex, int startingNewFloodValue) {
  int x0 = chunkInfo[chunkIndex].x;
  int y0 = chunkInfo[chunkIndex].y;
  int x1 = x0 + chunkInfo[chunkIndex].w;
  int y1 = y0 + chunkInfo[chunkIndex].h;
  int oldFloodValue = chunkInfo[chunkIndex].floodValue;
  
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (floodedMask[y][x] == oldFloodValue) {
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, oldFloodValue, startingNewFloodValue);
	startingNewFloodValue++;
      }
    }
  }
  int highestValue = startingNewFloodValue-1;
  bool alreadyDoubleReflooded = false;
  for (int y = y0; y < y1; y++) {
    for (int x = x0; x < x1; x++) {
      if (floodedMask[y][x] == highestValue) {
	assert(!alreadyDoubleReflooded);//should only go once
	alreadyDoubleReflooded = true;
	refloodFromThisPixelInRange(x, y, x0, y0, x1, y1, highestValue, oldFloodValue);
      }
    }
  }
  return highestValue;
}

void
plantImage::calculateChunks(int numChunks) {
  chunkInfo.clear();
  badChunkInfo.clear();
  for (int chunk = 1; chunk < numChunks; chunk++) {
    int minX = 1000000;
    int maxX = -1000000;
    int minY = 1000000;
    int maxY = -1000000;
    int numPix = 0;
    for (int y = 0; y < (int)height; y++) {
      for (int x = 0; x < (int)width; x++) {
	if (floodedMask[y][x] == chunk) {
	  numPix++;
	  if (x < minX) {
	    minX = x;
	  }
	  if (x > maxX) {
	    maxX = x;
	  }
	  if (y < minY) {
	    minY = y;
	  }
	  if (y > maxY) {
	    maxY = y;
	  }
	}
      }
    }
    if(!(numPix > 0)){
      std::cerr << "chunk value " << chunk << " has no pixels\n";
      assert(false);
    }
    int w = maxX-minX+1;
    int h = maxY-minY+1;
    Chunk ck(minX, minY, w, h, chunk, numPix);
    
    if (numPix >= MinPixelsWorthSaving) {
      chunkInfo.push_back(ck);
      if(w*h > ck.maxChunkSize) {
	ck.maxChunkSize = w*h;
      }
    }else{
      badChunkInfo.push_back(ck);
    }
  }
}

int
plantImage::splitChunk(int chunkIndexToSplit, int startingNewChunkValue, bool horizontalSplit) {
  splitChunkInHalf(chunkIndexToSplit, horizontalSplit, startingNewChunkValue);

  startingNewChunkValue++;
  
  startingNewChunkValue = refloodChunk(chunkIndexToSplit, startingNewChunkValue);
  
  startingNewChunkValue = refloodChunk(chunkInfo.size()-1, startingNewChunkValue);
  return startingNewChunkValue;
}

void
plantImage::drawChunkRectangles(void){
  int red     = 0;
  int green   = (0xff * 120) / 360;
  for(int i = 0; i < (int)chunkInfo.size(); i++) {
    int borderColor = green;
    drawRectangleHSV(chunkInfo[i].x, chunkInfo[i].y, chunkInfo[i].w, chunkInfo[i].h, borderColor);
  }
  
  for(int i = 0; i < (int)badChunkInfo.size(); i++) {
    int borderColor = red;
    drawRectangleHSV(badChunkInfo[i].x, badChunkInfo[i].y, badChunkInfo[i].w, badChunkInfo[i].h, borderColor);
  }
  
}

void
plantImage::destroyBadChunks(void){
  int magenta = (0xff * 300) / 360;
  for(int i = 0; i < (int) badChunkInfo.size(); i++){
    Chunk *theChunk= &badChunkInfo[i];
    int floodVal = theChunk->floodValue;
    for(int y = theChunk->y; y < theChunk->y+theChunk->h; y++){
      for(int x = theChunk->x; x < theChunk->x+theChunk->w; x++){
	if(floodedMask[y][x] == floodVal){
	  mask[y][x] = false;
	  floodedMask[y][x] = 0;
	  hsvData[y][x][0] = magenta;
	  hsvData[y][x][1] = 255;
	  hsvData[y][x][2] = 255;
	}
      }
    }
  }
}

void
plantImage::doFloodingUsingMask(){
  for(uint32_t y = 0; y < height; y++){
    memset(floodedMask[y], 0, width*sizeof(int));
  }
  
  int chunkID = 1;
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      if ((floodedMask[y][x] == 0)&&(mask[y][x] == true)){
	floodFromThisPixel(x, y, chunkID);
	chunkID++;
      }
    }
  }
  
  int numChunks = chunkID;
  
  calculateChunks(numChunks);

  int maxSideLength = 400;
  for(int i = 0; i < (int)chunkInfo.size(); i++) {
    int w = chunkInfo[i].w;
    int h = chunkInfo[i].h;
    if (w>h) {
      if (w > maxSideLength) {
	numChunks = splitChunk(i, numChunks, false);
      }
    }else{
      if (h > maxSideLength) {
	numChunks = splitChunk(i, numChunks, true);
      }
    }
  }
  
  const bool colorChunks =false;

  if(colorChunks){
    int thing1   = (0xff * 0)   / 360;
    int thing2   = (0xff * 30)  / 360;
    int thing3   = (0xff * 60)  / 360;
    int thing4   = (0xff * 90)  / 360;
    int thing5   = (0xff * 120) / 360;
    int thing6   = (0xff * 150) / 360;
    int thing7   = (0xff * 180) / 360;
    int thing8   = (0xff * 210) / 360;
    int thing9   = (0xff * 240) / 360;
    int thing10  = (0xff * 270) / 360;
    int colorIndex[] = {thing1, thing2, thing3, thing4, thing5, thing6, thing7, thing8, thing9, thing10};
  
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < width; x++) {
	if(floodedMask[y][x] != 0) {
	  hsvData[y][x][0] = colorIndex[floodedMask[y][x]%(sizeof(colorIndex)/sizeof(colorIndex[0]))];
	  hsvData[y][x][1] = 0xff;
	  hsvData[y][x][2] = 0xff;
	}
      }
    }
  }
  
  calculateChunks(numChunks);
  //destroyBadChunks();
  if(colorChunks){
    drawChunkRectangles();
  }
}




void
plantImage::thresholdHSV(void)
{
  // int red     = 0;
  int green   = (0xff * 120) / 360;
  int magenta = (0xff * 300) / 360;
  for(uint32_t y = 0; y < height; y++){
    memset(mask[y], true, width*sizeof(bool));
  }

  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      int hue = hsvData[y][x][0];
      int sat = hsvData[y][x][1];
      int val = hsvData[y][x][2];
      int dhue = abs(hue - magenta);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (dhue < 30) {
	if (val > 100) {
	  mask[y][x] = false;
	}
      }
      if (val<20) {
	mask[y][x] = false;
      } else {
	if (val < 35) {
	  if (sat < 200) {
	    mask[y][x] = false;
	  }
	} else if (val < 70) {
	  if (sat < 100) {
	    mask[y][x] = false;
	  }
	} else {
	  if (sat < 10) {
	    mask[y][x] = false;
	  }
	}
      }
    }
  }
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      int hue = hsvData[y][x][0];
      // int sat = hsvData[y][x][1];
      int val = hsvData[y][x][2];
      int dhue = abs(hue - green);
      if (abs(dhue - 255) < dhue) {
	dhue = abs(dhue - 255);
      }
      
      if (val > 20) {
	if (dhue > 50) {
	  mask[y][x] = false;
	}
      } else if (val < 150) {
	if (dhue > 60) {
	  mask[y][x] = false;
	}
      } else {
	if (dhue > 70) {
	  mask[y][x] = false;
	}
      }
    }
  }
  
  /*
  for (int i = 0; i < 300; i++) {
    bool tempMask[this->height][this->width];
    memcpy(tempMask, mask, sizeof(tempMask));
    for (uint32_t y = 0; y < height; y++) {
      for (uint32_t x = 0; x < width; x++) {
	if (!mask[y][x]) {
	  int adjacentCount = 0;
	  for (int dy = -1; dy <= 1; dy++) {
	    for (int dx = -1; dx <= 1; dx++) {
	      if (!((dy == 0)&&(dx == 0))) {
		if ((x+dx < width)&&(y+dy < height)){
		  if (mask[y+dy][x+dx]) {
		    adjacentCount++;
		  }
		}
	      }
	    }
	  }
	  int countThreshold = 5;
	  if (i < 2) {
	    countThreshold = 3;
	  } else if (i < 5) {
	    countThreshold = 4;
	  }
	  if (adjacentCount >= countThreshold) {
	    tempMask[y][x] = true;
	  }
	  //}
	}
      }
    }
    memcpy(mask, tempMask, sizeof(mask));
  }
  */


  int numPixlesMasked = 0;
  for (uint32_t y = 0; y < height; ++y) {
    for (uint32_t x = 0; x < width; ++x) {
      if (!mask[y][x]) {
	numPixlesMasked++;
	hsvData[y][x][0] = magenta;
	hsvData[y][x][1] = 0xff;
	hsvData[y][x][2] = 0xff;
      }
    }
  }
  printf("masked %d pixels\n", numPixlesMasked);
}
