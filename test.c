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

#define SADGE(s)                                                               \
  {                                                                            \
    printf(s "\n");                                                            \
    exit(1);                                                                   \
  }

/** input = input as bytes, mem = main memory, s = length(mem) */
u32 *filled, *input, *mem, s;
/** gw: ensure mem is at least length a+1, filling in mem with 0s */
void gw(u32 a) {
  if (__builtin_expect(a >= s, 0)) {
    u32 i = s;
    s = a + 1;
    if (a > 0x100000)
      SADGE("Too Long");
    mem = realloc(mem, s * sizeof(u32));
    input = realloc(input, s * sizeof(u32));
    filled = realloc(input, s * sizeof(u32));
    for (; i < s; i++) {
      mem[i] = 0;
      input[i] = 0;
      filled[i] = 0;
    }
  }
}
/** mem set: set mem[idx] <- v at initial load, ensuring length at least idx+1*/
void ms_simple(u32 idx, u32 v) {
  gw(idx);
  if (filled[idx]) {
    printf("Already filled, at %08x\n", idx);
    exit(1);
  }
  u32 xor = v ^ P(idx);
  if (xor >> 8) {
    if (idx == 0) {
      SADGE("Can't 4-byter at beginning")
    } else if (filled[idx - 1]) {
      if ((input[idx - 1] & 0xF) != 0xF)
        SADGE("Not F, can't fill x4")
    } else {
      filled[idx - 1] = 1;
      u32 p = P(idx - 1);
      if (p >> 8 == 0)
        SADGE("Collision possible")
      filled[idx - 1] = 1;
      mem[idx - 1] = p ^ 0x0F;
      input[idx - 1] = 0xFF;
    }
  }
  filled[idx] = 1;
  mem[idx] = v;
  input[idx] = xor;
}

u32 padding_char(u32 idx) {
  for (u32 c = 0; c < 256; c++) {
    if ((c & 0xF) == 0xF)
      continue;
    u32 res = P(idx) ^ c;
    if (!(res >> 8 == 0 && ((res & 0xF) == 0xF || (res & 0xF) == 0))) {
      return c;
    }
  }
}

/**
 * Fills in [start, ..., end] with all padding: PPP every time, exactly
 * end - start + 1 PPPs (no instr )
 * Start and end are inclusive
 */
void padding(u32 start, u32 end) {
  for (u32 j = start; j <= end; j++) {
    // TODO: avoid accidental instruction
    ms_simple(j, P(j) ^ padding_char(j)); // bunch of PPP;
  }
}

void emit_from_mem() {
  printf("Emitting!\n");
  FILE *out = fopen("prog", "wb");
  u32 prev_f = 0;
  u32 bytes = 0;
  for (u32 i = 0; i < s; i++) {
    u32 x = filled[i] ? input[i] : padding_char(i);
    if (prev_f) {
      prev_f = 0;
      fputc(x >> 24, out);
      fputc(x >> 16, out);
      fputc(x >> 8, out);
      fputc(x, out);
      bytes += 4;
    } else {
      fputc(x, out);
      bytes++;
      if ((x & 0xF) == 0xF)
        prev_f = 1;
    }
  }
  fclose(out);
  printf("Bytes: %d\n", bytes);
}

/**
 * Set the value of `a` to `value` at idx + 21.
 * Requires writing spawn mem from idx to idx + 21 inclusive
 * Tramples over b and c but small
 * TODO: ensure no xF in XORed
 **/
u32 try_set_force_a_value(u32 idx, u32 value) {
  printf("Trying %08X\n", idx);
  for (u32 A = 0; A < 256; A++) {
    if ((A & 0xF) == 0xF)
      continue;
    for (u32 B = 0; B < 256; B++) {
      if ((B & 0xF) == 0xF)
        continue;
      for (u32 C = 0; C < 256; C++) {
        if ((C & 0xF) == 0xF)
          continue;
        for (u32 D = 0; D < 256; D++) {
          if ((D & 0xF) == 0xF)
            continue;
          u32 a, b, c, d;
          b = A ^ P(idx + 2);
          b = P(b);
          c = B ^ P(idx + 5);
          b = P(b), c = P(c);
          a = b + c;
          a = P(a), b = P(b), c = P(c);
          b = a;
          a = P(a), b = P(b), c = P(c);
          c = C ^ P(idx + 12);
          a = P(a), b = P(b), c = P(c);
          a = b + c;
          a = P(a), b = P(b), c = P(c);
          b = a;
          a = P(a), b = P(b), c = P(c);
          c = D ^ P(idx + 19);
          a = P(a), b = P(b), c = P(c);
          a = b + c;

          if (a == inverseP(value)) {
            printf("Succeeded at %08X\n", idx);
            ms_simple(idx + 1, 0x1F);             // b =
            ms_simple(idx + 2, A ^ P(idx + 2));   // A ^ P(idx + 2);
            ms_simple(idx + 4, 0x2F);             // P(); c =
            ms_simple(idx + 5, B ^ P(idx + 5));   // B ^ P(idx + 5);
            ms_simple(idx + 7, 0xB0);             // P(); a = b + c;
            ms_simple(idx + 9, 0x60);             // P(); b = a;
            ms_simple(idx + 11, 0x2F);            // P(); c =
            ms_simple(idx + 12, C ^ P(idx + 12)); // C ^ P(idx + 12);
            ms_simple(idx + 14, 0xB0);            // P(); a = b + c;
            ms_simple(idx + 16, 0x60);            // P(); b = a;
            ms_simple(idx + 18, 0x2F);            // P(); c =
            ms_simple(idx + 19, D ^ P(idx + 19)); // D ^ P(idx + 19);
            ms_simple(idx + 21, 0xB0);            // P(); a = b + c = value
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

/**
 * idx: a = value
 * Requires at least 21 mem before that (so 22 total)
 * Might need to backtrack some, so leave some space
 */
void force_a_value(u32 idx, u32 value) {
  printf("forcing %04X: a=%08X\n", idx, value);
  for (u32 i = idx; i >= idx - 21; i--) {
    if (filled[i])
      SADGE("Already filled in, at force_a_value")
  }
  for (;; idx--) {
    printf("Trying idx %04x\n", idx);
    u32 successful = try_set_force_a_value(idx - 21, value);
    if (successful)
      return;
    value = inverseP(value);
  }
}

#define SETSZ (1 << 18)
#define MINLOOKUP (1 << 11)
// value in set if thing[value % n % SETSZ] == value
typedef struct Set {
  u32 n;
  u32 thing[SETSZ];
} Set;

// SETSZ (1 << 18)
// MINLOOKUP (1 << 11)
// key = v % n % SETSZ
u32 MODS[] = {
    0,      268556, 266594, 262998, 262200, 266760, 263300, 264101, 264945,
    264688, 263644, 268799, 262339, 264458, 262869, 263532, 266197, 262400,
    265597, 266914, 263816, 267806, 263218, 265844, 265178, 266528, 265728,
    262553, 264788, 263580, 263293, 272927, 267147, 275720, 264410, 266000,
    273028, 262513, 265460, 265939, 266048, 265051, 262396, 264565, 265046,
    262921, 264533, 263678, 262828, 266683, 267995, 264316, 264954, 264960,
    264658, 264697, 265029, 267166, 262788, 272741, 264322, 266193, 263026,
    263604, 263296, 263843, 262636, 268127, 265140, 265273, 262226, 269344,
    263917, 262713, 263194, 263395, 277860, 264056, 266434, 266083, 265876,
    265298, 262301, 262803, 270084, 265861, 262748, 264343, 272010, 262588,
    265414, 262810, 265548, 265578, 262692, 262408, 262960, 264344, 263881,
    262933, 262338, 266613, 264878, 275967, 264353, 267757, 262674, 262700,
    263829, 264030, 263027, 273019, 262911, 263621, 263223, 264711, 262150,
    262809, 262178, 271027, 270432, 267378, 264596, 267265, 263866, 262285,
    263637, 278314, 272662, 265719, 264757, 264130, 262309, 262386, 263981,
    264732, 266344, 264994, 266785, 274996, 262769, 265548, 265022, 262883,
    265206, 262490, 264260, 264589, 262954, 263562, 263213, 262177, 263000,
    264884, 262898, 275057, 270493, 262253, 263626, 264866, 264843, 262505,
    265494, 264475, 264400, 277913, 263956, 266528, 265688, 270545, 262218,
    263069, 269278, 264588, 270709, 262201, 263333, 264365, 262782, 263007,
    266473, 266411, 262220, 263860, 263232, 272093, 263555, 265758, 266579,
    263136, 275772, 262914, 265626, 262949, 266161, 262506, 273037, 262807,
    263028, 268448, 262445, 266752, 264885, 269833, 264363, 264974, 262866,
    266536, 264655, 265213, 266664, 262921, 264158, 266078, 272643, 263639,
    268604, 264030, 270489, 264731, 262541, 274526, 263337, 270678, 263736,
    262873, 265167, 263683, 263697, 271390, 262482, 263926, 264695, 270607,
    262389, 262744, 264290, 263460, 263453, 263578, 263009, 265173, 267117,
    267579, 262442, 264243, 262860, 263114, 266765, 262302, 263944, 263505,
    265170, 264120, 262485, 262962,
};

u32 find_hash_mod(u32 value) {
  u32 round[SETSZ];
  for (u32 n = SETSZ;; n++) {
    u32 v = value;
    for (u32 i = 0;; i++, v = inverseP(v)) {
      u32 key = v % n % SETSZ;
      if (round[key] == n) {
        // collision!
        if (i > MINLOOKUP) {
          // big enough
          return n;
        } else {
          // try the next n
          break;
        }
      }
      round[key] = n;
    }
  }
}

void find_hash_mods() {
  printf("u32 MODS[] = {0,");
  fflush(stdout);
  for (int i = 1; i < 256; i++) {
    printf("%d,", find_hash_mod(i));
    fflush(stdout);
  }
  printf("};");
}

Set inverses_set(u32 value) {
  u32 n;
  if (value > 255) {
    printf("You've done an oopsie. value>255 so working hard to find n\n");
    n = find_hash_mod(value);
  } else {
    n = MODS[value];
  }
  Set s = {.n = n};
  u32 v = value;
  for (u32 i = 0;; i++, v = inverseP(v)) {
    u32 key = v % n % SETSZ;
    if (s.thing[key] > 0) {
      // collision!
      if (i > MINLOOKUP) {
        // big enough
        return s;
      } else {
        printf("Invalid mod %d for value %08X", n, value);
        exit(1);
      }
    }
    s.thing[key] = v;
  }
}

int set_has_value(Set *s, u32 value) {
  return s->thing[value % s->n % SETSZ] == value;
}

// end = P^{s}(start) if s = stepsFrom(start, end);
u32 stepsFrom(u32 start, u32 end) {
  printf("start=%08X, end=%08X\n", start, end);
  u32 v = start;
  for (u32 i = 0;; i++) {
    if (v == end)
      return i;
    v = P(v);
    if (v == start) {
      printf("No path from %08X to %08X", start, end);
      exit(1);
    }
  }
}

u32 inversePn(u32 end, u32 n) {
  for (u32 i = 0; i < n; i++) {
    end = inverseP(end);
  }
  return end;
}

/**
 * ms(setIdx, setValue)
 **/
void initMemset(u32 setIdx, u32 setValue) {
  printf("ms(%08x, %08x);", setIdx, setValue);
  // End goal:
  // at idx1: a = inverseP^{i - idx1 + 2 + stepsFrom(b1, setValue)}(setIdx)
  // at i-1: a = inverseP^{2 + stepsFrom(b1, setValue)}(setIdx)
  // then some 0x00s
  // i:   0xFF
  // i+1: 0x1F ^ P(i+1) // b =
  // i+2: B = 0x00 up to 0xFF. Disallow 0x0F mod 0x10 for now.
  //                           Don't want to deal with adding padding
  // at i + 2: b = b1 = B ^ P(i + 2)
  // then some 0x00s
  // at i + 2 + stepsFrom(b1, setValue): b = setValue

  Set inverses = inverses_set(setValue);
  // start at 25 to give some space for force_a_value
  for (u32 i = 25;; i++) {
    for (u32 B = 0; B < 256; B++) {
      if (B & 0x0F == 0x0F)
        continue;
      u32 b1 = B ^ P(i + 2);
      if (set_has_value(&inverses, b1)) {
        u32 endpt = i + 3 + stepsFrom(b1, setValue);
        printf("i = %08X; endpt = %08X\n", i, endpt);
        // check clean
        gw(endpt);
        for (u32 j = i - 22; j <= endpt; j++) {
          if (filled[j]) {
            for (i = j; filled[i];) {
              i++;
            }
            printf("nonempty\n");
            goto no_work;
          }
        }
        printf("forceroo\n");
        // Why +4??
        force_a_value(i - 1, inversePn(setIdx, endpt - (i - 1) - 4));
        // filleroo
        printf("filleroo\n");
        ms_simple(i + 1, 0x1F);         // b =
        ms_simple(i + 2, B ^ P(i + 2)); // B ^ P(i + 2);
        padding(i + 3, endpt - 2);
        ms_simple(endpt, 0x30); // ms(a, b)
        return;
      }
    no_work:
    }
  }
}

void pushExit() { ms_simple(s + 1, 0xF0); }

void cat_nonterminating() {
  u32 p = 0x4a00;
  initMemset(p, 0xE0);          // p: getchar
  ms_simple(p + 1, 0xD0);       // p + 1: putchar
  force_a_value(p + 30, p - 1); // p+30: a = p-1
  ms_simple(p + 32, 0xA0);      // p+34: a = p-1; ip = a;
  emit_from_mem();
}

void cat_terminating() {
  u32 p = 0x4a00;
  ms_simple(p, 0x1F);           // b = P(p+1)^0xFF;
  ms_simple(p + 2, 0xE0);       // a = getchar();
  initMemset(p + 3, 0x4F);      // if (a = b) ip =
  ms_simple(p + 4, p + 8);      // p + 8;
  ms_simple(p + 6, 0xF0);       // exit(0)
  ms_simple(p + 8, 0xD0);       // putchar(a)
  force_a_value(p + 40, p - 1); // p+40: a = p-1
  ms_simple(p + 42, 0xA0);      // p+42: a = p-1; ip = a;
  emit_from_mem();
}

int main() { cat_nonterminating(); }
