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

  // exit(0)
  put1(0xFF);
  put4(0xF0 ^ P(idx));
}

void hello_world() {
  put1(0x00);

  // putchar(72) // => 'H'
  put1(0xFF);
  put4(0x0F ^ P(2));
  put1(16);
  put1(0xFF);
  put4(0xD0 ^ P(5));

  // putchar(101) // => 'e'
  put1(0xFF);
  put4(0x0F ^ P(7));
  put1(62);
  put1(0xFF);
  put4(0xD0 ^ P(10));

  // putchar(108) // => 'l'
  put1(0xFF);
  put4(0x0F ^ P(12));
  put1(72);
  put1(0xFF);
  put4(0xD0 ^ P(15));

  // putchar(108) // => 'l'
  put1(0xFF);
  put4(0x0F ^ P(17));
  put1(120);
  put1(0xFF);
  put4(0xD0 ^ P(20));

  // putchar(111) // => 'o'
  put1(0xFF);
  put4(0x0F ^ P(22));
  put1(167);
  put1(0xFF);
  put4(0xD0 ^ P(25));

  // putchar(44) // => ','
  put1(0xFF);
  put4(0x0F ^ P(27));
  put1(200);
  put1(0xFF);
  put4(0xD0 ^ P(30));

  // putchar(32) // => ' '
  put1(0xFF);
  put4(0x0F ^ P(32));
  put1(254);
  put1(0xFF);
  put4(0xD0 ^ P(35));

  // putchar(87) // => 'W'
  put1(0xFF);
  put4(0x0F ^ P(37));
  put1(143);
  put1(0xFF);
  put4(0xD0 ^ P(40));

  // putchar(111) // => 'o'
  put1(0xFF);
  put4(0x0F ^ P(42));
  put1(189);
  put1(0xFF);
  put4(0xD0 ^ P(45));
  put1(0x00);

  // putchar(114) // => 'r'
  put1(0xFF);
  put4(0x0F ^ P(48));
  put1(141);
  put1(0xFF);
  put4(0xD0 ^ P(51));

  // putchar(108) // => 'l'
  put1(0xFF);
  put4(0x0F ^ P(53));
  put1(93);
  put1(0xFF);
  put4(0xD0 ^ P(56));

  // putchar(100) // => 'd'
  put1(0xFF);
  put4(0x0F ^ P(58));
  put1(183);
  put1(0xFF);
  put4(0xD0 ^ P(61));

  // putchar(33) // => '!'
  put1(0xFF);
  put4(0x0F ^ P(63));
  put1(164);
  put1(0xFF);
  put4(0xD0 ^ P(66));

  // putchar(10) // => '\n'
  put1(0xFF);
  put4(0x0F ^ P(68));
  put1(33);
  put1(0xFF);
  put4(0xD0 ^ P(71));

  // exit(0)
  put1(0xFF);
  put4(0xF0 ^ P(idx));
}

int main() {
  // for (u32 i = 0; i < 50; i++) {
  //   printf("%2x %08x\n", i, P(i));
  // }
  out = fopen("prog", "wb");
  hello_world();
  fclose(out);
}
