#define VY   31.1
#define VX   137
#define MY   191
#define FY   78.0
#define MX   3
#define HGR_H 193

int x_divisor_at_y(int y) {
  return FY + MY - y;
}

float y_divisor_at_y(int y) {
  return (FY + MY - y)/((MY-VY)/FY);
}

float x_shift(int y) {
  return (-VX*FY)/(FY + MY - y) + VX + MX;
}

float x_factor(int y) {
  return (256*FY)/x_divisor_at_y(y);
}

float y_factor(int y) {
  return MY-(((MY-y) * FY) / y_divisor_at_y(y));
}
