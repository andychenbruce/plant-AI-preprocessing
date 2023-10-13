#include <stdio.h>
#include "hsv.hpp"



inline uint32_t R3(uint8_t a, uint8_t b, uint8_t c)
{
  return (a << 16) | (b << 8) | c;
}

uint32_t
hsv2rgb(uint8_t hue, uint8_t sat, uint8_t v)
{
  if (sat == 0) {
    return R3(v, v, v);
  }
  uint8_t region = hue / 43;
  uint8_t remainder = (hue - (region * 43)) * 6;
  uint8_t p = (v * (255 - sat)) >> 8;
  uint8_t q = (v * (255 - ((sat * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;
  switch (region) {
  case 0: return R3(v, t, p);
  case 1: return R3(q, v, p);
  case 2: return R3(p, v, t);
  case 3: return R3(p, q, v);
  case 4: return R3(t, p, v);
  default:return R3(v, p, q);
  }
}

uint32_t
hsv2rgb(uint32_t hsv)
{
  uint8_t hue = (hsv >> 16) & 0xff;
  uint8_t sat = (hsv >>  8) & 0xff;
  uint8_t v   = (hsv >>  0) & 0xff;
  return hsv2rgb(hue, sat, v);
}

uint32_t
rgb2hsv(uint8_t red, uint8_t grn, uint8_t blu)
{
  uint8_t rgbMin = (red < grn) ?
    (red < blu ? red : blu) :
    (grn < blu ? grn : blu);
  uint8_t rgbMax = (red > grn) ?
    (red > blu ? red : blu) :
    (grn > blu ? grn : blu);
  uint8_t hue = 0;
  uint8_t val = rgbMax;
  if (val == 0) {
    return 0;
  }
  uint8_t sat = 255 * long(rgbMax - rgbMin) / val;
  if (sat == 0) {
    return R3(hue, sat, val);
  }
  if (rgbMax == red) {
    hue = 0 + 43 * (grn - blu) / (rgbMax - rgbMin);
  } else if (rgbMax == grn) {
    hue = 85 + 43 * (blu - red) / (rgbMax - rgbMin);
  } else {
    hue = 171 + 43 * (red - grn) / (rgbMax - rgbMin);
  }
  return R3(hue, sat, val);
}

uint32_t
rgb2hsv(uint32_t rgb)
{
  uint8_t red = (rgb >> 16) & 0xff;
  uint8_t grn = (rgb >>  8) & 0xff;
  uint8_t blu = (rgb >>  0) & 0xff;
  return rgb2hsv(red, grn, blu);
}
