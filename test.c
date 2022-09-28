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

int force_a_value(u32 idx, u32 value) {
  u32 x = P(idx + 2), y = P(idx + 5), z = P(idx + 12), w = P(idx + 18);
  for (u32 A = 0; A < 256; A++) {
    printf("A = %02X\n", A);
    for (u32 B = 0; B < 256; B++) {
      for (u32 C = 0; C < 256; C++) {
        for (u32 D = 0; D < 256; D++) {
          u32 a, b, c, d;
          b = P(P(A ^ x));
          c = P(B ^ y);

          b = P(P(b + c));
          c = C ^ z;

          b = P(P(b) + P(c));
          c = D ^ w;
          a = P(b) + P(c);

          if (a == inverseP(value)) {
            printf("%04X: %02X %02X %02X %02X => %08X\n", idx, A, B, C, D, a);
          }
        }
      }
    }
  }
}

u32 find_cycle_length(u32 x) {
  u32 startX = x;
  u32 i = 1;
  x = P(x);
  while (startX != x) {
    if (x >> 8 == 0)
      printf("0x%02X = P^{0x%08X}(0x%02X)\n", x, i, startX);
    i += 1;
    x = P(x);
  }
  printf("0x%02X = P^{0x%08X}(0x%02X)\n", x, i, startX);
  return i;
}

int main() { printf("%08x\n", find_cycle_length(0x00)); }
