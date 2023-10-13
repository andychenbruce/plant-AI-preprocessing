
class Chunk {
public:
  static int maxChunkSize;
  int x;
  int y;
  int w;
  int h;
  int floodValue;
  int pixelCount;
  Chunk(int _x, int _y, int _w, int _h, int _floodValue, int _pixelCount);
};
