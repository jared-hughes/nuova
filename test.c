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

/**
 * Set the value of `a` to `value` at idx + 20ish (TODO shift).
 * Tramples over b and c but only takes about 20 mem
 **/
int force_a_value(u32 idx, u32 value) {
  for (u32 A = 0; A < 256; A++) {
    printf("A = %02X\n", A);
    for (u32 B = 0; B < 256; B++) {
      for (u32 C = 0; C < 256; C++) {
        for (u32 D = 0; D < 256; D++) {
          u32 a, b, c, d;
          b = P(P(A ^ P(idx + 2)));
          c = P(B ^ P(idx + 5));

          b = P(P(b + c));
          c = C ^ P(idx + 12);

          b = P(P(b) + P(c));
          c = D ^ P(idx + 18);
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
  u32 v = start;
  for (u32 i = 0;; i++) {
    if (v == end)
      return i;
    v = P(v);
    if (v == start) {
      printf("No path from %08X to %08X", start, end);
      exit(1);
    }
    // if (i % 10000 == 0)
    //   printf("%d\n", i);
  }
}

/**
 * Force the value of `b` to be `value` somewhere afterish idx
 **/
void force_b_value(u32 idx, u32 value) {
  Set inverses = inverses_set(value);
  // i:   0xFF
  // i+1: 0x1F ^ P(idx) // b =
  // i+2: B = 0x00 up to 0xFF. Disallow 0x0F mod 0x10 for now.
  //                           Don't want to deal with adding padding
  // end result: b = b1 = B ^ P(i + 2)
  // we want b1 in inverses
  // Then i + 2 + stepsFrom(b1, value) is the first step where b = value
  for (u32 i = idx;; i++) {
    for (u32 B = 0; B < 256; B++) {
      if (B & 0x0F == 0x0F)
        continue;
      u32 b1 = B ^ P(i + 2);
      if (set_has_value(&inverses, b1)) {
        printf("Found! %d %02X %d\n", i, B, stepsFrom(b1, value));
        return;
      }
    }
  }
}

int main() { force_b_value(20, 0x01); }
