#include <NeoPixelBrightnessBus.h>

const int max_intensity = 255;  //  Max intensity

//  Color definitions
// RgbColor red(max_intensity, 0, 0);
// RgbColor green(0, max_intensity, 0);
RgbColor pink(255, 20, 30);
// RgbColor blue(0, 0, max_intensity);
RgbColor cyan(0, max_intensity, max_intensity);
RgbColor purple(200, 0, max_intensity);
RgbColor yellow(max_intensity, 200, 0);
RgbColor white(max_intensity, max_intensity, max_intensity);
// RgbColor orange(max_intensity, 50, 0);
RgbColor black(0, 0, 0);

RgbColor colors[] = {
  // red,
  // orange,
  pink,
  // green,
  cyan,
  // blue,
  purple,
  yellow,
  white,
  black
};