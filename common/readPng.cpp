#include <opencv2/opencv.hpp> 

uint8_t *
readPng(const char *fn, uint8_t *rgbData, uint32_t *width, uint32_t *height)
{
  cv::Mat img = cv::imread(fn);
  int w = img.cols;
  int h = img.rows;
  *width = w;
  *height = h;

  int nbytes = w*h*3;
  rgbData = (uint8_t *) malloc(nbytes);//unsafe
  uint8_t *rp = rgbData;

  for(int y = 0; y < h; y++){
    for(int x = 0; x < w; x++){;
      *rp++ = img.at<cv::Vec3b>(y,x)[2];//reversed since bgr
      *rp++ = img.at<cv::Vec3b>(y,x)[1];
      *rp++ = img.at<cv::Vec3b>(y,x)[0];
    }
  }

  return rgbData;
}
