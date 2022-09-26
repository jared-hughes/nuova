#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint32_t u32;

#define LEN(X) (sizeof((X)) / sizeof(*(X)))

u32 P(u32 x) {
  x ^= x >> 17;
  x *= 0xed5ad4bbU;
  x ^= x >> 11;
  x *= 0xac4c1b51U;
  x ^= x >> 15;
  x *= 0x31848babU;
  x ^= x >> 14;
  return x;
}

u32 inverseP(u32 x) {
  x ^= x >> 28 << 14;
  x ^= x << 4 >> 18;
  x *= 0x32b21703U;
  x ^= x >> 30 << 15;
  x ^= x << 2 >> 17;
  x *= 0x469e0db1U;
  x ^= x >> 22 << 11;
  x ^= x << 10 >> 21;
  x *= 0x79a85073U;
  x ^= x >> 17;
  return x;
}

FILE *out;

u32 idx = 0;

void put1(u32 x) {
  fputc(x, out);
  idx++;
}

void put4(u32 x) {
  fputc(x >> 24, out);
  fputc(x >> 16, out);
  fputc(x >> 8, out);
  fputc(x, out);
  idx++;
}

void hi() {
  // H
  put1(0x00);
  put1(0XFF);          // PPP
  put4(0x0F ^ P(idx)); // a =
  put1(0x10);          // 1 byte control
  put1(0xFF);          // PPP
  put4(0xD0 ^ P(idx)); // putchar(a)
  // i
  put1(0X00);
  put1(0XFF);          // PPP
  put4(0x0F ^ P(idx)); // a =
  put1(0xa0);          // 1 byte control
  put1(0xFF);          // PPP
  put4(0xD0 ^ P(idx)); // putchar(a)
}

int main() {
  // for (u32 i = 0; i < 50; i++) {
  //   printf("%2x %08x\n", i, P(i));
  // }
  out = fopen("prog", "wb");
  hi();
  fclose(out);
}
