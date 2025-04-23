#define VY   31.1
#define VX   137
#define MY   191
#define FY   78.0
#define MX   3
#define HGR_H 193

int depth(int y) {
  return FY + MY - y;
}

float y_divisor(int y) {
  return depth(y)/((MY-VY)/FY);
}

float x_factor(int y) {
  return (256*FY)/depth(y);
}

float x_shift(int y) {
  return (-VX*FY)/depth(y) + VX + MX;
}

float y_factor(int y) {
  return MY-(((MY-y) * FY) / y_divisor(y));
}
