#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// put a hash of a into b
// hash may be from https://github.com/skeeto/hash-prospector
#define P(a, b)                                                                \
  {                                                                            \
    u32 x = a;                                                                 \
    x ^= x >> 17;                                                              \
    x *= 0xed5ad4bbU;                                                          \
    x ^= x >> 11;                                                              \
    x *= 0xac4c1b51U;                                                          \
    x ^= x >> 15;                                                              \
    x *= 0x31848babU;                                                          \
    x ^= x >> 14;                                                              \
    b = x;                                                                     \
  }

typedef uint32_t u32;

/** mem = main memory, s = length(mem) */
u32 *mem, s;
/** gw: ensure mem is at least length a+1, filling in mem */
void gw(u32 a) {
  if (__builtin_expect(a >= s, 0)) {
    u32 i = s;
    s = a + 1;
    mem = realloc(mem, s * sizeof(u32));
    if (a > 0x10000)
      exit(1);
    while (i < s) {
      P(i, mem[i]);
      i++;
    }
  }
}
/** mem get: get mem[a], ensuring length at least a+1 */
u32 mg(u32 a) {
  gw(a);
  return mem[a];
}
/** mem set: set mem[a] <- v, ensuring length at least a+1 */
void ms(u32 a, u32 v) {
  gw(a);
  mem[a] = v;
}
/** t = secondary memory,
 * ai = address into this memory (always increases but overflow) */
uint16_t t[16640], ai;
int sse(int p, int c, int b) {
  int g = (b << 16) + (b << 6) - b - b;
  t[ai] += (g - t[ai]) >> 6;
  t[ai + 1] += (g - t[ai + 1]) >> 6;
  int w = p & 127;
  ai = (p >> 2) + (c << 6) + c;
  return (t[ai] * (128 - w) + t[ai + 1] * w) >> 15;
}
u32 pc(u32 x) {
  x -= (x >> 1) & 0x55555555;
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x + (x >> 4)) & 0x0f0f0f0f;
  return (x * 0x01010101) >> 24;
}
char *mnemonic(int v) {
  switch (v) {
  case 0x0F: return "a = mg(ip++)";
  case 0x1F: return "b = mg(ip++)";
  case 0x2F: return "c = mg(ip++)";
  case 0x3F: return "ip = mg(ip++)";
  case 0x4F: return "if (a <= b) ip = mg(ip++)";
  case 0x5F: return "if (b >= c) ip = mg(ip++)";
  case 0x6F: return "a = sse(b & 0xFF, c & 0xFF, pc(mg(ip++)) & 1)";
  case 0x00: return "ms(ip++, a)";
  case 0x10: return "ms(ip++, b)";
  case 0x20: return "ms(ip++, c)";
  case 0x30: return "ms(a, b)";
  case 0x40: return "ms(a, c)";
  case 0x50: return "ms(a, mg(a))";
  case 0x60: return "b = a";
  case 0x70: return "c = a";
  case 0x80: return "a = b";
  case 0x90: return "a = c";
  case 0xA0: return "ip = a";
  case 0xB0: return "a = b + c";
  case 0xC0: return "a = b - c";
  case 0xD0: return "putchar(a)";
  case 0xE0: return "a = getchar()";
  case 0xF0: return "exit(0)";
  default: return "P(a,a);P(b,b);P(c,c)";
  }
}
int main(int argc, char *argv[]) {
  for (int i = 0; i < 256; i++)
    for (int j = 0; j < 65; j++)
      t[i * 65 + j] = i == 0 ? j << 10 : t[j];
  FILE *in = fopen(argv[1], "rb");
  u32 idx = 0;
  while (!feof(in)) {
    uint8_t instr = fgetc(in);
    ms(idx, mg(idx) ^ instr);
    idx++;
    if ((instr & 15) == 15) {
      u32 value = 0;
      // consume four MORE bytes to get a full 32-bit value to fill the next idx
      for (int i = 0; i < 4; i++)
        value = (value << 8) | fgetc(in);
      ms(idx, mg(idx) ^ value);
      idx++;
    }
  }
  for (int j = 0; j < s; j++) {
    printf("%04X: %08X\n", j, mem[j]);
  }
  printf("\n");
  fclose(in);
  // set the last word with complicated formula
  for (u32 i = 1; i < idx; i++)
    ms(idx, mg(idx) ^ sse(mg(i - 1) & 255, mg(i) & 255, pc(mg(i)) & 1));
  // the action begins
  // a,b,c registers; a is special
  u32 a = 0x66, b = 0xF0, c = 0x0F, ip = 0;

  for (u32 i = 6; i--;) {
    printf("%04X: ", ip);
    u32 v = mg(ip++);
    printf("ip=%04X; mg(ip)=%08X; ", ip, mg(ip));
    printf("a=%08X; b=%08X; c=%08X; ", a, b, c);
    printf((v >> 8) ? "0x%08X: " : "0x%02X: ", v);
    puts(mnemonic(v));
    switch (v) {
    case 0x0F: a = mg(ip++); break;
    case 0x1F: b = mg(ip++); break;
    case 0x2F: c = mg(ip++); break;
    case 0x3F: ip = mg(ip++); break;
    case 0x4F:
      if (a <= b)
        ip = mg(ip++);
      break;
    case 0x5F:
      if (b >= c)
        ip = mg(ip++);
      break;
    case 0x6F: a = sse(b & 0xFF, c & 0xFF, pc(mg(ip++)) & 1); break;
    case 0x00: ms(ip++, a); break;
    case 0x10: ms(ip++, b); break;
    case 0x20: ms(ip++, c); break;
    case 0x30: ms(a, b); break;
    case 0x40: ms(a, c); break;
    case 0x50: ms(a, mg(a)); break;
    case 0x60: b = a; break;
    case 0x70: c = a; break;
    case 0x80: a = b; break;
    case 0x90: a = c; break;
    case 0xA0: ip = a; break;
    case 0xB0: a = b + c; break;
    case 0xC0: a = b - c; break;
    case 0xD0:
      putchar(a);
      printf(" 0x%02X\n", a);
      break;
    case 0xE0: a = getchar(); break;
    case 0xF0: return 0;
    default:
      P(a, a);
      P(b, b);
      P(c, c);
    }
  }
}
